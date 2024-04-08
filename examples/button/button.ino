/**
 * @file button.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM Button Test
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
}

void loop() {
    PoECAM.update();
    if (PoECAM.BtnA.wasPressed()) {
        Serial.println("BtnA Pressed");
    }
    if (PoECAM.BtnA.wasReleased()) {
        Serial.println("BtnA Released");
    }
}
