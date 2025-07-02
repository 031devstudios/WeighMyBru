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

// Cache for display settings to avoid repeated slow EEPROM reads
static int cachedDecimals = -1; // -1 indicates not cached yet
static unsigned long lastDecimalCacheTime = 0;
const unsigned long DECIMAL_CACHE_TIMEOUT = 300000; // 5 minutes cache timeout

int getCachedDecimals() {
    // Fast path - return immediately if already cached and recent
    if (cachedDecimals != -1 && (millis() - lastDecimalCacheTime < DECIMAL_CACHE_TIMEOUT)) {
        return cachedDecimals;
    }
    
    unsigned long startTime = millis();
    
    if (preferences.begin("display", false)) {
        cachedDecimals = preferences.getInt("decimals", 1);
        preferences.end();
        lastDecimalCacheTime = millis();
        Serial.printf("Display: OK in %lums\n", millis() - startTime);
    } else {
        cachedDecimals = 1; // Use default
        lastDecimalCacheTime = millis();
        Serial.println("Display: FAIL");
    }
    
    return cachedDecimals;
}

void setCachedDecimals(int decimals) {
    Serial.println("Saving decimal setting...");
    unsigned long startTime = millis();
    
    if (preferences.begin("display", false)) {
        preferences.putInt("decimals", decimals);
        preferences.end();
        cachedDecimals = decimals; // Update cache
        lastDecimalCacheTime = millis();
        Serial.printf("Decimal setting saved in %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Failed to save decimal setting to EEPROM");
    }
}

void diagnoseEEPROMPerformance() {
    Serial.println("=== EEPROM Performance Diagnostics ===");
    
    // Test WiFi preferences
    unsigned long startTime = millis();
    Preferences testPrefs;
    if (testPrefs.begin("test", false)) {
        testPrefs.putInt("testkey", 42);
        int val = testPrefs.getInt("testkey", 0);
        testPrefs.end();
        Serial.printf("EEPROM test write/read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open test preferences namespace");
    }
    
    // Test existing namespaces
    startTime = millis();
    if (testPrefs.begin("wifi", true)) {
        String ssid = testPrefs.getString("ssid", "");
        testPrefs.end();
        Serial.printf("WiFi namespace read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open wifi preferences namespace");
    }
    
    startTime = millis();
    if (testPrefs.begin("display", true)) {
        int decimals = testPrefs.getInt("decimals", 1);
        testPrefs.end();
        Serial.printf("Display namespace read took: %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Cannot open display preferences namespace");
    }
    
    Serial.println("=== End Diagnostics ===");
}

AsyncWebServer server(80);

/*
 * API Endpoints for External Brewing Systems (e.g., GaggiMate):
 * 
 * Ultra-fast weight reading (minimal latency):
 * GET /api/brew/weight
 * Response: "45.2" (weight in grams, 1 decimal)
 * 
 * Fast brewing status:
 * GET /api/brew/status  
 * Response: {"w":45.2,"f":2.1} (weight and flowrate)
 * 
 * Standard dashboard:
 * GET /api/dashboard
 * Response: {"weight":45.23,"flowrate":2.15}
 */

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale) {
  if (!LittleFS.begin()) {
    return;
  }

  // Run EEPROM diagnostics
  diagnoseEEPROMPerformance();

  // Pre-cache settings to avoid delays on first page load
  Serial.println("Pre-caching settings for faster page loads...");
  getCachedDecimals();        // This will cache the decimal setting
  getStoredSSID();            // This will cache WiFi credentials

  // Register API route first
  server.on("/api/dashboard", HTTP_GET, [&scale, &flowRate](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"weight\":" + String(scale.getCurrentWeight(), 2) + ",";
    json += "\"flowrate\":" + String(flowRate.getFlowRate(), 1);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/weight", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(scale.getCurrentWeight()));
  });

  // Lightweight weight-only endpoint for brewing applications
  server.on("/api/weight-fast", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    // Minimal processing for fastest response
    request->send(200, "text/plain", String(scale.getCurrentWeight(), 2));
  });

  // Brewing mode endpoints for external devices like GaggiMate
  server.on("/api/brew/weight", HTTP_GET, [&scale](AsyncWebServerRequest *request) {
    // Ultra-fast response for brewing systems
    float weight = scale.getCurrentWeight();
    request->send(200, "text/plain", String(weight, 1)); // 1 decimal for speed
  });
  
  server.on("/api/brew/status", HTTP_GET, [&scale, &flowRate](AsyncWebServerRequest *request) {
    // Minimal JSON for brewing systems
    String json = "{\"w\":" + String(scale.getCurrentWeight(), 1) + 
                  ",\"f\":" + String(flowRate.getFlowRate(), 1) + "}";
    request->send(200, "application/json", json);
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
    int decimals = getCachedDecimals();
    String json = "{\"decimals\":" + String(decimals) + "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/decimal-setting", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("decimals", true)) {
      int decimals = request->getParam("decimals", true)->value().toInt();
      if (decimals < 0) decimals = 0;
      if (decimals > 2) decimals = 2;
      setCachedDecimals(decimals);
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
    // Get WiFi credentials (from cache)
    String ssid = getStoredSSID();
    String password = getStoredPassword();
    
    // Get decimal setting (from cache)
    int decimals = getCachedDecimals();
    
    // Combine into single JSON response
    String json = "{";
    json += "\"ssid\":\"" + ssid + "\",";
    json += "\"password\":\"" + password + "\",";
    json += "\"decimals\":" + String(decimals);
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // Emergency NVS reset endpoint (use with caution)
  server.on("/api/reset-nvs", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("confirm", true) && request->getParam("confirm", true)->value() == "yes") {
      Serial.println("Resetting NVS storage...");
      
      // Clear all preferences
      Preferences clearPrefs;
      clearPrefs.begin("wifi", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      clearPrefs.begin("display", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      clearPrefs.begin("scale", false);
      clearPrefs.clear();
      clearPrefs.end();
      
      request->send(200, "text/plain", "NVS storage reset. Device will restart in 3 seconds.");
      
      // Restart the ESP32 after a short delay
      delay(3000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Missing confirmation parameter. Use 'confirm=yes' to reset NVS.");
    }
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
