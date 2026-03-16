#include "WeatherClient.h"
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>

static const char* HOST = "api.openweathermap.org";
static const int PORT = 443;

WeatherClient::WeatherClient(const char* apiKey, float lat, float lon)
{
  _apiKey = apiKey;
  _lat = lat;
  _lon = lon;

  data.valid = false;
}

bool WeatherClient::fetchWeather()
{
  WiFiSSLClient client;

  if (!client.connect(HOST, PORT))
  {
    Serial.println("Weather connection failed");
    return false;
  }

  String url =
    "/data/2.5/weather?lat=" + String(_lat, 4) +
    "&lon=" + String(_lon, 4) +
    "&appid=" + String(_apiKey) +
    "&units=metric";

  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.openweathermap.org");
  client.println("Connection: close");
  client.println();

  String payload = "";
  bool jsonStart = false;

  while (client.connected() || client.available())
  {
    String line = client.readStringUntil('\n');

    if (line == "\r")
      jsonStart = true;

    if (jsonStart)
      payload += line;
  }

  client.stop();

  return parseResponse(payload);
}

bool WeatherClient::parseResponse(String payload)
{
  StaticJsonDocument<1024> doc;

  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    Serial.println("Weather JSON parse failed");
    return false;
  }

  data.temperatureC = doc["main"]["temp"];
  data.humidity = doc["main"]["humidity"];
  data.windSpeed = doc["wind"]["speed"];

  data.condition = String((const char*)doc["weather"][0]["main"]);

  data.raining =
      data.condition == "Rain" ||
      data.condition == "Drizzle" ||
      data.condition == "Thunderstorm";

  data.valid = true;

  return true;
}

WeatherData WeatherClient::getWeather()
{
  return data;
}