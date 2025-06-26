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
  float weight = scale.getWeight();
  flowRate.update(weight);
  
  // Update Bluetooth scale
  bluetoothScale.update();
  
  delay(100); // Adjust as needed for your update rate
}
