#pragma once
#include <Arduino.h>

/**
 * Establishes Wi-Fi connection and ensures the RTC is synced
 * for proper TLS operation.
 *
 * Blocks until connected or retries indefinitely.
 */
void arduinoWiFiConnect();

/**
 * Performs a connectivity + authentication check against
 * the Rachio API by calling GET /1/public/person/info.
 *
 * @return HTTP status code (200–299 means OK; otherwise failure).
 */
int rachioHealthCheck();

/**
 * Starts a specific Rachio zone for a given duration.
 *
 * @param zoneId   The UUID of the Rachio zone.
 * @param seconds  Duration to run the zone, in seconds.
 * @return true if the request was accepted (HTTP 2xx), false otherwise.
 */
bool rachioStartZone(const char* zoneId, int seconds);

/**
 * Stops all active watering on the given Rachio device.
 *
 * @param deviceId The UUID of the Rachio device (controller).
 * @return true if the request was accepted (HTTP 2xx), false otherwise.
 */
bool rachioStopAll(const char* deviceId);

