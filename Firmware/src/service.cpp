#include "service.h"
#include "driver/temp_sensor.h"
#include <WiFi.h>
#include "capture.h"
#include "config.h"
#include "global.h"
#include "tf.h"
#include "log.h"
#include "task.h"
#include "esp_camera.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "SD_MMC.h"
#include <WebServer.h>
#include <esp_bt.h>
#include <ESPmDNS.h>
#include "AsyncUDP.h"
WebServer server(80);
AsyncUDP udp;
void service_reconnect()
{
    // WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP("NightSkyCam","");
    String ssid = "";
    ssid += global_get_ssid();
    String password = "";
    password += global_get_password();
    LOG_UART("ssid:%s pwd:%s\n", ssid.c_str(), password.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    if (MDNS.begin("cam")) 
    {
        //browse "http://cam.local"
    }
    // int timeout = 20;
    // LOG_UART("connecting");
    // while (WiFi.status() != WL_CONNECTED && timeout > 0)
    // {
    //     delay(500);
    //     LOG_UART(".");
    //     timeout--;
    // }
    // LOG_UART("\n");
    // WiFi.setSleep(true);
    // LOG_UART("IP address: %s\n", WiFi.localIP().toString().c_str());
}
void wifi_task() // void *arg
{
    // while (!global_has_ssid() || !global_has_password())
    // {
    //     global_set_ssid("NightSkyCam");
    //     global_set_password("12345678");
    //     delay(1000);
    // }
    service_reconnect();
    
    server.onNotFound([=]()
                      {
        String uri = server.uri();
        LOG_UART("uri:%s\n",uri.c_str());
        if(uri.startsWith("/capture/"))
        {

        }
        else if(uri.startsWith("/sd/"))
        {
            
        }
        else if(uri.startsWith("/time/"))
        {
            
        }
        else
        {
            switch(server.method())
            {
                case HTTP_GET:
                {
                    if (uri.endsWith("/"))
                    { 
                        tf_list_dir(uri, &server);
                    }
                    else
                    {
#ifdef USE_MMC
                        File f = tf_read(uri.c_str());
#else
                        ExFile f = tf_read(uri.c_str());
#endif
                        if(uri.endsWith("html"))
                        {
                            server.streamFile(f,"text/html");
                        }
                        else if(uri.endsWith("json"))
                        {
                            server.streamFile(f,"application/json");
                        }
                        else if(uri.endsWith("jpeg")||uri.endsWith("jpg"))
                        {
                            server.streamFile(f,"image/jpeg");
                        }
                        else
                        {
                            server.streamFile(f,"application/octet-stream");
                        }
                        f.close();
                    }
                }
                break;
                
                case HTTP_DELETE:
                {
                    if (uri.endsWith("/"))
                    { 
                        tf_remove_dir(uri.c_str());
                        server.send(200, "text/plain", "dir removed");
                        return;
                    }
                    else
                    {
#ifdef USE_MMC
                        File f = SD_MMC.open(uri.c_str(),"r");
                        if(f&&!f.isDirectory())
                        {
                            f.close();
                            SD_MMC.remove(uri.c_str());
                            server.send(200, "text/plain", "file removed");
                            return;
                        }
#endif
                    }
                    server.send(200, "text/plain", "no removed");
                    return;
                }
                break;

            }
            return;
        }
        server.send(200, "text/plain", "hello from ezcam!"); });

    server.on("/task/add", [=]()
    { 
        //http://192.168.31.194/task/add?delay=1000&during=3000&count=10&mode=3&exposure=1.0&gain=0&resolution=0
        task_add(server.arg("delay").toInt(), server.arg("interval").toInt(), server.arg("frames").toInt(), (RECORD_MODE)server.arg("mode").toInt(),server.arg("coarse").toInt(),server.arg("fine").toInt(),server.arg("r_gain").toFloat(),server.arg("gr_gain").toFloat(),server.arg("gb_gain").toFloat(),server.arg("b_gain").toFloat(),server.arg("resolution").toInt());
        server.send(200, "text/json", "{\"err\":0}"); 
    });
    server.on("/capture/set", [=]()
    {
        sensor_t *s = esp_camera_sensor_get();
        if(server.hasArg("test"))
        {
            s->set_colorbar(s,(framesize_t)server.arg("test").toInt());
        }
        if(server.hasArg("reg")&&server.hasArg("val"))
        {
            s->set_reg(s,server.arg("reg").toInt(),server.arg("val").toInt());
        }
        if(server.hasArg("agc"))
        {
            s->set_gain_ctrl(s,(framesize_t)server.arg("agc").toInt());
        }
        if(server.hasArg("aec"))
        {
            s->set_exposure_ctrl(s,(int)server.arg("aec").toInt());
        }
        if(server.hasArg("coarse")&&server.hasArg("fine"))
        {
            s->set_aec_exposure(s,server.arg("coarse").toInt(),server.arg("fine").toInt());
        } 
        if(server.hasArg("binning"))
        {
            s->set_binning(s,server.arg("binning").toInt());
        }
        if(server.hasArg("binning_mode"))
        {
            s->set_binning_mode(s,server.arg("binning_mode").toInt());
        }
        if(server.hasArg("pixformat"))
        {
            s->set_pixformat(s,(pixformat_t)server.arg("pixformat").toInt());
        }
        if(server.hasArg("vstart"))
        {
            s->set_vstart(s,(framesize_t)server.arg("vstart").toInt());
        }
        if(server.hasArg("hstart"))
        {
            s->set_hstart(s,(framesize_t)server.arg("hstart").toInt());
        }
        if(server.hasArg("framesize"))
        {
            s->set_framesize(s,(framesize_t)server.arg("framesize").toInt());
        }
        if(server.hasArg("hmirror"))
        {
            s->set_hmirror(s,(framesize_t)server.arg("hmirror").toInt());
        }
        if(server.hasArg("vflip"))
        {
            s->set_vflip(s,(framesize_t)server.arg("vflip").toInt());
        }
        if(server.hasArg("blc"))
        {
            s->set_blc(s,server.arg("blc").toInt());
        }
        if(server.hasArg("quality"))
        {
            s->set_quality(s,server.arg("quality").toInt());
        }
        if(server.hasArg("gain"))
        {
            float gain = server.arg("gain").toFloat();
            s->set_agc_gain(s,gain);
        }
        if(server.hasArg("hblank"))
        {
            s->set_hblank(s, server.arg("hblank").toInt());
        }
        if(server.hasArg("vblank"))
        {
            s->set_vblank(s, server.arg("vblank").toInt());
        }
        if(server.hasArg("xclk"))
        {
            s->set_xclk(s,server.arg("xclk").toInt());
        }
        if(server.hasArg("r_gain")&&server.hasArg("gr_gain")&&server.hasArg("gb_gain")&&server.hasArg("b_gain"))
        {
            s->set_agc_gain_custom(s,server.arg("r_gain").toFloat(),server.arg("gr_gain").toFloat(),server.arg("gb_gain").toFloat(),server.arg("b_gain").toFloat());
        }
        server.send(200, "text/json", "{\"err\":0}"); 
    });
    server.on("/capture/shot", [=]()
    { 
        char *json_str = (char*)malloc(256);
        sensor_t *s = esp_camera_sensor_get();
        sprintf(json_str,"{\"err\":0,\"time\":\"%d\"}\0",s->status.coarse);
        server.send(200, "text/json", json_str);
        free(json_str); 
        capture_take(true); 
    });
    server.on("/storage/set", [=]()
    { 
        if(server.hasArg("mode"))
        {
            if(server.arg("mode").equals("udisk"))
            {
                tf_unmount();
            }
            else if(server.arg("mode").equals("camera"))
            {
                tf_mount();
            }
        }
        char *json_str = (char*)malloc(256);
        sprintf(json_str,"{\"err\":0}\0");
        server.send(200, "text/json", json_str);
        free(json_str); 
    });
    server.on("/time/set", [=]()
    { 
        if(server.hasArg("time"))
        {
            global_set_time(server.arg("time").toInt());
            server.send(200, "text/json", "{\"err\":0}"); 
        }
        else
        {
            server.send(200, "text/json", "{\"err\":-1}");
        } 
    });

    server.on("/task/status", [=]()
    { 
        char *json_str = (char*)malloc(128);
        sprintf(json_str, "{\"err\":0,\"name\":\"%s\",\"total\":%d,\"current\":%d}\0", task_get_name(),task_get_total(),task_get_index());
        server.send(200, "text/json", json_str); 
        free(json_str); 
    });
    server.on("/task/add", [=]()
    { 
        task_add(server.arg("delay").toInt(), server.arg("during").toInt(), server.arg("frames").toInt(), (RECORD_MODE)server.arg("mode").toInt(),server.arg("coarse").toInt(),server.arg("fine").toInt(),server.arg("r_gain").toFloat(),server.arg("gr_gain").toFloat(),server.arg("gb_gain").toFloat(),server.arg("b_gain").toFloat(),server.arg("resolution").toInt());
        server.send(200, "text/json", "{\"err\":0}"); 
    });
    server.on("/task/start", [=]()
    { 
        task_start();
        char *json_str = (char*)malloc(128);
        sprintf(json_str, "{\"err\":0,\"name\":\"%s\"}\0", task_get_name());
        server.send(200, "text/json", json_str); 
        free(json_str); 
    });
    server.on("/task/stop", [=]()
    { 
        task_stop();
        server.send(200, "text/json", "{\"err\":0}"); 
    });
    server.on("/task/resume", [=]()
    { 
        server.send(200, "text/json", "{\"version\":\"1.0\"}"); 
    });
    server.on("/time/get", [=]()
    { 
        char*str = (char*)malloc(128);
        sprintf(str,"{\"err\":0,\"time\":%d}\0",global_get_time());
        server.send(200, "text/json", str); 
        free(str); 
    });
    server.on(
        "/upload", HTTP_POST, [=]()
    { 
        server.send(200, "application/json", "{\"res\":\"uploaded\"}"); 
    },
    [=]()
    {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
            String filename = server.arg(0);
            if (SD_MMC.exists((char *)filename.c_str()))
            {
                SD_MMC.remove((char *)filename.c_str());
            }
            tf_begin_write(filename.c_str());
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
            tf_write_file(upload.buf, upload.currentSize);
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
            tf_end_write();
        }
    });
    server.on("/info", HTTP_GET, [=]()
    { 
        if(server.hasArg("test"))
        {
            LOG_UART("arg:%.2f\n",server.arg("test").toFloat());
        }
        char *json_str = (char*)malloc(128);
        sprintf(json_str, "{\"err\":0,\"version\":\"EZCAM V1.0.0\"}\0");
        server.send(200, "text/json", json_str); 
        free(json_str); 
    });
    server.enableCrossOrigin(true);
    server.enableCORS(true);
    server.enableDelay(true);
    server.begin();
}
static bool wifi_on = true;
void service_turn_off()
{
    if (wifi_on)
    {
        LOG_UART("wifi off\n");
        wifi_on = false;
        esp_wifi_disconnect();
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    }
}
void service_turn_on()
{
    if (!wifi_on)
    {
        LOG_UART("wifi on\n");
        wifi_on = true;
        esp_wifi_connect();
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }
}
char *msg;
int msg_idx = 0;
void service_init()
{
    msg = (char *)malloc(128);
    memset(msg, 0, 128);
    wifi_task();
}
const char *service_get_ip()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (WiFi.localIP().toString().equals("0.0.0.0"))
        {
            return WiFi.softAPIP().toString().c_str();
        }
        return WiFi.localIP().toString().c_str();
    }
    return WiFi.softAPIP().toString().c_str();
}

