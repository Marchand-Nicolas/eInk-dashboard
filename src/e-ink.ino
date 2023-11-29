// Import environnement variables
#include "env.h"

// ESP32
#include <esp_sleep.h>

// GxEPD2_MinimumExample.ino by Jean-Marc Zingg

// purpose is e.g. to determine minimum code and ram use by this library

// see GxEPD2_wiring_examples.h of GxEPD2_Example for wiring suggestions and examples
// if you use a different wiring, you need to adapt the constructor parameters!

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX, to use less code and ram
// #include <GFX.h>

// #include <OneWire.h>

#include <GxEPD2_BW.h> // including both doesn't use more code or ram
#include <GxEPD2_3C.h> // including both doesn't use more code or ram

// Small
#include <Fonts/FreeMonoBoldOblique9pt7b.h>
// Medium
#include <Fonts/FreeMonoBold9pt7b.h>
// Big
#include <Fonts/FreeMonoBold12pt7b.h>
// Huge
#include <Fonts/FreeMonoBold18pt7b.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// alternately you can copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h of GxEPD2_Example to here
// e.g. for Wemos D1 mini:
// GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4)); // GDEH0154D67
// #include <DallasTemperature.h>

// Include one wire library (Dallas Temperature)
#include <OneWire.h>
#include <DallasTemperature.h>

// HTTP :
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

// APIs (process JSON responses)
#include <ArduinoJson.h>

const uint32_t SLEEP_DURATION = sleepDurationSeconds * 1000000; // µs

// WiFi
WiFiMulti wifiMulti;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

void setup()
{
  Serial.begin(9600);
  // Wifi
  wifiMulti.addAP(wifiSSID, wifiPassword);
  // Temperature
  sensors.begin();

  // Screen
  display.init();
  display.setFullWindow();
  // Horizontal
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_TIMER:
    break;
  default:
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0, 20);
    display.print("Connecting to Wifi...");
    display.display(true);
  }
}

bool canRefresh = true;

void loop()
{
  if (canRefresh)
  {
    refresh();
  }
};

void refresh()
{
  if ((wifiMulti.run() == WL_CONNECTED))
  {
    // Clear screen
    display.fillScreen(GxEPD_WHITE);
    refreshDate();
    refreshBlockNumber();
    refreshBattery();
    refreshTemperature();
    display.display(true);
    hibernate(SLEEP_DURATION);
  }
  else
  {
    delay(100);
  }
}

void refreshTemperature()
{
  // Temperature
  sensors.requestTemperatures();
  // Bottom left
  float temperature = sensors.getTempCByIndex(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 120);
  // 1 decimal only
  display.print(String(temperature, 1) + "°C");
}

void refreshBattery()
{
  // Battery
  display.setFont(&FreeMonoBoldOblique9pt7b);
  // Bottom right
  int chargeLevel = getChargeLevelFromConversionTable(2.08 * getRawVoltage());
  if (chargeLevel < 5)
  {
    canRefresh = false;
    display.setCursor(0, 20);
    display.setFont(&FreeMonoBold18pt7b);
    display.print("Battery too low (" + String(chargeLevel) + "%)");
  }
  else
  {
    display.setCursor(210, 120);
    display.setFont(&FreeMonoBold9pt7b);
    display.print(String(chargeLevel) + "%");
  }
}

void refreshDate()
{
  HTTPClient http;
  http.begin(timeApiURL);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      // Convert to JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      // Extract values
      String date = doc["datetime"];
      // Convert to dd/mm/yy - hh:mm
      String day = date.substring(8, 10);
      String month = date.substring(5, 7);
      String year = date.substring(2, 4);
      String hour = date.substring(11, 13);
      String minute = date.substring(14, 16);
      // Display
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(0, 20);
      display.print(day + "/" + month + "/" + year + " - " + hour + ":" + minute);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void refreshBlockNumber()
{
  HTTPClient http;
  http.begin(ethApiURL);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      // Convert to JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      // Extract values
      String blockNumber = doc["result"];
      // Display
      display.setFont(&FreeMonoBold18pt7b);
      display.setCursor(0, 60);
      display.print(blockNumber);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

double conversionTable[] = {3.200, 3.250, 3.300, 3.350, 3.400, 3.450, 3.500, 3.550, 3.600, 3.650, 3.700,
                            3.703, 3.706, 3.710, 3.713, 3.716, 3.719, 3.723, 3.726, 3.729, 3.732,
                            3.735, 3.739, 3.742, 3.745, 3.748, 3.752, 3.755, 3.758, 3.761, 3.765,
                            3.768, 3.771, 3.774, 3.777, 3.781, 3.784, 3.787, 3.790, 3.794, 3.797,
                            3.800, 3.805, 3.811, 3.816, 3.821, 3.826, 3.832, 3.837, 3.842, 3.847,
                            3.853, 3.858, 3.863, 3.868, 3.874, 3.879, 3.884, 3.889, 3.895, 3.900,
                            3.906, 3.911, 3.917, 3.922, 3.928, 3.933, 3.939, 3.944, 3.950, 3.956,
                            3.961, 3.967, 3.972, 3.978, 3.983, 3.989, 3.994, 4.000, 4.008, 4.015,
                            4.023, 4.031, 4.038, 4.046, 4.054, 4.062, 4.069, 4.077, 4.085, 4.092,
                            4.100, 4.111, 4.122, 4.133, 4.144, 4.156, 4.167, 4.178, 4.189, 4.200};

int getChargeLevelFromConversionTable(double volts)
{
  int index = 50;
  int previousIndex = 0;
  int half = 0;

  while (previousIndex != index)
  {
    half = abs(index - previousIndex) / 2;
    previousIndex = index;
    if (conversionTable[index] == volts)
    {
      return index;
    }
    index = (volts >= conversionTable[index])
                ? index + half
                : index - half;
  }

  return index;
}

double getRawVoltage()
{
  // read analog and make it more linear
  double reading = analogRead(35);
  for (int i = 0; i < 10; i++)
  {
    reading = (analogRead(35) + reading * 9) / 10;
  }
  if (reading < 1 || reading > 4095)
    return 0;

  return -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
}

void hibernate(uint64_t duration)
{
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
  // Configure Timer as wakeup source
  esp_sleep_enable_timer_wakeup(duration);
  esp_deep_sleep_start();
}