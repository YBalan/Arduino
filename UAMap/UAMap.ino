#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#define VER 1.1
#define RELEASE

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_TRACE_MAIN
#define ENABLE_INFO_MAIN

//#define ENABLE_INFO_ALARMS
//#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

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

#ifdef NETWORK_STATISTIC
#include <map>
struct NetworkStatInfo{ int code; int count; String description; };
std::map<int, NetworkStatInfo> networkStat;
#endif

#define ALARMS_UPDATE_TIMEOUT 30000
#define ALARMS_CHECK_WITHOUT_STATUS true
#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 25
#define BRIGHTNESS_STEP 10

#define LED_STATUS_IDX 0;
#define LED_PORTAL_MODE_COLOR CRGB::Green
#define LED_PORTAL_MODE_TIMEOUT 10

WiFiOps::WiFiOps wifiOps("UAMap WiFi Manager", "UAMapAP", "password");
AlarmsApi api;
CRGBArray<LED_COUNT> leds;


struct Settings
{
  uint8_t Brightness = 5;
} _settings;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("!!!! Start UA Map !!!!");
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);

  
  /*wifiOps
  .AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN")
  .AddParameter("telegramToken", "Telegram Token", "telegram_token", "TELEGRAM_TOKEN");*/

  wifiOps.TryToConnectOrOpenConfigPortal();

  api.setApiKey(api_token);

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT);
  FastLED.clear();
  SetBrightness();
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
    int status = AlarmsApiStatus::NO_WIFI;
    String statusMsg;
    if ((WiFi.status() == WL_CONNECTED)) 
    {      
      bool statusChanged = api.IsStatusChanged(status, statusMsg);
      if(status == AlarmsApiStatus::API_OK)
      {
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");
        if(statusChanged || ALARMS_CHECK_WITHOUT_STATUS)
        {
          std::vector<uint8_t> alarmedLedIdx;
          auto alarmedRegions = api.getAlarmedRegions(status, statusMsg);    
          INFO("Alarmed regions count: ", alarmedRegions.size());
          if(status == AlarmsApiStatus::API_OK)
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
    PrintNetworkStatToSerial();
    
    return true;
  }
  return false;
}

void SetAlarmedLED(const std::vector<uint8_t> &alarmedLedIdx)
{
  TRACE("SetAlarmedLED: ", alarmedLedIdx.size());
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = leds[ledIdx];
    if(std::find(alarmedLedIdx.begin(), alarmedLedIdx.end(), ledIdx) != alarmedLedIdx.end())
    {
      //TRACE("RED: ", ledIdx);
      led = CRGB::Red;
    }
    else
    {
      //TRACE("BLUE: ", ledIdx);
      led = CRGB::Blue;      
    }
    FastLED.show(_settings.Brightness);
  }
  FastLED.show(_settings.Brightness);
}

void SetStatusLED(const int &status, const String &msg)
{
  FillNetworkStat(status, status != AlarmsApiStatus::API_OK ? msg : "OK (200)");
  if(status != AlarmsApiStatus::API_OK)
  {
    INFO("Status: ", status == AlarmsApiStatus::WRONG_API_KEY ? "Unauthorized" : (status == AlarmsApiStatus::NO_WIFI ? "No WiFi" : "No Connection"), " | ", msg);
  }
}

void FillNetworkStat(const int& code, const String &desc)
{
  #ifdef NETWORK_STATISTIC
  networkStat[code].count++;
  networkStat[code].code = code;
  if(networkStat[code].description == "")
    networkStat[code].description = desc;
  #endif
}

void PrintNetworkStatToSerial()
{
  #ifdef NETWORK_STATISTIC
  Serial.print("Network Statistic: ");
  if(networkStat.count(AlarmsApiStatus::API_OK) > 0)
    PrintNetworkStatInfoToSerial(networkStat[AlarmsApiStatus::API_OK]);
  for(auto de : networkStat)
  {
    auto &info = std::get<1>(de);
    if(info.code != AlarmsApiStatus::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
}

void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{
  #ifdef NETWORK_STATISTIC
  Serial.print("[\""); Serial.print(info.description); Serial.print("\": "); Serial.print(info.count); Serial.print("]; ");
  #endif
}

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  {
    wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/true);   
    api.setApiKey(api_token);
  }

  if(debugButtonFromSerial == 2) // Show Network Statistic
  {
    PrintNetworkStatToSerial();
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();

    uint8_t minusCount = std::count(readFromSerial.begin(), readFromSerial.end(), '-');
    if(minusCount > 0)
    {
      auto step = minusCount * BRIGHTNESS_STEP;
      if(_settings.Brightness - step > 0)
        _settings.Brightness -= step;
      else
        _settings.Brightness = 5;

      SetBrightness();
    }

    uint8_t plusCount = std::count(readFromSerial.begin(), readFromSerial.end(), '+');
    if(plusCount > 0)
    {
      auto step = plusCount * BRIGHTNESS_STEP;
      if(_settings.Brightness + step < 255)
        _settings.Brightness += step;
      else
        _settings.Brightness = 255;

      SetBrightness();
    }

    INFO("Input: ", readFromSerial);

    debugButtonFromSerial = readFromSerial.toInt(); 
  }
}

void SetBrightness()
{
  FastLED.setBrightness(_settings.Brightness);
  FastLED.show(_settings.Brightness);
  INFO("BR: ", _settings.Brightness);
}
