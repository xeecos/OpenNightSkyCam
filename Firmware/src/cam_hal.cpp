// Copyright 2010-2020 Espressif Systems (Shanghai) PTE LTD
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
#include <string.h>
#include "esp_heap_caps.h"
#include "ll_cam.h"
#include "cam_hal.h"
#include "config.h"
#include "log.h"
#include <Arduino.h>
#if CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/ets_sys.h"
#endif // ESP_IDF_VERSION_MAJOR
#define ESP_CAMERA_ETS_PRINTF ets_printf

static const char *TAG = "cam_hal";
static cam_obj_t *cam_obj = NULL;

static bool cam_get_next_frame(int *frame_pos)
{
    // LOG_UART("cam_get_next_frame:%d %d\n",*frame_pos,cam_obj->frames[*frame_pos].en);
    if (!cam_obj->frames[*frame_pos].en)
    {
        for (int x = 0; x < cam_obj->frame_cnt; x++)
        {
            if (cam_obj->frames[x].en)
            {
                *frame_pos = x;
                return true;
            }
        }
    }
    else
    {
        return true;
    }
    return false;
}

static bool cam_start_frame(int *frame_pos)
{
    if (cam_get_next_frame(frame_pos))
    {
        if (ll_cam_start(cam_obj, *frame_pos))
        {
            // Vsync the frame manually
            ll_cam_do_vsync(cam_obj);
            uint64_t us = (uint64_t)esp_timer_get_time();
            cam_obj->frames[*frame_pos].fb.timestamp.tv_sec = us / 1000000UL;
            cam_obj->frames[*frame_pos].fb.timestamp.tv_usec = us % 1000000UL;
            return true;
        }
    }
    return false;
}

void IRAM_ATTR ll_cam_send_event(cam_obj_t *cam, cam_event_t cam_event, BaseType_t *HPTaskAwoken)
{
    if (xQueueSendFromISR(cam->event_queue, (void *)&cam_event, HPTaskAwoken) != pdTRUE)
    {
        ll_cam_stop(cam);
        cam->state = CAM_STATE_IDLE;
        ESP_CAMERA_ETS_PRINTF(DRAM_STR("cam_hal: EV-%s-OVF\r\n"), cam_event == CAM_IN_SUC_EOF_EVENT ? DRAM_STR("EOF") : DRAM_STR("VSYNC"));
    }
}

// Copy fram from DMA dma_buffer to fram dma_buffer
static void cam_task(void *arg)
{
    int cnt = 0;
    int frame_pos = 0;
    cam_obj->state = CAM_STATE_IDLE;
    cam_event_t cam_event = (cam_event_t)0;

    xQueueReset(cam_obj->event_queue);

    while (1)
    {
        xQueueReceive(cam_obj->event_queue, (void *)&cam_event, portMAX_DELAY);
        // DBG_PIN_SET(1);
        // LOG_UART("state:%d %d\n", cam_obj->state, cam_event);
        switch (cam_obj->state)
        {

        case CAM_STATE_IDLE:
        {
            if (cam_event == CAM_VSYNC_EVENT)
            {
                // DBG_PIN_SET(1);
                if (cam_start_frame(&frame_pos))
                {
                    cam_obj->frames[frame_pos].fb.len = 0;
                    cam_obj->state = CAM_STATE_READ_BUF;
                }
                cnt = 0;
            }
        }
        break;

        case CAM_STATE_READ_BUF:
        {
            size_t pixels_per_dma = (cam_obj->dma_half_buffer_size * cam_obj->fb_bytes_per_pixel) / (cam_obj->dma_bytes_per_item * cam_obj->in_bytes_per_pixel);

            if (cam_event == CAM_IN_SUC_EOF_EVENT)
            {
                cnt++;
            }
            else if (cam_event == CAM_VSYNC_EVENT)
            {
                LOG_UART("cnt:%d\n", cnt);
                // DBG_PIN_SET(1);
                ll_cam_stop(cam_obj);
                camera_fb_t *frame_buffer_event = &cam_obj->frames[frame_pos].fb;
                if (cnt)
                {
                    cam_obj->frames[frame_pos].en = 0;
                    frame_buffer_event->len = cam_obj->recv_size;
                    // send frame
                    BaseType_t HPTaskAwoken = pdFALSE;
                    xQueueSendFromISR(cam_obj->frame_buffer_queue, (void *)&frame_buffer_event, &HPTaskAwoken);
                    if (HPTaskAwoken == pdTRUE)
                    {
                        portYIELD_FROM_ISR();
                    }
                }

                // if(!cam_start_frame(&frame_pos)){
                cam_obj->state = CAM_STATE_IDLE;
                // } else
                // {
                //     cam_obj->frames[frame_pos].fb.len = 0;
                // }
                cnt = 0;
            }
        }
        break;
        }
        // DBG_PIN_SET(0);
    }
}

