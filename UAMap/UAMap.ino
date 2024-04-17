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
#include "LedState.h"
#include "Settings.h"
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

#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 25
#define BRIGHTNESS_STEP 10

UAMap::Settings _settings;
WiFiOps::WiFiOps<3> wifiOps("UAMap WiFi Manager", "UAMapAP", "password");
AlarmsApi api;
CRGBArray<LED_COUNT> leds;
std::vector<uint8_t> alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("!!!! Start UA Map !!!!");
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, _settings.PortalModeColor};

  wifiOps
  .AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN")
  .AddParameter("telegramToken", "Telegram Token", "telegram_token", "TELEGRAM_TOKEN");

  wifiOps.TryToConnectOrOpenConfigPortal();

  api.setApiKey(api_token);

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.clear();  

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 1000, -1, false, false, _settings.NoConnectionColor};
  ledsState[0] = {0 /*Crymea*/, 0, 0, true, false, _settings.AlarmedColor};
  ledsState[4] = {4 /*Luh*/, 0, 0, true, false, _settings.AlarmedColor};

  SetAlarmedLED(alarmedLedIdx);
  CheckAndUpdateRealLeds(millis());
  SetBrightness(); 
}

void loop() 
{
  static bool firstRun = true;
  static uint32_t currentTicks = millis();
  currentTicks = millis();

  if(CheckAndUpdateAlarms(currentTicks))
  {
    //FastLEDShow(true);
  }

  if(CheckAndUpdateRealLeds(currentTicks))
  {
    FastLEDShow(true);
  }
  
  FastLEDShow(false); 

  HandleDebugSerialCommands();    

  firstRun = false;
}

const bool CheckAndUpdateRealLeds(const unsigned long &currentTicks)
{
  //INFO("CheckStatuses");

  bool changed = false;
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;

    if(led.Idx < 0 && led.Idx >= LED_COUNT) break;

    //TRACE("Led idx: ", led.Idx, " Total timeout: ", led.BlinkTotalTime, " Period timeout: ", led.BlinkPeriod, " current: ", currentTicks);

    if(led.TotalTimeTicks == 0 && led.BlinkTotalTime > 0)    
      led.TotalTimeTicks = currentTicks;      
    if(led.PeriodTicks == 0 && led.BlinkPeriod != 0)
      led.PeriodTicks = currentTicks;

    if(led.BlinkTotalTime > 0 && (led.TotalTimeTicks > 0 && currentTicks - led.TotalTimeTicks >= led.BlinkTotalTime))
    { 
      led.PeriodTicks = 0;
      led.TotalTimeTicks = 0;    
      led.BlinkPeriod = 0;
      led.BlinkTotalTime = 0;
      TRACE("Led idx: ", led.Idx, " Total timeout: ", led.BlinkTotalTime, " FINISHED");
    }   

    if(led.BlinkPeriod != 0 && (led.PeriodTicks > 0 && currentTicks - led.PeriodTicks >= abs(led.BlinkPeriod)))
    { 
      leds[led.Idx] = led.BlinkPeriod > 0 ? 0 : led.Color;     
      led.BlinkPeriod *= -1;
      led.PeriodTicks = currentTicks;
      changed = true;
      TRACE("Led idx: ", led.Idx, " BLINK! : period: ", led.BlinkPeriod, " Total time: ", led.BlinkTotalTime);
    }   

    if(led.BlinkTotalTime == 0)
    {
      bool colorChanges = leds[led.Idx] == led.Color;
      if(!colorChanges)
        TRACE("Led idx: ", led.Idx, " Color Changed");
      changed = (changed || !colorChanges);
      leds[led.Idx] = led.Color;   
      //changed = true;      
    }
  }
  return changed;
}

//uint32_t alarmsTicks = 0;
const bool CheckAndUpdateAlarms(const unsigned long &currentTicks)
{
  //INFO("CheckAndUpdateAlarms");
  static uint32_t alarmsTicks = 0;
  bool changed = false;
  if(alarmsTicks == 0 || currentTicks - alarmsTicks >= _settings.alarmsUpdateTimeout)
  {      
    alarmsTicks = currentTicks;

    // wait for WiFi connection
    int status = AlarmsApiStatus::NO_WIFI;
    String statusMsg;
   
    if ((WiFi.status() == WL_CONNECTED)) 
    {     
      INFO("WiFi - CONNECTED");

      bool statusChanged = api.IsStatusChanged(status, statusMsg);
      if(status == AlarmsApiStatus::API_OK)
      {        
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");

        if(statusChanged || _settings.alarmsCheckWithoutStatus)
        {              
          auto alarmedRegions = api.getAlarmedRegions(status, statusMsg);    
          INFO("HTTP response Alarmed regions count: ", alarmedRegions.size());
          if(status == AlarmsApiStatus::API_OK)
          {            
            alarmedLedIdx.clear();
            for(auto rId : alarmedRegions)
            {
              auto ledIdx = api.getLedIndexByRegionId(rId);
              INFO("regionId:", rId, "\tled index: ", ledIdx);
              alarmedLedIdx.push_back(ledIdx);
            }

            SetAlarmedLED(alarmedLedIdx);        
            changed = true;
          }          
        }       
      }                       
    }   
    
    bool statusChanged = SetStatusLED(status, statusMsg);
    changed = (changed || statusChanged);

    INFO("");
    INFO("Alarmed regions count: ", alarmedLedIdx.size());
    INFO("Waiting ", ALARMS_UPDATE_TIMEOUT, "ms. before the next round...");
    PrintNetworkStatToSerial();
  }
  return changed;
}

