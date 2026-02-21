// include/task_sensor.h
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "aht21_driver.h"
#include "ens160_driver.h"
#include "max30102_driver.h"
#include "mpu6500_driver.h"

// --------------------------------------------------
// Shared sensor data — extern so other tasks
// (task_display, task_power, etc.) can read it
// --------------------------------------------------
struct SensorData {
    // AHT21
    float    temperature;
    float    humidity;

    // ENS160
    uint16_t eco2;
    uint16_t tvoc;
    int8_t   aqi;
    bool     ens160Ready;

    // MAX30102
    float    heartRate;
    float    spO2;
    bool     fingerDetected;

    // MPU6500
    float    accelX, accelY, accelZ;
    float    gyroX,  gyroY,  gyroZ;
};

extern SensorData        g_sensorData;
extern SemaphoreHandle_t g_dataMutex;
extern SemaphoreHandle_t g_i2cMutex;

// Inits all sensors and launches all FreeRTOS tasks
bool sensorInit();