#pragma once
#include <stdint.h>
typedef enum
{
    TASK_TIMELAPSE_JPEG = 1,
    TASK_TIMELAPSE_RAW,
    TASK_TIMELAPSE_MJPEG,
    TASK_STARTRAILS,
    TASK_STARTRAILS_WITH_SEQ,
    TASK_STARTRAILS_MJPEG,
}RECORD_MODE;
typedef enum
{
    TASK_IDLE,
    TASK_CAPTURING,
    TASK_PROCESSING,
    TASK_STORING
}TASK_STATUS;
struct task_info
{
    int delay;
    int interval;
    int count;
    int total;
    RECORD_MODE mode;
    int coarse;
    int fine;
    float gr_gain;
    float gb_gain;
    float r_gain;
    float b_gain;
    int resolution;
};
#define TASK_RING_SIZE 8
void task_init();
void task_add(int delay, int interval, int count, RECORD_MODE mode, int coarse, int fine, float r_gain, float gr_gain, float gb_gain, float b_gain, int resolution);
void task_start();
void task_capture();
void task_stop();
char* task_get_name();
int task_get_index();
int task_get_total();
int task_get_rest();
TASK_STATUS task_get_status();
void task_set_status(TASK_STATUS status);
void task_append_data(uint8_t* buf,unsigned long len);
void task_append_end();
void task_run();
bool task_status();
void task_test();