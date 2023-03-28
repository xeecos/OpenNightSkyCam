#pragma once
#include <stdint.h>
#include <stdio.h>
bool fmt2jpg(uint8_t *src, size_t src_len, uint16_t width, uint16_t height, pixformat_t format,uint8_t quality, uint8_t ** out, size_t * out_len);
void convert_line_format(uint8_t *src, pixformat_t format, uint8_t *dst, size_t width, size_t in_channels, size_t line);