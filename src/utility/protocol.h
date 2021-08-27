#pragma once
#include "esp_camera.h"
#include "timer_cam_config.h"

uint8_t* DealConfigMsg(int cmd_in, const uint8_t* data, int len, int *out_len);
