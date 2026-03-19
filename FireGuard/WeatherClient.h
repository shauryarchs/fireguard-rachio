#pragma once
#include <Arduino.h>

struct WeatherData
{
  float temperatureC;
  float humidity;
  float windSpeed;
  float windDirection;   

  String condition;
  bool raining;

  bool valid;
};

class WeatherClient
{
public:
  WeatherClient(const char* apiKey, float lat, float lon);

  bool fetchWeather();
  WeatherData getWeather();

private:
  const char* _apiKey;
  float _lat;
  float _lon;

  WeatherData data;

  bool parseResponse(String payload);
};