#include <Wire.h>
#include "aht21_driver.h"
#include <Adafruit_AHTX0.h>

bool AHT21Driver::begin() {
    return aht.begin();
}

bool AHT21Driver::read(float &temperature, float &humidity) {
    sensors_event_t temp, hum;
    aht.getEvent(&hum, &temp);
    temperature = temp.temperature;
    humidity = hum.relative_humidity;
    return true;
}