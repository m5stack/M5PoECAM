#pragma once

#include "nvs_flash.h"

/* Define ------------------------------------------------------------------- */
// pin config
#define CAM_PIN_PWDN -1
#define CAM_PIN_RESET 15 //software reset will be performed
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 14
#define CAM_PIN_SIOC 12
#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 32
#define CAM_PIN_VSYNC 22
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21

#define CAM_XCLK_FREQ   10000000

#define CAMERA_LED_GPIO 4

#define Ext_PIN_1 33
#define Ext_PIN_2 25

#define BAT_HOLD_PIN 33
#define BAT_ADC_PIN 33

// cmd list
typedef enum {
    kImage = 0x00,

// cam config
    kFrameSize,
    kQuality,
    kContrast,
    kBrightness,
    kSaturation,
    kGainceiling,
    kColorbar,
    kAwb,
    kAgc,
    kAec,
    kHmirror,
    kVflip,
    kAwbGain,
    kAgcGain,
    kAecValue,
    kAec2,
    kDcw,
    kBpc,
    kWpc,
    kRawGma,
    kLenc,
    kSpecialEffect,
    kWbMode,
    kAeLevel,
    kCamCmdEnd,

// usr cmd
    kSetDeviceMode,
    kGetDeviceMode,
    kSaveDeviceConfig,
    
    kGetCamConfig,
    kSaveCamConfig,

    kSetWiFi,
    kGetWifiSSID,
    kGetWifiIP,
    kGetWifiState,


    kTimingTime,
    kFactoryTest,
    kErrorOccur,
    kRestart,
    kGetToken,
    kSetLed,
    kGetWifiRssi,
} CmdList_t;

typedef enum {
    kPSRAMError,
    kCamError,
    kWifiMsgError,
    kbm8563Error,
    kNotFoundWifiMsg,
    kGroveError,
} ErrorDetail_t;

// mod config
#define SSID_MAX_LEN 36
#define PWD_MAX_LEN 38

typedef enum {
    kWifiSta,
    kUart,
    kTiming,
    kPOE,
} CamMode_t;

typedef struct _timer_cam_config {
    uint8_t mode;
    char wifi_ssid[SSID_MAX_LEN];
    char wifi_pwd[PWD_MAX_LEN];
    int TimingTime;
} TimerCamConfig_t;


void InitTimerCamConfig();

void SaveTimerCamConfig();

CamMode_t GetDeviceMode();

bool GetWifiConfig(char *ssid, char *pwd);

TimerCamConfig_t* GetTimerCamCfg();

uint8_t* GetCamConfig(int *length);

void SetTimingTime(uint32_t time);

void SaveCamConfig();

bool UpdateDeviceMode(CamMode_t mode);

bool UpdateWifiConfig(const char *ssid, const char *pwd);

char* GetWifiSSID();

void SetToken(char *token_in);

char* GetToken();

