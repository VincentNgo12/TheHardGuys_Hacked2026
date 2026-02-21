#include "max30102_driver.h"
#include <Arduino.h>

bool MAX30102Driver::begin() {
    if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
        return false;
    }

    // Configure sensor for HR/SpO2
    // powerLevel=0x24 (7.2mA), sampleAverage=4, ledMode=2 (Red+IR), 
    // sampleRate=100, pulseWidth=411, adcRange=4096
    sensor.setup(0x24, 4, 2, 100, 411, 4096);
    
    // Fill buffers with zeros initially
    for (int i = 0; i < BUFFER_SIZE_INTERNAL; i++) {
        redBuffer[i] = 0;
        irBuffer[i] = 0;
    }
    
    return true;
}

bool MAX30102Driver::read(float &heartRate, float &spO2, bool &fingerDetected) {
    // 1. Read from FIFO (Hold I2C briefly)
    sensor.check();
    
    bool newData = false;
    while (sensor.available()) {
        // Shift buffers left
        for (int i = 1; i < BUFFER_SIZE_INTERNAL; i++) {
            redBuffer[i - 1] = redBuffer[i];
            irBuffer[i - 1] = irBuffer[i];
        }
        
        redBuffer[BUFFER_SIZE_INTERNAL - 1] = sensor.getFIFORed();
        irBuffer[BUFFER_SIZE_INTERNAL - 1] = sensor.getFIFOIR();
        sensor.nextSample();
        
        samplesCollected++;
        newData = true;
    }

    if (!newData && samplesCollected < 25) {
        // No new data, just return last values
        heartRate = (hrValid) ? (float)lastHeartRate : 0.0f;
        spO2      = (spo2Valid) ? (float)lastSpO2 : 0.0f;
        fingerDetected = (irBuffer[BUFFER_SIZE_INTERNAL - 1] > 50000);
        return true;
    }

    // 2. Simple finger detection
    fingerDetected = (irBuffer[BUFFER_SIZE_INTERNAL - 1] > 50000);

    // 3. Run algorithm every ~1 second (25 samples at 25Hz effective)
    if (samplesCollected >= 25 && fingerDetected) {
        // This is CPU intensive but runs in the MAX30102 task context
        maxim_heart_rate_and_oxygen_saturation(
            irBuffer, BUFFER_SIZE_INTERNAL, redBuffer, 
            &lastSpO2, &spo2Valid, &lastHeartRate, &hrValid
        );
        samplesCollected = 0;
    } else if (!fingerDetected) {
        lastHeartRate = 0;
        lastSpO2 = 0;
        hrValid = 0;
        spo2Valid = 0;
        samplesCollected = 0;
    }

    // 4. Update output values
    heartRate = (hrValid && lastHeartRate > 0 && lastHeartRate < 200) ? (float)lastHeartRate : 0.0f;
    spO2      = (spo2Valid && lastSpO2 > 70 && lastSpO2 <= 100) ? (float)lastSpO2 : 0.0f;

    return true;
}
