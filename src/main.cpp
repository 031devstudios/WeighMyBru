#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "WebServer.h"
#include "Scale.h"
#include "WiFiManager.h"
#include "FlowRate.h"
#include "Calibration.h"
#include "BluetoothScale.h"

// Pins and calibration
uint8_t dataPin = 12;
uint8_t clockPin = 11;
float calibrationFactor = 4762.1621;
Scale scale(dataPin, clockPin, calibrationFactor);
FlowRate flowRate;
BluetoothScale bluetoothScale;

void setup() {
  Serial.begin(115200);

  setupWiFi();

  scale.begin();
  
  // Initialize Bluetooth scale
  bluetoothScale.begin(&scale);

  setupWebServer(scale, flowRate, bluetoothScale);
}

void loop() {
  static unsigned long lastDebugTime = 0;
  static unsigned long loopCount = 0;
  static unsigned long lastWeightUpdate = 0;
  
  unsigned long loopStart = millis();
  
  // Update weight more frequently for brewing accuracy
  if (millis() - lastWeightUpdate >= 10) { // Update every 10ms
    float weight = scale.getWeight();
    flowRate.update(weight);
    lastWeightUpdate = millis();
  }
  
  // Update Bluetooth scale (less critical timing)
  bluetoothScale.update();
  
  loopCount++;
  
  // Debug performance every 10 seconds (reduced frequency)
  if (millis() - lastDebugTime > 10000) {
    unsigned long avgLoopTime = (millis() - lastDebugTime) / loopCount;
    Serial.printf("Loop: %lu loops, avg %lu ms/loop\n", loopCount, avgLoopTime);
    lastDebugTime = millis();
    loopCount = 0;
  }
  
  unsigned long loopDuration = millis() - loopStart;
  if (loopDuration > 50) { // Warn if loop takes more than 50ms
    Serial.printf("SLOW LOOP: %lu ms\n", loopDuration);
  }
  
  // Shorter delay for more responsive weight readings
  delay(5); // Reduced from 100ms to 5ms for brewing responsiveness
}
