#pragma once


#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCFFA1" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCFFA1"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCFFA1"
typedef enum {
    BLE_OK = 0,
    BLE_ERR,
    BLE_NAME_GET,
    BLE_NAME_SET,
    BLE_WIFI_SET,
    BLE_IP_GET
}BLE_METHOD;
void ble_init();
void ble_send(int status,String msg);
void ble_send(char* msg);
void ble_send_ok();
void ble_parse();
bool ble_connected();
void ble_run();