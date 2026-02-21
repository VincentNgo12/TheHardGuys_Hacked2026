#include "ens160_driver.h"

bool ENS160Driver::begin() {
    if (!ens.begin()) {
        return false;
    }
    ens.setMode(ENS160_OPMODE_STD);
    return true;
}

bool ENS160Driver::read(uint16_t &eco2, uint16_t &tvoc, int8_t &aqi) {
    ens.measure(true);
    eco2 = ens.eco2;
    tvoc = ens.tvoc;
    aqi = ens.aqi;
    return true;
}