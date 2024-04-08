/**
 * @file led.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief PoECAM LED Test
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
    PoECAM.setLed(true);
    vTaskDelay(pdMS_TO_TICKS(500));
    PoECAM.setLed(false);
    vTaskDelay(pdMS_TO_TICKS(500));
}
