#include "Sensors.h"

static float readTemperatureC() {
  int reading = analogRead(PIN_TMP36);
  float voltage = reading * (5.0f / 1023.0f);
  return (voltage - 0.5f) * 100.0f;
}

void sensorsInit() {
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_SMOKE, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

SensorState sensorsRead(float tempThresholdC) {
  SensorState s;
  s.flame = digitalRead(PIN_FLAME);
  s.smoke = digitalRead(PIN_SMOKE);
  s.tempC = readTemperatureC();
  s.fireDetected = (s.flame == 0) || (s.smoke == 0) || (s.tempC > tempThresholdC);
  return s;
}

void buzzerAlert() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(150);
  digitalWrite(PIN_BUZZER, LOW);
}
