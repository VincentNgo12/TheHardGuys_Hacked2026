// src/tasks/task_display.cpp
#include "task_display.h"
#include <Arduino.h>

static TFTDriver tft;
static Screen    currentScreen    = SCREEN_HOME;
static bool      fallAlertActive  = false;
static uint32_t  fallAlertStart   = 0;

static void displayTask(void *pvParameters) {
    bool heartBeatTick = false;

    // Initial splash
    tft.drawSplash();
    vTaskDelay(pdMS_TO_TICKS(1500));
    tft.clearScreen();

    while (true) {
        // ── Copy sensor data under lock ───────────────────
        DisplayData d = {};
        if (xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            d.temperature = g_sensorData.temperature;
            d.humidity    = g_sensorData.humidity;
            d.eCO2        = g_sensorData.eco2;
            d.eTVOC       = g_sensorData.tvoc;
            d.aqi         = g_sensorData.aqi;
            d.heartRate   = (uint8_t)g_sensorData.heartRate;
            d.spo2        = (uint8_t)g_sensorData.spO2;
            d.accelX      = g_sensorData.accelX;
            d.accelY      = g_sensorData.accelY;
            d.accelZ      = g_sensorData.accelZ;
            xSemaphoreGive(g_dataMutex);
        }

        // ── Fall detection ────────────────────────────────
        float mag = sqrt(d.accelX * d.accelX +
                         d.accelY * d.accelY +
                         d.accelZ * d.accelZ);
        if ((mag < 2.0f || mag > 25.0f) && !fallAlertActive) {
            fallAlertActive = true;
            fallAlertStart  = millis();
            Serial.println("[Display] Fall detected!");
        }

        // ── Serial screen switching ───────────────────────
        if (Serial.available()) {
            char c = Serial.read();
            if (c == 'E' || c == 'e') {
                currentScreen = SCREEN_ENV;
                tft.clearScreen();
            } else if (c == 'H' || c == 'h') {
                currentScreen = SCREEN_HOME;
                tft.clearScreen();
            } else if (c == 'F' || c == 'f') {
                fallAlertActive = true;
                fallAlertStart  = millis();
            }
        }

        // ── Draw ─────────────────────────────────────────
        if (fallAlertActive) {
            if (millis() - fallAlertStart < 5000) {
                tft.drawFallAlert(fallAlertStart);
            } else {
                fallAlertActive = false;
                tft.clearScreen();
            }
        } else {
            heartBeatTick = !heartBeatTick;
            if (currentScreen == SCREEN_HOME) {
                tft.drawHomeScreen(d, heartBeatTick);
            } else {
                tft.drawEnvScreen(d);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Redraw every 1s
    }
}

bool displayInit() {
    if (!tft.begin()) {
        Serial.println("[Display] TFT init failed!");
        return false;
    }
    Serial.println("[Display] TFT ready.");

    xTaskCreate(displayTask, "Display", 8192, NULL, 1, NULL);
    return true;
}