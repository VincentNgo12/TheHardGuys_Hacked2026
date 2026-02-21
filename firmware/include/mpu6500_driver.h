// include/mpu6500_driver.h
#pragma once

#include <MPU6500_WE.h> // This header provides the MPU6500_WE class

class MPU6500Driver {
public:
    bool begin();
    bool read(float &accelX, float &accelY, float &accelZ,
              float &gyroX,  float &gyroY,  float &gyroZ);

private:
    // Use the correct class for the MPU6500 sensor with its default I2C address (0x68)
    MPU6500_WE mpu = MPU6500_WE();
};