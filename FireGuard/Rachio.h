#pragma once
#include <Arduino.h>

void rachioConnect();
int  rachioHealthCheck();
bool rachioStartZone(const char* zoneId, int seconds);
bool rachioStopAll(const char* deviceId);
