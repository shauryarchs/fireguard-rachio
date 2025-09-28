#pragma once
#include <Arduino.h>

static const uint8_t PIN_FLAME = 6;
static const uint8_t PIN_SMOKE = 7;
static const uint8_t PIN_BUZZER = 5;
static const uint8_t PIN_TMP36 = A0;

struct SensorState {
  int flame;
  int smoke;
  float tempC;
  bool fireDetected;
};

void sensorsInit();
SensorState sensorsRead(float tempThresholdC);
void buzzerAlert();
