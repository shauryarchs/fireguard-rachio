#pragma once
#include <WiFiS3.h>
#include "Sensors.h"

/**
 * Fetches the current risk index from embersensor.com.
 * Updates internal state; retrieve the value with getCurrentRiskIndex().
 *
 * @return true if the request succeeded and was parsed, false otherwise.
 */
bool fetchRiskIndexFromCloud();

/**
 * Returns the last successfully fetched risk index.
 * Defaults to 1 (safe) until a successful fetch occurs.
 */
int getCurrentRiskIndex();

/**
 * Posts current sensor state to embersensor.com/api/update.
 *
 * @param s SensorState to transmit.
 */
void sendToCloud(SensorState s);

/**
 * Handles a single incoming request on the local HTTP server.
 * Responds to GET /status with a JSON sensor snapshot.
 *
 * @param server The WiFiServer instance to poll.
 */
void handleClient(WiFiServer& server);
