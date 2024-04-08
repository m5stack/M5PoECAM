/**
 * @file rtsp_stream.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM RTSP Stream
 * @version 0.1
 * @date 2024-01-02
 *
 *
 * @Hardwares: PoECAM
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5PoECAM: https://github.com/m5stack/M5PoECAM
 * Micro-RTSP: https://github.com/geeksville/Micro-RTSP
 */

#include "M5PoECAM.h"
#include <WiFi.h>
#include "CStreamer.h"

#define ssid     "ssid"
#define password "password"

class PoECAMRTSP : public CStreamer {
    Camera_Class &_camera;

   public:
    PoECAMRTSP(Camera_Class &camera, int width, int height)
        : CStreamer(width, height), _camera(camera){};
    virtual void streamImage(uint32_t curMsec) {
        if (_camera.get()) {
            streamFrame(_camera.fb->buf, _camera.fb->len, millis());
            _camera.free();
        }
    }
};

PoECAMRTSP *streamer;
WiFiServer server(8554);

void setup() {
    PoECAM.begin();

    if (!PoECAM.Camera.begin()) {
        Serial.println("Camera Init Fail");
        return;
    }
    Serial.println("Camera Init Success");

    PoECAM.Camera.sensor->set_pixformat(PoECAM.Camera.sensor, PIXFORMAT_JPEG);
    PoECAM.Camera.sensor->set_framesize(PoECAM.Camera.sensor, FRAMESIZE_QVGA);

    PoECAM.Camera.sensor->set_vflip(PoECAM.Camera.sensor, 1);
    PoECAM.Camera.sensor->set_hmirror(PoECAM.Camera.sensor, 0);

    if (PoECAM.Camera.get()) {
        int width  = PoECAM.Camera.fb->width;
        int height = PoECAM.Camera.fb->height;
        streamer   = new PoECAMRTSP(PoECAM.Camera, width, height);
        PoECAM.Camera.free();
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");

    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("RTSP URL: rtsp://");
    Serial.print(WiFi.localIP());
    Serial.println(":8554/mjpeg/1");

    server.begin();
}

void loop() {
    uint32_t msecPerFrame     = 100;
    static uint32_t lastimage = millis();

    // If we have an active client connection, just service that until gone
    streamer->handleRequests(0);  // we don't use a timeout here,
    // instead we send only if we have new enough frames
    uint32_t now = millis();
    if (streamer->anySessions()) {
        if (now > lastimage + msecPerFrame ||
            now < lastimage) {  // handle clock rollover
            streamer->streamImage(now);
            lastimage = now;

            // check if we are overrunning our max frame rate
            now = millis();
            if (now > lastimage + msecPerFrame) {
                Serial.printf("warning exceeding max frame rate of %d ms\n",
                              now - lastimage);
            }
        }
    }

    WiFiClient rtspClient = server.accept();
    if (rtspClient) {
        Serial.print("client: ");
        Serial.print(rtspClient.remoteIP());
        Serial.println();
        streamer->addSession(rtspClient);
    }
}
