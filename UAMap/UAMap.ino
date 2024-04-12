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

/*#define ENABLE_INFO_ALARMS
#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI*/

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
#define LED_COUNT 24

AlarmsApi api;
CRGB leds[LED_COUNT];

byte ledsMap[LED_COUNT] = 
    {
      9999, //Idx: 0 - "Crimea"
      23,   //Idx: 1 - "Kherson"
      12,   //Idx: 2 - "Zap"
      28,   //Idx: 3 - "Don"
      16,   //Idx: 4 - "Luh"
      //....
    };

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
const bool &CheckAndUpdateALarms(const unsigned long &currentTicks)
{
  if(alarmsTicks == 0 || currentTicks - alarmsTicks >= ALARMS_UPDATE_TIMEOUT)
  {      
    alarmsTicks = currentTicks;

    // wait for WiFi connection
    AlarmsApiStatus status = AlarmsApiStatus::NoWiFi;
    if ((WiFi.status() == WL_CONNECTED)) 
    {      
      bool statusChanged = api.IsStatusChanged(status);
      if(status == AlarmsApiStatus::OK)
      {
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");
        if(statusChanged || ALARMS_CHECK_WITHOUT_STATUS)
        {
          auto regions = api.getAlarmedRegions(status);    
          if(status == AlarmsApiStatus::OK)
          {
            for(auto rId : regions)
            {
              INFO("regionId:", rId);
            }
          }          
        }
      }                 
    }

    SetStatusLED(status);

    INFO("");
    INFO("Waiting ", ALARMS_UPDATE_TIMEOUT, "ms. before the next round...");
    
    return true;
  }
  return false;
}

void SetStatusLED(const AlarmsApiStatus &status)
{
  if(status != AlarmsApiStatus::OK)
  {
    INFO("Status: ", status == AlarmsApiStatus::WRONG_API ? "Anauthorized" : (status == AlarmsApiStatus::NoWiFi ? "No WiFi" : "No Connection"));
  }
}


uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // SHOW DateTime
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
