#pragma once
#include "sensor.h"

int ar0130_init(sensor_t *sensor);
int ar0130_detect(int slv_addr, sensor_id_t *id);