#pragma once
#include <stdint.h>
void global_init();
bool global_has_name();
void global_set_name(const char *name);
void global_set_ssid(const char *ssid);
const char *global_get_ssid();
const char *global_get_password();
bool global_has_ssid();
bool global_has_password();
void global_set_password(const char *password);
bool global_has_exposure();
uint32_t global_get_coarse();
void global_set_coarse(uint32_t v);
void global_store_exposure();
uint16_t global_get_fine();
void global_set_fine(uint16_t v);
bool global_has_gain();
int global_get_r_gain();
void global_set_r_gain(uint8_t v);
int global_get_gr_gain();
void global_set_gr_gain(uint8_t v);
int global_get_gb_gain();
void global_set_gb_gain(uint8_t v);
int global_get_b_gain();
void global_set_b_gain(uint8_t v);
void global_store_gain();
const char *global_get_name();
const char *global_get_version();
void global_set_time(long time);
long global_get_time();
void global_run();
void global_parse(char *in, char *cmd, char *data);
void global_reset();