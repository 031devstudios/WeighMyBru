#include <HX711.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include "PageIndex.h"

#define LOADCELL_DOUT_PIN 12
#define LOADCELL_SCK_PIN 11

#define weight_of_object_for_calibration 111
#define FIRMWARE_VERSION "1.0.0"

const char* ssid = "Robb_Ext";
const char* password = "0004ed931192";

unsigned long lastTime = 0;
unsigned long timerDelay = 50;

float LOAD_CALIBRATION_FACTOR;

int weight_In_g;
int last_weight_In_g;
float weight_In_oz;

HX711 LOADCELL_HX711;
Preferences preferences;
JSONVar JSON_All_Data;

AsyncWebServer server(80);
AsyncEventSource events("/events");

volatile bool hx711Busy = false;

void scale_Tare() {
  // Wait until HX711 is not busy
  while (hx711Busy) { delay(1); }
  hx711Busy = true;
  // Try to re-initialize if not ready
  if (!LOADCELL_HX711.is_ready()) {
    LOADCELL_HX711.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    delay(100);
  }
  if (LOADCELL_HX711.is_ready()) {
    LOADCELL_HX711.tare();
    unsigned long start = millis();
    while (!LOADCELL_HX711.is_ready() && millis() - start < 1000) {
      delay(1);
    }
    if (!LOADCELL_HX711.is_ready()) {
      Serial.println("Error: HX711 not ready after tare.");
      hx711Busy = false;
      return;
    }
    delay(100);
    Serial.println("Scale Tared");
  } else {
    Serial.println("Error: HX711 not found.");
    hx711Busy = false;
    return;
  }
  hx711Busy = false;
}

void loadcell_Calibration() {
  Serial.println("Start calibration...");

  if (LOADCELL_HX711.is_ready()) {
    LOADCELL_HX711.set_scale();
    delay(100);
    LOADCELL_HX711.tare();
    delay(100);

    Serial.println("Place a known object on the scale...");

    long sensor_Reading_Results = 0;
    for (byte i = 0; i < 5; i++) {
      sensor_Reading_Results = LOADCELL_HX711.get_units(10);
      Serial.printf("Reading %d: %ld\n", i + 1, sensor_Reading_Results);
      delay(1000);
    }

    float CALIBRATION_FACTOR = sensor_Reading_Results / weight_of_object_for_calibration;
    preferences.putFloat("CFVal", CALIBRATION_FACTOR);
    LOAD_CALIBRATION_FACTOR = preferences.getFloat("CFVal", 0);

    Serial.printf("Calibration complete. Factor: %.6f\n", LOAD_CALIBRATION_FACTOR);
    LOADCELL_HX711.set_scale(LOAD_CALIBRATION_FACTOR);
  } else {
    Serial.println("Error: HX711 not found.");
    while (1) delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start Setup...");

  preferences.begin("CF", false);
  LOAD_CALIBRATION_FACTOR = preferences.getFloat("CFVal", 0);

  Serial.print("Calibration factor: ");
  Serial.println(LOAD_CALIBRATION_FACTOR);

  LOADCELL_HX711.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  LOADCELL_HX711.set_scale(LOAD_CALIBRATION_FACTOR);
  LOADCELL_HX711.tare();

  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);

  int timeout = 40;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed. Restarting...");
    ESP.restart();
  }

  Serial.println("\nWiFi conn ected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", MAIN_page);
  });

  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });

  server.addHandler(&events);

  // Fix TARE button: accept GET as well as POST for /tare
  server.on("/tare", HTTP_ANY, [](AsyncWebServerRequest *request) {
    scale_Tare();
    request->send(200, "text/plain", "Tared");
  });

  server.on("/fwver", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", FIRMWARE_VERSION);
  });

  server.begin();

  Serial.println("Server started. Scale is ready to use.");
}

void loop() {
  if (LOADCELL_HX711.wait_ready_timeout(100)) {
    if (!hx711Busy) {
      hx711Busy = true;
      weight_In_g = LOADCELL_HX711.get_units(2);
      weight_In_oz = (float)weight_In_g / 28.3495;
      hx711Busy = false;

      if ((millis() - lastTime) > timerDelay && weight_In_g != last_weight_In_g) {
        JSON_All_Data["weight_In_g"] = weight_In_g;
        String JSON_All_Data_Send = JSON.stringify(JSON_All_Data);

        events.send("ping", NULL, millis());
        events.send(JSON_All_Data_Send.c_str(), "allDataJSON", millis());

        last_weight_In_g = weight_In_g;
        lastTime = millis();
      }
    }
  }
}
