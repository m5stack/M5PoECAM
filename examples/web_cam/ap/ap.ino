/**
 * @file ap.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM WEB CAM AP Mode
 * @version 0.1
 * @date 2024-01-02
 *
 *
 * @Hardwares: PoECAM
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5PoECAM: https://github.com/m5stack/M5PoECAM
 */
#include "M5PoECAM.h"
#include <WiFi.h>

const char* ssid     = "PoECAM-AP";
const char* password = "12345678";

WiFiServer server(80);
static void jpegStream(WiFiClient* client);

void setup() {
    PoECAM.begin();

    if (!PoECAM.Camera.begin()) {
        Serial.println("Camera Init Fail");
        return;
    }
    Serial.println("Camera Init Success");

    PoECAM.Camera.sensor->set_pixformat(PoECAM.Camera.sensor, PIXFORMAT_JPEG);
    PoECAM.Camera.sensor->set_framesize(PoECAM.Camera.sensor, FRAMESIZE_SVGA);
    PoECAM.Camera.sensor->set_vflip(PoECAM.Camera.sensor, 1);
    PoECAM.Camera.sensor->set_hmirror(PoECAM.Camera.sensor, 0);

    if (!WiFi.softAP(ssid, password)) {
        log_e("Soft AP creation failed.");
        while (1)
            ;
    }

    Serial.println("AP SSID:");
    Serial.println(ssid);
    Serial.println("AP PASSWORD:");
    Serial.println(password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();
    Serial.print("Server At: ");
    Serial.print(IP);
}

void loop() {
    WiFiClient client = server.available();  // listen for incoming clients

    if (client) {                       // if you get a client,
        Serial.println("New Client.");  // print a message out the serial port
        while (client.connected()) {    // loop while the client's connected
            if (client.available()) {   // if there's bytes to read from the
                jpegStream(&client);
            }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
}

// used to image stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static void jpegStream(WiFiClient* client) {
    Serial.println("Image stream start");
    client->println("HTTP/1.1 200 OK");
    client->printf("Content-Type: %s\r\n", _STREAM_CONTENT_TYPE);
    client->println("Content-Disposition: inline; filename=capture.jpg");
    client->println("Access-Control-Allow-Origin: *");
    client->println();
    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    for (;;) {
        if (PoECAM.Camera.get()) {
            PoECAM.setLed(true);
            Serial.printf("pic size: %d\n", PoECAM.Camera.fb->len);

            client->print(_STREAM_BOUNDARY);
            client->printf(_STREAM_PART, PoECAM.Camera.fb);
            int32_t to_sends    = PoECAM.Camera.fb->len;
            int32_t now_sends   = 0;
            uint8_t* out_buf    = PoECAM.Camera.fb->buf;
            uint32_t packet_len = 8 * 1024;
            while (to_sends > 0) {
                now_sends = to_sends > packet_len ? packet_len : to_sends;
                if (client->write(out_buf, now_sends) == 0) {
                    goto client_exit;
                }
                out_buf += now_sends;
                to_sends -= packet_len;
            }

            int64_t fr_end     = esp_timer_get_time();
            int64_t frame_time = fr_end - last_frame;
            last_frame         = fr_end;
            frame_time /= 1000;
            Serial.printf("MJPG: %luKB %lums (%.1ffps)\r\n",
                          (long unsigned int)(PoECAM.Camera.fb->len / 1024),
                          (long unsigned int)frame_time,
                          1000.0 / (long unsigned int)frame_time);

            PoECAM.Camera.free();
            PoECAM.setLed(false);
        }
    }

client_exit:
    PoECAM.Camera.free();
    PoECAM.setLed(false);
    client->stop();
    Serial.printf("Image stream end\r\n");
}
