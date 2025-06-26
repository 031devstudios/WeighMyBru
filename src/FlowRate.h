#pragma once

#define FLOWRATE_AVG_WINDOW 10

class FlowRate {
public:
    FlowRate();
    void update(float currentWeight);
    float getFlowRate() const; // grams per second
private:
    float lastWeight;
    unsigned long lastTime;
    float flowRate;
    float flowRateBuffer[FLOWRATE_AVG_WINDOW];
    int bufferIndex;
    int bufferCount;
};
