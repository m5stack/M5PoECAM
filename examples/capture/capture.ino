/**
 * @file capture.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM Take Photo Test
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

void setup() {
    PoECAM.begin();

    if (!PoECAM.Camera.begin()) {
        Serial.println("Camera Init Fail");
        return;
    }
    Serial.println("Camera Init Success");

    PoECAM.Camera.sensor->set_pixformat(PoECAM.Camera.sensor,
                                          PIXFORMAT_JPEG);
    PoECAM.Camera.sensor->set_framesize(PoECAM.Camera.sensor,
                                          FRAMESIZE_QVGA);

    PoECAM.Camera.sensor->set_vflip(PoECAM.Camera.sensor, 1);
    PoECAM.Camera.sensor->set_hmirror(PoECAM.Camera.sensor, 0);
}

void loop() {
    if (PoECAM.Camera.get()) {
        Serial.printf("pic size: %d\n", PoECAM.Camera.fb->len);
        PoECAM.Camera.free();
    }
}
