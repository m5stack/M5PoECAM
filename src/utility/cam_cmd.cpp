#include "cam_cmd.h"
#include "malloc.h"

typedef struct _CamCmd_t {
    int cmd;
    cmd_fun fun;
    struct _CamCmd_t* next;    
} CamCmd_t;

// used to save cmd link list
static CamCmd_t* cmd_link;


void AddCamCmdFun(int cmd, cmd_fun cmd_fun_in) {
    CamCmd_t* cam_cmd_new = cmd_link;

    while (cam_cmd_new->next != NULL) {
        if (cam_cmd_new->next->cmd == cmd) {
            cam_cmd_new->next->fun = cmd_fun_in;
            return ;
        }
        cam_cmd_new = cam_cmd_new->next;
    }

    cam_cmd_new->next = (CamCmd_t *)calloc(1, sizeof(CamCmd_t));
    cam_cmd_new = cam_cmd_new->next;

    cam_cmd_new->cmd = cmd;
    cam_cmd_new->fun = cmd_fun_in;
    cam_cmd_new->next = NULL;
}

cmd_fun GetCamCmdFun(int cmd) {
    CamCmd_t* cam_cmd_new = cmd_link;
    while (cam_cmd_new != NULL) {
        if (cam_cmd_new->cmd == cmd) {
            return cam_cmd_new->fun;
        }
        cam_cmd_new = cam_cmd_new->next;
    }

    return NULL;
}

int CallCamCmd(int cmd, int value) {
    cmd_fun fun = GetCamCmdFun(cmd);
    if (fun == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    sensor_t *s = esp_camera_sensor_get();
    return fun(s, value);
}

void InitCamFun() {
    cmd_link = (CamCmd_t *)calloc(1, sizeof(CamCmd_t)); 
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        return ;
    }
    AddCamCmdFun(kFrameSize,     (cmd_fun)s->set_framesize);
    AddCamCmdFun(kQuality,       (cmd_fun)s->set_quality);
    AddCamCmdFun(kContrast,      (cmd_fun)s->set_contrast);
    AddCamCmdFun(kBrightness,    (cmd_fun)s->set_brightness);
    AddCamCmdFun(kSaturation,    (cmd_fun)s->set_saturation);
    AddCamCmdFun(kGainceiling,   (cmd_fun)s->set_gainceiling);
    AddCamCmdFun(kColorbar,      (cmd_fun)s->set_colorbar);
    AddCamCmdFun(kAwb,           (cmd_fun)s->set_whitebal);
    AddCamCmdFun(kAgc,           (cmd_fun)s->set_gain_ctrl);
    AddCamCmdFun(kAec,           (cmd_fun)s->set_exposure_ctrl);
    AddCamCmdFun(kHmirror,       (cmd_fun)s->set_hmirror);
    AddCamCmdFun(kVflip,         (cmd_fun)s->set_vflip);
    AddCamCmdFun(kAwbGain,       (cmd_fun)s->set_awb_gain);
    AddCamCmdFun(kAgcGain,       (cmd_fun)s->set_agc_gain);
    AddCamCmdFun(kAecValue,      (cmd_fun)s->set_aec_value);
    AddCamCmdFun(kAec2,          (cmd_fun)s->set_aec2);
    AddCamCmdFun(kDcw,           (cmd_fun)s->set_dcw);
    AddCamCmdFun(kBpc,           (cmd_fun)s->set_bpc);
    AddCamCmdFun(kWpc,           (cmd_fun)s->set_wpc);
    AddCamCmdFun(kRawGma,        (cmd_fun)s->set_raw_gma);
    AddCamCmdFun(kLenc,          (cmd_fun)s->set_lenc);
    AddCamCmdFun(kSpecialEffect, (cmd_fun)s->set_special_effect);
    AddCamCmdFun(kWbMode,        (cmd_fun)s->set_wb_mode);
    AddCamCmdFun(kAeLevel,       (cmd_fun)s->set_ae_level);
} 