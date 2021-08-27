#include "esp_http_server.h"
#include "esp_camera.h"
#include "timer_cam_config.h"
#include "esp_log.h"
#include "network.h"
#include "protocol.h"
#include "Arduino.h"

#define TAG "HTTPD"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            continue ;
        } 

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        esp_camera_fb_return(fb);

        if(res != ESP_OK){
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)", (uint32_t)(_jpg_buf_len/1024), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}

esp_err_t config_httpd_handler(httpd_req_t *req) {
    uint32_t buf_len;
    char *buf;

    char cmd_str[30] = {0};
    char value_str[30] = {0};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    
    if (buf_len < 2) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    buf = (char *)malloc(buf_len);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, buf, buf_len) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    if(httpd_query_key_value(buf, "cmd", cmd_str, sizeof(cmd_str)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    if(httpd_query_key_value(buf, "value", value_str, sizeof(value_str)) != ESP_OK) {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    int cmd = atoi(cmd_str);
    int value = atoi(value_str);
    
    int respond_len = 0;
    uint8_t* respond_buff;
    bool restart = false;
    
    if (cmd == kSetDeviceMode && GetDeviceMode() != value) {
        restart = true;
    }

    respond_buff = DealConfigMsg(cmd, (uint8_t *)&value, 2, &respond_len);
    uint8_t *buff = (uint8_t *)calloc(respond_len + 1, sizeof(uint8_t));
    memcpy(&buff[1], respond_buff, respond_len);
    buff[0] = cmd | 0x80;
    httpd_resp_send(req, (char *)buff, respond_len + 1);
    
    if (restart) {
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }

    free(buf);
    free(buff);
    
    return ESP_OK;
}

void start_webserver(const char *ssid, const char *pwd) {
    wifi_init_sta(ssid, pwd);
    wifi_wait_connect(portMAX_DELAY);

    httpd_handle_t server = NULL;
    httpd_handle_t stream_httpd = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    Serial.printf("Starting server on port: '%d'\r\n", config.server_port);

    httpd_uri_t jpeg_stream_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = jpg_stream_httpd_handler,
        .user_ctx = NULL
    };

    httpd_uri_t test_uri = {
        .uri = "/config",
        .method = HTTP_GET,
        .handler = config_httpd_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        httpd_register_uri_handler(server, &test_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &jpeg_stream_uri);
    }

    ESP_LOGI(TAG, "Starting http server!");
}


