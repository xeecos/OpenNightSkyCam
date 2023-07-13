#include "capture.h"
#include "service.h"
#include "config.h"
#include "cam_hal.h"
#include "esp_camera.h"
#include "fmt2jpg.h"
#include "fmt2bmp.h"
#include "tf.h"
#include "task.h"
#include "log.h"
#include "esp_wifi.h"

#include <SPI.h>
#include "config.h"
#include "esp_jpeg_enc.h"

// #define USE_BMP

static camera_config_t camera_config = {
    .pin_pwdn = PWDN,
    .pin_xclk = XCLK,
    .pin_sccb_sda = SDATA,
    .pin_sccb_scl = SCLK,
    .pin_d11 = D11,
    .pin_d10 = D10,
    .pin_d9 = D9,
    .pin_d8 = D8,
    .pin_d7 = D7,
    .pin_d6 = D6,
    .pin_d5 = D5,
    .pin_d4 = D4,
    .pin_d3 = D3,
    .pin_d2 = D2,
    .pin_d1 = D1,
    .pin_d0 = D0,
    .pin_vsync = VSYNC,
    .pin_href = HREF,
    .pin_pclk = PIXCLK,
    .xclk_freq_hz = XCLK_FREQ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RAW,
    .frame_size = CMOS_FRAMESIZE,
    .bits = CMOS_BITS,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
};
int capture_mode = 3;
bool capture_started = false;
bool previewing = false;
bool __ready = false;
bool __image_ready = false;
double _exposure_offset = 0;
uint8_t *out_bmp_buf;
size_t out_bmp_buf_len;
int out_jpg_buf_len = 0;
uint8_t *out_jpg_buf = NULL;

