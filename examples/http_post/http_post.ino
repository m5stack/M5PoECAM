/**
 * @file http_post.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM HTTP Post Test
 * @version 0.1
 * @date 2024-01-02
 *
 *
 * @Hardwares: PoECAM
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5PoECAM: https://github.com/m5stack/M5PoECAM
 * ArduinoHttpClient: https://github.com/arduino-libraries/ArduinoHttpClient
 */

#include "M5PoECAM.h"
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#define ssid     "ssid"
#define password "password"

#define SERVER "httpbin.org"

WiFiClient wifi;
HttpClient client = HttpClient(wifi, SERVER);

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
}

void loop() {
    if (PoECAM.Camera.get()) {
        Serial.println("making POST request");

        String contentType = "image/jpeg";

        // client.post("/post", contentType, postData);
        client.post("/post", contentType.c_str(), PoECAM.Camera.fb->len,
                    PoECAM.Camera.fb->buf);

        // read the status code and body of the response
        int statusCode  = client.responseStatusCode();
        String response = client.responseBody();

        Serial.print("Status code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);

        Serial.println("Wait five seconds");
        PoECAM.Camera.free();
        delay(5000);
    }
}
