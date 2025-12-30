#pragma once
#include <TFT_eSPI.h>
#include "nws_api.h"

// ============================================================
// UI Functions
// ============================================================

// Initialize the display
void initDisplay(TFT_eSPI &tft);

// Show a status message (e.g., during startup)
void uiStatus(TFT_eSPI &tft, const String &msg);

// Draw the complete weather layout with all data
void drawWeatherLayout(TFT_eSPI &tft, const WeatherData &data);

// Update only the time card (efficient refresh)
void updateTimeOnly(TFT_eSPI &tft);
