#ifndef _TIMERCAM_NETWORK_H_
#define _TIMERCAM_NETWORK_H_

typedef enum {
    CONNECTING,
    CONNECT_FAIL,
    CONNECT_FAIL_BEACON_TIMEOUT,
    CONNECT_FAIL_AUTH_FAIL,
    CONNECT_FAIL_NO_AP_FOUND,
    CONNECT_SUCCESS,
    NOT_CONNECT,
} WifiConnectStatus_t;

// Non-blocking
void wifi_sta_connect(const char* ssid, const char* pwd);

void wifi_init_sta(const char* ssid, const char* pwd);

bool wifi_wait_connect(int32_t timeout);

// Non-blocking
void wifi_init_ap(const char *ssid, const char *pwd);

int GetWifiConnectStatus();

uint32_t GetWifiIP();

#endif