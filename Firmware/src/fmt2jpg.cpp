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
#include <stddef.h>
#include <string.h>
#include "esp_attr.h"
#include "soc/efuse_reg.h"
#include "esp_heap_caps.h"
#include "task.h"
#include "jpge.h"
#include "sensor.h"
#include "fmt2jpg.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "log.h"
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define TAG "to_jpg"
#else
#include "esp_log.h"
#include "ar0130.h"
static const char *TAG = "to_jpg";
#endif

IRAM_ATTR void convert_line_format(uint8_t *src, pixformat_t format, uint8_t *dst, size_t width, size_t in_channels, size_t line)
{
    sensor_t *s = esp_camera_sensor_get();
    uint16_t w = resolution[s->status.framesize].width>>s->status.binning;
    uint16_t h = resolution[s->status.framesize].height>>s->status.binning;
    int i = 0, o = 0, l = w;
    uint8_t r = 0, g = 0, b = 0;
    if(format==PIXFORMAT_GRAYSCALE)
    {
        memcpy(dst, src + line * width, width);
    }
    else if(format==PIXFORMAT_RAW)
    {
        int ll = l*2;
        if (line == 0)
        {
            r = src[1 + ll];
            g = src[0 + ll];
            b = src[l + ll];
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
            for (int j = 1; j < l - 1; j++)
            {
                if ((j & 1) == 0)
                {
                    r = (src[j + 1 + ll] + src[j - 1 + ll]) >> 1;
                    g = src[j + ll];
                    b = (src[j + l + ll]);
                }
                else
                {
                    r = src[j + ll];
                    g = (src[j - 1 + ll] + src[j + 1 + ll]) >> 1; //+src[j+(line+1)*l]+src[j+(line-1)*l]
                    b = (src[j - 1 + l + ll] + src[j + 1 + l + ll]) >> 1;
                }
                dst[o++] = r;
                dst[o++] = g;
                dst[o++] = b;
            }
            int end_idx = l - 1;
            r = src[end_idx + ll];
            g = (src[end_idx + l + ll] + src[end_idx - 1 + ll]) >> 1;
            b = src[end_idx - 1 + l + ll];
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
        }
        else if (line == h - 1)
        {
            r = src[1 + (line - 1) * l];
            g = (src[(line - 1) * l] + src[1 + (line)*l]) >> 1;
            b = src[(line)*l];
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
            for (int j = 1; j < l - 1; j++)
            {
                r = (src[j + (line + 1) * l] + src[j + (line - 1) * l]) >> 1;
                g = src[j + (line)*l];
                b = (src[j + 1 + line * l] + src[j - 1 + line * l]) >> 1;

                if ((j & 1) == 0)
                {
                    r = (src[j - 1 + (line - 1) * l] + src[j + 1 + (line - 1) * l]) >> 1;
                    g = (src[j + (line - 1) * l]);
                    b = src[j + (line)*l];
                }
                else
                {
                    r = src[j + (line - 1) * l];
                    g = src[j + (line)*l];
                    b = (src[j + 1 + line * l] + src[j - 1 + line * l]) >> 1;
                }
                dst[o++] = r;
                dst[o++] = g;
                dst[o++] = b;
            }
            int end_idx = l - 1;
            r = src[end_idx + (line - 1) * l];
            g = src[end_idx + (line)*l];
            b = src[end_idx - 1 + line * l];
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
        }
        else
        {
            if ((line & 1) == 0)
            {
                r = src[1 + line * l];
                g = src[(line)*l];
                b = (src[(line + 1) * l] + src[(line - 1) * l]) >> 1;
            }
            else
            {
                r = (src[1 + (line - 1) * l] + src[1 + (line + 1) * l]) >> 1;
                g = (src[(line + 1) * l] + src[(line - 1) * l]) >> 1;
                b = src[(line)*l];
            }
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
            for (int j = 1; j < l - 1; j++)
            {
                if ((line & 1) == 0)
                {
                    if ((j & 1) == 0)
                    {
                        r = (src[j + 1 + line * l] + src[j - 1 + line * l]) >> 1;
                        g = src[j + (line)*l];
                        b = (src[j + (line + 1) * l] + src[j + (line - 1) * l]) >> 1;
                    }
                    else
                    {
                        r = src[j + (line)*l];
                        g = (src[j - 1 + (line)*l] + src[j + 1 + (line)*l]) >> 1; 
                        b = (src[j - 1 + (line - 1) * l] + src[j - 1 + (line + 1) * l] + src[j + 1 + (line - 1) * l] + src[j + 1 + (line + 1) * l]) >> 2;
                    }
                }
                else
                {
                    if ((j & 1) == 0)
                    {
                        r = (src[j - 1 + (line - 1) * l] + src[j - 1 + (line + 1) * l] + src[j + 1 + (line - 1) * l] + src[j + 1 + (line + 1) * l]) >> 2;
                        g = (src[j + (line + 1) * l] + src[j + (line - 1) * l]) >> 1;
                        b = src[j + (line)*l];
                    }
                    else
                    {
                        r = (src[j + (line + 1) * l] + src[j + (line - 1) * l]) >> 1;
                        g = src[j + (line)*l];
                        b = (src[j + 1 + line * l] + src[j - 1 + line * l]) >> 1;
                    }
                }
                dst[o++] = r;
                dst[o++] = g;
                dst[o++] = b;
            }
            int end_idx = l - 1;
            if ((line & 1) == 0)
            {
                r = src[end_idx + (line)*l];
                g = (src[end_idx + (line + 1) * l] + src[end_idx - 1 + (line)*l] + src[end_idx + (line - 1) * l]) / 3;
                b = (src[end_idx - 1 + (line - 1) * l] + src[end_idx - 1 + (line + 1) * l]) >> 1;
            }
            else
            {
                r = (src[end_idx + (line + 1) * l] + src[end_idx + (line - 1) * l]) >> 1;
                g = src[end_idx + (line)*l];
                b = src[end_idx - 1 + line * l];
            }
            dst[o++] = r;
            dst[o++] = g;
            dst[o++] = b;
        }
    }
}

