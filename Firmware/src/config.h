#pragma once

#define D11          4
#define D10          5
#define D9           6
#define D8           7
#define D7          15
#define D6          16
#define D5          17
#define D4          18
#define D3           8
#define D2           3
#define D1          46
#define D0           9
#define PIXCLK       2
#define XCLK        42
#define HREF        41
#define VSYNC       40
#define SDATA       39
#define SCLK        38
#define EXPOSURE    45

#define CH_IN       12
#define SD_D0       21
#define SD_D1       48
#define SD_D2       11
#define SD_D3       14
#define SD_CMD      13
#define SD_CLK      47
#define KEY_IO       0
#define LED_IO       1
#define PWDN        -1
#define RESET       -1
#define I2C_EN      -1
#define CMOS_BITS   8
#define CMOS_FRAMESIZE FRAMESIZE_MXGA
#define XCLK_FREQ   27000000

#define LCD_CAM_DMA_NODE_BUFFER_MAX_SIZE (2048)

void *_malloc(int count);