#ifndef _M5_POE_CAM_H_
#define _M5_POE_CAM_H_

#include "./utility/Camera_Class.h"
#include "./utility/Button_Class.hpp"
#include "esp_camera.h"
#include "Arduino.h"
#include "driver/gpio.h"

#define M5_POE_CAM_LED_PIN      0
#define M5_POE_CAM_BTN_A_PIN    37
#define M5_POE_CAM_ETH_CS_PIN   4
#define M5_POE_CAM_ETH_MOSI_PIN 13
#define M5_POE_CAM_ETH_MISO_PIN 38
#define M5_POE_CAM_ETH_CLK_PIN  23

namespace m5 {

class M5PoECAM {
   private:
    /* data */
    bool _enableLed;

   public:
    void begin(bool enableLed = true);
    void setLed(bool status);
    Camera_Class Camera;
    Button_Class BtnA;
    void update(void);
};
}  // namespace m5
extern m5::M5PoECAM PoECAM;

#endif