#pragma once
#include <Arduino.h>

struct WeatherData
{
  int currentTempF;
  int forecastTempF;
  String stationId;
  String period;
  String shortForecast;
  bool isValid;

  WeatherData() : currentTempF(0), forecastTempF(0), isValid(false) {}
};

// ============================================================
// NWS API Functions
// ============================================================

// Initialize time sync with NTP
bool syncTimeNtpCentral(uint32_t timeoutMs);

// Get formatted time strings
String nowTimeString();
String nowDateString();

// Fetch weather data from NWS API
bool refreshWeather(WeatherData &data, String &errorMsg);