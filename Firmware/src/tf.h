#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <WebServer.h>
#define USE_MMC
#ifdef USE_MMC
#include "SD_MMC.h"
#else
#include "SdFat.h"
#endif
void tf_mount();
void tf_unmount();
void tf_list_dir(String dirname, WebServer* server);
bool tf_exists(const char *path);
bool tf_exists_dir(const char *path);
void tf_create_dir(const char *path);
// File tf_open(const char *path, const char* mode);
bool tf_remove_dir(const char * dirname);
void tf_begin_write(const char*path);
void tf_write_file(uint8_t *data, size_t size, int blocksize=0);
void tf_end_write();
void tf_append_file(const char *path, const char *message);
void tf_rename_file(const char *path1, const char *path2);
bool tf_delete_file(const char *path);
bool tf_available();
bool tf_test();
bool tf_fulldisk();
void tf_reset();
#ifdef USE_MMC
File tf_read(const char *path);
#else
ExFile tf_read(const char *path);
#endif