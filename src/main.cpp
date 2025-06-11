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

float weight_In_g = 0.0;
float last_weight_In_g = 0.0;
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

// Calibration handler function
String runCalibration(float knownWeight) {
  while (hx711Busy) { delay(1); }
  hx711Busy = true;

  // Wait up to 1 second for HX711 to become ready
  unsigned long start = millis();
  while (!LOADCELL_HX711.is_ready() && millis() - start < 1000) {
    delay(1);
  }
  if (!LOADCELL_HX711.is_ready()) {
    LOADCELL_HX711.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    delay(50);
    start = millis();
    while (!LOADCELL_HX711.is_ready() && millis() - start < 1000) {
      delay(1);
    }
    if (!LOADCELL_HX711.is_ready()) {
      hx711Busy = false;
      return "Error: HX711 not ready.";
    }
  }

  // Take 15 readings with the known weight on the scale
  long sum = 0;
  int n = 15;
  for (int i = 0; i < n; i++) {
    sum += LOADCELL_HX711.read();
    delay(50);
  }
  long reading = sum / n;

  // Get the tared (zero) offset from the HX711 object
  long tareOffset = LOADCELL_HX711.get_offset();

  long delta = reading - tareOffset;
  if (knownWeight == 0 || delta == 0) {
    hx711Busy = false;
    return "Known weight and measured delta must be nonzero.";
  }

  // Print debug info
  Serial.printf("Tare offset: %ld, Loaded reading: %ld, Delta: %ld, Known weight: %.2f\n", tareOffset, reading, delta, knownWeight);

  // If delta is negative, invert calibration factor (for boards where ADC decreases with weight)
  float calibrationFactor = (float)delta / knownWeight;
  if (calibrationFactor < 0) {
    calibrationFactor = -calibrationFactor;
    Serial.println("Note: Calibration factor sign inverted due to negative delta.");
  }
  Serial.printf("Calibration factor: %.6f\n", calibrationFactor);

  preferences.putFloat("CFVal", calibrationFactor);
  LOAD_CALIBRATION_FACTOR = calibrationFactor;
  LOADCELL_HX711.set_scale(LOAD_CALIBRATION_FACTOR);

  hx711Busy = false;
  return "Calibration complete. Factor: " + String(calibrationFactor, 6);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start Setup...");

  preferences.begin("CF", false);
  // Always load calibration factor from preferences at boot
  LOAD_CALIBRATION_FACTOR = preferences.getFloat("CFVal", 0);
  Serial.print("Calibration factor loaded from EEPROM: ");
  Serial.println(LOAD_CALIBRATION_FACTOR, 6);

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = MAIN_page;
    page.replace("class=\"nav-link active\" id=\"settings-link\"", "class=\"nav-link\" id=\"settings-link\"");
    page.replace("id=\"dashboard\" class=\"page-section\" style=\"display:\"", "id=\"dashboard\" class=\"page-section\" style=\"display:\"");
    page.replace("id=\"settings\" class=\"page-section\" style=\"display:none\"", "id=\"settings\" class=\"page-section\" style=\"display:none\"");
    request->send(200, "text/html", page);
  });

  // Add a link to the calibration page on the main page if not already present
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

  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("weight")) {
        float knownWeight = request->getParam("weight")->value().toFloat();
        if (knownWeight <= 0) {
            request->send(400, "text/plain", "Invalid weight value.");
            return;
        }
        // Run calibration logic
        String calibrationResult = runCalibration(knownWeight); // This returns a String message

        request->send(200, "text/plain", calibrationResult);
    } else {
        request->send(400, "text/plain", "Missing weight parameter.");
    }
  });

  server.on("/setcf", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("cf")) {
        float cf = request->getParam("cf")->value().toFloat();

        preferences.begin("CF", false); // Use "CF" namespace
        preferences.putFloat("CFVal", cf); // Use "CFVal" key
        preferences.end();

        // Update global variable and HX711 scale immediately!
        LOAD_CALIBRATION_FACTOR = cf;
        LOADCELL_HX711.set_scale(LOAD_CALIBRATION_FACTOR);

        Serial.print("Calibration factor saved to EEPROM and applied: ");
        Serial.println(cf, 6);

        request->send(200, "text/plain", "Calibration factor saved: " + String(cf, 6));
    } else {
        request->send(400, "text/plain", "Missing calibration factor");
    }
});

  server.on("/getcf", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("CF", true); // Use "CF" namespace
    float cf = preferences.getFloat("CFVal", 0.0f); // Use "CFVal" key
    preferences.end();

    Serial.print("Calibration factor read from EEPROM: ");
    Serial.println(cf, 6);

    request->send(200, "text/plain", String(cf, 6));
  });

  // Add endpoints for units persistence
  server.on("/setunits", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("units")) {
      request->send(400, "text/plain", "Missing units parameter.");
      return;
    }
    String units = request->getParam("units")->value();
    preferences.begin("SETTINGS", false);
    preferences.putString("units", units);
    preferences.end();
    request->send(200, "text/plain", "Units saved: " + units);
  });

  server.on("/getunits", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.begin("SETTINGS", true);
    String units = preferences.getString("units", "g");
    preferences.end();
    request->send(200, "text/plain", units);
  });

  server.on("/calibration", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", MAIN_calibration_page);
  });

  server.on("/updates", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", MAIN_updates_page);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String page = MAIN_page;
    page.replace("class=\"nav-link active\" id=\"dashboard-link\"", "class=\"nav-link\" id=\"dashboard-link\"");
    page.replace("class=\"nav-link\" id=\"settings-link\"", "class=\"nav-link active\" id=\"settings-link\"");
    page.replace("id=\"dashboard\" class=\"page-section\" style=\"display:\"", "id=\"dashboard\" class=\"page-section\" style=\"display:none\"");
    page.replace("id=\"settings\" class=\"page-section\" style=\"display:none\"", "id=\"settings\" class=\"page-section\" style=\"display:\"");
    request->send(200, "text/html", page);
  });

  server.begin();

  Serial.println("Server started. Scale is ready to use.");
}

void loop() {
  if (LOADCELL_HX711.wait_ready_timeout(100)) {
    if (!hx711Busy) {
      hx711Busy = true;
      weight_In_g = LOADCELL_HX711.get_units(2);
      weight_In_oz = weight_In_g / 28.3495;
      hx711Busy = false;

      float weight_rounded = roundf(weight_In_g * 10000.0f) / 10000.0f;
      float last_weight_rounded = roundf(last_weight_In_g * 10000.0f) / 10000.0f;
      if ((millis() - lastTime) > timerDelay && weight_rounded != last_weight_rounded) {
        JSON_All_Data["weight_In_g"] = String(weight_rounded, 4);
        String JSON_All_Data_Send = JSON.stringify(JSON_All_Data);

        // Only send events if there are connected clients
        if (events.count() > 0) {
          events.send(JSON_All_Data_Send.c_str(), "allDataJSON", millis());
        }

        last_weight_In_g = weight_In_g;
        lastTime = millis();
      }
    }
  }
}
