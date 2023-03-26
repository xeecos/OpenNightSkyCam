#include <stdio.h>
#include "sensor.h"

const camera_sensor_info_t camera_sensor[CAMERA_MODEL_MAX] = {
    // The sequence must be consistent with camera_model_t
    {CAMERA_AR0130, "AR0130", AR0130_SCCB_ADDR, AR0130_PID, FRAMESIZE_MXGA, false},
    {CAMERA_AR0237, "AR0237", AR0237_SCCB_ADDR, AR0237_PID, FRAMESIZE_FHD, false},
    {CAMERA_IMX290, "IMX290", IMX290_SCCB_ADDR, IMX290_PID, FRAMESIZE_FHD, false},
    {CAMERA_OV2640, "OV2640", OV2640_SCCB_ADDR, OV2640_PID, FRAMESIZE_UXGA, false},
    // {CAMERA_MT9M001, "MT9M001", MT9M001_SCCB_ADDR, MT9M001_PID, FRAMESIZE_SXGA, false},
    {CAMERA_MT9P001, "MT9P001", MT9P001_SCCB_ADDR, MT9P001_PID, FRAMESIZE_QXGA, false},
    {CAMERA_BF3003, "BF3003", BF3003_SCCB_ADDR, BF3003_PID, FRAMESIZE_VGA, false},
    {CAMERA_MT9V034, "MT9V034", MT9V034_SCCB_ADDR, MT9V034_PID, FRAMESIZE_VGA, false},
    // {CAMERA_SC200AI, "SC200AI", SC200AI_SCCB_ADDR, SC200AI_PID, FRAMESIZE_FHD, false},
};

const resolution_info_t resolution[FRAMESIZE_INVALID] = {
    {   96,   96, ASPECT_RATIO_1X1   }, /* 96x96 */
    {  160,  120, ASPECT_RATIO_4X3   }, /* QQVGA */
    {  176,  144, ASPECT_RATIO_5X4   }, /* QCIF  */
    {  240,  176, ASPECT_RATIO_4X3   }, /* HQVGA */
    {  240,  240, ASPECT_RATIO_1X1   }, /* 240x240 */
    {  320,  240, ASPECT_RATIO_4X3   }, /* QVGA  */
    {  400,  296, ASPECT_RATIO_4X3   }, /* CIF   */
    {  480,  320, ASPECT_RATIO_3X2   }, /* HVGA  */
    {  640,  480, ASPECT_RATIO_4X3   }, /* VGA   */
    {  800,  600, ASPECT_RATIO_4X3   }, /* SVGA  */
    { 1024,  768, ASPECT_RATIO_4X3   }, /* XGA   */
    { 1280,  720, ASPECT_RATIO_16X9  }, /* HD    */
    { 1280,  960, ASPECT_RATIO_4X3   }, /* MXGA */
    { 1280, 1024, ASPECT_RATIO_5X4   }, /* SXGA  */
    { 1600, 1200, ASPECT_RATIO_4X3   }, /* UXGA  */
    // 3MP Sensors
    { 1920, 1080, ASPECT_RATIO_16X9  }, /* FHD   */
    {  720, 1280, ASPECT_RATIO_9X16  }, /* Portrait HD   */
    {  864, 1536, ASPECT_RATIO_9X16  }, /* Portrait 3MP   */
    { 2048, 1536, ASPECT_RATIO_4X3   }, /* QXGA  */
    // 5MP Sensors
    { 2560, 1440, ASPECT_RATIO_16X9  }, /* QHD    */
    { 2560, 1600, ASPECT_RATIO_16X10 }, /* WQXGA  */
    { 1088, 1920, ASPECT_RATIO_9X16  }, /* Portrait FHD   */
    { 2560, 1920, ASPECT_RATIO_4X3   }, /* QSXGA  */
};

camera_sensor_info_t *esp_camera_sensor_get_info(sensor_id_t *id)
{
    for (int i = 0; i < CAMERA_MODEL_MAX; i++) {
        if (id->PID == camera_sensor[i].pid) {
            return (camera_sensor_info_t *)&camera_sensor[i];
        }
    }
    return NULL;
}