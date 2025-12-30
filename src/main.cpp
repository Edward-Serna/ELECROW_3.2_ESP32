#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "secrets.h"
#include "nws_api.h"
#include "ui.h"

// Global Objects
TFT_eSPI tft;
WeatherData weatherData;

// Power Control
#define BOOT_BUTTON 0  // GPIO 0 is the BOOT button

void IRAM_ATTR handlePowerOff() {
  // This will be called when BOOT button is pressed
}

void powerOff() {
  Serial.println("[Power] Shutting down...");
  
  // Turn off display
  #ifdef TFT_BL
    digitalWrite(TFT_BL, !TFT_BACKLIGHT_ON);
  #endif
  
  // Disconnect WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Show shutdown message briefly
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("[Power] Entering deep sleep. Press BOOT to wake up.");
  delay(100);
  
  // Configure BOOT button as wake source
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Wake on LOW (button press)
  
  // Enter deep sleep (device fully off, only wakes on BOOT press)
  esp_deep_sleep_start();
}


void setup() {
  Serial.begin(115200);
  delay(200);

  // Setup BOOT button for power off
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOOT_BUTTON), handlePowerOff, FALLING);

  // Initialize display
  initDisplay(tft);
  
  // Connect to WiFi
  uiStatus(tft, "Connecting WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    uiStatus(tft, "WiFi failed");
    Serial.println("[WiFi] Connection failed!");
    return;
  }

  Serial.printf("[WiFi] Connected! IP=%s RSSI=%ld\n", 
                WiFi.localIP().toString().c_str(), WiFi.RSSI());

  // Sync time with NTP
  uiStatus(tft, "Syncing time...");
  
  if (!syncTimeNtpCentral(15000)) {
    Serial.println("[TIME] NTP sync failed (time may be wrong)");
  } else {
    Serial.println("[TIME] NTP sync OK");
  }

  // Load initial weather data
  uiStatus(tft, "Loading weather...");
  
  String err;
  if (!refreshWeather(weatherData, err)) {
    Serial.println("[Weather] ERROR: " + err);
    uiStatus(tft, "Weather error: " + err);
    return;
  }

  Serial.println("[Weather] Initial data loaded successfully");
  
  // Draw the complete weather layout
  drawWeatherLayout(tft, weatherData);
}


void loop() {
  static uint32_t lastTimeMs = 0;
  static uint32_t lastWeatherMs = 0;
  static uint32_t buttonPressStart = 0;
  static bool buttonPressed = false;

  // Check for press on BOOT button to power off
  if (digitalRead(BOOT_BUTTON) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressStart = millis();
    } else {
      powerOff();
    }
  } else {
    buttonPressed = false;
  }

  // Update time display every minute
  if (millis() - lastTimeMs >= TIME_UPDATE_INTERVAL) {
    lastTimeMs = millis();
    updateTimeOnly(tft);
    Serial.println("[UI] Time updated");
  }

  // Refresh weather data every minute
  if (millis() - lastWeatherMs >= WEATHER_UPDATE_INTERVAL) {
    lastWeatherMs = millis();

    String err;
    if (refreshWeather(weatherData, err)) {
      drawWeatherLayout(tft, weatherData);
      Serial.println("[Weather] Data refreshed successfully");
    } else {
      Serial.println("[Weather] Refresh failed: " + err);
      // Keep previous display on error
    }
  }

  delay(100); // Small delay to prevent watchdog issues
}
