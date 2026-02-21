// src/main.cpp
#include <Arduino.h>
#include <Wire.h>
#include "task_sensor.h"
#include "task_display.h"

// Task to print the sensor data snapshot to Serial
void printSnapshotTask(void *pvParameters) {
    while (true) {
        if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            Serial.println("--- Sensor Snapshot ---");
            Serial.printf("Temp: %.2f C, Hum: %.2f %%\n", g_sensorData.temperature, g_sensorData.humidity);
            Serial.printf("eCO2: %d ppm, TVOC: %d ppb, AQI: %d\n", g_sensorData.eco2, g_sensorData.tvoc, g_sensorData.aqi);
            Serial.printf("HR: %.1f bpm, SpO2: %.1f %% \t(Finger: %s)\n", g_sensorData.heartRate, g_sensorData.spO2, g_sensorData.fingerDetected ? "Yes" : "No");
            Serial.printf("Accel (X,Y,Z): %.2f, %.2f, %.2f\n", g_sensorData.accelX, g_sensorData.accelY, g_sensorData.accelZ);
            Serial.printf("Gyro (X,Y,Z):  %.2f, %.2f, %.2f\n", g_sensorData.gyroX, g_sensorData.gyroY, g_sensorData.gyroZ);
            Serial.println("-----------------------");
            xSemaphoreGive(g_dataMutex);
        } else {
            Serial.println("[PrintTask] Could not get data mutex.");
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Print every 2 seconds
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

    if (!displayInit()) {
        Serial.println("Display init failed! Halting.");
        while (1);
    }

    // Create the test harness task
    xTaskCreate(printSnapshotTask, "PrintSnapshot", 4096, NULL, 1, NULL); // Increased stack size

    Serial.println("=== All systems go ===");
}

void loop() {
    vTaskDelete(NULL);
}