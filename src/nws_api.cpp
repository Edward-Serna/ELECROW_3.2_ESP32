#include "nws_api.h"
#include "config.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Internal Structures
struct NwsEndpoints
{
  String forecastUrl;
  String stationsUrl;
};

// Helper Functions
static float cToF(float c)
{
  return (c * 9.0f / 5.0f) + 32.0f;
}

static bool httpBeginNws(HTTPClient &http, WiFiClientSecure &client,
                         const String &url, bool forceHttp10)
{
  client.setInsecure();
  client.setTimeout(20000);

  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (forceHttp10)
    http.useHTTP10(true);

  if (!http.begin(client, url))
    return false;

  http.setUserAgent("ESP32-TFT-Weather");
  http.addHeader("Accept", "application/geo+json");
  return true;
}

static String httpGetNwsSmall(const String &url, int &outCode)
{
  WiFiClientSecure client;
  HTTPClient http;

  if (!httpBeginNws(http, client, url, false))
  {
    outCode = -999;
    return "";
  }

  outCode = http.GET();
  if (outCode < 0)
  {
    Serial.printf("[HTTP] GET failed (%d): %s\n",
                  outCode, http.errorToString(outCode).c_str());
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();
  return payload;
}

// NWS API: Get Forecast and Observation URLs
static bool getEndpoints(NwsEndpoints &ep, String &err)
{
  err = "";
  int code = 0;

  String pointsUrl = "https://api.weather.gov/points/" +
                     String(LAT, 6) + "," + String(LON, 6);
  Serial.printf("[NWS] points URL: %s\n", pointsUrl.c_str());

  String body = httpGetNwsSmall(pointsUrl, code);
  Serial.printf("[NWS] points HTTP %d\n", code);

  if (code != 200 || body.isEmpty())
  {
    err = "points HTTP " + String(code);
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, body))
  {
    err = "points JSON";
    return false;
  }

  const char *f = doc["properties"]["forecast"];
  const char *s = doc["properties"]["observationStations"];
  if (!f || !s)
  {
    err = "points missing urls";
    return false;
  }

  ep.forecastUrl = String(f);
  ep.stationsUrl = String(s);
  return true;
}

// NWS API: Get Forecast Summary
static bool getForecastSummary(const String &forecastUrl,
                               String &outPeriod,
                               String &outShort,
                               int &outForecastTemp,
                               String &err)
{
  err = "";
  int code = 0;

  String body = httpGetNwsSmall(forecastUrl, code);
  Serial.printf("[NWS] forecast HTTP %d\n", code);

  if (code != 200 || body.isEmpty())
  {
    err = "forecast HTTP " + String(code);
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, body))
  {
    err = "forecast JSON";
    return false;
  }

  JsonObject p0 = doc["properties"]["periods"][0];
  if (p0.isNull())
  {
    err = "no periods[0]";
    return false;
  }

  outPeriod = String((const char *)(p0["name"] | "Forecast"));
  outShort = String((const char *)(p0["shortForecast"] | ""));
  outForecastTemp = (int)(p0["temperature"] | 0);

  return true;
}

// NWS API: Try One Station for Observed Temperature
static bool tryStationObservedTempF(const String &stationId,
                                    int &outTempF, String &err)
{
  err = "";

  WiFiClientSecure client;
  HTTPClient http;

  String obsUrl = "https://api.weather.gov/stations/" +
                  stationId + "/observations/latest";

  if (!httpBeginNws(http, client, obsUrl, true))
  {
    err = "obs begin";
    return false;
  }

  int code = http.GET();
  Serial.printf("[NWS] latest obs HTTP %d (station %s)\n",
                code, stationId.c_str());

  if (code < 0)
  {
    Serial.printf("[HTTP] obs GET failed (%d): %s\n",
                  code, http.errorToString(code).c_str());
    err = "obs HTTP " + String(code);
    http.end();
    return false;
  }
  if (code != 200)
  {
    err = "obs HTTP " + String(code);
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, http.getStream());
  http.end();

  if (e)
  {
    err = String("obs JSON: ") + e.c_str();
    return false;
  }

  JsonVariant tC = doc["properties"]["temperature"]["value"];
  if (tC.isNull())
  {
    err = "temp null";
    return false;
  }

  float tempC = tC.as<float>();
  outTempF = (int)lroundf(cToF(tempC));
  return true;
}