long capture_index = 1;
bool showing = false;
bool isCapturing = false;
bool isPreviewing = false;
static camera_fb_t *output;
void store_task(void *arg)
{
    sensor_t *s = esp_camera_sensor_get();
    while (1)
    {
        if (task_get_status() == TASK_PROCESSING)
        {
        }
        else
        {
            delay(1);
        }
    }
}
void capture_take(bool previewing)
{
    isCapturing = true;
    isPreviewing = previewing;
}
void IRAM_ATTR capture_task(void *arg)
{
    sensor_t *s = esp_camera_sensor_get();
    // #ifndef USE_BMP
    out_jpg_buf = (uint8_t *)heap_caps_malloc(1024 * 800, MALLOC_CAP_SPIRAM);
    // #endif
    output = (camera_fb_t *)calloc(sizeof(camera_fb_t), 1);
    output->buf = (uint8_t *)heap_caps_malloc(resolution[s->status.framesize].width * resolution[s->status.framesize].height, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // fmt2bmp(resolution[s->status.framesize].width, resolution[s->status.framesize].height, &out_bmp_buf, &out_bmp_buf_len);
    while (1)
    {
        if (isCapturing)
        {
            isCapturing = false;
            s->take_photo(s, isPreviewing, FILEFORMAT_JPEG);
        }
        if (capture_started)
        {
            task_set_status(TASK_CAPTURING);
            camera_fb_t *pic = esp_camera_fb_get();
            output->len = ll_cam_memcpy(cam_get_obj(), output->buf, pic->buf, pic->len);
            capture_started = false;
            esp_camera_fb_return(pic);
            task_set_status(TASK_PROCESSING);
            uint16_t w = resolution[s->status.framesize].width;
            uint16_t h = resolution[s->status.framesize].height;

            // if (previewing)
            {
                LOG_UART("JPEG Encode:%d\n", esp_camera_sensor_get()->status.quality);
                long t = millis();

                jpeg_enc_info_t info = DEFAULT_JPEG_ENC_CONFIG();
                info.width = w;
                info.height = h;
                info.src_type = JPEG_RAW_TYPE_RGB888;
                info.quality = esp_camera_sensor_get()->status.quality;
                if (info.src_type == JPEG_RAW_TYPE_GRAY)
                {
                    void *el = jpeg_enc_open(&info);
                    jpeg_enc_process(el, output->buf, output->len, out_jpg_buf, 1024 * 800, &out_jpg_buf_len);
                    jpeg_enc_close(el);
                }
                else if (info.src_type == JPEG_RAW_TYPE_RGB888)
                {
                    uint8_t *rgb = (uint8_t *)ps_malloc(w * h * 3);
                    if(rgb)
                    {
                        for (int line = 0; line < h; line++)
                        {
                            convert_line_format(output->buf, PIXFORMAT_RAW, rgb + line * w * 3, w, 3, line);
                        }
                        void *el = jpeg_enc_open(&info);
                        jpeg_enc_process(el, rgb, w * h * 3, out_jpg_buf, 1024 * 800, &out_jpg_buf_len);
                        jpeg_enc_close(el);
                        free(rgb);
                    }
                }

                LOG_UART("Finish JPEG Encode:%d size:%d\n", millis() - t, out_jpg_buf_len);
                __image_ready = true;
            }
            __ready = true;
        }
        delay(1);
    }
    free(out_bmp_buf);
}
void capture_init()
{
    ez_camera_init(&camera_config);
    xTaskCreatePinnedToCore(capture_task, "capture_task", 8192, NULL, 10, NULL, 1);
    // xTaskCreatePinnedToCore(store_task, "store_task", 8192, NULL, 10, NULL, 1);
}
void capture_start(bool isPreview, int mode)
{
    capture_mode = mode;
    previewing = isPreview;
    capture_started = true;
    showing = false;
}
void capture_stop()
{
    capture_started = false;
}
void capture_run()
{
    if (__ready)
    {
        if (previewing)
        {
            if (capture_mode == FILEFORMAT_UART)
            {
                LOG_UART("M10 L%d ", out_jpg_buf_len);
                for (int i = 0; i < out_jpg_buf_len; i++)
                {
                    USBSerial.write(out_jpg_buf[i]);
                }
                USBSerial.write(out_jpg_buf, out_jpg_buf_len);
                LOG_UART("\n");
                LOG_UART("end\n");
            }
            else
            {
                if(service_is_requesting_image())
                {
                    LOG_UART("jpg start\n");
                    service_send_image((char*)out_jpg_buf, out_jpg_buf_len);
                    LOG_UART("jpg end\n");
                }
                else
                {
                    #ifndef ENABLE_SOCKET_SERVER
                    tf_begin_write("/preview/latest.jpg");
                    tf_write_file(out_jpg_buf, out_jpg_buf_len);
                    tf_end_write();
                    #endif
                }
                service_turn_on();
            }

            previewing = false;
            isPreviewing = false;
        }
        else
        {
#ifndef ENABLE_SOCKET_SERVER
            if ((capture_mode == TASK_STARTRAILS && task_get_rest() == 0) || capture_mode != TASK_STARTRAILS)
            {
#ifdef USE_BMP
                LOG_UART("bmp start\n");
                long t = millis();
                task_append_data(out_bmp_buf, out_bmp_buf_len);
                tf_write_file(output->buf, output->len);
                task_append_end();

                capture_calc_exposure(output->buf, output->width, output->height);

                LOG_UART("bmp written: %d\n", millis() - t);
#else
                task_append_data(out_jpg_buf, out_jpg_buf_len);
                task_append_end();
#endif
                if (!task_status())
                {
                    service_turn_on();
                }
            }
#endif
        }
        task_set_status(TASK_IDLE);
        __ready = false;
    }
}

bool capture_ready()
{
    bool tmp = __image_ready;
    if (__image_ready)
    {
        __image_ready = false;
    }
    return tmp;
}
unsigned char *capture_get()
{
    return (unsigned char *)out_jpg_buf;
}
int capture_length()
{
    return out_jpg_buf_len;
}
void capture_calc_exposure(unsigned char *buf, int w, int h)
{
    _exposure_offset = 0;
    int wall_half = 500;
    int hall_half = 300;
    int wall = (w - wall_half * 2) / 5;
    int hall = (h - hall_half * 2) / 5;
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            int level = buf[(y * hall + hall_half) * w + (x * wall + wall_half)];
            _exposure_offset += (level - 64.0);
        }
    }
    _exposure_offset /= 25.0;
    LOG_UART("exp offset:%.2f\n", _exposure_offset);
}
double capture_exposure_offset()
{
    return _exposure_offset;
}