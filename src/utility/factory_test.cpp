#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "uart_frame.h"

static EventGroupHandle_t wifi_event_group;//定义一个事件的句柄
const int SCAN_DONE_BIT = BIT0;//定义事件，占用事件变量的第0位，最多可以定义32个事件。
int8_t max_rssi = 0;
bool was_init = false;

static wifi_scan_config_t scanConf  = {
    .ssid = NULL,
    .bssid = NULL,
    .channel = 0,
    .show_hidden = 1
};

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            xEventGroupSetBits(wifi_event_group, SCAN_DONE_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void scan_task(void *pvParameters) {
    wifi_event_group = xEventGroupCreate();    //创建一个事件标志组
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    cfg.static_tx_buf_num = 24;
    cfg.static_rx_buf_num = 8;
    cfg.dynamic_rx_buf_num = 8;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));//Set the WiFi operating mode
    ESP_ERROR_CHECK(esp_wifi_start());
    while(1) {
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, true));//扫描所有可用的AP。

        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);//Get number of APs found in last scan
        
        wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);//定义一个wifi_ap_record_t的结构体的链表空间
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));//获取上次扫描中找到的AP列表。
        max_rssi = list[0].rssi;
        free(list);
    }
}

void factory_test() {
    if (was_init) {
        return ; 
    }
    was_init = true;
    xTaskCreatePinnedToCore(&scan_task, "scan_task", 4096, NULL, 1, NULL, 1);
}
