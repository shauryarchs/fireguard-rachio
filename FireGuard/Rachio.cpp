#include "Rachio.h"
#include "Config.h"
#include <WiFiS3.h>

namespace {
  constexpr const char* kHost = "api.rach.io";
  constexpr int kPort = 443;
  constexpr unsigned long kTimeWaitMs = 45000; //Timeout Duration to check for time sync for Arduino
  constexpr time_t kMinValidEpoch = 1700000000UL; // ~Nov 2023

  WiFiSSLClient client;

  

  bool openHttps() {
    client.stop();
    // small retry to ride out transient socket issues
    for (int i = 0; i < 3; ++i) {
      if (client.connect(kHost, kPort)) return true;
      delay(150);
    }
    return false;
  }

  String readLine() {
    String line;
    for (;;) {
      while (client.available()) {
        int c = client.read();
        if (c < 0) continue;
        if (c == '\r') { if (client.peek() == '\n') client.read(); return line; }
        if (c == '\n') return line;
        line += char(c);
      }
      if (!client.connected() && !client.available()) return line;
      delay(1);
    }
  }

  int sendRequest(const String& method, const String& path, const String& body) {
    if (!openHttps()) return -1;

    String req;
    req.reserve(256 + body.length());
    req += method; req += " "; req += path; req += " HTTP/1.1\r\n";
    req += "Host: "; req += kHost; req += "\r\n";
    req += "Authorization: Bearer "; req += RACHIO_API_TOKEN; req += "\r\n";
    if (!body.isEmpty()) {
      req += "Content-Type: application/json\r\n";
      req += "Content-Length: "; req += String(body.length()); req += "\r\n";
    }
    req += "Connection: close\r\n\r\n";
    if (!body.isEmpty()) req += body;

    client.print(req);

    String status = readLine(); // e.g. "HTTP/1.1 204 No Content"
    int code = -1;
    int sp1 = status.indexOf(' ');
    int sp2 = status.indexOf(' ', sp1 + 1);
    if (sp1 > 0 && sp2 > sp1) code = status.substring(sp1 + 1, sp2).toInt();

    // skip headers
    String h;
    do { h = readLine(); } while (h.length() > 0);

    client.stop();
    return code;
  }
} // namespace

void arduinoWiFiConnect() {
  int s = WL_IDLE_STATUS; //s represents "status"
  while (s != WL_CONNECTED) {
    s = WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(800);
  }
  
  //sync time to make sure that we are good to connect to Rachio
  const unsigned long start = millis(); 
  time_t now = 0;
  while (now < kMinValidEpoch && (millis() - start) < kTimeWaitMs) {
    now = WiFi.getTime();
    delay(500);
  }
}

int rachioHealthCheck() {
  return sendRequest("GET", "/1/public/person/info", "");
}

bool rachioStartZone(const char* zoneId, int seconds) {
  String body = String("{\"id\":\"") + zoneId + "\",\"duration\":" + String(seconds) + "}";
  int code = sendRequest("PUT", "/1/public/zone/start", body);
  return (code >= 200 && code < 300);
}

bool rachioStopAll(const char* deviceId) {
  String body = String("{\"id\":\"") + deviceId + "\"}";
  int code = sendRequest("PUT", "/1/public/device/stop_water", body);
  return (code >= 200 && code < 300);
}
