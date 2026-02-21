#include "task_sensor.h"
#include <Arduino.h>

// --------------------------------------------------
// Globals
// --------------------------------------------------
SensorData        g_sensorData = {};
SemaphoreHandle_t g_dataMutex  = nullptr;
SemaphoreHandle_t g_i2cMutex   = nullptr;

static AHT21Driver   aht21;
static ENS160Driver  ens160;
static MAX30102Driver max30102;
static MPU6500Driver mpu6500;

// --------------------------------------------------
// Forward declarations
// --------------------------------------------------
static void aht21Task   (void *pvParameters);
static void ens160Task  (void *pvParameters);
static void max30102Task(void *pvParameters);
static void mpu6500Task (void *pvParameters);

// --------------------------------------------------
// sensorInit — init all sensors + launch all tasks
// --------------------------------------------------
bool sensorInit() {
    g_dataMutex = xSemaphoreCreateMutex();
    g_i2cMutex  = xSemaphoreCreateMutex();

    if (!g_dataMutex || !g_i2cMutex) {
        Serial.println("[Sensor] Failed to create mutexes!");
        return false;
    }

    if (!aht21.begin()) {
        Serial.println("[Sensor] AHT21 not found!");
        return false;
    }
    Serial.println("[Sensor] AHT21 ready.");

    if (!ens160.begin()) {
        Serial.println("[Sensor] ENS160 not found!");
        return false;
    }
    Serial.println("[Sensor] ENS160 ready. Warming up...");

    if (!max30102.begin()) {
        Serial.println("[Sensor] MAX30102 not found!");
        return false;
    }
    Serial.println("[Sensor] MAX30102 ready.");

    if (!mpu6500.begin()) {
        Serial.println("[Sensor] MPU6500 not found!");
        return false;
    }
    Serial.println("[Sensor] MPU6500 ready.");

    // Launch all tasks here — main.cpp stays clean
    xTaskCreate(aht21Task,    "AHT21",    2048, NULL, 1, NULL);
    xTaskCreate(ens160Task,   "ENS160",   2048, NULL, 1, NULL);
    xTaskCreate(max30102Task, "MAX30102", 4096, NULL, 1, NULL); // larger stack, signal processing heavy
    xTaskCreate(mpu6500Task,  "MPU6500",  2048, NULL, 1, NULL);

    return true;
}

// --------------------------------------------------
// AHT21 Task — temp + humidity every 2s
// --------------------------------------------------
static void aht21Task(void *pvParameters) {
    while (true) {
        float temp, hum;

        if (xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            bool ok = aht21.read(temp, hum);
            xSemaphoreGive(g_i2cMutex);

            if (ok) {
                if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sensorData.temperature = temp;
                    g_sensorData.humidity    = hum;
                    xSemaphoreGive(g_dataMutex);
                }
            } else {
                Serial.println("[AHT21] Read failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// --------------------------------------------------
// ENS160 Task — air quality every 2s
// Uses AHT21 temp+humidity for compensation
// --------------------------------------------------
static void ens160Task(void *pvParameters) {
    while (true) {
        // Grab latest AHT21 data for compensation
        float temp, hum;
        if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            temp = g_sensorData.temperature;
            hum  = g_sensorData.humidity;
            xSemaphoreGive(g_dataMutex);
        }

        uint16_t eco2, tvoc;
        int8_t   aqi;

        if (xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            ens160.setCompensation(temp, hum);
            bool ok = ens160.read(eco2, tvoc, aqi);
            xSemaphoreGive(g_i2cMutex);

            if (ok) {
                if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sensorData.eco2        = eco2;
                    g_sensorData.tvoc        = tvoc;
                    g_sensorData.aqi         = aqi;

                    // Latch g_sensorData.ens160Ready to true once we get a real reading — never go back to false
                    if (!g_sensorData.ens160Ready && eco2 > 0) {
                        g_sensorData.ens160Ready = true;
                        Serial.println("[ENS160] Warm-up complete, data is valid.");
                    }

                    xSemaphoreGive(g_dataMutex);
                }
            } else {
                Serial.println("[ENS160] Read failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// --------------------------------------------------
// MAX30102 Task — heart rate + SpO2 every 100ms
// Faster poll rate — sensor needs continuous sampling
// --------------------------------------------------
static void max30102Task(void *pvParameters) {
    while (true) {
        float hr, spo2;
        bool  fingerDetected;

        if (xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            bool ok = max30102.read(hr, spo2, fingerDetected);
            xSemaphoreGive(g_i2cMutex);

            if (ok) {
                if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sensorData.heartRate      = hr;
                    g_sensorData.spO2           = spo2;
                    g_sensorData.fingerDetected = fingerDetected;
                    xSemaphoreGive(g_dataMutex);
                }
            } else {
                Serial.println("[MAX30102] Read failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --------------------------------------------------
// MPU6500 Task — accel + gyro every 50ms
// --------------------------------------------------
static void mpu6500Task(void *pvParameters) {
    while (true) {
        float ax, ay, az;
        float gx, gy, gz;

        if (xSemaphoreTake(g_i2cMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            bool ok = mpu6500.read(ax, ay, az, gx, gy, gz);
            xSemaphoreGive(g_i2cMutex);

            if (ok) {
                if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_sensorData.accelX = ax;
                    g_sensorData.accelY = ay;
                    g_sensorData.accelZ = az;
                    g_sensorData.gyroX  = gx;
                    g_sensorData.gyroY  = gy;
                    g_sensorData.gyroZ  = gz;
                    xSemaphoreGive(g_dataMutex);
                }
            } else {
                Serial.println("[MPU6500] Read failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}