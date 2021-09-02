/*
    Description: Connect PoECAM to Ethernet, check the serial port output log to get the preview URL address "192.168.xxx.xxx/stream"
    Long press the button on the right side of the fuselage to switch between UART/PoE mode.
    Please install library before compiling:  
    Ethernet Lib download Link: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/lib/Ethernet.zip
    After downloaded unzip the lib zip file to the Arduino Lib path
*/

#include <WiFi.h>
#include "esp_task_wdt.h"
#include "esp_camera.h"
#include "PoE_CAM.h"
#include <esp_heap_caps.h>

char wifi_ssid[36];
char wifi_pwd[36];

bool restart = false;
volatile bool init_finish = false;

void frame_post_callback(uint8_t cmd) {
  if (restart && (cmd == (kSetDeviceMode | 0x80))) {
    Serial.println("kSetDeviceMode");
    esp_restart();
  } else if (cmd == (kRestart | 0x80)) {
    Serial.println("kRestart");
    esp_restart();
  } else if (cmd == (kSetWiFi | 0x80)) {
    Serial.println("kSetWiFi");
    // esp_restart();
  }
}

void frame_recv_callback(int cmd_in, const uint8_t* data, int len) {
  if (init_finish == false) {
    return ;
  }

  Serial.printf("Recv cmd %d\r\n", cmd_in);

  if (cmd_in == kRestart) {
    uint8_t respond_data = 0;
    uart_frame_send(cmd_in | 0x80, &respond_data, 1, false);
    return ;
  }

  if (cmd_in == kSetDeviceMode || GetDeviceMode() != data[0]) {
    restart = true;
  }

  uint8_t* respond_buff;
  int respond_len = 0;
  respond_buff = DealConfigMsg(cmd_in, data, len, &respond_len);
  uart_frame_send(cmd_in | 0x80, respond_buff, respond_len, false);
}

void start_uart_server(void) {
  camera_fb_t * fb = NULL;
  size_t _jpg_buf_len;
  uint8_t * _jpg_buf;
  static int64_t last_frame = 0;
  if(!last_frame) {
    last_frame = esp_timer_get_time();   
  }

  esp_task_wdt_init(1, true);
  esp_task_wdt_add(NULL);

  while(true){
    fb = esp_camera_fb_get();
    esp_task_wdt_reset();
    if (!fb) {
      ESP_LOGE(TAG, "Camera capture failed");
      continue ;
    } 

    _jpg_buf_len = fb->len;
    _jpg_buf = fb->buf;

    if (!(_jpg_buf[_jpg_buf_len - 1] != 0xd9 || _jpg_buf[_jpg_buf_len - 2] != 0xd9)) {
      esp_camera_fb_return(fb);
      continue;
    }

    uart_frame_send(kImage, _jpg_buf, _jpg_buf_len, true);
    esp_camera_fb_return(fb);

    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000;
  }
  last_frame = 0;
}

void start_timing_server() {
    TimerCamConfig_t* config = GetTimerCamCfg();
    if (config->wifi_ssid[0] == '\0') {
        Serial.println("WIFI INFO ERROR");
        Serial.println(config->wifi_ssid);
        Serial.println(config->wifi_pwd);
        return ;
    }
    wifi_init_sta(config->wifi_ssid, config->wifi_pwd);
    while (wifi_wait_connect(100) == false) {
        vTaskDelay(100);
    }
    vTaskDelay(100);
    ESP_LOGI(TAG, "connect wifi success");

    bool result = false;
    uint8_t mac[6];
    String a;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    esp_base_mac_addr_set(mac);

    // c4dd57b9a9408c4862544d3b0d2631da
    char *token = NULL;
    uint32_t len = 0;
    char *data = NULL;
    asprintf(&token, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    for (;;) {
        result = get_token("http://camera.m5stack.com/token_get", &data, &len);

        if (result == false) {
            ESP_LOGE(TAG, "Got Token Error");
            vTaskDelay(1000);
            continue;
        }

        if (data[1] == 'T') {
            free(data);
            result = get_token("http://camera.m5stack.com/token", &data, &len);
        }
        
        if (result == true) {
            SetToken(data + 1);
            ESP_LOGI(TAG, "Got Token: %s", data + 1);
            break ;
        }
    }

    for (;;) {
        config = GetTimerCamCfg();
        vTaskDelay(config->TimingTime * 1000);
        camera_fb_t* fb = esp_camera_fb_get();
        const char *url = "http://camera.m5stack.com/timer-cam/image";
        result =  http_post_image(url, token, fb->buf, fb->len, 3000);
        ESP_LOGI(TAG, "Post %d", result);
        esp_camera_fb_return(fb);
    }
}

void startW5500TimingServer() {
  w5500Init();

  bool result = false;
  uint32_t len = 0;
  char *data = NULL;

  for (;;) {
    result = w5500GotToken("http://camera.m5stack.com/token_get", &data, &len);

    if (result == false) {
      ESP_LOGE(TAG, "Got Token Error");
      vTaskDelay(1000);
      continue;
    }

    if (data[0] == 'T') {
      free(data);
      result = w5500GotToken("http://camera.m5stack.com/token", &data, &len);
    }
    
    if (result == true) {
      SetToken(data);
      ESP_LOGI(TAG, "Got Token: %s", data);
      break ;
    }
  }

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  esp_base_mac_addr_set(mac);

  TimerCamConfig_t* config;
  char *token = NULL;
  asprintf(&token, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  for (;;) {
    config = GetTimerCamCfg();
    vTaskDelay(config->TimingTime * 1000);
    camera_fb_t* fb = esp_camera_fb_get();
    const char *url = "http://camera.m5stack.com/timer-cam/image";
    w5500PostImage(url, token, fb->buf, fb->len, 3000);
    esp_camera_fb_return(fb);
    Serial.printf("Post image by w5500\r\n");
  }
}

void btn_switch_mode_task(void *arg) {
  while(1){
    if (!digitalRead(37))
    {
      vTaskDelay(pdMS_TO_TICKS(500));
      if(!digitalRead(37)) { 
          if(GetDeviceMode() == kUart){
            UpdateDeviceMode(kPOE);
          }else{
            UpdateDeviceMode(kUart);
          }
          SaveTimerCamConfig();
          vTaskDelay(pdMS_TO_TICKS(1000));
          esp_restart();
        }
      }
      vTaskDelay(pdMS_TO_TICKS(100));
  }
}

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

  xTaskCreatePinnedToCore(btn_switch_mode_task, "btn_switch_mode_task", 4 * 1024, NULL, 2, NULL, 0);

  if (GetDeviceMode() == kUart) {
    Serial.printf("SELECT UART MODE\r\n");
    start_uart_server();
  } else if (GetDeviceMode() == kTiming) {
    Serial.printf("SELECT Timer MODE\r\n");
    start_timing_server();
  } else if (GetDeviceMode() == kPOE) {
    Serial.printf("SELECT PoE MODE\r\n");
    w5500Init();
    w5500ImageLoop();
  } else {
    while (GetWifiConfig(wifi_ssid, wifi_pwd) == false) {
      uint8_t error_code = kWifiMsgError;
      uart_frame_send(kErrorOccur, &error_code, 1, false);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    Serial.printf("SELECT WiFi MODE\r\n");
    Serial.printf("ssid: %s, pwd: %s\r\n", wifi_ssid, wifi_pwd);
    start_webserver(wifi_ssid, wifi_pwd);
  }
}

void loop() {
  delay(100);
}