static lldesc_t *allocate_dma_descriptors(uint32_t count, uint16_t size, uint8_t *buffer)
{
    lldesc_t *dma = (lldesc_t *)heap_caps_malloc(count * sizeof(lldesc_t), MALLOC_CAP_DMA);
    if (dma == NULL)
    {
        return dma;
    }

    for (int x = 0; x < count; x++)
    {
        dma[x].size = size;
        dma[x].length = 0;
        dma[x].sosf = 0;
        dma[x].eof = 0;
        dma[x].owner = 1;
        dma[x].buf = (buffer + size * x);
        dma[x].empty = (uint32_t)&dma[(x + 1) % count];
    }
    return dma;
}

static esp_err_t cam_dma_config(const camera_config_t *config)
{
    bool ret = ll_cam_dma_sizes(cam_obj,config);
    if (0 == ret)
    {
        return ESP_FAIL;
    }
    cam_obj->dma_node_cnt = (cam_obj->dma_buffer_size) / cam_obj->dma_node_buffer_size; // Number of DMA nodes
    cam_obj->frame_copy_cnt = cam_obj->recv_size / cam_obj->dma_half_buffer_size;       // Number of interrupted copies, ping-pong copy

    // LOG_UART("\nbuffer_size: %d, half_buffer_size: %d, node_buffer_size: %d, node_cnt: %d, total_cnt: %d\n",  cam_obj->dma_buffer_size, cam_obj->dma_half_buffer_size, cam_obj->dma_node_buffer_size, cam_obj->dma_node_cnt, cam_obj->frame_copy_cnt);

    cam_obj->dma_buffer = NULL;
    cam_obj->dma = NULL;

    cam_obj->frames = (cam_frame_t *)heap_caps_calloc(1, cam_obj->frame_cnt * sizeof(cam_frame_t), MALLOC_CAP_DEFAULT);
    // CAM_CHECK(cam_obj->frames != NULL, "frames malloc failed", ESP_FAIL);

    uint8_t dma_align = 0;
    size_t fb_size = cam_obj->fb_size;
    // if (cam_obj->psram_mode)
    {
        dma_align = ll_cam_get_dma_align(cam_obj);
        if (cam_obj->fb_size < cam_obj->recv_size)
        {
            fb_size = cam_obj->recv_size;
        }
    }

    /* Allocate memory for frame buffer */
    size_t alloc_size = fb_size * sizeof(uint8_t) + dma_align;
    uint32_t _caps = MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM;
    for (int x = 0; x < cam_obj->frame_cnt; x++)
    {
        cam_obj->frames[x].dma = NULL;
        cam_obj->frames[x].fb_offset = 0;
        cam_obj->frames[x].en = 0;
        // LOG_UART("Allocating %d Byte frame buffer in %s\n", alloc_size, _caps & MALLOC_CAP_SPIRAM ? "PSRAM" : "OnBoard RAM");
        cam_obj->frames[x].fb.buf = (uint8_t *)heap_caps_malloc(alloc_size, _caps);
        // CAM_CHECK(cam_obj->frames[x].fb.buf != NULL, "frame buffer malloc failed", ESP_FAIL);
        // align PSRAM buffer. TODO: save the offset so proper address can be freed later
        cam_obj->frames[x].fb_offset = dma_align - ((uint32_t)cam_obj->frames[x].fb.buf & (dma_align - 1));
        cam_obj->frames[x].fb.buf += cam_obj->frames[x].fb_offset;
        // LOG_UART("Frame[%d]: Offset: %u, Addr: 0x%08X\n", x, cam_obj->frames[x].fb_offset, (uint32_t)cam_obj->frames[x].fb.buf);
        cam_obj->frames[x].dma = allocate_dma_descriptors(cam_obj->dma_node_cnt, cam_obj->dma_node_buffer_size, cam_obj->frames[x].fb.buf);
        // CAM_CHECK(cam_obj->frames[x].dma != NULL, "frame dma malloc failed", ESP_FAIL);
        cam_obj->frames[x].en = 1;
    }

    return ESP_OK;
}

esp_err_t cam_init(const camera_config_t *config)
{

    esp_err_t ret = ESP_OK;
    cam_obj = (cam_obj_t *)heap_caps_calloc(1, sizeof(cam_obj_t), MALLOC_CAP_DMA);

    cam_obj->swap_data = 0;
    cam_obj->vsync_pin = config->pin_vsync;
    cam_obj->vsync_invert = true;

    ll_cam_set_pin(cam_obj, config);
    ret = ll_cam_config(cam_obj, config);

    // LOG_UART("cam init ok\n");
    return ESP_OK;
}

