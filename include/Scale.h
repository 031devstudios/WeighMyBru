#ifndef SCALE_H
#define SCALE_H

#include <HX711.h>
#include <Preferences.h>

class Scale {
public:
    Scale(uint8_t dataPin, uint8_t clockPin, float calibrationFactor);
    void begin();
    void tare(uint8_t times = 20);
    void set_scale(float factor);
    float getWeight();
    float getCurrentWeight();
    long getRawValue();
    void saveCalibration(); // Save calibration factor to NVS
    void loadCalibration(); // Load calibration factor from NVS
    float getCalibrationFactor() const { return calibrationFactor; } // Getter for API
private:
    HX711 hx711;
    Preferences preferences;
    uint8_t dataPin;
    uint8_t clockPin;
    float calibrationFactor = 0.0f;
    float currentWeight;
};

#endif
