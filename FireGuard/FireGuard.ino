#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Sensors.h"
#include "Rachio.h"
#include "WeatherClient.h"
#include <WiFiS3.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- WEATHER CLIENT ----------------
WeatherClient weather(WEATHER_API_KEY, WEATHER_LAT, WEATHER_LON);

// ---------------- TIMERS ----------------
unsigned long previousMs = 0;
unsigned long lastTriggerMs = 0;
unsigned long lastWeatherFetchMs = 0;

// ---------------- WEATHER DATA ----------------
WeatherData currentWeather;

// ---------------- LCD STATE ----------------
String currentMessage = "";

WiFiServer server(80);

void sendToCloud(SensorState s) {
  WiFiClient client;

  if (client.connect("embersensor.com", 80)) {
    String json = "{";
    json += "\"temperature\":" + String(s.tempC) + ",";
    json += "\"smoke\":" + String(s.smoke) + ",";
    json += "\"flame\":" + String(s.flame) + ",";
    json += "\"humidity\":" + String(currentWeather.humidity) + ",";
    json += "\"wind\":" + String(currentWeather.windSpeed) + ",";
    json += "\"windDirection\":" + String(currentWeather.windDirection) + ",";
    json += "\"raining\":" + String(currentWeather.raining ? "true" : "false") + ",";
    json += "\"condition\":\"" + currentWeather.condition + "\",";
    json += "\"fireDetected\":" + String(s.fireDetected ? "true" : "false");
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

    Serial.println("☁️ Data sent to cloud");
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
    sensorTempF -= 40;  //-40 is bc temp sensor has +-3C or 40F precision

    String json = "{";
    json += "\"temperature\":" + String(sensorTempF) + ",";
    json += "\"smoke\":" + String(s.smoke) + ",";
    json += "\"flame\":" + String(s.flame) + ",";
    json += "\"humidity\":" + String(currentWeather.humidity) + ",";
    json += "\"wind\":" + String(currentWeather.windSpeed) + ",";
    json += "\"windDirection\":" + String(currentWeather.windDirection) + ",";
    json += "\"raining\":" + String(currentWeather.raining ? "true" : "false") + ",";
    json += "\"condition\":\"" + currentWeather.condition + "\",";
    json += "\"fireDetected\":" + String(s.fireDetected ? "true" : "false");
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

  // Initial weather fetch
  weather.fetchWeather();
  currentWeather = weather.getWeather();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Weather online");

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


  // ---------------- PERIODIC WEATHER UPDATE ----------------
  if (nowMs - lastWeatherFetchMs >= WEATHER_LOOP_INTERVAL_MS) {

    if (weather.fetchWeather()) {
      currentWeather = weather.getWeather();

      Serial.print("Weather Temp=");
      float tempF = currentWeather.temperatureC * 9.0 / 5.0 + 32.0;
      Serial.print(tempF);
      Serial.print("F ");

      Serial.print(" Humidity=");
      Serial.print(currentWeather.humidity);

      Serial.print("  Wind=");
      Serial.print(currentWeather.windSpeed);

      Serial.print("  WindDir=");
      Serial.print(currentWeather.windDirection);
      Serial.println("");
    }

    lastWeatherFetchMs = nowMs;
  }


  // ---------------- SENSOR LOOP ----------------
  if (nowMs - previousMs >= SENSOR_LOOP_INTERVAL_MS) {
    previousMs = nowMs;

    // Read sensors
    SensorState s = sensorsRead(TEMP_THRESHOLD_C);
    sendToCloud(s);

    // Print diagnostics
    Serial.print("Sensor  Temp=");
    float sensorTempF = s.tempC * 9.0 / 5.0 + 32.0;
    sensorTempF -= 40;  //-40 is bc temp sensor has +-3C or 40F precision
    Serial.print(sensorTempF, 1);
    Serial.print("F ");

    Serial.print("  Flame=");
    Serial.print(s.flame);

    Serial.print("         Smoke=");
    Serial.print(s.smoke);

    Serial.println("");
    Serial.print("Condition=");
    Serial.print(currentWeather.condition);

    if (currentWeather.raining) {
      Serial.print("  Rain detected — sprinklers disabled");
    } else {
      Serial.print("    No rain—sprinklers enabled");
    }

    Serial.println("");
    Serial.println(s.fireDetected ? "Fire Detected!" : "No Fire Detected...");
    Serial.println("");


    // ---------------- WEATHER FIRE RISK ----------------
    bool weatherRisk = false;

    if (currentWeather.valid) {
      if (currentWeather.windSpeed > 8.0 && currentWeather.humidity < 25) {
        weatherRisk = true;
      }
    }


    // ---------------- FINAL FIRE DECISION ----------------
    bool fireCondition =
      !currentWeather.raining && (s.fireDetected || weatherRisk);

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