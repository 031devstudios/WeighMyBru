#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

void setupWiFi();
void saveWiFiCredentials(const char* ssid, const char* password);
void loadWiFiCredentials(char* ssid, char* password, size_t maxLen);
String getStoredSSID();
String getStoredPassword();

#endif
