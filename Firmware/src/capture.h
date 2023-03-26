#pragma once
void capture_init();
void capture_start(bool isPreview=false,int mode=1);
void capture_stop();
void capture_take(bool previewing=true);
void capture_run();
bool capture_ready();
unsigned char* capture_get();
void capture_calc_exposure(unsigned char*buf, int w, int h);
double capture_exposure_offset();
int capture_length();