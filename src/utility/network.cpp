#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "network.h"
#include "string.h"
#include "mdns.h"
#include "esp_event_loop.h"
#include "Arduino.h"

#define TAG "Network"

#define MAX_STA_CONN       1

static EventGroupHandle_t wifi_event_group = NULL;
// static ip4_addr_t ip_addr; // For platform = espressif32@ ^3.5.0
static esp_ip4_addr_t ip_addr; // For platform = espressif32@ ^5.1.0

const int CONNECTED_BIT = BIT0;
const int CONNECTED_FAIL_BIT = BIT1;
volatile WifiConnectStatus_t con_result = CONNECT_FAIL;

static esp_err_t event_handler(void* ctx, system_event_t* event) { 
    static int connect_fail_nums = 0;
    static int reconnect_nums = 65535;
    system_event_sta_disconnected_t *disconn = &event->event_info.disconnected;

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupClearBits(wifi_event_group, CONNECTED_FAIL_BIT);
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            // Serial.printf("got ip:%s\r\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            Serial.printf("got ip:%s\r\n", ip4addr_ntoa((ip4_addr_t *)&event->event_info.got_ip.ip_info.ip));
            ip_addr = event->event_info.got_ip.ip_info.ip;
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupClearBits(wifi_event_group, CONNECTED_FAIL_BIT);
            // init_mdns();
            break;

        case SYSTEM_EVENT_AP_STACONNECTED:
            connect_fail_nums = 0;
            reconnect_nums = 65536;
            log_i(TAG, "station:" MACSTR " join, AID=%d", MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            xEventGroupClearBits(wifi_event_group, CONNECTED_FAIL_BIT);
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d", MAC2STR(event->event_info.sta_disconnected.mac), event->event_info.sta_disconnected.aid);
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            switch (disconn->reason) {
                case WIFI_REASON_BEACON_TIMEOUT:
                    con_result = CONNECT_FAIL_BEACON_TIMEOUT;
                    break;
                case WIFI_REASON_NO_AP_FOUND:
                    con_result = CONNECT_FAIL_NO_AP_FOUND;
                    break;
                case WIFI_REASON_AUTH_FAIL:
                    con_result = CONNECT_FAIL_AUTH_FAIL;
                    break;
                case WIFI_REASON_ASSOC_LEAVE:
                    con_result = NOT_CONNECT;
                default:
                    con_result = CONNECT_FAIL;
                    break;
            }

            if (connect_fail_nums < reconnect_nums && disconn->reason != WIFI_REASON_AUTH_FAIL && disconn->reason != WIFI_REASON_ASSOC_LEAVE) {
                connect_fail_nums += 1;
                ESP_LOGI(TAG, "Reconnect %d", connect_fail_nums);
                // vTaskDelay(500);
                // esp_wifi_connect();
            }

            ESP_LOGI(TAG, "Disconnect reson: %d", disconn->reason);
            xEventGroupSetBits(wifi_event_group, CONNECTED_FAIL_BIT);
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;

        default:
            break;
    }
    return ESP_OK;
}

bool wifi_wait_connect(int32_t timeout) {
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | CONNECTED_FAIL_BIT, false, false, pdMS_TO_TICKS(timeout));
    return ((bits & CONNECTED_BIT) == CONNECTED_BIT);
}

void wifi_sta_connect(const char* ssid, const char* pwd) {
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, pwd, strlen(pwd));

    wifi_config.sta.ssid[strlen(ssid)] = '\0';
    wifi_config.sta.password[strlen(pwd)] = '\0';

    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    esp_wifi_stop();
    // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_init_sta(const char* ssid, const char* pwd)
{   
    if (wifi_event_group == NULL) {
        wifi_event_group = xEventGroupCreate();

        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        cfg.static_tx_buf_num = 24;
        cfg.static_rx_buf_num = 8;
        cfg.dynamic_rx_buf_num = 8;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }

    xEventGroupClearBits(wifi_event_group, CONNECTED_FAIL_BIT);
    wifi_sta_connect(ssid, pwd);
}

void wifi_init_ap(const char *ssid, const char *pwd) {
    if (wifi_event_group == NULL) {
        wifi_event_group = xEventGroupCreate();

        tcpip_adapter_init();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = false;
        cfg.static_tx_buf_num = 24;
        cfg.static_rx_buf_num = 8;
        cfg.dynamic_rx_buf_num = 8;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    wifi_config.ap.ssid_len = strlen(ssid);
    memcpy(wifi_config.ap.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.ap.password, pwd, strlen(pwd));

    wifi_config.ap.ssid[strlen(ssid)] = '\0';
    wifi_config.ap.password[strlen(pwd)] = '\0';

    if (strlen(pwd) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ssid, pwd);
}

int GetWifiConnectStatus() {
    if (wifi_event_group == NULL) {
        return NOT_CONNECT;
    }
    
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);

    if (bits & CONNECTED_BIT) {
        return CONNECT_SUCCESS;
    }

    if (bits & CONNECTED_FAIL_BIT) {
        return con_result;
    }

    return CONNECTING;
}

uint32_t GetWifiIP() {
    return ip_addr.addr;
}
