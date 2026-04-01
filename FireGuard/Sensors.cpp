#include "Sensors.h"
#include "Config.h"

// -------- TEMPERATURE SENSOR --------
static float readTemperatureC() {
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
    sum += analogRead(PIN_TMP36);
    if (i < ADC_SAMPLE_COUNT - 1) delay(ADC_SAMPLE_DELAY_MS);
  }
  float voltage = (sum / ADC_SAMPLE_COUNT) * (5.0f / 1023.0f);
  return (voltage - 0.5f) * 100.0f;
}

void sensorsInit() {
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

// -------- SENSOR READING --------
SensorState sensorsRead() {

  SensorState s;

  // flame sensor (digital) — require all FLAME_CONFIRM_READS consecutive LOWs
  // to avoid false triggers from sunlight or transient noise
  int lowCount = 0;
  for (int i = 0; i < FLAME_CONFIRM_READS; i++) {
    if (digitalRead(PIN_FLAME) == LOW) lowCount++;
    if (i < FLAME_CONFIRM_READS - 1) delay(FLAME_CONFIRM_DELAY_MS);
  }
  s.flame = (lowCount == FLAME_CONFIRM_READS) ? 0 : 1;

  // smoke sensor (analog) — averaged to reduce WiFi-induced ADC noise
  long smokeSum = 0;
  for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
    smokeSum += analogRead(PIN_SMOKE);
    if (i < ADC_SAMPLE_COUNT - 1) delay(ADC_SAMPLE_DELAY_MS);
  }
  s.smoke = smokeSum / ADC_SAMPLE_COUNT;

  // temperature
  s.tempC = readTemperatureC();

  return s;
}

// -------- BUZZER --------
void buzzerAlert() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(150);
  digitalWrite(PIN_BUZZER, LOW);
}