bool convert_image(uint8_t *src, uint16_t width, uint16_t height, pixformat_t format, uint8_t quality, jpge::output_stream *dst_stream)
{
    int num_channels = 3;
    jpge::subsampling_t subsampling = jpge::H2V2;

    if(format == PIXFORMAT_GRAYSCALE)
    {
        num_channels = 1;
        subsampling = jpge::Y_ONLY;
    }

    if (!quality)
    {
        quality = 1;
    }
    else if (quality > 100)
    {
        quality = 100;
    }

    jpge::params comp_params = jpge::params();
    comp_params.m_subsampling = subsampling;
    comp_params.m_quality = quality;

    jpge::jpeg_encoder dst_image;

    if (!dst_image.init(dst_stream, width, height, num_channels, comp_params))
    {
        return false;
    }

    uint8_t *line = (uint8_t *)heap_caps_malloc(width * num_channels, MALLOC_CAP_DMA);
    if (!line)
    {
        return false;
    }
    for (int i = 0; i < height; i++)
    {
        convert_line_format(src, format, line, width, num_channels, i);
        if (!dst_image.process_scanline(line))
        {
            free(line);
            return false;
        }
    }
    free(line);

    if (!dst_image.process_scanline(NULL))
    {
        LOG_UART( "JPG image finish failed");
        return false;
    }
    dst_image.deinit();
    return true;
}

class memory_stream : public jpge::output_stream
{
protected:
    uint8_t *out_buf;
    size_t max_len, index;

public:
    memory_stream(void *pBuf, uint buf_size) : out_buf(static_cast<uint8_t *>(pBuf)), max_len(buf_size), index(0) {}

    virtual ~memory_stream() {}

    virtual bool put_buf(const void *pBuf, int len)
    {
        if (!pBuf)
        {
            // end of image
            return true;
        }
        if ((size_t)len > (max_len - index))
        {
            LOG_UART( "JPG output overflow: %d bytes", len - (max_len - index));
            len = max_len - index;
        }
        if (len)
        {
            memcpy(out_buf + index, pBuf, len);
            index += len;
        }
        return true;
    }

    virtual size_t get_size() const
    {
        return index;
    }
};

uint8_t *jpg_buf = NULL; //(uint8_t *)ps_malloc(jpg_buf_len);
bool fmt2jpg(uint8_t *src, size_t src_len, uint16_t width, uint16_t height, pixformat_t format, uint8_t quality, uint8_t **out, size_t *out_len)
{
    // todo: allocate proper buffer for holding JPEG data
    // this should be enough for CIF frame size
    int jpg_buf_len = 800 * 1024;

    if (jpg_buf == NULL)
    {
        jpg_buf = (uint8_t *)heap_caps_malloc(jpg_buf_len,MALLOC_CAP_SPIRAM);
    }
    memory_stream dst_stream(jpg_buf, jpg_buf_len);

    if (!convert_image(src, width, height, format, quality, &dst_stream))
    {
        free(jpg_buf);
        jpg_buf = NULL;
        return false;
    }

    *out = jpg_buf;
    *out_len = dst_stream.get_size();
    return true;
}
