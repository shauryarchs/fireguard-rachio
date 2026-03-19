#pragma once
#include <Arduino.h>

// -------- SENSOR PINS --------
static const uint8_t PIN_FLAME  = 6;
static const uint8_t PIN_SMOKE  = A1;   // analog smoke sensor
static const uint8_t PIN_BUZZER = 5;
static const uint8_t PIN_TMP36  = A0;

// -------- SMOKE THRESHOLD --------
static const int SMOKE_THRESHOLD = 600;

struct SensorState {
  int flame;
  int smoke;
  float tempC;
};

void sensorsInit();
SensorState sensorsRead(float tempThresholdC);
void buzzerAlert();