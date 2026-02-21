// include/mpu6500_driver.h
#pragma once

class MPU6500Driver {
public:
    bool begin();
    bool read(float &accelX, float &accelY, float &accelZ,
              float &gyroX,  float &gyroY,  float &gyroZ);
};