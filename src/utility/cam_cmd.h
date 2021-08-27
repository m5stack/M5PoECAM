#pragma once

#include "esp_camera.h"
#include "timer_cam_config.h"

typedef int(*cmd_fun)(sensor_t *, int);

int CallCamCmd(int cmd, int value);

void InitCamFun();

