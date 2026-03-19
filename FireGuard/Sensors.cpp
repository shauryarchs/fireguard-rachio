#include "Sensors.h"

// -------- TEMPERATURE SENSOR --------
static float readTemperatureC() {
  int reading = analogRead(PIN_TMP36);
  float voltage = reading * (5.0f / 1023.0f);
  return (voltage - 0.5f) * 100.0f;
}

void sensorsInit() {
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

// -------- SENSOR READING --------
SensorState sensorsRead(float tempThresholdC) {

  SensorState s;

  // flame sensor (digital)
  s.flame = digitalRead(PIN_FLAME);

  // smoke sensor (analog)
  s.smoke = analogRead(PIN_SMOKE);

  // temperature
  s.tempC = readTemperatureC();

  // trigger logic
  bool flameTrigger = (s.flame == 0);
  bool smokeTrigger = (s.smoke > SMOKE_THRESHOLD);
  bool tempTrigger  = (s.tempC > tempThresholdC);

  return s;
}

// -------- BUZZER --------
void buzzerAlert() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(150);
  digitalWrite(PIN_BUZZER, LOW);
}