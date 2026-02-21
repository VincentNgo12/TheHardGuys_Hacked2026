#include "mpu6500_driver.h"
#include <Arduino.h>

bool MPU6500Driver::begin() {
    // The MPU6500_WE class's init() will correctly identify the MPU6500 at its default address.
    if (!mpu.init()) {
        return false;
    }

    // Configure the sensor settings. These are similar to the previous library.
    mpu.setSampleRateDivider(5);
    mpu.setAccRange(MPU6500_ACC_RANGE_8G);
    mpu.enableAccDLPF(true);
    mpu.setAccDLPF(MPU6500_DLPF_6);
    
    mpu.setGyrRange(MPU6500_GYRO_RANGE_500);
    mpu.enableGyrDLPF(); // Corrected: no argument needed
    mpu.setGyrDLPF(MPU6500_DLPF_6);

    return true;
}

bool MPU6500Driver::read(float &accelX, float &accelY, float &accelZ,
                         float &gyroX,  float &gyroY,  float &gyroZ) {
    // The MPU6500_WE class uses the same xyzFloat struct and methods.
    xyzFloat gValue = mpu.getGValues();
    xyzFloat gyr = mpu.getGyrValues();

    accelX = gValue.x;
    accelY = gValue.y;
    accelZ = gValue.z;

    gyroX = gyr.x;
    gyroY = gyr.y;
    gyroZ = gyr.z;

    return true;
}
