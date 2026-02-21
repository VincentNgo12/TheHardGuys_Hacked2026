#include <Arduino.h>
#include <Wire.h>

#include "Adafruit_AHTX0"
#include "ens160_driver.h"

AHT21Driver aht21;
ENS160Driver ens160;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Starting I2C...");
    Wire.begin(21, 22);  // SDA, SCL for ESP32

    Serial.println("Initializing AHT21...");
    if (!aht21.begin()) {
        Serial.println("AHT21 FAILED!");
    } else {
        Serial.println("AHT21 OK.");
    }

    Serial.println("Initializing ENS160...");
    if (!ens160.begin()) {
        Serial.println("ENS160 FAILED!");
    } else {
        Serial.println("ENS160 OK.");
    }
}

void loop() {
    float temp, hum;
    uint16_t eco2, tvoc;
    int8_t aqi;

    aht21.read(temp, hum);
    ens160.read(eco2, tvoc, aqi);

    Serial.println("------ SENSOR DATA ------");
    Serial.printf("AHT21: %.2f °C, %.2f %%RH\n", temp, hum);
    Serial.printf("ENS160: eCO2=%u ppm, TVOC=%u ppb, AQI=%d\n",
                  eco2, tvoc, aqi);

    delay(1000);
}