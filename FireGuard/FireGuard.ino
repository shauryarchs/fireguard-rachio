#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "Sensors.h"
#include "Rachio.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long previousMs = 0;
unsigned long lastTriggerMs = 0;
String currentMessage = "";

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("FireGuard Ready");
  lcd.setCursor(0, 1); lcd.print("WiFi connecting");

  sensorsInit();
  rachioConnect();

  Serial.print("[ready] Zone ID: ");   Serial.println(RACHIO_ZONE_ID);
  Serial.print("[ready] Device ID: "); Serial.println(RACHIO_DEVICE_ID);
  Serial.print("[ready] Duration: ");  Serial.print(DURATION_SECONDS); Serial.println("s");

  Serial.println("[rachio] Connectivity & auth check: GET /1/public/person/info");
  int hc = rachioHealthCheck();
  if (hc >= 200 && hc < 300) Serial.println("[rachio] ✅ API reachable and token accepted.");
  else                       Serial.println("[rachio] ⚠️ API check failed; zone start may fail.");

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("System Ready");
  lcd.setCursor(0, 1); lcd.print("Rachio armed");
  currentMessage = "All Clear,System Ready!";

  Serial.println("[ready] Type 's' to start zone now, 'x' to stop all watering.");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println("[manual] Forcing zone start...");
      bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);
      Serial.println(ok ? "[manual] Zone start sent." : "[manual] Failed to start zone.");
    } else if (c == 'x' || c == 'X') {
      Serial.println("[manual] Stopping all watering...");
      bool ok = rachioStopAll(RACHIO_DEVICE_ID);
      Serial.println(ok ? "[manual] Stop sent." : "[manual] Failed to stop.");
    }
  }

  unsigned long nowMs = millis();
  if (nowMs - previousMs >= LOOP_INTERVAL_MS) {
    previousMs = nowMs;
    SensorState s = sensorsRead(TEMP_THRESHOLD_C);

    Serial.print("[sens] flame="); Serial.print(s.flame);
    Serial.print(" smoke=");      Serial.print(s.smoke);
    Serial.print(" tempC=");      Serial.print(s.tempC, 1);
    Serial.print(" fireDetected="); Serial.println(s.fireDetected ? "YES" : "no");

    String newMessage;
    if (s.fireDetected) {
      newMessage = "Fire Detected,Sprinklers ON!";
      bool allowTrigger = (lastTriggerMs == 0) || (nowMs - lastTriggerMs > TRIGGER_COOLDOWN_MS);
      if (allowTrigger) {
        Serial.println("[rachio] FIRE DETECTED → starting zone...");
        bool ok = rachioStartZone(RACHIO_ZONE_ID, DURATION_SECONDS);
        Serial.println(ok ? "[rachio] ✅ Zone start accepted." : "[rachio] ❌ Zone start FAILED.");
        lastTriggerMs = nowMs;
      } else {
        unsigned long remain = (TRIGGER_COOLDOWN_MS - (nowMs - lastTriggerMs)) / 1000;
        Serial.print("[logic] Fire detected but in cooldown. Seconds remaining: ");
        Serial.println(remain);
      }
      buzzerAlert();
    } else {
      newMessage = "All Clear,System Ready!";
      digitalWrite(PIN_BUZZER, LOW);
    }

    if (newMessage != currentMessage) {
      lcd.clear(); delay(5);
      lcd.setCursor(0, 0);
      if (newMessage == "Fire Detected,Sprinklers ON!") {
        lcd.print("Fire Detected,"); lcd.setCursor(0, 1); lcd.print("Sprinklers ON!");
      } else {
        lcd.print("All Clear,"); lcd.setCursor(0, 1); lcd.print("System Ready!");
      }
      currentMessage = newMessage;
    }
  }
}
