
#include "M5PoECAM.h"

using namespace m5;

M5PoECAM PoECAM;

void M5PoECAM::begin(bool enableLed) {
    Serial.begin(115200);

    pinMode(M5_POE_CAM_BTN_A_PIN, INPUT);
    _enableLed = enableLed;
    if (_enableLed) {
        pinMode(M5_POE_CAM_LED_PIN, OUTPUT);
    }
}

void M5PoECAM::setLed(bool status) {
    if (_enableLed) {
        digitalWrite(M5_POE_CAM_LED_PIN, status);
    }
}

void M5PoECAM::update(void) {
    uint32_t ms = millis();
    BtnA.setRawState(ms, !digitalRead(M5_POE_CAM_BTN_A_PIN));
}