// src/tasks/task_display.cpp
#include "task_display.h"
#include "task_sensor.h"
#include <Arduino.h>

static TFTDriver tft;
static Screen    currentScreen    = SCREEN_HOME;
static bool      fallAlertActive  = false;
static uint32_t  fallAlertStart   = 0;

static void displayTask(void *pvParameters) {
    bool heartBeatTick = false;
    bool forceRedraw = true;

    // Initial splash
    tft.drawSplash();
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    while (true) {
        // ── Copy sensor data under lock ───────────────────
        // *** g_dataMutex is from task_sensor.h ***
        // *** g_sensorData used to read sensors state is also from task_sensor.h ***
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
        if ((mag < 0.5f || mag > 2.0f) && !fallAlertActive) {
            fallAlertActive = true;
            fallAlertStart  = millis();
            forceRedraw = true;
            Serial.println("[Display] Fall detected!");
        }

        // ── Serial screen switching ───────────────────────
        if (Serial.available()) {
            char c = Serial.read();
            if ((c == 'E' || c == 'e') && currentScreen != SCREEN_ENV) {
                currentScreen = SCREEN_ENV;
                forceRedraw = true;
            } else if ((c == 'H' || c == 'h') && currentScreen != SCREEN_HOME) {
                currentScreen = SCREEN_HOME;
                forceRedraw = true;
            } else if (c == 'F' || c == 'f') {
                fallAlertActive = true;
                fallAlertStart  = millis();
                forceRedraw = true;
            }
        }

        // ── Draw ─────────────────────────────────────────
        if (fallAlertActive) {
            if (millis() - fallAlertStart < 5000) {
                if(forceRedraw) tft.clearScreen();
                tft.drawFallAlert(fallAlertStart);
            } else {
                fallAlertActive = false;
                forceRedraw = true;
                tft.clearScreen();
            }
        } else {
            heartBeatTick = !heartBeatTick;
            if (currentScreen == SCREEN_HOME) {
                if(forceRedraw) tft.clearScreen();
                tft.updateHomeScreen(d, heartBeatTick);
            } else {
                if(forceRedraw) tft.drawEnvScreen_BG();
                tft.updateEnvScreen(d);
            }
        }
        forceRedraw = false; // reset after draw
        vTaskDelay(pdMS_TO_TICKS(1000)); // Redraw every 1s
    }
}

bool displayInit() {
    if (!tft.begin()) {
        Serial.println("[Display] TFT init failed!");
        return false;
    }
    Serial.println("[Display] TFT ready.");

    xTaskCreate(displayTask, "Display", 8192, NULL, 15, NULL);
    return true;
}