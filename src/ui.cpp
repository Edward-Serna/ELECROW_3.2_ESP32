#include "ui.h"

// Layout Structure
struct Layout {
  int margin;
  int gap;
  int headerH;

  int cardY;
  int cardW;
  int cardH;
  int leftX;
  int rightX;

  int timeX;
  int timeY;
  int timeW;
  int timeH;
};

// Internal Helper Functions
static Layout computeLayout(TFT_eSPI& tft) {
  Layout L{};
  L.margin = 10;
  L.gap = 10;
  L.headerH = 36;

  const int W = tft.width();
  const int H = tft.height();

  // Top row: two cards side-by-side
  L.cardY = L.headerH + 10;
  L.cardW = (W - 2 * L.margin - L.gap) / 2;
  L.cardH = 120;

  L.leftX = L.margin;
  L.rightX = L.margin + L.cardW + L.gap;

  // Bottom row: wide time card
  L.timeX = L.margin;
  L.timeY = L.cardY + L.cardH + 10;
  L.timeW = W - 2 * L.margin;
  L.timeH = H - L.timeY - 10;

  return L;
}

static void uiHeader(TFT_eSPI& tft, const String& title) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.fillRect(0, 0, tft.width(), 36, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(title, 10, 8);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

static void uiCard(TFT_eSPI& tft, int x, int y, int w, int h, 
                   const String& label) {
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(label, x + 10, y + 6);
  tft.drawFastHLine(x + 8, y + 18, w - 16, TFT_WHITE);
}

// Public UI Functions
void initDisplay(TFT_eSPI& tft) {
  tft.begin();
  
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  tft.setRotation(1); // 480x320 landscape
}

void uiStatus(TFT_eSPI& tft, const String& msg) {
  uiHeader(tft, "San Antonio Weather");
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString(msg, tft.width() / 2, tft.height() / 2);
}

void drawWeatherLayout(TFT_eSPI& tft, const WeatherData& data) {
  uiHeader(tft, "San Antonio Weather");

  Layout L = computeLayout(tft);

  // Draw card borders
  uiCard(tft, L.leftX, L.cardY, L.cardW, L.cardH, "CURRENT (Observed)");
  uiCard(tft, L.rightX, L.cardY, L.cardW, L.cardH, "FORECAST (NWS)");
  uiCard(tft, L.timeX, L.timeY, L.timeW, L.timeH, "LOCAL TIME (Central)");

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // CURRENT card content
  tft.setTextSize(6);
  tft.drawString(String(data.currentTempF) + "F", L.leftX + 60, L.cardY + 40);

  tft.setTextSize(2);
  tft.drawString("Station:", L.leftX + 12, L.cardY + 100);
  tft.drawString(data.stationId, L.leftX + 120, L.cardY + 100);

  // FORECAST card content
  String shortFc = data.shortForecast;
  if (shortFc.length() > 26) {
    shortFc = shortFc.substring(0, 26) + "...";
  }

  tft.setTextSize(2);
  tft.drawString(data.period, L.rightX + 12, L.cardY + 30);

  tft.setTextSize(5);
  tft.drawString(String(data.forecastTempF) + "F", 
                 L.rightX + 60, L.cardY + 55);

  tft.setTextSize(2);
  tft.drawString(shortFc, L.rightX + 12, L.cardY + 100);

  // TIME card content
  tft.setTextSize(2);
  tft.drawString(nowDateString(), L.timeX + 14, L.timeY + 34);

  tft.setTextSize(6);
  tft.drawString(nowTimeString(), L.timeX + 80, L.timeY + 65);
}

void updateTimeOnly(TFT_eSPI& tft) {
  Layout L = computeLayout(tft);

  // Clear only the inside of the time card
  int clearX = L.timeX + 2;
  int clearY = L.timeY + 22;
  int clearW = L.timeW - 4;
  int clearH = L.timeH - 24;

  tft.fillRect(clearX, clearY, clearW, clearH, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setTextSize(2);
  tft.drawString(nowDateString(), L.timeX + 14, L.timeY + 34);

  tft.setTextSize(6);
  tft.drawString(nowTimeString(), L.timeX + 80, L.timeY + 65);
}
