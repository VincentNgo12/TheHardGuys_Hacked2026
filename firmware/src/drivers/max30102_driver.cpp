// src/drivers/max30102_driver.cpp
#include "max30102_driver.h"
#include <Arduino.h>

bool MAX30102Driver::begin() {
    Serial.println("[MAX30102] Placeholder driver — begin() OK.");
    return true;
}

bool MAX30102Driver::read(float &heartRate, float &spO2, bool &fingerDetected) {
    // Fake plausible values until real implementation
    heartRate      = 72.4f;
    spO2           = 98.1f;
    fingerDetected = true;
    return true;
}