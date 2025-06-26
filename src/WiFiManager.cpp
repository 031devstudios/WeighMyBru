#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>

Preferences wifiPrefs;

// Station credentials
char stored_ssid[33] = {0};
char stored_password[65] = {0};

// AP credentials
const char* ap_ssid = "WeighMyBru-AP";
const char* ap_password = "";

unsigned long startAttemptTime = 0;
const unsigned long timeout = 10000; // 10 seconds

void saveWiFiCredentials(const char* ssid, const char* password) {
    wifiPrefs.begin("wifi", false);
    wifiPrefs.putString("ssid", ssid);
    wifiPrefs.putString("password", password);
    wifiPrefs.end();
}

void loadWiFiCredentials(char* ssid, char* password, size_t maxLen) {
    wifiPrefs.begin("wifi", true);
    String s = wifiPrefs.getString("ssid", "");
    String p = wifiPrefs.getString("password", "");
    wifiPrefs.end();
    strncpy(ssid, s.c_str(), maxLen);
    strncpy(password, p.c_str(), maxLen);
}

String getStoredSSID() {
    wifiPrefs.begin("wifi", true);
    String s = wifiPrefs.getString("ssid", "");
    wifiPrefs.end();
    return s;
}

String getStoredPassword() {
    wifiPrefs.begin("wifi", true);
    String p = wifiPrefs.getString("password", "");
    wifiPrefs.end();
    return p;
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
