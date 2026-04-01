#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Sensors.h"
#include "Rachio.h"
#include "Cloud.h"
#include <WiFiS3.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- TIMERS ----------------
unsigned long previousSendMs  = 0;
unsigned long previousFetchMs = FETCH_OFFSET_MS;  // offset so fetch trails send by 2.5s
unsigned long lastTriggerMs   = 0;

// ---------------- AUTO-TRIGGER LOCKOUT ----------------
int autoTriggerCount      = 0;
bool inLockout            = false;
unsigned long lockoutStartMs = 0;

// ---------------- LCD STATE ----------------
String currentMessage = "";

WiFiServer server(80);

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
  lcd.print("Sensor&LiveData");

  currentMessage = "System Ready";

  Serial.println("Manual commands:");
  Serial.println("  s = start sprinkler");
  Serial.println("  x = stop sprinkler / reset lockout");
}


void loop() {
  handleClient(server);

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

      if (inLockout) {
        inLockout = false;
        autoTriggerCount = 0;
        Serial.println("Auto-trigger lockout cleared");
      }
    }
  }

  unsigned long nowMs = millis();

  // ---------------- SENSOR READ + SEND ----------------
  // Reads sensors and uploads to cloud every SENSOR_LOOP_INTERVAL_MS.
  if (nowMs - previousSendMs >= SENSOR_LOOP_INTERVAL_MS) {
    previousSendMs = nowMs;

    SensorState s = sensorsRead();
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
  }

  // ---------------- FETCH + FIRE DECISION ----------------
  // Fetches riskIndex from cloud FETCH_OFFSET_MS after each send,
  // giving the cloud time to process the latest sensor data.
  if (nowMs - previousFetchMs >= SENSOR_LOOP_INTERVAL_MS) {
    previousFetchMs = nowMs;

    fetchRiskIndexFromCloud();
    bool fireCondition = getCurrentRiskIndex() > 7;

    Serial.println("");
    Serial.println(fireCondition ? "Fire Risk Detected!" : "No Fire Risk Detected...");
    Serial.println("");

    String newMessage;

    if (fireCondition) {
      newMessage = "Fire Detected";

      // Check if 1-hour lockout has expired
      if (inLockout && (nowMs - lockoutStartMs >= AUTO_TRIGGER_LOCKOUT_MS)) {
        inLockout = false;
        autoTriggerCount = 0;
        Serial.println("Auto-trigger lockout expired, resetting");
      }

      bool allowTrigger = !inLockout &&
        ((lastTriggerMs == 0) || (nowMs - lastTriggerMs > TRIGGER_COOLDOWN_MS));

      if (allowTrigger) {
        Serial.print("Activating sprinkler zone ");
        Serial.print(RACHIO_ZONE_ID);

        Serial.print(" for ");
        Serial.print(DURATION_SECONDS);
        Serial.println(" seconds");

        bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);

        Serial.println(ok ? "Sprinkler started" : "Sprinkler failed");

        lastTriggerMs = nowMs;
        autoTriggerCount++;

        if (autoTriggerCount >= MAX_AUTO_TRIGGERS) {
          inLockout = true;
          lockoutStartMs = nowMs;
          Serial.println("⚠️ Max auto-triggers reached. Locked out for 1hr. Send 'x' to reset.");
        }

      } else if (inLockout) {
        Serial.println("⚠️ Auto-trigger locked out. Send 'x' to reset.");
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
