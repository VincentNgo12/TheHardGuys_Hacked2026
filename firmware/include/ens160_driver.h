#pragma once
#include <ScioSense_ENS160.h>

class ENS160Driver {
public:
    bool begin();
    bool read(uint16_t &eco2, uint16_t &tvoc, int8_t &aqi);
    void    setCompensation(float temperature, float humidity);
    bool    isDataReady();   // true when sensor has fresh, valid data

private:
    ScioSense_ENS160 ens160{ENS160_I2CADDR_1}; // 0x53 — if ADDR pin pulled high
};