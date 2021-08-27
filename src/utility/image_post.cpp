#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "image_post.h"

bool http_post_image(const char* url, const char* tok, const uint8_t *data, uint32_t len, uint32_t voltage) {
    bool result = false;

    char *params = NULL; 
    asprintf(&params, "%s?tok=%s&voltage=%d", url, tok, voltage);
    if (params == NULL) {
        ESP_LOGE("HTTP_REQUEST", "Params malloc fails");
        return false;
    }

    esp_http_client_config_t config;
    memset(&config, 0, sizeof(esp_http_client_config_t));
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_url(client, params);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/binary");
    esp_http_client_set_post_field(client, (char *)data, len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            result = true;
        }
    }
    esp_http_client_cleanup(client);
    if (params) {
        free(params);
    }
    return result;
}

bool get_token(const char* url, char **data, uint32_t* len) {
    bool result = false;

    char *params = NULL;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    asprintf(&params, "{\"mac\": \"%02x:%02x:%02x:%02x:%02x:%02x\"}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (params == NULL) {
        ESP_LOGE("HTTP_REQUEST", "Params malloc fails");
        return false;
    }
    
    esp_http_client_config_t config;
    memset(&config, 0, sizeof(esp_http_client_config_t));
    config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, params, strlen(params));
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
        result = true;
        int content_length = esp_http_client_get_content_length(client);
        if (content_length < 100) {
            char* buffer = (char *)heap_caps_malloc(content_length + 1, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL); 
            if (esp_http_client_read(client, buffer, content_length) != content_length) {
                result = false;
                heap_caps_free(buffer);
            } else {
                buffer[content_length - 1] = '\0';
                *data = buffer;
                *len = content_length;
                result = true;
            }
        } else {
            ESP_LOGE("HTTP_REQUEST", "Token got too long");
        }
    }
    esp_http_client_cleanup(client);
    if (params) {
        free(params);
    }
    return result;
}
