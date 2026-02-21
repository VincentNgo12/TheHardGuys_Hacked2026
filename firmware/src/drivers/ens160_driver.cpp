#include <Wire.h>
#include "ens160_driver.h"
#include <ScioSense_ENS160.h>

bool ENS160Driver::begin() {
    bool ok = ens160.begin();

    if (!ens160.available()) return false;

    // Use standard mode for straightforward eCO2/TVOC/AQI output
    return ens160.setMode(ENS160_OPMODE_STD);
}

bool ENS160Driver::read(uint16_t &eco2, uint16_t &tvoc, int8_t &aqi) {
    if (!ens160.available()) return false;

    ens160.measure(true);      // triggers a blocking measurement
    ens160.measureRaw(false);  // raw not needed for standard readings

    eco2 = ens160.geteCO2();
    tvoc = ens160.getTVOC();
    aqi  = ens160.getAQI();

    return true;
}