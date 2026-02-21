#pragma once
#include <Adafruit_AHTX0.h>

class AHT21Driver {
public:
    bool begin();
    bool read(float &temperature, float &humidity);

private:
    Adafruit_AHTX0 aht;
};