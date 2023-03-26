// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"
#include "sys/time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sensor.h"
#include "sccb.h"
#include "cam_hal.h"
#include "esp_camera.h"
#include "xclk.h"
#include "config.h"
#include "sensor.h"
#include "Arduino.h"
#include "log.h"
#include "sensors/ar0130.h"
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define TAG ""
#else
#include "esp_log.h"
static const char *TAG = "camera";
#endif

typedef struct {
    sensor_t sensor;
    camera_fb_t fb;
} camera_state_t;

static const char *CAMERA_SENSOR_NVS_KEY = "sensor";
static const char *CAMERA_PIXFORMAT_NVS_KEY = "pixformat";
static camera_state_t *s_state = NULL;

// #if CONFIG_IDF_TARGET_ESP32S3 // LCD_CAM module of ESP32-S3 will generate xclk
// #define CAMERA_ENABLE_OUT_CLOCK(v)
// #define CAMERA_DISABLE_OUT_CLOCK()
// #else
#define CAMERA_ENABLE_OUT_CLOCK(v) camera_enable_out_clock((v))
#define CAMERA_DISABLE_OUT_CLOCK() camera_disable_out_clock()
// #endif

typedef struct {
    int (*detect)(int slv_addr, sensor_id_t *id);
    int (*init)(sensor_t *sensor);
} sensor_func_t;