int indexOf(char *str, char cmd, size_t len, int startIndex)
{
    for (int i = startIndex; i < len; i++)
    {
        if (str[i] == cmd)
        {
            return i;
        }
    }
    return -1;
}
void service_parse(char *msg)
{
    if (msg[0] == 'M' || msg[0] == 'm')
    {
        int cmd = strtod(msg + 1, NULL), end, time;
        float gain_r, gain_g, gain_b;
        sensor_t *s = esp_camera_sensor_get();
        switch (cmd)
        {
        case 10:
        {
            end = indexOf(msg, 'T', msg_idx, 1) + 1;
            if(end>0)
            {
                time = strtod(msg + end, NULL);
                s->set_aec_exposure(s, time, 10);
                end = indexOf(msg, 'R', msg_idx, 1) + 1;
                gain_r = strtof(msg + end, NULL);
                end = indexOf(msg, 'G', msg_idx, 1) + 1;
                gain_g = strtof(msg + end, NULL);
                end = indexOf(msg, 'B', msg_idx, 1) + 1;
                gain_b = strtof(msg + end, NULL);
                if(gain_r>0 && gain_g>0 && gain_b>0)
                {
                    s->set_agc_gain_custom(s, gain_r, gain_g, gain_g, gain_b);
                }
            }
            
            s->take_photo(s, true, FILEFORMAT_UART);
        }
        break;
        case 11:
        {
            end = indexOf(msg, 'S', msg_idx, 1) + 1;
            int mode = strtod(msg + end, NULL);
            if (mode == 0)
            {
                tf_unmount();
            }
            else if (mode == 1)
            {
                tf_mount();
            }
        }
        break;
        case 20:
        {
            end = indexOf(msg, 'T', msg_idx, 1) + 1;
            time = strtod(msg + end, NULL);
            s->set_aec_exposure(s, time, 1);
        }
        break;
        case 21:
        {
            end = indexOf(msg, 'R', msg_idx, 1) + 1;
            gain_r = strtof(msg + end, NULL);
            end = indexOf(msg, 'G', msg_idx, 1) + 1;
            gain_g = strtof(msg + end, NULL);
            end = indexOf(msg, 'B', msg_idx, 1) + 1;
            gain_b = strtof(msg + end, NULL);
            s->set_agc_gain_custom(s, gain_r, gain_g, gain_g, gain_b);
        }
        break;
        case 30:
        {
            end = indexOf(msg, 'D', msg_idx, 1) + 1;
            int delay = strtof(msg + end, NULL);
            end = indexOf(msg, 'I', msg_idx, 1) + 1;
            int interval = strtof(msg + end, NULL);
            end = indexOf(msg, 'F', msg_idx, 1) + 1;
            int frames = strtof(msg + end, NULL);
            end = indexOf(msg, 'T', msg_idx, 1) + 1;
            int time = strtof(msg + end, NULL);
            end = indexOf(msg, 'R', msg_idx, 1) + 1;
            gain_r = strtof(msg + end, NULL);
            end = indexOf(msg, 'G', msg_idx, 1) + 1;
            gain_g = strtof(msg + end, NULL);
            end = indexOf(msg, 'B', msg_idx, 1) + 1;
            gain_b = strtof(msg + end, NULL);
            // M30 D1 I2 F1000 T2034 R23.4 G34.24 B94.56
            task_add(delay, interval, frames, TASK_TIMELAPSE_RAW, time, 1, gain_r, gain_g, gain_g, gain_b, 0);
        }
        break;
        case 31:
        {
            task_start();
        }
        break;
        case 32:
        {
            task_stop();
        }
        break;
        case 40:
        {
            LOG_UART("M40 C%d T%d\n", task_get_index(), task_get_total());
        }
        break;
        }
    }
    // capture M10 Txxxxx
    // mode switch M11 Sx
    // set coarse M20 Txxxxx
    // set gain M21 Rxxx.x Gxxx.x Bxxx.x
    // add task M30  Dssss Issss Fxxxx Txxxxx Rxxx.x Gxxx.x Bxxx.x
    // start task M31
    // stop task M32
    // get status M40
}
void service_run()
{
    if(task_get_status() == TASK_IDLE)
    {
        while (USBSerial.available())
        {
            char c = USBSerial.read();
            msg[msg_idx++] = c;
            if (c == '\n')
            {
                service_parse(msg);
                msg_idx = 0;
                memset(msg, 0, 128);
            }
            else
            {
                if (msg_idx > 127)
                {
                    msg_idx = 0;
                }
            }
        }
    }
    server.handleClient();
    delay(1);
}

void service_clear()
{
}