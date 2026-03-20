#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Sensors.h"
#include "Rachio.h"
#include <WiFiS3.h>
#include <ArduinoJson.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- TIMERS ----------------
unsigned long previousMs = 0;
unsigned long lastTriggerMs = 0;
int currentRiskIndex = 1;

// ---------------- LCD STATE ----------------
String currentMessage = "";

WiFiServer server(80);

bool fetchRiskIndexFromCloud() {
  WiFiClient client;

  // radius is optional; using 25 here
  if (!client.connect("embersensor.com", 80)) {
    Serial.println("❌ Status connection failed");
    return false;
  }

  client.println("GET /api/status HTTP/1.1");
  client.println("Host: embersensor.com");
  client.println("Connection: close");
  client.println();

  String payload = "";
  bool jsonStart = false;

  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');

    if (line == "\r") {
      jsonStart = true;
      continue;
    }

    if (jsonStart) {
      payload += line;
    }
  }

  client.stop();

  if (payload.length() == 0) {
    Serial.println("❌ Empty /api/status response");
    return false;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.println("❌ Failed to parse /api/status JSON");
    Serial.println(payload);
    return false;
  }

  currentRiskIndex = doc["riskIndex"] | 1;

  Serial.println("");
  Serial.println("☁️ riskIndex from cloud = " + String(currentRiskIndex));

  return true;
}

void sendToCloud(SensorState s) {
  WiFiClient client;

  if (client.connect("embersensor.com", 80)) {
    float sensorTempF = s.tempC * 9.0 / 5.0 + 32.0;
    sensorTempF -= 80;  //-80 is bc temp sensor has +-6C or 40F precision

    String json = "{";
    json += "\"sensorTemperature\":" + String(sensorTempF) + ",";
    json += "\"smoke\":" + String(s.smoke) + ",";
    json += "\"flame\":" + String(s.flame);
    json += "}";

    client.println("POST /api/update HTTP/1.1");
    client.println("Host: embersensor.com");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println();
    client.println(json);

    delay(200);
    client.stop();
    Serial.println("☁️ Data sent to cloud: " + json);
  } else {
    Serial.println("❌ Cloud connection failed");
  }
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/status") != -1) {
    SensorState s = sensorsRead(TEMP_THRESHOLD_C);
    float sensorTempF = s.tempC * 9.0 / 5.0 + 32.0;
    sensorTempF -= 80;  //-80 is bc temp sensor has +-6C or 40F precision

    String json = "{";
    json += "\"sensorTemperature\":" + String(sensorTempF) + ",";
    json += "\"smoke\":" + String(s.smoke) + ",";
    json += "\"flame\":" + String(s.flame);
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(json);
  }

  client.stop();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // LCD startup screen
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System starting");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  // Initialize hardware
  sensorsInit();

  // Connect to WiFi
  arduinoWiFiConnect();

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("HTTP server started");

  // Display Rachio configuration
  Serial.print("Zone ID: ");
  Serial.println(RACHIO_ZONE_ID);

  Serial.print("Device ID: ");
  Serial.println(RACHIO_DEVICE_ID);

  Serial.print("Run time: ");
  Serial.print(DURATION_SECONDS);
  Serial.println(" seconds");

  // Test Rachio API
  Serial.println("Checking Rachio API...");
  int hc = rachioHealthCheck();

  if (hc >= 200 && hc < 300) {
    Serial.println("Rachio connection OK");
  } else {
    Serial.print("Rachio check failed HTTP ");
    Serial.println(hc);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Fetching online weather data");

  currentMessage = "System Ready";

  Serial.println("Manual commands:");
  Serial.println("  s = start sprinkler");
  Serial.println("  x = stop sprinkler");
}


void loop() {
  handleClient();

  // ---------------- SERIAL COMMANDS ----------------
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 's' || c == 'S') {
      Serial.println("Manual start request");

      bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);

      Serial.println(ok ? "Start accepted" : "Start failed");
    }

    else if (c == 'x' || c == 'X') {
      Serial.println("Manual stop request");

      bool ok = rachioStopAll(RACHIO_DEVICE_ID);

      Serial.println(ok ? "Stop accepted" : "Stop failed");
    }
  }

  unsigned long nowMs = millis();

  // ---------------- SENSOR LOOP ----------------
  if (nowMs - previousMs >= SENSOR_LOOP_INTERVAL_MS) {
    previousMs = nowMs;

    // Read sensors
    SensorState s = sensorsRead(TEMP_THRESHOLD_C);
    sendToCloud(s);

    // Print diagnostics
    Serial.print("Sensor Temp=");
    float sensorTempF = s.tempC * 9.0 / 5.0 + 32.0;
    sensorTempF -= 80;  //-80 is bc temp sensor has +-6C or 40F precision
    Serial.print(sensorTempF, 1);
    Serial.print("F ");

    Serial.print(" Flame=");
    Serial.print(s.flame);

    Serial.print(" Smoke=");
    Serial.print(s.smoke);

    // ---------------- FINAL FIRE DECISION ----------------
    // Cloudflare computes the decision engine.
    // Start sprinkler only if riskIndex > 7.
    fetchRiskIndexFromCloud();
    bool fireCondition = currentRiskIndex > 7;

    Serial.println("");
    Serial.println(fireCondition ? "Fire Risk Detected!" : "No Fire Risk Detected...");
    Serial.println("");

    String newMessage;

    if (fireCondition) {
      newMessage = "Fire Detected";

      bool allowTrigger =
        (lastTriggerMs == 0) || (nowMs - lastTriggerMs > TRIGGER_COOLDOWN_MS);

      if (allowTrigger) {
        Serial.print("Activating sprinkler zone ");
        Serial.print(RACHIO_ZONE_ID);

        Serial.print(" for ");
        Serial.print(DURATION_SECONDS);
        Serial.println(" seconds");

        bool ok =
          rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);

        Serial.println(ok ? "Sprinkler started"
                          : "Sprinkler failed");

        lastTriggerMs = nowMs;
      }

      buzzerAlert();
    }

    else {
      newMessage = "System Ready";
      digitalWrite(PIN_BUZZER, LOW);
    }


    // ---------------- LCD UPDATE ----------------
    if (newMessage != currentMessage) {
      lcd.clear();
      delay(5);

      if (fireCondition) {
        lcd.setCursor(0, 0);
        lcd.print("Fire Detected!");

        lcd.setCursor(0, 1);
        lcd.print("Sprinklers ON");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("All Clear");

        lcd.setCursor(0, 1);
        lcd.print("System Ready");
      }

      currentMessage = newMessage;
    }
  }
}
