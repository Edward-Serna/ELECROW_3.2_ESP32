#pragma once
#include <cstdint>

// ============================================================
// Location (San Antonio, TX)
// Update these to your coordinates
// ============================================================
static const float LAT = 29.565944f;
static const float LON = -98.656139f;

// ============================================================
// Timezone: US Central (Texas) with DST
// Update TZ_INFO for your timezone
// See: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// ============================================================
static const char *TZ_INFO = "CST6CDT,M3.2.0/2,M11.1.0/2";
static const char *NTP1 = "time.nist.gov";
static const char *NTP2 = "pool.ntp.org";

// ============================================================
// Update Intervals
// ============================================================
static const uint32_t TIME_UPDATE_INTERVAL = 60000;    // 1 minute
static const uint32_t WEATHER_UPDATE_INTERVAL = 60000; // 1 minutes
