#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>

Preferences wifiPrefs;

// Station credentials
char stored_ssid[33] = {0};
char stored_password[65] = {0};

// Cache for WiFi credentials to avoid repeated slow EEPROM reads
static String cachedSSID = "";
static String cachedPassword = "";
static bool credentialsCached = false;
static unsigned long lastCacheTime = 0;
const unsigned long CACHE_TIMEOUT = 300000; // 5 minutes cache timeout

// AP credentials
const char* ap_ssid = "WeighMyBru-AP";
const char* ap_password = "";

unsigned long startAttemptTime = 0;
const unsigned long timeout = 10000; // 10 seconds

void saveWiFiCredentials(const char* ssid, const char* password) {
    Serial.println("Saving WiFi credentials...");
    unsigned long startTime = millis();
    
    if (wifiPrefs.begin("wifi", false)) {
        wifiPrefs.putString("ssid", ssid);
        wifiPrefs.putString("password", password);
        wifiPrefs.end();
        
        // Update cache immediately
        cachedSSID = String(ssid);
        cachedPassword = String(password);
        credentialsCached = true;
        lastCacheTime = millis();
        
        Serial.printf("WiFi credentials saved in %lu ms\n", millis() - startTime);
    } else {
        Serial.println("ERROR: Failed to open WiFi preferences for writing");
    }
}

bool loadWiFiCredentialsFromEEPROM() {
    // Check if cache is still valid first (no serial output for speed)
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return true;
    }
    
    unsigned long startTime = millis();
    
    // Add timeout protection
    const unsigned long EEPROM_TIMEOUT = 5000; // 5 second timeout
    bool success = false;
    
    unsigned long attemptStart = millis();
    if (wifiPrefs.begin("wifi", true)) {
        // Check if operation is taking too long
        if (millis() - attemptStart > EEPROM_TIMEOUT) {
            wifiPrefs.end();
            cachedSSID = "";
            cachedPassword = "";
        } else {
            cachedSSID = wifiPrefs.getString("ssid", "");
            cachedPassword = wifiPrefs.getString("password", "");
            success = true;
        }
        wifiPrefs.end();
        
        credentialsCached = true;
        lastCacheTime = millis();
        
        // Minimal serial output to reduce blocking
        Serial.printf("WiFi: %s in %lums\n", success ? "OK" : "TIMEOUT", millis() - startTime);
        return success;
    } else {
        // Use empty defaults if EEPROM fails
        cachedSSID = "";
        cachedPassword = "";
        credentialsCached = true;
        lastCacheTime = millis();
        Serial.println("WiFi: EEPROM FAIL");
        return false;
    }
}

void loadWiFiCredentials(char* ssid, char* password, size_t maxLen) {
    loadWiFiCredentialsFromEEPROM();
    strncpy(ssid, cachedSSID.c_str(), maxLen - 1);
    strncpy(password, cachedPassword.c_str(), maxLen - 1);
    ssid[maxLen - 1] = '\0';
    password[maxLen - 1] = '\0';
}

String getStoredSSID() {
    // Fast path - return immediately if already cached and recent
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return cachedSSID;
    }
    
    // Only do EEPROM read if cache is invalid
    loadWiFiCredentialsFromEEPROM();
    return cachedSSID;
}

String getStoredPassword() {
    // Fast path - return immediately if already cached and recent
    if (credentialsCached && (millis() - lastCacheTime < CACHE_TIMEOUT)) {
        return cachedPassword;
    }
    
    // Only do EEPROM read if cache is invalid
    loadWiFiCredentialsFromEEPROM();
    return cachedPassword;
}

void setupWiFi() {
    char ssid[33] = {0};
    char password[65] = {0};
    loadWiFiCredentials(ssid, password, sizeof(ssid));
    startAttemptTime = millis();

    // Start AP mode
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("AP IP: " + WiFi.softAPIP().toString());

    // Start Station mode if credentials exist
    if (strlen(ssid) > 0) {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to STA with IP: " + WiFi.localIP().toString());
        } else {
            Serial.println("\nFailed to connect to STA. Continuing in AP mode...");
        }
    }
}
