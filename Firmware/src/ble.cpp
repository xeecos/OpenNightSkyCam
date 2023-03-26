

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include "ble.h"
#include "WiFi.h"
#include "config.h"
#include "global.h"
#include "service.h"
#include "log.h"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
char toClient[128] = {0};
char fromClient[128] = {0};
int fromIndex = 0;
int toIndex = 0;

class BTServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        service_reconnect();
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};
class BTCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0)
        {
            for (int i = 0; i < rxValue.length(); i++)
            {
                char c = rxValue[i];
                if (c == '\n')
                {
                    fromClient[fromIndex] = '\0';
                    fromClient[fromIndex + 1] = '\0';
                    if (fromIndex > 6)
                    {
                        ble_parse();
                    }
                    fromIndex = 0;
                }
                else
                {
                    fromClient[fromIndex] = c;
                    fromIndex++;
                }
            }
            // String input =
            //     "{\"model\":\"ezcam\",\"wifi\":[],\"ip\":\"192.168.31.118\",\"status\":0,\"time\":1351824120,\"ver\":\"1.0.0\"}";
            //     //{"action":"info"}
            //     //{"action":"status"}
            //     //{"action":"list"}
            //     //{"action":"connect","ssid":"","password":""}
            //     //{"res":"ok"}

            // String output;
            // serializeJson(doc, output);
        }
    }
};
void ble_init()
{
    BLEDevice::init(global_get_name());
    // Create the BLE Device

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BTServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new BTCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
}

void ble_send(int status, String msg)
{
    if (deviceConnected)
    {
        pTxCharacteristic->setValue(msg.c_str());
        pTxCharacteristic->notify();
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    }
}
void ble_send(char *msg)
{
    if (deviceConnected)
    {
        char *out = (char *)ps_malloc(20);
        int idx = 0;
        bool isEnd = false;
        while (true)
        {
            for(int i=0;i<20;i++)
            {
                out[i] = msg[idx];
                idx++;
                if(out[i]==0||idx>64)
                {
                    isEnd = true;
                    break;
                }
            }
            pTxCharacteristic->setValue(out);
            pTxCharacteristic->notify();
            delay(10); // bluetooth stack will go into congestion, if too many packets are sent
            if(isEnd)
            {
                break;
            }
        }
        free(out);
    }
}
void ble_send_ok()
{
    if (deviceConnected)
    {
        pTxCharacteristic->setValue("{\"res\":\"ok\"}\n");
        pTxCharacteristic->notify();
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    }
}
char cmd[128] = {0};
char data[128] = {0};
void ble_parse()
{
    global_parse(fromClient, cmd, data);
    String cmdStr = cmd;
    String dataStr = data;
    // LOG_UART("parse:%s %s\n", cmdStr.c_str(), dataStr.c_str());
    if (cmdStr.equals("getname"))
    {
        char *jsonstr = (char *)ps_malloc(128);
        sprintf(jsonstr, "{\"name\":\"%s\"}\n\0", global_get_name());
        ble_send(jsonstr);
        free(jsonstr);
    }
    else if (cmdStr.equals("setname"))
    {
        global_set_name(data);
        ble_send_ok();
    }
    else if (cmdStr.equals("setssid"))
    {
        global_set_ssid(data);
        ble_send_ok();
    }
    else if (cmdStr.equals("setpassword"))
    {
        global_set_password(data);
        ble_send_ok();
    }
    else if (cmdStr.equals("getip"))
    {
        char *jsonstr = (char *)malloc(128);
        memset(jsonstr,0,128);
        if(WiFi.localIP().toString().equals("0.0.0.0"))
        {
            sprintf(jsonstr, "{\"ip\":\"%s\"}\n", (char*)WiFi.softAPIP().toString().c_str());
        }
        else
        {
            sprintf(jsonstr, "{\"ip\":\"%s\"}\n", (char*)WiFi.localIP().toString().c_str());
        }
        ble_send(jsonstr);
        LOG_UART(jsonstr);
        free(jsonstr);
    }
    else if (cmdStr.equals("reset"))
    {
        global_reset();
    }
}
bool ble_connected()
{
    return deviceConnected;
}
void ble_run()
{
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}