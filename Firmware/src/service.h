#pragma once
void service_init();
void service_run();
const char* service_get_ip();
void service_clear();
void service_reconnect();
void service_turn_off();
void service_turn_on();
bool service_is_requesting_image();
void service_send_image(char*buf, int len);