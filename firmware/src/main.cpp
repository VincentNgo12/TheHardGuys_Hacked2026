// src/main.cpp
#include <Arduino.h>
#include <Wire.h>
#include "task_sensor.h"

void printTask(void *pvParameters) {
    // Give sensors a moment to produce first readings
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (true) {
        if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            SensorData d = g_sensorData;
            xSemaphoreGive(g_dataMutex);

            Serial.println("\n========== SENSOR DATA ==========");

            Serial.println("-- AHT21 --");
            Serial.printf("   Temperature : %.2f C\n",   d.temperature);
            Serial.printf("   Humidity    : %.2f %%RH\n", d.humidity);

            Serial.println("-- ENS160 --");
            if (d.ens160Ready) {
                Serial.printf("   eCO2        : %d ppm\n", d.eco2);
                Serial.printf("   TVOC        : %d ppb\n", d.tvoc);
                Serial.printf("   AQI         : %d\n",     d.aqi);
            } else {
                Serial.println("   Status      : Warming up...");
            }

            Serial.println("-- MAX30102 --");
            if (d.fingerDetected) {
                Serial.printf("   Heart Rate  : %.1f bpm\n", d.heartRate);
                Serial.printf("   SpO2        : %.1f %%\n",  d.spO2);
            } else {
                Serial.println("   Status      : No finger detected");
            }

            Serial.println("-- MPU6500 --");
            Serial.printf("   Accel X/Y/Z : %.2f / %.2f / %.2f m/s2\n",
                          d.accelX, d.accelY, d.accelZ);
            Serial.printf("   Gyro  X/Y/Z : %.2f / %.2f / %.2f deg/s\n",
                          d.gyroX,  d.gyroY,  d.gyroZ);

            Serial.println("=================================");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void setup() {
    Serial.begin(115200);

    delay(2000); // extra settle time for serial debug only (remove in production code)

    Wire.begin(22, 20); // ESP32 Feather V2: SDA=22, SCL=20

    if (!sensorInit()) {
        Serial.println("Sensor init failed! Halting.");
        while (1);
    }

    Serial.println("All sensors initialized. Starting tasks...");

    // Bump stack to 4096 — printf with floats is heavy
    xTaskCreate(printTask, "Print", 4096, NULL, 1, NULL);
}

void loop() {
    vTaskDelete(NULL);
}