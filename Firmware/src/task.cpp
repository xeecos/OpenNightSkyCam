#include "task.h"
#include "esp32-hal-timer.h"
#include "DateTime.h"
#include "capture.h"
#include "Arduino.h"
#include "esp_camera.h"
#include "tf.h"
#include "service.h"
#include "config.h"
#include "log.h"
TASK_STATUS _task_status;
static hw_timer_t *_task_timer = NULL;
task_info _tasks_ring[TASK_RING_SIZE];
int _tasks_ring_pop = 0;
int _tasks_ring_push = 0;
bool _task_running = false;
bool _task_testing = false;
long _task_start_time = 0;
int _task_index = 0;
int _last_coarse = 100;
bool _task_stopping = false;
bool _task_next_capturing = false;
void task_timelapse_start()
{

    // const char *resolution = "800x600";
    // const char *location = "jpgs";
    // const char *fps = "10";
    // const char *out_file = DEFAULT_DESTINATION;
    // int len = 0;

    // File fp = SD.open(out_file, "wb");
    // avi_create(fp, width, height, location, 10);
    // avi_append_test(fp, location, 41);
    // uint32_t fsize = avi_close(fp);
    // LOG_UART("File created: %s (%s)\n", out_file, file_size_to_text(fsize));
}
void IRAM_ATTR task_handle()
{
    if (_task_running)
    {
        int idx = _tasks_ring_pop % TASK_RING_SIZE;
        int count = _tasks_ring[idx].count;
        int delay = _tasks_ring[idx].delay;
        int interval = _tasks_ring[idx].interval;
        long time = millis() - _task_start_time;
        if (count > 0)
        {
            if (_task_status == TASK_IDLE)
            {
                if (delay > 0)
                {
                    if (time >= delay)
                    {
                        _tasks_ring[idx].delay = 0;
                        _task_next_capturing = true;
                        _task_running = false;
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    if (time >= interval)
                    {
                        _task_next_capturing = true;
                        _task_running = false;
                    }
                }
            }
        }
        else
        {
            if (_tasks_ring_pop < _tasks_ring_push)
            {
                _tasks_ring_pop++;
            }
            else
            {
                _task_stopping = true;
                _tasks_ring_pop = 0;
                _tasks_ring_push = 0;
            }
        }
    }
    timerAlarmWrite(_task_timer, 1000, true);
}
void task_init()
{
    #ifdef KEY_IO
    pinMode(KEY_IO,INPUT_PULLUP);
    #endif
    _task_status = TASK_IDLE;
    _task_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(_task_timer, &task_handle, true);
    timerAlarmWrite(_task_timer, 1000, true);
    timerAlarmEnable(_task_timer);
}
void task_add(int delay, int interval, int count, RECORD_MODE mode, int coarse, int fine, float r_gain, float gr_gain, float gb_gain, float b_gain, int resolution)
{
    int idx = _tasks_ring_push % TASK_RING_SIZE;
    _tasks_ring[idx].delay = delay;
    _tasks_ring[idx].interval = interval;
    _tasks_ring[idx].count = count;
    _tasks_ring[idx].total = count;
    _tasks_ring[idx].mode = mode;
    _tasks_ring[idx].coarse = coarse;
    _tasks_ring[idx].fine = fine;
    _tasks_ring[idx].r_gain = r_gain;
    _tasks_ring[idx].gr_gain = gr_gain;
    _tasks_ring[idx].gb_gain = gb_gain;
    _tasks_ring[idx].b_gain = b_gain;
    _tasks_ring[idx].resolution = resolution;
    _tasks_ring_push++;
    
}
char _task_name[64];
char _task_time[64];
void task_start()
{
    memset(_task_name, 0, 64);
    sprintf(_task_name, "%04d%02d%02d/%02d%02d%02d\0", year(), month(), day(), hour(), minute(), second());
    int mode = _tasks_ring[_tasks_ring_pop].mode;
    // if (mode == TASK_TIMELAPSE_MJPEG || mode == TASK_STARTRAILS_MJPEG)
    // {
    //     if (!tf_available())
    //     {
    //         return;
    //     }
    //     char *video_store_url = (char *)malloc(128);
    //     sprintf(video_store_url, "/timelapse/%s.avi\0", task_get_name());
    //     _video = tf_open(video_store_url, "wb");
    //     sensor_t *s = esp_camera_sensor_get();
    //     uint16_t w = resolution[s->status.framesize].width >> s->status.binning;
    //     uint16_t h = resolution[s->status.framesize].height >> s->status.binning;
    //     avi_create(_video, w, h, 25);
    //     free(video_store_url);
    // }
    
    _task_start_time = millis();
    _task_testing = true;
    _task_running = true;
}
char *task_get_name()
{
    return _task_name;
}
char *task_get_timestamp()
{
    
    memset(_task_time, 0, 64);
    sprintf(_task_time, "%02d%02d%02d%02d%02d\0", month(), day(), hour(), minute(), second());
    return _task_time;
}
int task_get_index()
{
    return _tasks_ring[_tasks_ring_pop].total - _tasks_ring[_tasks_ring_pop].count;
}
int task_get_total()
{
    return _tasks_ring[_tasks_ring_pop].total;
}
void task_capture()
{
    _task_start_time = millis();
    sensor_t *s = esp_camera_sensor_get();
    if (_tasks_ring[_tasks_ring_pop].count == _tasks_ring[_tasks_ring_pop].total)
    {
        float r_gain = _tasks_ring[_tasks_ring_pop].r_gain;
        float gr_gain = _tasks_ring[_tasks_ring_pop].gr_gain;
        float gb_gain = _tasks_ring[_tasks_ring_pop].gb_gain;
        float b_gain = _tasks_ring[_tasks_ring_pop].b_gain;
        int fine = _tasks_ring[_tasks_ring_pop].fine;
        int resolution = _tasks_ring[_tasks_ring_pop].resolution;
        
        s->set_agc_gain_custom(s, r_gain, gr_gain, gb_gain, b_gain);
        // ar0130_set_bin(resolution);
    }
    int coarse = _tasks_ring[_tasks_ring_pop].coarse;
    if(coarse>10)
    {
        s->set_aec_exposure(s, coarse, 1);
    }
    else
    {
        double offset = capture_exposure_offset();
        double diff = 1;
        if(_last_coarse<100)
        {
            diff = 1;
            offset /=5;
        }
        else if(_last_coarse<200)
        {
            diff = 2;
        }
        else if(_last_coarse<500)
        {
            diff = 5;
        }
        else if(_last_coarse<1000)
        {
            diff = 10;
        }
        else if(_last_coarse<2000)
        {
            diff = 25;
        }
        else if(_last_coarse<5000)
        {
            diff = 40;
        }
        else if(_last_coarse<10000)
        {
            diff = 80;
        }
        else if(_last_coarse<20000)
        {
            diff = 160;
        }
        double absoft = offset<0?-offset:offset;
        _last_coarse -= offset*((diff));
        if(_last_coarse<1)
        {
            _last_coarse = 1;
        }
        if(_last_coarse>20000)
        {
            _last_coarse = 20000;
        }
        s->set_aec_exposure(s, _last_coarse, 1);
    }
    int mode = _tasks_ring[_tasks_ring_pop].mode;
    _tasks_ring[_tasks_ring_pop].count--;
    _task_status = TASK_CAPTURING;
    s->take_photo(s, false, (fileformat_t)mode);
}
void task_stop()
{
    int mode = _tasks_ring[_tasks_ring_pop].mode;
    // if (mode == TASK_TIMELAPSE_MJPEG || mode == TASK_STARTRAILS_MJPEG)
    // {
    //     avi_close(_video);
    // }
    
    _task_running = false;
    _tasks_ring_pop = 0;
    _tasks_ring_push = 0;
    _tasks_ring[_tasks_ring_pop].total = 0;
    _tasks_ring[_tasks_ring_pop].count = 0;
}
// static char *image_store_url = NULL;
void IRAM_ATTR task_append_data(uint8_t *buf, unsigned long len)
{
    int mode = _tasks_ring[_tasks_ring_pop].mode;
    // if (mode == TASK_STARTRAILS)
    {   
        // if(image_store_url)
        // {
        //     free(image_store_url);
        // }
        char *image_store_url = (char *)malloc(255);
        memset(image_store_url,0,255);
        sprintf(image_store_url, "/images/%s/P0%s.jpg\0", task_get_name(), task_get_timestamp());
        tf_begin_write((const char*)image_store_url);
        tf_write_file(buf, len);
        free(image_store_url);
    }
    // else if (mode == TASK_STARTRAILS_WITH_SEQ)
    // {
    //     char *image_store_url = (char *)malloc(128);
    //     sprintf(image_store_url, "/timelapse/%s/P%05d.jpg\0", task_get_name(), task_get_index());
    //     tf_begin_write(image_store_url);
    //     tf_write_file(buf, len);
    //     tf_end_write();
    //     free(image_store_url);
    // }
    // else if (mode == TASK_STARTRAILS_MJPEG)
    // {
    //     avi_append_data(_video, buf, len);
    //     char *image_store_url = (char *)malloc(128);
    //     sprintf(image_store_url, "/timelapse/%s/P%05d.jpg\0", task_get_name(), task_get_index());
    //     tf_begin_write(image_store_url);
    //     tf_write_file(buf, len);
    //     tf_end_write();
    //     free(image_store_url);
    // }
    // else if (mode == TASK_TIMELAPSE_JPEG)
    // {
    //     char *image_store_url = (char *)malloc(128);
    //     sprintf(image_store_url, "/timelapse/%s/P%05d.jpg\0", task_get_name(), task_get_index());
    //     tf_begin_write(image_store_url);
    //     tf_write_file(buf, len);
    //     tf_end_write();
    //     free(image_store_url);
    // }
    // else if (mode == TASK_TIMELAPSE_MJPEG)
    // {
    //     avi_append_data(_video, buf, len);
    //     char *image_store_url = (char *)malloc(128);
    //     sprintf(image_store_url, "/timelapse/%s/P%05d.jpg\0", task_get_name(), task_get_index());
    //     tf_begin_write(image_store_url);
    //     tf_write_file(buf, len);
    //     tf_end_write();
    //     free(image_store_url);
    // }
}
void task_append_end()
{
    tf_end_write();
    // free(image_store_url);
    // image_store_url = NULL;
}
int task_get_rest()
{
    return _tasks_ring[_tasks_ring_pop].count;
}
TASK_STATUS task_get_status()
{
    return _task_status;
}
void task_set_status(TASK_STATUS status)
{
    _task_status = status;
}
void task_run()
{
    if (_task_stopping)
    {
        _task_stopping = false;
        task_stop();
    }
    #ifdef KEY_IO
    int n = 0;
    pinMode(KEY_IO,INPUT_PULLUP);
    for(int i=0;i<5;i++)
    {
        n += digitalRead(KEY_IO);
        delay(1);
    }
    if(n<2)
    {
        if(_task_running)
        {
            task_stop();
            return;
        }
    }
    #endif
    if (_task_next_capturing)
    {
	    service_turn_off();
        _task_next_capturing = false;
        task_capture();
        _task_running = true;
    }
}
bool task_status()
{
    return _task_running;
}
void task_test()
{
    if(_task_testing)
    {
        _task_testing = false;
        // tf_test();
    }
}