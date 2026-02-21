// src/main.cpp
#include <Arduino.h>
#include <Wire.h>
#include "task_sensor.h"
#include "task_display.h"


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

    Serial.println("=== All systems go ===");
}

void loop() {
    vTaskDelete(NULL);
}