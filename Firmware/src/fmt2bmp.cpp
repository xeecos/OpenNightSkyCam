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
#include "fmt2bmp.h"
#include "soc/efuse_reg.h"
#include "esp_heap_caps.h"
#include "sdkconfig.h"

#include "esp_system.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define TAG ""
#else
#include "esp_log.h"
static const char* TAG = "to_bmp";
#endif

static const int BMP_HEADER_LEN = 54;

typedef struct {
    uint32_t filesize;
    uint32_t reserved;
    uint32_t fileoffset_to_pixelarray;
    uint32_t dibheadersize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsperpixel;
    uint32_t compression;
    uint32_t imagesize;
    uint32_t ypixelpermeter;
    uint32_t xpixelpermeter;
    uint32_t numcolorspallette;
    uint32_t mostimpcolor;
} bmp_header_t;


bool fmt2bmp(uint16_t width, uint16_t height, uint8_t ** out, size_t * out_len)
{

    *out = NULL;
    *out_len = 0;

    int pix_count = width*height;

    // With BMP, 8-bit greyscale requires a palette.
    // For a 640x480 image though, that's a savings
    // over going RGB-24.
    int bpp =  1;
    int palette_size = 4 * 256;
    size_t out_size =  BMP_HEADER_LEN + palette_size;
    uint8_t * out_buf = (uint8_t *)heap_caps_malloc(out_size, MALLOC_CAP_SPIRAM);
    if(!out_buf) {
        ESP_LOGE(TAG, "_malloc failed! %u", out_size);
        return false;
    }

    out_buf[0] = 'B';
    out_buf[1] = 'M';
    bmp_header_t * bitmap  = (bmp_header_t*)&out_buf[2];
    bitmap->reserved = 0;
    bitmap->filesize = out_size;
    bitmap->fileoffset_to_pixelarray = BMP_HEADER_LEN + palette_size;
    bitmap->dibheadersize = 40;
    bitmap->width = width;
    bitmap->height = -height;//set negative for top to bottom
    bitmap->planes = 1;
    bitmap->bitsperpixel = bpp * 8;
    bitmap->compression = 0;
    bitmap->imagesize = pix_count * bpp;
    bitmap->ypixelpermeter = 0x0B13 ; //2835 , 72 DPI
    bitmap->xpixelpermeter = 0x0B13 ; //2835 , 72 DPI
    bitmap->numcolorspallette = 0;
    bitmap->mostimpcolor = 0;
    uint8_t * palette_buf = out_buf + BMP_HEADER_LEN;
    uint8_t * pix_buf = palette_buf + palette_size;
    if (palette_size > 0) {
        // Grayscale palette
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < 3; ++j) {
                *palette_buf = i;
                palette_buf++;
            }
            // Reserved / alpha channel.
            *palette_buf = 0;
            palette_buf++;
        }
    }
    *out = out_buf;
    *out_len = out_size;
    return true;
}