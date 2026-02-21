// include/mpu6500_driver.h
#pragma once
#include <MPU9250_WE.h>

class MPU6500Driver {
public:
    bool begin();
    bool read(float &accelX, float &accelY, float &accelZ,
              float &gyroX,  float &gyroY,  float &gyroZ);
};