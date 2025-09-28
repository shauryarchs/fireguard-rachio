#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Sensors.h"
#include "Rachio.h"

// 16x2 I2C LCD at 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// timing
unsigned long previousMs   = 0;
unsigned long lastTriggerMs = 0;

// LCD state
String currentMessage = "";

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // LCD boot banner
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("System starting");
  lcd.setCursor(0, 1); lcd.print("Connecting WiFi");

  // IO and network
  sensorsInit();
  rachioConnect();

  // Summary
  Serial.print("Zone ID: ");   Serial.println(RACHIO_ZONE_ID);
  Serial.print("Device ID: "); Serial.println(RACHIO_DEVICE_ID);
  Serial.print("Run time: ");  Serial.print(DURATION_SECONDS); Serial.println("s");

  // Quick API reachability/auth probe
  Serial.println("Rachio check: GET /1/public/person/info");
  int hc = rachioHealthCheck();
  if (hc >= 200 && hc < 300) {
    Serial.println("Rachio check: OK");
  } else {
    Serial.print("Rachio check failed, HTTP "); Serial.println(hc);
  }

  // Ready UI
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Ready");
  lcd.setCursor(0, 1); lcd.print("Rachio online");
  currentMessage = "System Ready!";

  Serial.println("Serial commands: 's' start zone, 'x' stop all.");
}

void loop() {
  // Manual commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println("Manual start request");
      bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);
      Serial.println(ok ? "Manual start: accepted" : "Manual start: failed");
    } else if (c == 'x' || c == 'X') {
      Serial.println("Manual stop-all request");
      bool ok = rachioStopAll(RACHIO_DEVICE_ID);
      Serial.println(ok ? "Manual stop-all: accepted" : "Manual stop-all: failed");
    }
  }

  // Periodic work
  unsigned long nowMs = millis();
  if (nowMs - previousMs >= LOOP_INTERVAL_MS) {
    previousMs = nowMs;

    // Read sensors
    SensorState s = sensorsRead(TEMP_THRESHOLD_C);

    // Diagnostics
    Serial.print("SENS flame="); Serial.print(s.flame);
    Serial.print(" smoke=");     Serial.print(s.smoke);
    Serial.print(" tempC=");     Serial.print(s.tempC, 1);
    Serial.print(" fire=");      Serial.println(s.fireDetected ? "1" : "0");

    // Decision
    String newMessage;
    if (s.fireDetected) {
      newMessage = "Fire Detected,Sprinklers ON!";

      // trigger immediately the first time, then honor cooldown
      bool allowTrigger = (lastTriggerMs == 0) || (nowMs - lastTriggerMs > TRIGGER_COOLDOWN_MS);
      if (allowTrigger) {
        Serial.print("Start zone: "); Serial.print(RACHIO_ZONE_ID);
        Serial.print(" for "); Serial.print(DURATION_SECONDS); Serial.println("s");
        bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);
        Serial.println(ok ? "Start zone: accepted" : "Start zone: failed");
        lastTriggerMs = nowMs;
      } else {
        unsigned long remain = (TRIGGER_COOLDOWN_MS - (nowMs - lastTriggerMs)) / 1000;
        Serial.print("Cooldown active, seconds remaining: ");
        Serial.println(remain);
      }

      buzzerAlert();
    } else {
      newMessage = "System Ready!";
      digitalWrite(PIN_BUZZER, LOW);
    }

    // LCD update on change
    if (newMessage != currentMessage) {
      lcd.clear();
      delay(5);
      lcd.setCursor(0, 0);
      if (newMessage == "Fire Conditions Detected,Sprinklers ON!") {
        lcd.print("Fire conditions detected");
        lcd.setCursor(0, 1);
        lcd.print("Sprinklers ON");
      } else {
        lcd.print("All Clear");
        lcd.setCursor(0, 1);
        lcd.print("System Ready");
      }
      currentMessage = newMessage;
    }
  }
}
