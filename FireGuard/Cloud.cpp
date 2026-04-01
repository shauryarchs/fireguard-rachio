#include "Cloud.h"
#include "Config.h"
#include <ArduinoJson.h>

static int currentRiskIndex = 1;

// Converts raw TMP36 Celsius to the display/transmit Fahrenheit value.
// The -80 offset compensates for the sensor's ±6°C / ~40°F precision error.
static float toDisplayTempF(float tempC) {
  return tempC * 9.0 / 5.0 + 32.0 - 80.0;
}

bool fetchRiskIndexFromCloud() {
  WiFiClient client;

  if (!client.connect("embersensor.com", 80)) {
    Serial.println("❌ Status connection failed");
    currentRiskIndex = 1;
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
    currentRiskIndex = 1;
    return false;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.println("❌ Failed to parse /api/status JSON");
    Serial.println(payload);
    currentRiskIndex = 1;
    return false;
  }

  currentRiskIndex = doc["riskIndex"] | 1;

  Serial.println("");
  Serial.println("☁️ riskIndex from cloud = " + String(currentRiskIndex));

  return true;
}

int getCurrentRiskIndex() {
  return currentRiskIndex;
}

void sendToCloud(SensorState s) {
  WiFiClient client;

  if (client.connect("embersensor.com", 80)) {
    float sensorTempF = toDisplayTempF(s.tempC);

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

void handleClient(WiFiServer& server) {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("/status") != -1) {
    SensorState s = sensorsRead();
    float sensorTempF = toDisplayTempF(s.tempC);

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