void SetAlarmedLED(const std::vector<uint8_t> &alarmedLedIdx)
{
  TRACE("SetAlarmedLED: ", alarmedLedIdx.size());
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = ledsState[ledIdx];
    led.Idx = ledIdx;
    if(std::find(alarmedLedIdx.begin(), alarmedLedIdx.end(), ledIdx) != alarmedLedIdx.end())
    {
      if(!led.IsAlarmed)
      {
        led.Color = _settings.AlarmedColor;
        led.BlinkPeriod = LED_NEW_ALARMED_PERIOD;
        led.BlinkTotalTime = LED_NEW_ALARMED_TOTALTIME;
      }       
      led.IsAlarmed = true;      
    }
    else
    {
      led.IsAlarmed = false;
      led.Color = _settings.NotAlarmedColor;      
      led.StopBlink();
    }       

    RecalculateBrightness(led, false);    
  }  
}

const uint8_t GetScaledBrightness(const uint8_t& brScale, const bool& scaleDown)
{
  auto scale = (uint8_t)(float(_settings.Brightness / 100.0) * (float)brScale);
  //int res = _settings.Brightness + (scaleDown ? -scale : scale);
  int res = _settings.Brightness + (scaleDown ? -scale : scale);
  return res < 0 ? 0 : (res > 255 ? 255 : res);
}

void RecalculateBrightness(LedState &led, const bool &showTrace)
{
  if(_settings.alarmedScale > 0)
    {
      if(_settings.alarmScaleDown)
      {
        if(!led.IsAlarmed || led.FixedBrightnessIfAlarmed)
        {
          auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
          if(showTrace)
            TRACE("Led: ", led.Idx, " Br: ", br);
        }
      }
      else
      {
        if(led.IsAlarmed && !led.FixedBrightnessIfAlarmed)
        {
          auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
          if(showTrace)
            TRACE("Led: ", led.Idx, " Br: ", br);
        } 
      }
    }
}

void RecalculateBrightness()
{
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;
    RecalculateBrightness(led, true);
  }
}

//int prevStatus = AlarmsApiStatus::UNKNOWN;
const bool SetStatusLED(const int &status, const String &msg)
{
  static int prevStatus = AlarmsApiStatus::UNKNOWN;
  bool changed = prevStatus != status;
  prevStatus = status;

  FillNetworkStat(status, status != AlarmsApiStatus::API_OK ? msg : "OK (200)");
  
  if(status != AlarmsApiStatus::API_OK)
  {
    INFO("Status: ", status == AlarmsApiStatus::WRONG_API_KEY ? "Unauthorized" : (status == AlarmsApiStatus::NO_WIFI ? "No WiFi" : "No Connection"), " | ", msg);
    switch(status)
    {
      default:
        TRACE("STATUS NOT OK START BLINK");
        ledsState[LED_STATUS_IDX].Color = _settings.NoConnectionColor;
        ledsState[LED_STATUS_IDX].StartBlink(LED_STATUS_NO_CONNECTION_PERIOD, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
    }
  }
  else
  {
    TRACE("STATUS OK STOP BLINK");
    auto &led = ledsState[LED_STATUS_IDX];
    led.Color = led.IsAlarmed ? _settings.AlarmedColor : _settings.NotAlarmedColor;
    RecalculateBrightness(led, false);
    led.StopBlink();   
  }

  return changed;
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
    ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, _settings.PortalModeColor};
    FastLEDShow(true);
    wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/true);   
    api.setApiKey(api_token);
  }

  if(debugButtonFromSerial == 2) // Show Network Statistic
  {
    INFO("BR: ", _settings.Brightness);
    INFO("Alarmed regions count: ", alarmedLedIdx.size());
    INFO("alarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus);
    PrintNetworkStatToSerial();
  }

  if(debugButtonFromSerial == 15) // Blink Test
  {
    ledsState[LED_STATUS_IDX].Color = CRGB::Yellow;
    ledsState[LED_STATUS_IDX].StartBlink(200, 20000);
  }

  if(debugButtonFromSerial == 100)
  {
    wifiOps.AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN");
    TRACE("WifiOps parameters count: ", wifiOps.ParametersCount());
  //.AddParameter("telegramToken", "Telegram Token", "telegram_token", "TELEGRAM_TOKEN");
  }

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO("alarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();
    INFO("Input: ", readFromSerial);

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

    debugButtonFromSerial = readFromSerial.toInt(); 
  }
}

void SetBrightness()
{
  INFO("BR: ", _settings.Brightness);
  FastLED.setBrightness(_settings.Brightness);  
  RecalculateBrightness(); 
  FastLEDShow(true); 
}

void FastLEDShow(const bool &showTrace)
{
  if(showTrace)
    INFO("FastLEDShow()");
  FastLED.show();  
}
