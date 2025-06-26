#include "FlowRate.h"
#include "Calibration.h"
#include <Arduino.h>

FlowRate::FlowRate() : lastWeight(0), lastTime(0), flowRate(0), bufferIndex(0), bufferCount(0) {
    for (int i = 0; i < FLOWRATE_AVG_WINDOW; ++i) flowRateBuffer[i] = 0;
}

void FlowRate::update(float currentWeight) {
    unsigned long now = millis();
    if (lastTime > 0) {
        float deltaWeight = currentWeight - lastWeight;
        float deltaTime = (now - lastTime) / 1000.0f; // seconds
        if (deltaTime > 0) {
            float instantRate = deltaWeight / deltaTime;
            flowRateBuffer[bufferIndex] = instantRate;
            bufferIndex = (bufferIndex + 1) % FLOWRATE_AVG_WINDOW;
            if (bufferCount < FLOWRATE_AVG_WINDOW) bufferCount++;
            // Calculate average
            float sum = 0;
            for (int i = 0; i < bufferCount; ++i) sum += flowRateBuffer[i];
            flowRate = sum / bufferCount;
        }
    }
    lastWeight = currentWeight;
    lastTime = now;
}

float FlowRate::getFlowRate() const {
    return flowRate;
}
