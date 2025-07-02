#include "Scale.h"
#include "WebServer.h"
#include "Calibration.h"

Scale::Scale(uint8_t dataPin, uint8_t clockPin, float calibrationFactor)
    : dataPin(dataPin), clockPin(clockPin), calibrationFactor(calibrationFactor), currentWeight(0.0f) {}

void Scale::begin() {
    preferences.begin("scale", false);
    calibrationFactor = preferences.getFloat("calib", calibrationFactor);
    preferences.end();
    hx711.begin(dataPin, clockPin);
    hx711.set_scale(calibrationFactor);
    tare();
}

void Scale::tare(uint8_t times) {
    hx711.tare(times);
}

void Scale::set_scale(float factor) {
    // Only save if the calibration factor actually changed
    if (calibrationFactor != factor) {
        calibrationFactor = factor;
        hx711.set_scale(calibrationFactor);
        saveCalibration();
    }
}

void Scale::saveCalibration() {
    preferences.begin("scale", false);
    preferences.putFloat("calib", calibrationFactor);
    preferences.end();
}

void Scale::loadCalibration() {
    preferences.begin("scale", true);
    calibrationFactor = preferences.getFloat("calib", calibrationFactor);
    preferences.end();
}

float Scale::getWeight() {
    float weight = hx711.get_units(1);
    if (!isnan(weight)) {
        currentWeight = weight;
    }
    return currentWeight;
}

float Scale::getCurrentWeight() {
    return currentWeight;
}

long Scale::getRawValue() {
    return hx711.get_value(1); // Get raw value from HX711
}
