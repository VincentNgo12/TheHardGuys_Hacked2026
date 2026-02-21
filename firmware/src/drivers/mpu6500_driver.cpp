// src/drivers/mpu6500_driver.cpp
#include "mpu6500_driver.h"
#include <MPU9250_WE.h>
#include <Arduino.h>

bool MPU6500Driver::begin() {
    Serial.println("[MPU6500] Placeholder driver — begin() OK.");
    return true;
}

bool MPU6500Driver::read(float &accelX, float &accelY, float &accelZ,
                         float &gyroX,  float &gyroY,  float &gyroZ) {
    // Fake plausible values until real implementation
    accelX = 0.01f; accelY = 0.02f; accelZ = 9.81f;
    gyroX  = 0.1f;  gyroY  = 0.2f;  gyroZ  = 0.0f;
    return true;
}