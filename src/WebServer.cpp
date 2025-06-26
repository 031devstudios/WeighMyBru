#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "WebServer.h"
#include "Scale.h"
#include "WiFiManager.h"
#include <Preferences.h>
#include "FlowRate.h"
#include "Calibration.h"
#include "BluetoothScale.h"

Preferences preferences;

// No duplicate function definitions here; only use those from WiFiManager.cpp

AsyncWebServer server(80);

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale) {
  if (!LittleFS.begin()) {
    return;
  }

  // Pre-initialize preferences namespaces to speed up first-time access
  preferences.begin("display", false);
  preferences.getInt("decimals", 1); // Touch the namespace to initialize it
  preferences.end();
  
  // Pre-initialize WiFi preferences namespace
  Preferences wifiPrefs;
  wifiPrefs.begin("wifi", false);
  wifiPrefs.getString("ssid", ""); // Touch the namespace to initialize it
  wifiPrefs.end();

  // Register API route first
  server.on("/api/weight", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(scale.getCurrentWeight()));
  });

  server.on("/api/tare", HTTP_POST, [&scale](AsyncWebServerRequest *request){
    scale.tare(20);
    request->send(200, "text/plain", "Scale tared!");
  });

  server.on("/api/set-calibrationfactor", HTTP_POST, [&scale](AsyncWebServerRequest *request){
  if (request->hasParam("calibrationfactor", true)) {
    String value = request->getParam("calibrationfactor", true)->value();
    float calibrationFactor = value.toFloat();
    Serial.printf("Updated calibration factor weight: %.2f\n", calibrationFactor);
    request->send(200, "text/plain", "Calibration factor updated to " + value);
    scale.set_scale(calibrationFactor); // Assuming you have a method to set the calibration factor in Scale class
  } else {
    request->send(400, "text/plain", "Missing 'calibrationfactor' parameter");
  }
});

  server.on("/api/calibrate", HTTP_POST, [&scale](AsyncWebServerRequest *request){
    if (request->hasParam("knownWeight", true)) {
      String value = request->getParam("knownWeight", true)->value();
      float knownWeight = value.toFloat();
      // Read raw value from the scale (uncalibrated)
      long raw = scale.getRawValue();
      if (knownWeight > 0 && raw != 0) {
        float newCalibrationFactor = (float)raw / knownWeight;
        scale.set_scale(newCalibrationFactor);
        Serial.printf("Calibration complete. New factor: %.6f\n", newCalibrationFactor);
        request->send(200, "text/plain", "Scale calibrated! New factor: " + String(newCalibrationFactor, 6));
      } else {
        request->send(400, "text/plain", "Invalid known weight or scale reading");
      }
    } else {
      request->send(400, "text/plain", "Missing 'knownWeight' parameter");
    }
  });

  server.on("/api/calibrationfactor", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(scale.getCalibrationFactor(), 6));
  });

  server.on("/api/wifi-creds", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ssid = getStoredSSID();
    String password = getStoredPassword();
    String json = "{\"ssid\":\"" + ssid + "\",\"password\":\"" + password + "\"}";
    request->send(200, "application/json", json);
  });

  server.on("/api/wifi-creds", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      saveWiFiCredentials(ssid.c_str(), password.c_str());
      request->send(200, "text/plain", "WiFi credentials saved. Reboot to connect.");
    } else {
      request->send(400, "text/plain", "Missing SSID or password");
    }
  });

  server.on("/api/decimal-setting", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("display", false); // Use false to create namespace if it doesn't exist
    int decimals = preferences.getInt("decimals", 1);
    preferences.end();
    String json = "{\"decimals\":" + String(decimals) + "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/decimal-setting", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("decimals", true)) {
      int decimals = request->getParam("decimals", true)->value().toInt();
      if (decimals < 0) decimals = 0;
      if (decimals > 2) decimals = 2;
      preferences.begin("display", false);
      preferences.putInt("decimals", decimals);
      preferences.end();
      request->send(200, "text/plain", "Decimal setting saved.");
    } else {
      request->send(400, "text/plain", "Missing decimals parameter");
    }
  });

  server.on("/api/flowrate", HTTP_GET, [&flowRate](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(flowRate.getFlowRate(), 1));
  });

  // Bluetooth status API
  server.on("/api/bluetooth/status", HTTP_GET, [&bluetoothScale](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"connected\":" + String(bluetoothScale.isConnected() ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
  });

  // Combined settings endpoint for faster loading
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Get WiFi credentials
    String ssid = getStoredSSID();
    String password = getStoredPassword();
    
    // Get decimal setting
    preferences.begin("display", false);
    int decimals = preferences.getInt("decimals", 1);
    preferences.end();
    
    // Combine into single JSON response
    String json = "{";
    json += "\"ssid\":\"" + ssid + "\",";
    json += "\"password\":\"" + password + "\",";
    json += "\"decimals\":" + String(decimals);
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // Serve static files for non-API paths
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // 404 Not Found handler for unmatched routes
  server.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    // If the request is for an API endpoint that doesn't exist, return 404
    if (path.startsWith("/api/")) {
      request->send(404, "text/plain", "API endpoint not found");
      return;
    }
    // For all other unmatched paths, serve index.html (SPA fallback)
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.begin();
}
