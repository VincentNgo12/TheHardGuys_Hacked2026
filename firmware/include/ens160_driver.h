#pragma once
#include <ScioSense_ENS160.h>

class ENS160Driver {
public:
    bool begin();
    bool read(uint16_t &eco2, uint16_t &tvoc, int8_t &aqi);

private:
    ScioSense_ENS160 ens160;
};