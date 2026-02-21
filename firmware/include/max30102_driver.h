// include/max30102_driver.h
#pragma once

class MAX30102Driver {
public:
    bool begin();
    bool read(float &heartRate, float &spO2, bool &fingerDetected);
};