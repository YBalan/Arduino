#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define VER 1.0
#define RELEASE

#define ENABLE_TRACE
//#define ENABLE_TRACE_MAIN
#define ENABLE_INFO_MAIN

//#define ENABLE_INFO_ALARMS
//#define ENABLE_TRACE_ALARMS

//#define ENABLE_INFO_WIFI
//#define ENABLE_TRACE_WIFI

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "WiFiOps.h"

#ifdef ENABLE_INFO_MAIN
#define INFO(...) SS_TRACE(__VA_ARGS__)
#else
#define INFO(...) {}
#endif

#ifdef ENABLE_TRACE_MAIN
#define TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define TRACE(...) {}
#endif

#define ALARMS_UPDATE_TIMEOUT 30000
#define ALARMS_CHECK_WITHOUT_STATUS true
#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 25

AlarmsApi api;
CRGB leds[LED_COUNT];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  Serial.println("!!!! Start UA Map !!!!");
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);

  TryToConnect();

  api.setApiKey(api_token);

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(50);
}

void loop() 
{
  static uint32_t current = millis();
  current = millis();

  CheckAndUpdateALarms(current);
  
  HandleDebugSerialCommands();
}

uint32_t alarmsTicks = 0;
const bool CheckAndUpdateALarms(const unsigned long &currentTicks)
{
  if(alarmsTicks == 0 || currentTicks - alarmsTicks >= ALARMS_UPDATE_TIMEOUT)
  {      
    alarmsTicks = currentTicks;

    // wait for WiFi connection
    AlarmsApiStatus status = AlarmsApiStatus::NoWiFi;
    String statusMsg;
    if ((WiFi.status() == WL_CONNECTED)) 
    {      
      bool statusChanged = api.IsStatusChanged(status, statusMsg);
      if(status == AlarmsApiStatus::OK)
      {
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");
        if(statusChanged || ALARMS_CHECK_WITHOUT_STATUS)
        {
          std::vector<uint8_t> alarmedLedIdx;
          auto alarmedRegions = api.getAlarmedRegions(status, statusMsg);    
          INFO("Alarmed regions count: ", alarmedRegions.size());
          if(status == AlarmsApiStatus::OK)
          {
            for(auto rId : alarmedRegions)
            {
              auto ledIdx = api.getLedIndexByRegionId(rId);
              INFO("regionId:", rId, "\tled index: ", ledIdx);
              alarmedLedIdx.push_back(ledIdx);
            }

            SetAlarmedLED(alarmedLedIdx);
          }          
        }
      }                 
    }

    SetStatusLED(status, statusMsg);

    INFO("");
    INFO("Waiting ", ALARMS_UPDATE_TIMEOUT, "ms. before the next round...");
    
    return true;
  }
  return false;
}

void SetAlarmedLED(const std::vector<uint8_t> &alarmedLedIdx)
{
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {
    auto &led = leds[ledIdx];
    if(std::find(alarmedLedIdx.begin(), alarmedLedIdx.end(), ledIdx) != alarmedLedIdx.end())
    {
      led = CRGB(CRGB::Red);
    }
    else
    {
      led = CRGB(CRGB::Yellow);      
    }
  }
  FastLED.show();
}

void SetStatusLED(const AlarmsApiStatus &status, const String &msg)
{
  if(status != AlarmsApiStatus::OK)
  {
    INFO("Status: ", status == AlarmsApiStatus::WRONG_API ? "Unauthorized" : (status == AlarmsApiStatus::NoWiFi ? "No WiFi" : "No Connection"), " | ", msg);
  }
}


uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  {
    TryToConnect(/*resetSettings:*/true);   
    api.setApiKey(api_token);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();

    INFO("Input: ", readFromSerial);

    debugButtonFromSerial = readFromSerial.toInt(); 
  }
}
