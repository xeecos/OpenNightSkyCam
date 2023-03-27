#ifndef __SENSOR_H__
#define __SENSOR_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        AR0130_PID = 0x2402,
    } camera_pid_t;

    typedef enum
    {
        CAMERA_AR0130,
        CAMERA_MODEL_MAX,
        CAMERA_NONE,
    } camera_model_t;

    typedef enum
    {
        AR0130_SCCB_ADDR = 0x10,
    } camera_sccb_addr_t;

    typedef enum
    {
        PIXFORMAT_RGB565,    // 2BPP/RGB565
        PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
        PIXFORMAT_RAW,       // RAW
        PIXFORMAT_JPEG,       // JPEG
    } pixformat_t;

    typedef enum
    {
        FILEFORMAT_JPEG, // JPEG
        FILEFORMAT_DNG,  // RAW
        FILEFORMAT_BIN,  // BIN
        FILEFORMAT_UART
    } fileformat_t;

    typedef enum
    {
        FRAMESIZE_96X96,   // 96x96
        FRAMESIZE_QQVGA,   // 160x120
        FRAMESIZE_QCIF,    // 176x144
        FRAMESIZE_HQVGA,   // 240x176
        FRAMESIZE_240X240, // 240x240
        FRAMESIZE_QVGA,    // 320x240
        FRAMESIZE_CIF,     // 400x296
        FRAMESIZE_HVGA,    // 480x320
        FRAMESIZE_VGA,     // 640x480
        FRAMESIZE_SVGA,    // 800x600
        FRAMESIZE_XGA,     // 1024x768
        FRAMESIZE_HD,      // 1280x720
        FRAMESIZE_MXGA,    // 1280x960
        FRAMESIZE_SXGA,    // 1280x1024
        FRAMESIZE_UXGA,    // 1600x1200
        // 3MP Sensors
        FRAMESIZE_FHD,   // 1920x1080
        FRAMESIZE_P_HD,  //  720x1280
        FRAMESIZE_P_3MP, //  864x1536
        FRAMESIZE_QXGA,  // 2048x1536
        // 5MP Sensors
        FRAMESIZE_QHD,   // 2560x1440
        FRAMESIZE_WQXGA, // 2560x1600
        FRAMESIZE_P_FHD, // 1080x1920
        FRAMESIZE_QSXGA, // 2560x1920
        FRAMESIZE_INVALID
    } framesize_t;

    typedef struct
    {
        const camera_model_t model;
        const char *name;
        const camera_sccb_addr_t sccb_addr;
        const camera_pid_t pid;
        const framesize_t max_size;
        const bool support_jpeg;
    } camera_sensor_info_t;

    typedef enum
    {
        ASPECT_RATIO_4X3,
        ASPECT_RATIO_3X2,
        ASPECT_RATIO_16X10,
        ASPECT_RATIO_5X3,
        ASPECT_RATIO_16X9,
        ASPECT_RATIO_21X9,
        ASPECT_RATIO_5X4,
        ASPECT_RATIO_1X1,
        ASPECT_RATIO_9X16
    } aspect_ratio_t;

    typedef enum
    {
        GAINCEILING_2X,
        GAINCEILING_4X,
        GAINCEILING_8X,
        GAINCEILING_16X,
        GAINCEILING_32X,
        GAINCEILING_64X,
        GAINCEILING_128X,
    } gainceiling_t;

    typedef struct
    {
        uint16_t max_width;
        uint16_t max_height;
        uint16_t start_x;
        uint16_t start_y;
        uint16_t end_x;
        uint16_t end_y;
        uint16_t offset_x;
        uint16_t offset_y;
        uint16_t total_x;
        uint16_t total_y;
    } ratio_settings_t;

    typedef struct
    {
        const uint16_t width;
        const uint16_t height;
        const aspect_ratio_t aspect_ratio;
    } resolution_info_t;

    // Resolution table (in sensor.c)
    extern const resolution_info_t resolution[];
    // camera sensor table (in sensor.c)
    extern const camera_sensor_info_t camera_sensor[];

    typedef struct
    {
        uint8_t MIDH;
        uint8_t MIDL;
        uint16_t PID;
        uint8_t VER;
    } sensor_id_t;

    typedef struct
    {
        framesize_t framesize; // 0 - 10
        bool scale;
        int8_t binning;
        int8_t binning_mode;
        uint16_t vstart;
        uint16_t hstart;
        uint8_t quality;   // 1 - 100
        int8_t brightness; //-2 - 2
        int8_t contrast;   //-2 - 2
        int8_t saturation; //-2 - 2
        int8_t sharpness;  //-2 - 2
        uint8_t denoise;
        uint8_t special_effect; // 0 - 6
        uint8_t wb_mode;        // 0 - 4
        uint8_t awb;
        uint8_t awb_gain;
        uint8_t aec;
        uint8_t aec2;
        int8_t ae_level;    //-2 - 2
        uint16_t aec_value; // 0 - 1200
        uint8_t agc;
        uint8_t agc_gain;    // 0 - 30
        uint8_t gainceiling; // 0 - 6
        uint8_t bpc;
        uint8_t wpc;
        uint8_t raw_gma;
        uint8_t lenc;
        uint8_t hmirror;
        uint8_t vflip;
        uint8_t dcw;
        uint8_t colorbar;
        uint8_t blc;
        uint16_t coarse;
        uint16_t fine;
    } camera_status_t;

    typedef struct _sensor sensor_t;
    typedef struct _sensor
    {
        sensor_id_t id;   // Sensor ID.
        uint8_t slv_addr; // Sensor I2C slave address.
        pixformat_t pixformat;
        camera_status_t status;
        int xclk_freq_hz;

        // Sensor function pointers
        int (*init_status)(sensor_t *sensor);
        int (*reset)(sensor_t *sensor); // Reset the configuration of the sensor, and return ESP_OK if reset is successful
        int (*set_pixformat)(sensor_t *sensor, pixformat_t pixformat);
        int (*set_framesize)(sensor_t *sensor, framesize_t framesize);
        int (*take_photo)(sensor_t *sensor, bool preview, fileformat_t mode);
        int (*take_photo_end)(sensor_t *sensor);
        int (*set_binning)(sensor_t *sensor, uint16_t binning);
        int (*set_vstart)(sensor_t *sensor, uint16_t vstart);
        int (*set_hstart)(sensor_t *sensor, uint16_t hstart);
        int (*set_hblank)(sensor_t *sensor, int hblank);
        int (*set_vblank)(sensor_t *sensor, int vblank);
        int (*set_binning_mode)(sensor_t *sensor, int binning_mode);
        int (*set_contrast)(sensor_t *sensor, int level);
        int (*set_brightness)(sensor_t *sensor, int level);
        int (*set_saturation)(sensor_t *sensor, int level);
        int (*set_sharpness)(sensor_t *sensor, int level);
        int (*set_denoise)(sensor_t *sensor, int level);
        int (*set_gainceiling)(sensor_t *sensor, gainceiling_t gainceiling);
        int (*set_quality)(sensor_t *sensor, int quality);
        int (*set_colorbar)(sensor_t *sensor, int enable);
        int (*set_whitebal)(sensor_t *sensor, int enable);
        int (*set_gain_ctrl)(sensor_t *sensor, int enable);
        int (*set_exposure_ctrl)(sensor_t *sensor, int enable);
        int (*set_hmirror)(sensor_t *sensor, int enable);
        int (*set_vflip)(sensor_t *sensor, int enable);
        int (*set_blc)(sensor_t *sensor, int enable);

        int (*set_aec2)(sensor_t *sensor, int enable);
        int (*set_awb_gain)(sensor_t *sensor, int enable);
        int (*set_agc_gain)(sensor_t *sensor, float gain_level);
        int (*set_agc_gain_custom)(sensor_t *sensor, float gain_r, float gain_gr, float gain_gb, float gain_b);

        int (*set_aec_exposure)(sensor_t *sensor, uint16_t coarse_value, uint16_t fine_value);

        int (*set_special_effect)(sensor_t *sensor, int effect);
        int (*set_wb_mode)(sensor_t *sensor, int mode);
        int (*set_ae_level)(sensor_t *sensor, int level);

        int (*set_dcw)(sensor_t *sensor, int enable);
        int (*set_bpc)(sensor_t *sensor, int enable);
        int (*set_wpc)(sensor_t *sensor, int enable);

        int (*set_raw_gma)(sensor_t *sensor, int enable);
        int (*set_lenc)(sensor_t *sensor, int enable);

        int (*get_reg)(sensor_t *sensor, int reg, int mask);
        void (*set_reg)(sensor_t *sensor, uint16_t reg, uint16_t value);
        int (*set_res_raw)(sensor_t *sensor, int startX, int startY, int endX, int endY, int offsetX, int offsetY, int totalX, int totalY, int outputX, int outputY, bool scale, bool binning);
        int (*set_pll)(sensor_t *sensor, int bypass, int mul, int sys, int root, int pre, int seld5, int pclken, int pclk);
        int (*set_xclk)(sensor_t *sensor, int xclk);
    } sensor_t;

    camera_sensor_info_t *esp_camera_sensor_get_info(sensor_id_t *id);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H__ */