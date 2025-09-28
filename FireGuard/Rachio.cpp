#include "Rachio.h"
#include "Config.h"
#include <WiFiS3.h>

static const char* HOST = "api.rach.io";
static const int   HTTPS_PORT = 443;
static WiFiSSLClient client;

static void ensureClockSync() {
  unsigned long t0 = millis();
  time_t now = 0;
  while (now < 1700000000UL && millis() - t0 < 45000) {
    now = WiFi.getTime();
    delay(250);
  }
}

void rachioConnect() {
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(1000);
  }
  ensureClockSync();
}

static bool openHttps() {
  client.stop();
  return client.connect(HOST, HTTPS_PORT);
}

static String readLine() {
  String line;
  while (true) {
    while (client.available()) {
      int c = client.read();
      if (c < 0) continue;
      if (c == '\r') { int n = client.peek(); if (n == '\n') client.read(); return line; }
      if (c == '\n') return line;
      line += (char)c;
    }
    if (!client.connected() && !client.available()) return line;
    delay(1);
  }
}

static int httpsRequest(const String& method, const String& path, const String& body) {
  if (!openHttps()) return -1;
  String req = method + " " + path + " HTTP/1.1\r\n";
  req += "Host: "; req += HOST; req += "\r\n";
  req += "Authorization: Bearer "; req += RACHIO_API_TOKEN; req += "\r\n";
  if (body.length() > 0) {
    req += "Content-Type: application/json\r\n";
    req += "Content-Length: "; req += String(body.length()); req += "\r\n";
  }
  req += "Connection: close\r\n\r\n";
  if (body.length() > 0) req += body;
  client.print(req);
  String statusLine = readLine();
  int code = -1;
  int sp1 = statusLine.indexOf(' ');
  int sp2 = statusLine.indexOf(' ', sp1 + 1);
  if (sp1 > 0 && sp2 > sp1) code = statusLine.substring(sp1 + 1, sp2).toInt();
  client.stop();
  return code;
}

int rachioHealthCheck() {
  return httpsRequest("GET", "/1/public/person/info", "");
}

bool rachioStartZone(const char* zoneId, int seconds) {
  String body = String("{\"id\":\"") + zoneId + "\",\"duration\":" + String(seconds) + "}";
  int code = httpsRequest("PUT", "/1/public/zone/start", body);
  return (code >= 200 && code < 300);
}

bool rachioStopAll(const char* deviceId) {
  String body = String("{\"id\":\"") + deviceId + "\"}";
  int code = httpsRequest("PUT", "/1/public/device/stop_water", body);
  return (code >= 200 && code < 300);
}
