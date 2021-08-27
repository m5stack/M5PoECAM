#include "timer_cam_config.h"
#include "string.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAM_NAMESPACE "timer-cam"
#define CAM_CONFIG_KEY "cam-config"

static TimerCamConfig_t timer_cam_config;
static int16_t cam_config_list[kCamCmdEnd];
static char *token = NULL;

void InitTimerCamConfig() {
    nvs_handle handle;
    esp_err_t err;
    err = nvs_open(CAM_NAMESPACE, NVS_READWRITE, &handle);
    size_t lenght;
    err = nvs_get_blob(handle, CAM_CONFIG_KEY, &timer_cam_config, &lenght);
    // nvs not found device config
    if (err != ESP_OK && lenght != sizeof(TimerCamConfig_t)) { 
        timer_cam_config.mode = kPOE;
        timer_cam_config.wifi_pwd[0] = '\0';
        timer_cam_config.wifi_ssid[0] = '\0';
        timer_cam_config.TimingTime = 60;
        nvs_set_blob(handle, CAM_CONFIG_KEY, &timer_cam_config, sizeof(TimerCamConfig_t));
        nvs_commit(handle);
    }
    nvs_close(handle);
    
    err = esp_camera_load_from_nvs(CAM_NAMESPACE);

    if (err != ESP_OK) {
        sensor_t *s = esp_camera_sensor_get();
        s->set_framesize(s, FRAMESIZE_VGA);
        s->set_vflip(s, 0);
    }
}

void SaveTimerCamConfig() {
    nvs_handle handle;
    nvs_open(CAM_NAMESPACE, NVS_READWRITE, &handle);
    nvs_set_blob(handle, CAM_CONFIG_KEY, &timer_cam_config, sizeof(TimerCamConfig_t));
    nvs_commit(handle);
    nvs_close(handle);
    SaveCamConfig();
}

void SetTimingTime(uint32_t time) {
    timer_cam_config.TimingTime = time;
}

CamMode_t GetDeviceMode() {
    return (CamMode_t)timer_cam_config.mode;
}

bool GetWifiConfig(char *ssid, char *pwd) {
    if (strlen(timer_cam_config.wifi_ssid) == 0) {
        return false;
    }

    memcpy(ssid, timer_cam_config.wifi_ssid, strlen(timer_cam_config.wifi_ssid) + 1);
    memcpy(pwd, timer_cam_config.wifi_pwd, strlen(timer_cam_config.wifi_pwd) + 1);
    return true;
}

uint8_t* GetCamConfig(int *length) {
    sensor_t *s = esp_camera_sensor_get();
    cam_config_list[kFrameSize] = s->status.framesize;//0 - 10
    cam_config_list[kQuality] = s->status.quality;//0 - 63
    cam_config_list[kBrightness] = s->status.brightness;//-2 - 2
    cam_config_list[kContrast] = s->status.contrast;//-2 - 2
    cam_config_list[kSaturation] = s->status.saturation;//-2 - 2
    cam_config_list[kSpecialEffect] = s->status.special_effect;//0 - 6
    cam_config_list[kWbMode] = s->status.wb_mode;//0 - 4
    cam_config_list[kAwb] = s->status.awb;
    cam_config_list[kAwbGain] = s->status.awb_gain;
    cam_config_list[kAec] = s->status.aec;
    cam_config_list[kAec2] = s->status.aec2;
    cam_config_list[kAeLevel] = s->status.ae_level;//-2 - 2
    cam_config_list[kAecValue] = s->status.aec_value;//0 - 1200
    cam_config_list[kAgc] = s->status.agc;
    cam_config_list[kAgcGain] = s->status.agc_gain;//0 - 30
    cam_config_list[kGainceiling] = s->status.gainceiling;//0 - 6
    cam_config_list[kBpc] = s->status.bpc;
    cam_config_list[kWpc] = s->status.wpc;
    cam_config_list[kRawGma] = s->status.raw_gma;
    cam_config_list[kLenc] = s->status.lenc;
    cam_config_list[kHmirror] = s->status.hmirror;
    cam_config_list[kVflip] = s->status.vflip;
    cam_config_list[kDcw] = s->status.dcw;
    cam_config_list[kColorbar] = s->status.colorbar;
    *length = kCamCmdEnd * 2;
    return (uint8_t *)cam_config_list;
}

TimerCamConfig_t* GetTimerCamCfg() {
    return &timer_cam_config;
}

void SaveCamConfig() {
    esp_camera_save_to_nvs(CAM_NAMESPACE);
}

bool UpdateDeviceMode(CamMode_t mode) {
    if (mode == timer_cam_config.mode) {
        return false;
    }
    timer_cam_config.mode = mode;
    return true;
}

bool UpdateWifiConfig(const char *ssid, const char *pwd) {
    int ssid_len = strlen(ssid);
    int pwd_len = strlen(pwd);

    if (ssid_len >= SSID_MAX_LEN || pwd_len >= PWD_MAX_LEN) {
        return false;
    }
    
    timer_cam_config.wifi_ssid[ssid_len] = '\0';
    timer_cam_config.wifi_pwd[pwd_len] = '\0';

    memcpy(timer_cam_config.wifi_ssid, ssid, ssid_len);
    memcpy(timer_cam_config.wifi_pwd, pwd, pwd_len);

    return true;
}

char* GetWifiSSID() {
    return timer_cam_config.wifi_ssid;
}

void SetToken(char *token_in) {
    token = token_in;
}

char* GetToken() {
    return token;
}