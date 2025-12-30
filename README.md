# ESP32 Weather Station
Weather display for ESP32 with ILI9488 TFT showing current conditions and forecast.

<p align="center">
  <img src="resource/image.png" alt="demo" width="400"/>
</p>

## Setup Instructions

### 1. Create secrets.h file

Create `include/secrets.h` with your WiFi credentials:

```cpp
#pragma once

#define WIFI_SSID "YourNetworkName"
#define WIFI_PASS "YourPassword"
```

**IMPORTANT**: This file is in .gitignore and will NOT be committed to git.

### 2. Configure Location

Edit `include/config.h`:
- Update `LAT` and `LON` to your coordinates
- Update `TZ_INFO` for your timezone if not Central Time

### 3. Build and Upload

```bash
pio run -t upload
pio device monitor
```

## Project Structure

```
├── include/
│   ├── secrets.h        # WiFi credentials (NOT in git)
│   ├── config.h         # Location & settings
│   ├── nws_api.h        # Weather API headers
│   ├── ui.h             # Display UI headers
│   └── User_Setup.h     # TFT display config
├── src/
│   ├── main.cpp         # Main program
│   ├── nws_api.cpp      # Weather API implementation
│   └── ui.cpp           # Display UI implementation
└── platformio.ini       # Build configuration
```

## Features

- Current observed temperature from nearest weather station
- NWS forecast for your location
- Real-time clock with timezone support
- Auto-refresh every minute
- Modular, organized code structure

## Security

- WiFi credentials stored in separate `secrets.h` file
- `secrets.h` is excluded from git via `.gitignore`
- Safe to share project without exposing credentials