esp_err_t cam_config(const camera_config_t *config, framesize_t frame_size, uint16_t sensor_pid)
{
    uint16_t w = resolution[frame_size].width;
    uint16_t h = resolution[frame_size].height;
    // CAM_CHECK(NULL != config, "config pointer is invalid", ESP_ERR_INVALID_ARG);
    esp_err_t ret = ESP_OK;

    ret = ll_cam_set_sample_mode(cam_obj, config->xclk_freq_hz, config);
    cam_obj->jpeg_mode = false;
    cam_obj->psram_mode = true;

    cam_obj->frame_cnt = config->fb_count;
    cam_obj->width = w;
    cam_obj->height = h;

    cam_obj->recv_size = cam_obj->width * cam_obj->height * cam_obj->in_bytes_per_pixel;
    cam_obj->fb_size = cam_obj->width * cam_obj->height * cam_obj->fb_bytes_per_pixel;

    ret = cam_dma_config(config);
    cam_obj->event_queue = xQueueCreate(cam_obj->dma_half_buffer_cnt - 1, sizeof(cam_event_t));

    size_t frame_buffer_queue_len = cam_obj->frame_cnt;
    if (config->grab_mode == CAMERA_GRAB_LATEST && cam_obj->frame_cnt > 1)
    {
        frame_buffer_queue_len = cam_obj->frame_cnt - 1;
    }
    cam_obj->frame_buffer_queue = xQueueCreate(frame_buffer_queue_len, sizeof(camera_fb_t *));

    ret = ll_cam_init_isr(cam_obj);

    xTaskCreatePinnedToCore(cam_task, "cam_task", 4096, NULL, configMAX_PRIORITIES - 2, &cam_obj->task_handle, 0);
    // LOG_UART("cam config ok\n");
    return ESP_OK;
}

esp_err_t cam_deinit(void)
{
    if (!cam_obj)
    {
        return ESP_FAIL;
    }

    cam_stop();
    if (cam_obj->task_handle)
    {
        vTaskDelete(cam_obj->task_handle);
    }
    if (cam_obj->event_queue)
    {
        vQueueDelete(cam_obj->event_queue);
    }
    if (cam_obj->frame_buffer_queue)
    {
        vQueueDelete(cam_obj->frame_buffer_queue);
    }
    if (cam_obj->dma)
    {
        free(cam_obj->dma);
    }
    if (cam_obj->dma_buffer)
    {
        free(cam_obj->dma_buffer);
    }
    if (cam_obj->frames)
    {
        for (int x = 0; x < cam_obj->frame_cnt; x++)
        {
            free(cam_obj->frames[x].fb.buf - cam_obj->frames[x].fb_offset);
            if (cam_obj->frames[x].dma)
            {
                free(cam_obj->frames[x].dma);
            }
        }
        free(cam_obj->frames);
    }

    ll_cam_deinit(cam_obj);

    free(cam_obj);
    cam_obj = NULL;
    return ESP_OK;
}

void cam_stop(void)
{
    ll_cam_vsync_intr_enable(cam_obj, false);
    ll_cam_stop(cam_obj);
    sensor_t *s = esp_camera_sensor_get();
    s->take_photo_end(s);
}

void cam_start(void)
{
    ll_cam_vsync_intr_enable(cam_obj, true);
}
cam_obj_t * cam_get_obj()
{
    return cam_obj;
}
camera_fb_t *cam_take(TickType_t timeout)
{
    // LOG_UART("cam take");
    camera_fb_t *output_buffer = NULL;
    TickType_t start = xTaskGetTickCount();
    xQueueReceive(cam_obj->frame_buffer_queue, (void *)&output_buffer, timeout);
    // LOG_UART("cam take finish: %d\n",dma_buffer);
    if (output_buffer)
    {
        // if (cam_obj->in_bytes_per_pixel != cam_obj->fb_bytes_per_pixel)
        // {
        //     // currently this is used only for YUV to GRAYSCALE
        //     output_buffer->len = ll_cam_memcpy(cam_obj, output_buffer->buf, output_buffer->buf, output_buffer->len);
        // }
        return output_buffer;
    }
    else
    {
        LOG_UART("Failed to get the frame on time!");
    }
    return NULL;
}

void cam_give(camera_fb_t *dma_buffer)
{
    for (int x = 0; x < cam_obj->frame_cnt; x++)
    {
        if (&cam_obj->frames[x].fb == dma_buffer)
        {
            cam_obj->frames[x].en = 1;
            break;
        }
    }
}