// NWS API: Iterate Stations Until Valid Temperature Found
static bool getCurrentObservedTempF(const String &stationsUrl,
                                    int &outTempF,
                                    String &outStation,
                                    String &err)
{
  err = "";
  outStation = "";

  WiFiClientSecure client;
  HTTPClient http;

  if (!httpBeginNws(http, client, stationsUrl, true))
  {
    err = "stations begin";
    return false;
  }

  int code = http.GET();
  Serial.printf("[NWS] stations HTTP %d\n", code);

  if (code < 0)
  {
    Serial.printf("[HTTP] stations GET failed (%d): %s\n",
                  code, http.errorToString(code).c_str());
    err = "stations HTTP " + String(code);
    http.end();
    return false;
  }
  if (code != 200)
  {
    err = "stations HTTP " + String(code);
    http.end();
    return false;
  }

  const int MAX_STATIONS_TO_TRY = 12;

  JsonDocument filter;
  for (int i = 0; i < MAX_STATIONS_TO_TRY; i++)
  {
    filter["features"][i]["properties"]["stationIdentifier"] = true;
  }

  JsonDocument doc;
  DeserializationError e = deserializeJson(
      doc, http.getStream(),
      DeserializationOption::Filter(filter));
  http.end();

  if (e)
  {
    Serial.printf("[NWS] stations deserialize error: %s\n", e.c_str());
    err = String("stations JSON: ") + e.c_str();
    return false;
  }

  for (int i = 0; i < MAX_STATIONS_TO_TRY; i++)
  {
    const char *id = doc["features"][i]["properties"]["stationIdentifier"];
    if (!id)
      continue;

    String sid = String(id);
    int tempF = 0;
    String obsErr;

    Serial.printf("[NWS] Trying station: %s\n", sid.c_str());

    if (tryStationObservedTempF(sid, tempF, obsErr))
    {
      outStation = sid;
      outTempF = tempF;
      Serial.printf("[NWS] Selected station: %s temp=%dF\n",
                    outStation.c_str(), outTempF);
      return true;
    }
    else
    {
      Serial.printf("[NWS] Station %s skipped: %s\n",
                    sid.c_str(), obsErr.c_str());
    }
  }

  err = "no station with temp";
  return false;
}

// Public Functions: Time
bool syncTimeNtpCentral(uint32_t timeoutMs)
{
  configTzTime(TZ_INFO, NTP2, NTP1);

  uint32_t start = millis();
  while (millis() - start < timeoutMs)
  {
    time_t now = time(nullptr);
    if (now > 1700000000)
      return true;
    delay(200);
  }
  return false;
}

String nowTimeString()
{
  time_t now = time(nullptr);
  struct tm tm_info;
  localtime_r(&now, &tm_info);
  char buf[32];
  strftime(buf, sizeof(buf), "%I:%M %p", &tm_info);
  return String(buf);
}

String nowDateString()
{
  time_t now = time(nullptr);
  struct tm tm_info;
  localtime_r(&now, &tm_info);
  char buf[32];
  strftime(buf, sizeof(buf), "%a %b %d, %Y", &tm_info);
  return String(buf);
}

// Public Function: Weather Refresh
bool refreshWeather(WeatherData &data, String &errorMsg)
{
  errorMsg = "";

  NwsEndpoints ep;
  String err;

  if (!getEndpoints(ep, err))
  {
    errorMsg = err;
    return false;
  }

  int currentF = 0;
  String stationId;
  if (!getCurrentObservedTempF(ep.stationsUrl, currentF, stationId, err))
  {
    errorMsg = err;
    return false;
  }

  String period, fcShort;
  int fcTemp = 0;
  if (!getForecastSummary(ep.forecastUrl, period, fcShort, fcTemp, err))
  {
    errorMsg = err;
    return false;
  }

  data.currentTempF = currentF;
  data.stationId = stationId;
  data.period = period;
  data.shortForecast = fcShort;
  data.forecastTempF = fcTemp;
  data.isValid = true;

  return true;
}
