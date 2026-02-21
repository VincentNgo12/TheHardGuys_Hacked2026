// include/max30102_driver.h
#pragma once

#include <MAX30105.h>
#include <spo2_algorithm.h>

class MAX30102Driver {
public:
    bool begin();
    bool read(float &heartRate, float &spO2, bool &fingerDetected);

private:
    MAX30105 sensor;

    // Buffer for 100 samples (4 seconds at 25Hz)
    static const uint8_t BUFFER_SIZE_INTERNAL = 100;
    uint32_t irBuffer[BUFFER_SIZE_INTERNAL];
    uint32_t redBuffer[BUFFER_SIZE_INTERNAL];

    int32_t lastHeartRate = 0;
    int32_t lastSpO2 = 0;
    int8_t  hrValid = 0;
    int8_t  spo2Valid = 0;

    int samplesCollected = 0;
};