static const sensor_func_t g_sensors[] = {
    {ar0130_detect, ar0130_init},
};
static esp_err_t camera_probe( camera_config_t *config, camera_model_t *out_camera_model)
{
    esp_err_t ret = ESP_OK;
    *out_camera_model = CAMERA_NONE;
    if (s_state != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_state = (camera_state_t *) calloc(sizeof(camera_state_t), 1);
    if (!s_state) {
        return ESP_ERR_NO_MEM;
    }

    if (config->pin_xclk >= 0) {
        LOG_UART("Enabling XCLK output\n");
        CAMERA_ENABLE_OUT_CLOCK(config);
    }

    ret = SCCB_Init(config->pin_sccb_sda, config->pin_sccb_scl);

    if(ret != ESP_OK) {
        LOG_UART( "sccb init err\n");
        return ret;
    }

    if (config->pin_pwdn >= 0) {
        gpio_config_t conf = { 0 };
        conf.pin_bit_mask = 1LL << config->pin_pwdn;
        conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&conf);

        gpio_set_level((gpio_num_t)config->pin_pwdn, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // gpio_set_level((gpio_num_t)config->pin_pwdn, 0);
        // vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    if (config->pin_reset >= 0) {
        gpio_config_t conf = { 0 };
        conf.pin_bit_mask = 1LL << config->pin_reset;
        conf.mode = GPIO_MODE_OUTPUT;
        gpio_config(&conf);

        gpio_set_level((gpio_num_t)config->pin_reset, 0);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level((gpio_num_t)config->pin_reset, 1);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    LOG_UART("Searching for camera address\n");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    uint8_t slv_addr = SCCB_Probe();

    if (slv_addr == 0) {
        ret = ESP_ERR_NOT_FOUND;
        return ret;
    }

    LOG_UART("Detected camera at address=0x%02x\n", slv_addr);
    s_state->sensor.slv_addr = slv_addr;
    s_state->sensor.xclk_freq_hz = config->xclk_freq_hz;

    /**
     * Read sensor ID and then initialize sensor
     * Attention: Some sensors have the same SCCB address. Therefore, several attempts may be made in the detection process
     */
    sensor_id_t *id = &s_state->sensor.id;
    for (size_t i = 0; i < sizeof(g_sensors) / sizeof(sensor_func_t); i++) {
        if (g_sensors[i].detect(slv_addr, id)) {
            camera_sensor_info_t *info = esp_camera_sensor_get_info(id);
            if (NULL != info) {
                *out_camera_model = info->model;
                LOG_UART("Detected %s camera\n", info->name);
                g_sensors[i].init(&s_state->sensor);
                break;
            }
        }
    }

    if (CAMERA_NONE == *out_camera_model) { //If no supported sensors are detected
        LOG_UART( "Detected camera not supported.\n");
        ret = ESP_ERR_NOT_SUPPORTED;
        return ret;
    }

    LOG_UART("Camera PID=0x%02x VER=0x%02x MIDL=0x%02x MIDH=0x%02x\n",
             id->PID, id->VER, id->MIDH, id->MIDL);

    LOG_UART("Doing SW reset of sensor\n");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    return s_state->sensor.reset(&s_state->sensor);
}
esp_err_t ez_camera_init( camera_config_t *config)
{
    //配置模块时钟
    //配置信号引脚
    //HSYNC是否有效置位或清零 LCD_CAM_CAM_VH_DE_MODE_EN；
    //配置正确的接收通道模式和接收数据模式，置位 LCD_CAM_CAM_UPDATE；
    //根据 26.3.4 小节的描述，复位接收控制单元 (Camera_Ctrl) 和 Async Rx FIFO；
    //根据 26.5 小节的描述使能相应的中断；
    //配置 GDMA 接收链表，并在 LCD_CAM->CAM_REC_DATA_BYTELEN 中配置接收数据长度；
    //开始接收数据：在主机模式下，等待从机准备好后，置位 LCD_CAM_CAM_START 开始接收数据；
    //接收数据，并存到 ESP32-S3 存储器的指定地址。最终产生步骤 6 中设置的中断。
    /*
    • LCD_CAM->CAM_HS_INT ：当 Camera 接收行数大于等于 LCD_CAM->CAM_LINE_INT_NUM 配置的值 + 1 即触发此中断。
    • LCD_CAM->CAM_VSYNC_INT ：当 Camera 接收到帧指示信号 VSYNC 即触发此中断。
    */
   /**
    * 寄存器 LCD_CAM_CAM_CTRL_REG 中 LCD_CAM_CAM_CLK_SEL 用于选择时钟源：
        • 0：关闭 CAM 时钟源；
        • 1：选择 XTAL_CLK；
        • 2：选择 PLL_240M_CLK；
        • 3：选择 PLL_160M_CLK
    
        f_CAM_CLK = f_CAM_CLK_S/(N + b/a)
    
    其中，2 <= N <= 256，N 对应为 LCD_CAM_CAM_CTRL_REG 寄存器中 LCD_CAM_CAM_CLKM_DIV_NUM 的值，具体为：
        • LCD_CAM->CAM_CLKM_DIV_NUM = 0 时，N = 256；
        • LCD_CAM->CAM_CLKM_DIV_NUM = 1 时，N = 2；
        • LCD_CAM->CAM_CLKM_DIV_NUM 为其它值时，N = LCD_CAM_CAM_CLKM_DIV_NUM 的值。
        b 对应 LCD_CAM->CAM_CLKM_DIV_B 的值，a 对应 LCD_CAM_CAM_CLKM_DIV_A 的值。对于整数分频，LCD_CAM_CAM_CLKM_DIV_A 和 LCD_CAM_CAM_CLKM_DIV_B 都清零；对于小数分频，LCD_CAM_CAM_CLKM_DIV_B的值应小于 LCD_CAM_CAM_CLKM_DIV_A 的值。
    */
   /*
   主机接收模式:
        CAM_PCLK 输入 像素时钟输入信号
        CAM_CLK 输出 主机时钟输出信号
        CAM_V_SYNC 输入 帧同步输入信号
        CAM_H_SYNC 输入 行同步输入信号
        CAM_H_ENABLE 输入 行有效输入信号
        CAM_Data_in[N:0]2 输入 并行输入数据总线，支持 8/16 位并行数据输入
    */
   /*
    LCD_CAM 模块中各个单元可通过配置下列寄存器进行复位：
        • 接收控制单元 (Camera_Ctrl) 及其格式转换模块 (RGB/YCbCr Converter)：可配置 LCD_CAM->CAM_RESET 进行复位；
        • Async Tx FIFO：可配置 LCD_CAM->LCD_AFIFO_RESET 进行复位；
        • Async Rx FIFO：可配置 LCD_CAM->CAM_AFIFO_RESET 进行复位。
        注意：
        • 上述复位寄存器均为硬件自清，即写 1 后，硬件会自动清零；
        • 在模块和 FIFO 复位之前，需要先配置Camera模块时钟
    */
   /*
    • LCD_CAM->CAM_HS_INT ：当 Camera 接收行数大于等于 LCD_CAM_CAM_LINE_INT_NUM 配置的值 + 1 即触发此中断。
    • LCD_CAM->CAM_VSYNC_INT ：当 Camera 接收到帧指示信号 VSYNC 即触发此中断。
    */
    esp_err_t err;
    err = cam_init(config);

    if (err != ESP_OK) {
        LOG_UART( "Camera init failed with error 0x%x\n", err);
        return err;
    }
    camera_model_t camera_model = CAMERA_NONE;
    err = camera_probe(config, &camera_model);
    if (err != ESP_OK) {
        LOG_UART( "Camera probe failed with error 0x%x(%s)\n", err, esp_err_to_name(err));
        
        return err;
    }

    framesize_t frame_size = (framesize_t) config->frame_size;
    pixformat_t pix_format = (pixformat_t) config->pixel_format;

    if (frame_size > camera_sensor[camera_model].max_size) {
        LOG_UART( "The frame size exceeds the maximum for this sensor, it will be forced to the maximum possible value:%d %d\n",frame_size,camera_sensor[camera_model].max_size);
        frame_size = camera_sensor[camera_model].max_size;
    }
    
    err = cam_config(config, frame_size, s_state->sensor.id.PID);
    if (err != ESP_OK) {
        LOG_UART( "Camera config failed with error 0x%x\n", err);
        return err;
    }

    s_state->sensor.status.framesize = frame_size;
    s_state->sensor.pixformat = pix_format;
    LOG_UART("Setting frame size to %dx%d\n", resolution[frame_size].width, resolution[frame_size].height);
    if (s_state->sensor.set_framesize(&s_state->sensor, frame_size) != 0) {
        LOG_UART( "Failed to set frame size\n");
        err = ESP_ERR_CAMERA_FAILED_TO_SET_FRAME_SIZE;
        return err;
    }
    s_state->sensor.set_pixformat(&s_state->sensor, pix_format);
    s_state->sensor.init_status(&s_state->sensor);

    return ESP_OK;
}

esp_err_t ez_camera_deinit()
{
    esp_err_t ret = cam_deinit();

    return ret;
}

#define FB_GET_TIMEOUT (3600000 / portTICK_PERIOD_MS)

camera_fb_t *esp_camera_fb_get()
{
    LOG_UART("cam start\n");
    sensor_t *s = esp_camera_sensor_get();
    uint16_t w = resolution[s->status.framesize].width>>s->status.binning;
    uint16_t h = resolution[s->status.framesize].height>>s->status.binning;
    cam_start();
    camera_fb_t *fb = cam_take(FB_GET_TIMEOUT);
    //set the frame properties
    if (fb) {
        fb->width = w;
        fb->height = h;
    }
    cam_stop();

    LOG_UART("cam stop\n");
    return fb;
}

void esp_camera_fb_return(camera_fb_t *fb)
{
    cam_give(fb);
}

sensor_t *esp_camera_sensor_get()
{
    if (s_state == NULL) {
        return NULL;
    }
    return &s_state->sensor;
}
