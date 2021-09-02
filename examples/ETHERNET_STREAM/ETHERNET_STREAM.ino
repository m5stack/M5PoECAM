/*
    Description: Connect PoECAM to Ethernet, check the serial port output log to get the preview URL address "192.168.xxx.xxx/stream"
    Please install library before compiling:  
    Ethernet Lib download Link: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/lib/Ethernet.zip
    After downloaded unzip the lib zip file to the Arduino Lib path
*/

#include <WiFi.h>
#include "esp_task_wdt.h"
#include "esp_camera.h"
#include "PoE_CAM.h"
#include <esp_heap_caps.h>

bool restart = false;
volatile bool init_finish = false;

void setup() {
  Serial.begin(115200);

  pinMode(37, INPUT);
  pinMode(0, OUTPUT);
  digitalWrite(0, 0);

  delay(100);

  Serial.println("UART INIT");
  uart_init(Ext_PIN_1, Ext_PIN_2); 

  camera_config_t config;
  config.pin_pwdn = CAM_PIN_PWDN,
  config.pin_reset = CAM_PIN_RESET;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_sscb_sda = CAM_PIN_SIOD;
  config.pin_sscb_scl = CAM_PIN_SIOC;
  config.pin_d7 = CAM_PIN_D7;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_pclk = CAM_PIN_PCLK;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 16;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);

  delay(100);

  if (err) 
  {
    Serial.printf("Camera init failed\r\n");
    for (;;) {
      delay(1);
    }
  }

  Serial.printf("Cam Init Success\r\n");
  delay(100);

  InitTimerCamConfig();
  InitCamFun();
  init_finish = true;

  Serial.printf("PoE MODE\r\n");
  w5500Init();
  w5500ImageLoop();
}

void loop() {
  delay(100);
}
