#include <Arduino.h>
#include "task_sensor.h"
#include "task_display.h"
#include "task_power.h"

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("Booting Wristwatch Firmware...");

    // Create FreeRTOS tasks
    xTaskCreate(task_sensor, "SensorTask", 4096, NULL, 2, NULL);
    xTaskCreate(task_display, "DisplayTask", 4096, NULL, 2, NULL);
    xTaskCreate(task_power, "PowerTask", 4096, NULL, 1, NULL);
}

void loop() {
    // Let FreeRTOS take over
    vTaskDelay(portMAX_DELAY);
}