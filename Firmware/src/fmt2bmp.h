#pragma once
#include <stdint.h>
#include <string.h>
/**
 * @brief Convert image buffer to BMP buffer
 *
 * @param src       Source buffer in JPEG, RGB565, RGB888, YUYV or GRAYSCALE format
 * @param src_len   Length in bytes of the source buffer
 * @param width     Width in pixels of the source image
 * @param height    Height in pixels of the source image
 * @param format    Format of the source image
 * @param out       Pointer to be populated with the address of the resulting buffer
 * @param out_len   Pointer to be populated with the length of the output buffer
 *
 * @return true on success
 */
bool fmt2bmp(uint16_t width, uint16_t height, uint8_t ** out, size_t * out_len);
