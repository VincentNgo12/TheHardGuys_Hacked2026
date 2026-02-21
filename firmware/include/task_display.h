// include/task_display.h
#pragma once

#include "tft_driver.h"
#include "task_sensor.h"

// Inits TFT and launches display task
// Call once in setup() after sensorInit()
bool displayInit();