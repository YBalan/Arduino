//Estimates: https://docs.google.com/spreadsheets/d/1mYA_Bc687Y8no1yJDxv83fimtd0WU4nvcGI80_jnJME/edit?usp=sharing

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.35")
#else //ESP32
  #define VER F("1.42")
#endif

#define AVOID_FLICKERING

//#define RELEASE
//#define DEBUG

#define NETWORK_STATISTIC
//#define ENABLE_TRACE
//#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define WM_NODEBUG
//#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY

//#define USE_BUZZER_MELODIES 

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_BOT_MENU
#define ENABLE_TRACE_BOT_MENU

#define ENABLE_INFO_ALARMS
#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

// #define ENABLE_INFO_PMONITOR
// #define ENABLE_TRACE_PMONITOR

#define ENABLE_INFO_BUZZ
#define ENABLE_TRACE_BUZZ

#else

#define WM_NODEBUG
//#define WM_DEBUG_LEVEL 0

#endif

#define RELAY_OFF HIGH
#define RELAY_ON  LOW

//DebounceTime
#define DebounceTime 50

#ifdef ESP32
  #define IsESP32 true
  #include <SPIFFS.h>
#else
  #define IsESP32 false
#endif

// Platform specific
#ifdef ESP8266 
  #define ESPgetFreeContStack ESP.getFreeContStack()
  #define ESPresetHeap ESP.resetHeap()
  #define ESPresetFreeContStack ESP.resetFreeContStack()
#else //ESP32
  #define ESPgetFreeContStack F("Not supported")
  #define ESPresetHeap {}
  #define ESPresetFreeContStack {}
#endif

#define LED_TYPE    WS2812B
#ifdef ESP8266 
  #define FASTLED_ESP8266_RAW_PIN_ORDER
  //#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
  //#define FASTLED_ESP8266_D1_PIN_ORDER  
#else //ESP32  
  //#define FASTLED_ESP32_SPI_BUS HSPI
#endif
#ifdef AVOID_FLICKERING
  #define FASTLED_ALL_PINS_HARDWARE_SPI
  #define FASTLED_ALLOW_INTERRUPTS 0
  #define FASTLED_INTERRUPT_RETRY_COUNT 0  
#endif
#include <FastLED.h>  

#include "Config.h"

#include <map>
#include <set>
//#include <ezButton.h>

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "LedState.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBotHelper.h"
#include "BuzzHelper.h"
#include "UAAnthem.h"
#include "Prapors.h"
#include "PMonitor.h"
#include "TBotMenu.h"

#define LONG_PRESS_VALUE_MS 1000
#include "Button.h"

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
struct NetworkStatInfo{ int code; int count; String description; };
std::map<int, NetworkStatInfo> networkStat;
#endif

//CRGBArray<LED_COUNT> leds;
CRGB *leds = new CRGB[LED_COUNT];

typedef std::map<uint8_t, RegionInfo*> LedIndexMappedToRegionInfo;
LedIndexMappedToRegionInfo alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

Button resetBtn(PIN_RESET_BTN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);   
 
  pinMode(PIN_RELAY1, INPUT_PULLUP);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, INPUT_PULLUP);
  pinMode(PIN_RELAY2, OUTPUT);

  pinMode(PIN_BUZZ, INPUT_PULLUP);
  pinMode(PIN_BUZZ, OUTPUT);
  digitalWrite(PIN_BUZZ, LOW);

  digitalWrite(PIN_RELAY1, RELAY_OFF);
  digitalWrite(PIN_RELAY2, RELAY_OFF);    

  resetBtn.setDebounceTime(DebounceTime);  

  Serial.println();
  Serial.println(F("!!!! Start UA Map !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(VER);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  LoadSettings();
  LoadSettingsExt();
  LoadSettingsRelayExt();
  //_settings.reset();

  PMonitor::LoadSettings();
  PMonitor::Init(PIN_PMONITOR_SDA, PIN_PMONITOR_SCL);

  FastLED.addLeds<LED_TYPE, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
  //FastLED.setMaxRefreshRate(10);

  FastLED.clear();   
  fill_ua_prapor2();
  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);

  FastLEDShow(1000);
  
  WiFiOps::WiFiOps wifiOps(F("UAMap WiFi Manager"), F("UAMapAP"), F("password"));

  #ifdef WM_DEBUG_LEVEL
    INFO(F("WM_DEBUG_LEVEL: "), WM_DEBUG_LEVEL);    
  #else
    INFO(F("WM_DEBUG_LEVEL: "), F("Off"));
  #endif

  wifiOps
  .AddParameter(F("apiToken"), F("Alarms API Token"), F("api_token"), F("YOUR_ALARMS_API_TOKEN"), 47)  
  #ifdef USE_BOT
  .AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;    

  auto resetButtonState = resetBtn.getState();
  INFO(F("ResetBtn: "), resetButtonState == HIGH ? F("Off") : F("On"));
  INFO(F("ResetFlag: "), _settings.resetFlag);
  wifiOps.TryToConnectOrOpenConfigPortal(/*portalTimeout:*/60, /*restartAfterPortalTimeOut*/true, /*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  if(_settings.resetFlag == 1985)
  {
    _settings.resetFlag = 200;
    SaveSettings();
  }
  api->setApiKey(wifiOps.GetParameterValueById(F("apiToken"))); 
  api->setBaseUri(_settings.BaseUri); 
  INFO(F("Base Uri: "), _settings.BaseUri);  
 
  //ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyivska*/, 1000, -1, false, false, false, _settings.NoConnectionColor};
  //ledsState[0] = {0 /*Crimea*/, 0, 0, true, false, false, _settings.AlarmedColor};
  //ledsState[4] = {4 /*Luh*/, 0, 0, true, false, false, _settings.AlarmedColor};
  
  SetAlarmedLED();
  CheckAndUpdateRealLeds(millis(), /*effectStarted:*/false);
  SetBrightness(); 

  #ifdef USE_BOT  
  LoadChannelIDs();
  bot->setToken(wifiOps.GetParameterValueById(F("telegramToken")));  
  _botSettings.SetBotName(wifiOps.GetParameterValueById(F("telegramName")));  
  _botSettings.botSecure = wifiOps.GetParameterValueById(F("telegramSec"));
  bot->attach(HangleBotMessages);
  bot->setTextMode(FB_TEXT); 
  //bot->setPeriod(_settings.alarmsUpdateTimeout);
  bot->setLimit(1);
  bot->skipUpdates();
  #endif

  #ifdef DEBUG  
  Buzz::PlayMelody(PIN_BUZZ, F("500, 200"));
  #endif
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("Config Portal Started: "), manager->getConfigPortalSSID());
  FastLED.clear(); 
  leds[LED_STATUS_IDX] = _settings.PortalModeColor;

  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);  
  FastLEDShow(1000);    
}

void HandleButton(const uint32_t &currentTicks)
{
  resetBtn.loop();

  if(resetBtn.isPressed())
  {
    TRACE(BUTTON_IS_PRESSED_MSG, F(" BR: "), _settings.Brightness);
  }
  if(resetBtn.isReleased())
  {    
    if(resetBtn.isLongPress())
    {
      TRACE(BUTTON_IS_LONGPRESSED_MSG, F(" BR: "), _settings.Brightness, F("\t"), LONG_PRESS_VALUE_MS, F("ms..."));
      resetBtn.resetTicks();
      if(_settings.NotAlarmedColor == CRGB::Blue)
      {
        SetYellowColorSchema();
      }else
      if(_settings.NotAlarmedColor == CRGB::Yellow)
      {
        SetWhiteColorSchema();
      }
      else
      {
        SetBlueDefaultColorSchema();
      }
      SaveSettings();
    }else
    {
      static bool brBtnChangeDirectionUp = false;
      auto nextBr = _settings.Brightness + (brBtnChangeDirectionUp ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP);
      if(nextBr <= 0)
      {
        nextBr = 1;
        brBtnChangeDirectionUp = true;
      }else
      if(brBtnChangeDirectionUp && nextBr == BRIGHTNESS_STEP + 1)
      {
        nextBr = 2;
      }else
      if(brBtnChangeDirectionUp && nextBr == BRIGHTNESS_STEP + 2)
      {
        nextBr = 5;
      }else
      if(brBtnChangeDirectionUp && nextBr == BRIGHTNESS_STEP + 5)
      {
        nextBr = 10;
      }else
      if(brBtnChangeDirectionUp && nextBr == BRIGHTNESS_STEP + 10)
      {
        nextBr = BRIGHTNESS_STEP;
      }else
      if(nextBr > 255)
      {
        nextBr = 255;
        brBtnChangeDirectionUp = false;
      }

      _settings.Brightness = nextBr;

      TRACE(BUTTON_IS_RELEASED_MSG, F(" BR: "), _settings.Brightness);
      SetBrightness();   
      SaveSettings();
    } 
  }  
}

void loop() 
{
  static bool firstRun = true;
  static uint32_t currentTicks = millis();
  currentTicks = millis();  

  HandleButton(currentTicks);

  HandleEffects(currentTicks);

  if(_settingsExt.Mode == ExtMode::Souvenir && _effect == Effect::Normal)
  {
    yield(); // watchdog
    if(!effectStarted)
    {   
      INFO(F("\t\t"), F("MODE: "), GetExtModeStr(_settingsExt.Mode), F(": "), GetExtSouvenirModeStr(_settingsExt.SouvenirMode)); 
      if(_settingsExt.SouvenirMode == ExtSouvenirMode::UAPrapor)
      {        
        fill_ua_prapor2();
        SetStateFromRealLeds();        
      }else
      if(_settingsExt.SouvenirMode == ExtSouvenirMode::BGPrapor)
      {        
        fill_bg_prapor();
        SetStateFromRealLeds();        
      }else
      if(_settingsExt.SouvenirMode == ExtSouvenirMode::MDPrapor)
      {        
        fill_md_prapor();
        SetStateFromRealLeds();        
      }
      effectStarted = true;
    }
  }
  else
  if(_settingsExt.Mode == ExtMode::Alarms || _settingsExt.Mode == ExtMode::AlarmsOnlyCustomRegions)
  {  
    yield(); // watchdog 
  int httpCode;
  String statusMsg;
  if(_effect == Effect::Normal)
  { 
    effectStarted = false;
    #ifdef USE_STOPWATCH
    uint32_t sw = millis();
    #endif
    if(CheckAndUpdateAlarms(currentTicks, httpCode, statusMsg))
    {
      INFO(F("\t\t"), F("MODE: "), GetExtModeStr(_settingsExt.Mode));
      //when updated
      //FastLEDShow(true);

      #ifdef USE_STOPWATCH
      sw = millis() - sw;
      #endif

      #ifdef USE_BOT
        #ifdef USE_NOTIFY
          if(_settings.notifyHttpCode == 1 || //All http codes
              (_settings.notifyHttpCode != 0  // Notify is ON
                &&(
                      (_settings.notifyHttpCode < 0 && httpCode != abs(_settings.notifyHttpCode)) //-200 means - All except 200
                    || (_settings.notifyHttpCode == httpCode)
                  )
              )
           )
          {
            String notifyMessage = String(F("Notification: ")) + statusMsg + F(" ") + String(httpCode)
            #ifdef USE_STOPWATCH
            + F(" (") + String(sw) + F("ms") + F(")")
            #endif
            ;

            if(strlen(_settingsExt.NotifyChatId) > 0)
            {
              TRACE(notifyMessage, F(" ChatId: "), _settingsExt.NotifyChatId);
              bot->sendMessage(notifyMessage, _settingsExt.NotifyChatId);
            }
          }
        #endif
      #endif

      #ifdef USE_STOPWATCH
       TRACE(F("API Stop watch: "), sw, F("ms..."));
      #endif
    }   

    if(CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/false))
    {
      //FastLEDShow(false);
    }        
  }  
  }
  
  FastLEDShow(false); 

  HandleDebugSerialCommands();     

  #ifdef USE_BOT
  uint8_t botStatus = bot->tick();  
  yield(); // watchdog
  if(botStatus == 0)
  {
    // BOT_INFO(F("BOT UPDATE MANUAL: millis: "), millis(), F(" current: "), currentTicks, F(" "));
    // botStatus = bot->tickManual();
  }
  if(botStatus == 2)
  {
    BOT_INFO(F("Bot overloaded!"));
    bot->skipUpdates();
    //bot->answer(F("Bot overloaded!"), /**alert:*/ true); 
  }  
  #endif   

  #ifdef USE_POWER_MONITOR
  #ifdef PM_UPDATE_PERIOD    
  if(pmUpdatePeriod > 0 && pmUpdateTicks > 0 && millis() - pmUpdateTicks >= pmUpdatePeriod)
  {
    PM_TRACE(F("PM Update period: "), pmUpdatePeriod);
    pmUpdateTicks = millis();
    //EVERY_N_MILLISECONDS_I(PM, pmUpdatePeriod)
    {    
      const float &led_consumption_voltage_factor = GetLEDVoltageFactor();
      const auto &voltage = PMonitor::GetVoltage(led_consumption_voltage_factor);      

      for(auto &chatIDkv : pmChatIds)
      {
        const auto &chatID = chatIDkv.first;
        auto &chatIDInfo = chatIDkv.second;
        chatIDInfo.CurrentValue = voltage;
        SetPMMenu(chatID, chatIDInfo.MsgID, voltage, led_consumption_voltage_factor);

        if(chatIDInfo.AlarmValue > 0)
        {
          if(voltage <= chatIDInfo.AlarmValue)
          {
            String alarmMessage = String(PM_MENU_ALARM_NAME) + F(": ") + String(voltage, 2) + PM_MENU_VOLTAGE_UNIT;
            PM_TRACE(alarmMessage);
            bot->sendMessage(alarmMessage, chatID);
          }
        }
      }
    }
  }
  yield(); // watchdog
  #endif
  #endif

  firstRun = false;
}

const bool CheckAndUpdateRealLeds(const unsigned long &currentTicks, const bool &effectStarted)
{
  //INFO("CheckStatuses");

  bool changed = false;
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;

    if(led.Idx < 0 && led.Idx >= LED_COUNT) continue;

    //TRACE(F("Led idx: "), led.Idx, F(" Total timeout: "), led.BlinkTotalTime, F(" Period timeout: "), led.BlinkPeriod, F(" current: "), currentTicks);

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
      //TRACE(F("Led idx: "), led.Idx, F(" Total timeout: "), led.BlinkTotalTime, F(" FINISHED"));
    }   

    if(led.BlinkPeriod != 0 && (led.PeriodTicks > 0 && currentTicks - led.PeriodTicks >= abs(led.BlinkPeriod)))
    {       
      leds[led.Idx] = led.BlinkPeriod > 0 ? CRGB::Black : led.Color;      
      //led.BlinkPeriod *= -1;
      led.BlinkPeriod = led.BlinkPeriod > 0 ? (0 - led.BlinkPeriod) : (abs(led.BlinkPeriod));
      led.PeriodTicks = currentTicks;
      changed = true;
      //TRACE(F("Led idx: "), led.Idx, F(" BLINK! : period: "), led.BlinkPeriod, F(" Total time: "), led.BlinkTotalTime);
    }   

    if(led.BlinkTotalTime == 0)
    {
      bool colorChanges = leds[led.Idx] == led.Color;
      if(!colorChanges)
      {
        //TRACE(F("Led idx: "), led.Idx, F(" Color Changed"));
      }
      changed = (changed || !colorChanges);

      if(!effectStarted)
        led.SetColors(_settings);      
      RecalculateBrightness(led, false);
      leds[led.Idx] = led.Color;         
      //changed = true;      
    }
  }
  return changed;
}

//uint32_t alarmsTicks = 0;
const bool CheckAndUpdateAlarms(const unsigned long &currentTicks, int &httpCode, String &statusMsg)
{  
  //INFO("CheckAndUpdateAlarms");  
  static uint32_t alarmsTicks = 0;
  bool changed = false;
  if(alarmsTicks == 0 || currentTicks - alarmsTicks >= _settings.alarmsUpdateTimeout)
  {      
    alarmsTicks = currentTicks;

    // wait for WiFi connection
    httpCode = ApiStatusCode::NO_WIFI;
    statusMsg = F("No WiFi");
   
    if ((WiFi.status() == WL_CONNECTED)) 
    {     
      httpCode = ApiStatusCode::UNKNOWN;
      statusMsg.clear();
      //INFO("WiFi - CONNECTED");
      //ESP.resetHeap();
      //ESP.resetFreeContStack();
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESPgetFreeContStack);

      bool statusChanged = api->IsStatusChanged(httpCode, statusMsg);
      if(httpCode == ApiStatusCode::API_OK || httpCode == HTTP_CODE_METHOD_NOT_ALLOWED)
      {        
        INFO(F("IsStatusChanged: "), statusChanged ? F("true") : F("false"));
        TRACE(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus ? F("true") : F("false"));

        if(statusChanged || _settings.alarmsCheckWithoutStatus || ALARMS_CHECK_WITHOUT_STATUS)
        {              
          auto alarmedRegions = api->getAlarmedRegions2(httpCode, statusMsg);    
          TRACE(F("HTTP "), F("Alarmed regions count: "), alarmedRegions.size());
          if(httpCode == ApiStatusCode::API_OK)
          { 
            _settings.alarmsCheckWithoutStatus = false;           
            alarmedLedIdx.clear();
            for(uint8_t idx = 0; idx < alarmedRegions.size(); idx++)
            {
              RegionInfo * const alarmedRegion = alarmedRegions[idx];
              const auto &ledRange = getLedIndexesByRegion(alarmedRegion->Id);
              for(auto &ledIdx : ledRange)
              {
                TRACE(F(" Region: "), alarmedRegion->Id, F("\t"), F("Led idx: "), ledIdx, F("\tName: "), alarmedRegion->Name);
                alarmedLedIdx[ledIdx] = alarmedRegion;
              }
            }

            SetAlarmedLED();            
            changed = true;
          } 

          if(httpCode == ApiStatusCode::JSON_ERROR)
          {
            _settings.alarmsCheckWithoutStatus = true;
            ESPresetHeap;
            ESPresetFreeContStack;
          }         
        } 

        SetRelayStatus();           
      }                       
    }
    else
    {
      leds[LED_STATUS_IDX] = CRGB::Green;
      FastLEDShow(500);
      #ifdef ESP32      
      WiFi.disconnect();
      WiFi.reconnect();
      delay(200);
      #endif
    }   
    
    bool statusChanged = SetStatusLED(httpCode, statusMsg);
    changed = (changed || statusChanged);

    INFO(F(""));
    INFO(F("Alarmed regions count: "), alarmedLedIdx.size());
    INFO(F("Waiting "), _settings.alarmsUpdateTimeout, F("ms..."));
    PrintNetworkStatToSerial();

    return true;    
  }
  //return changed;
  return false;
}

const int GetAlarmedLedIdxSize()
{
  return alarmedLedIdx.size();
}

void SetAlarmedLedRegionInfo(const int &regionId, RegionInfo *const regionPtr)
{
  alarmedLedIdx[(UARegion)regionId] = regionPtr;
}

void SetAlarmedLED()
{
  TRACE(F("SetAlarmedLED: "), alarmedLedIdx.size());
  FastLED.setBrightness(_settings.Brightness);
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = ledsState[ledIdx];
    led.Idx = ledIdx;    
    if(alarmedLedIdx.count(ledIdx) > 0)
    {
      const auto regionInfo = alarmedLedIdx[ledIdx];
      const uint8_t &regionId = (uint8_t)regionInfo->Id;
      bool ifAlarmedCustomRegion = _settingsExt.Mode == ExtMode::AlarmsOnlyCustomRegions ? IsRelaysContains(regionId) : true;
      if(ifAlarmedCustomRegion)
      {
        if(!led.IsAlarmed)
        {
          led.Color = led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor;
          led.BlinkPeriod = LED_NEW_ALARMED_PERIOD;
          led.BlinkTotalTime = LED_NEW_ALARMED_TOTALTIME;
        }       
        led.IsAlarmed = true;  
        led.IsPartialAlarmed = regionInfo->AlarmStatus == ApiAlarmStatus::PartialAlarmed;
        RecalculateBrightness(led, false);    
        continue;
      }
    }
    
    {
      led.IsAlarmed = false;
      led.IsPartialAlarmed = false;
      led.Color = _settings.NotAlarmedColor;      
      led.StopBlink();
    }       

    RecalculateBrightness(led, false);    
  }  
}

void SetRelayStatus()
{
  TRACE(F("SetRelayStatus: ")); TRACE(F("Relay1"), F(": "), GetRelay1Str(nullptr)); TRACE(F("Relay2"), F(": "), GetRelay2Str(nullptr)); 
  //if(_settings.Relay1Region == 0 && _settings.Relay2Region == 0) return;
  if(IsRelay1Off() && IsRelay2Off()) return;
  
  bool found1 = false;
  bool found2 = false;  

  for(const auto &idx : alarmedLedIdx)
  {
    TRACE(F("Led idx: "), idx.first, F(" Region: "), idx.second->Id);
  
    //if(!found1 && idx.second->Id == (UARegion)_settings.Relay1Region)
    if(!found1 && IsRelay1Contains(idx.second->Id))
    {
      TRACE(F("Found1: "), idx.second->Id, F(" Name: "), idx.second->Name);
      found1 = true;        
    }      
    //if(!found2 && idx.second->Id == (UARegion)_settings.Relay2Region)
    if(!found2 && IsRelay2Contains(idx.second->Id))
    {
      TRACE(F("Found2: "), idx.second->Id, F(" Name: "), idx.second->Name);
      found2 = true;        
    }
  }

  if(found1)
  {
    if(digitalRead(PIN_RELAY1) == RELAY_OFF)
    {
      DoStrobe(/*alarmedColorSchema:*/true);
      Buzz::AlarmStart(PIN_BUZZ, _settings.BuzzTime);      
    }

    digitalWrite(PIN_RELAY1, RELAY_ON);    
    TRACE(F("Relay1"), F(": "), F("ON"), F(" Region: "), GetRelay1Str(nullptr));      
  }
  else
  {
    if(digitalRead(PIN_RELAY1) == RELAY_ON)
    { 
      digitalWrite(PIN_RELAY1, RELAY_OFF);
      Buzz::AlarmEnd(PIN_BUZZ, _settings.BuzzTime);
      TRACE(F("Relay1"), F(": "), F("Off"), F(" Region: "), GetRelay1Str(nullptr));
    }
  }

  if(found2)
  {
    digitalWrite(PIN_RELAY2, RELAY_ON);
    TRACE(F("Relay2"), F(": "), F("ON"), F(" Region: "), GetRelay2Str(nullptr));      
  }
  else
  {
    if(digitalRead(PIN_RELAY2) == RELAY_ON)
    {      
      digitalWrite(PIN_RELAY2, RELAY_OFF);
      TRACE(F("Relay2"), F(": "), F("Off"), F(" Region: "), GetRelay2Str(nullptr));
    }
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
    if(led.IsAlarmed && led.FixedBrightnessIfAlarmed)
    {
      auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      if(showTrace)
         INFO(F("Led idx: "), led.Idx, F(" BR: "), br);
    }
      // if(_settings.alarmScaleDown)
      // {
      //   if(!led.IsAlarmed || led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE(F("Led: "), led.Idx, F(" BR: "), br);
      //   }
      // }
      // else
      // {
      //   if(led.IsAlarmed && !led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE(F("Led: "), led.Idx, F(" BR: "), br);
      //   } 
      // }
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

const bool SetStatusLED(const int &status, const String &msg)
{
  static int prevStatus = ApiStatusCode::UNKNOWN;
  bool changed = prevStatus != status;
  prevStatus = status;

  FillNetworkStat(status, msg);
  auto &led = ledsState[LED_STATUS_IDX];
  if(status != ApiStatusCode::API_OK)
  {
    TRACE(F("Status: "), status == ApiStatusCode::WRONG_API_KEY ? F("Unauthorized") : (status == ApiStatusCode::NO_WIFI ? F("No WiFi") : F("No Connection")), F(" | "), msg);
    switch(status)
    {
      case ApiStatusCode::JSON_ERROR:
        TRACE(F("STATUS JSON ERROR START BLINK"));
        ledsState[LED_STATUS_IDX].Color = led.Color;
        ledsState[LED_STATUS_IDX].StartBlink(200, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
      case ApiStatusCode::NO_WIFI:
        INFO(F("STATUS NO WIFI"));
        ledsState[LED_STATUS_IDX].Color = CRGB::Green;
        ledsState[LED_STATUS_IDX].StartBlink(LED_STATUS_NO_CONNECTION_PERIOD, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
      default:
        INFO(F("STATUS NOT OK START BLINK"));
        ledsState[LED_STATUS_IDX].Color = _settings.NoConnectionColor;
        ledsState[LED_STATUS_IDX].StartBlink(LED_STATUS_NO_CONNECTION_PERIOD, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
    }
    #ifdef USE_BOT
    //SendMessageToAllRegisteredChannels(BOT_CONNECTION_ISSUES_MSG);
    #endif
    //FastLEDShow(1000);
  }
  else
  {
    TRACE(F("STATUS OK STOP BLINK"));    
    led.SetColors(_settings);
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

#ifdef NETWORK_STATISTIC  
void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{  
  Serial.print(F("[\"")); Serial.print(info.description); Serial.print(F("\": ")); Serial.print(info.count); Serial.print(F("]; "));   
}
#endif

void PrintNetworkStatToSerial()
{
  #ifdef NETWORK_STATISTIC
  Serial.print(F("Network Statistic: "));
  if(networkStat.size() > 0)
  {
    if(networkStat.count(ApiStatusCode::API_OK) > 0)
      PrintNetworkStatInfoToSerial(networkStat[ApiStatusCode::API_OK]);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != ApiStatusCode::API_OK)
        PrintNetworkStatInfoToSerial(info);
    }
  }
  Serial.print(F(" ")); Serial.print(millis() / 1000); Serial.print(F("sec"));
  Serial.println();
  #endif
}

void PrintNetworkStatInfo(const NetworkStatInfo &info, String &str)
{  
  str += String(F("[\"")) + info.description + F("\": ") + String(info.count) + F("]; ");
}

void PrintNetworkStatistic(String &str, const int& codeFilter)
{
  str = F("NSTAT: ");
  #ifdef NETWORK_STATISTIC  
  if(networkStat.size() > 0)
  {
    if(networkStat.count(ApiStatusCode::API_OK) > 0 && (codeFilter == 0 || codeFilter == ApiStatusCode::API_OK))
      PrintNetworkStatInfo(networkStat[ApiStatusCode::API_OK], str);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != ApiStatusCode::API_OK && (codeFilter == 0 || codeFilter == info.code))
        PrintNetworkStatInfo(info, str);
    }
  }
  const auto &millisec = millis();
  str += (networkStat.size() > 0 ? String(F(" ")) : String(F("")))
      + (millisec >= 60000 ? String(millisec / 60000) + String(F("min")) : String(millisec / 1000) + String(F("sec")));
      
  //str += F(" ") + String(ESP.getResetInfoPtr()->reason);
  #ifdef ESP8266
  str += F(" ") + GetResetReason();
  #else //ESP32
  str += String(F(" Rst:[")) + GetResetReason() + F("]");
  #endif
  #else
  str += F("Off");
  #endif
}

const String GetResetReason()
{
  #ifdef ESP8266
  return ESP.getResetReason();
  #else //ESP32
  // Get the reset reason
  esp_reset_reason_t resetReason = esp_reset_reason();

  // Print the reset reason to the Serial Monitor
  
  switch (resetReason) {
    case ESP_RST_UNKNOWN:
      return F("Unknown");
      break;
    case ESP_RST_POWERON:
      return F("Power on");
      break;
    case ESP_RST_EXT:
      return F("External reset");
      break;
    case ESP_RST_SW:
      return F("Software reset");
      break;
    case ESP_RST_PANIC:
      return F("Exception/panic");
      break;
    case ESP_RST_INT_WDT:
      return F("Watchdog reset (Interrupt)");
      break;
    case ESP_RST_TASK_WDT:
      return F("Watchdog reset (Task)");
      break;
    case ESP_RST_WDT:
      return F("Watchdog reset");
      break;
    case ESP_RST_DEEPSLEEP:
      return F("Deep sleep reset");
      break;
    case ESP_RST_BROWNOUT:
      return F("Brownout reset");
      break;
    case ESP_RST_SDIO:
      return F("SDIO reset");
      break;
    default:
      return F("Unknown reason");
      break;
  }
  #endif
}

void DoStrobe(const bool &alarmedColorSchema)
{
  for(uint8_t i = 0; i < LED_COUNT; i++)
  {
    auto &led = ledsState[i];
    led.StartBlink(70, 15000);
    if(alarmedColorSchema)
      led.SetColors(_settings);
  }
}

void HandleEffects(const unsigned long &currentTicks)
{
   if(effectStartTicks > 0 && currentTicks - effectStartTicks >= EFFECT_TIMEOUT)
  {
    effectStartTicks = 0;
    _effect = Effect::Normal;
  }

  if(_effect == Effect::Rainbow)
  {    
    //fill_rainbow(leds, LED_COUNT, 0, 1);
    fill_rainbow_circular(leds, LED_COUNT, 0, 1);
    effectStarted = true;
  }else
  if(_effect == Effect::Strobe)
  {    
    if(!effectStarted)
    {
      DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::Gay)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_rainbow_circular(leds, LED_COUNT, 0, 1);
      SetStateFromRealLeds();

      DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::FillRGB)
  {     
    if(!effectStarted)
    {
      //FastLED.setBrightness(255);      
      fill_solid(leds, LED_COUNT, _settings.NotAlarmedColor);
      SetStateFromRealLeds();      
      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::UA)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_ua_prapor2();
      SetStateFromRealLeds();

      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::UAWithAnthem)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_ua_prapor2();
      SetStateFromRealLeds();

      #ifdef USE_BUZZER
      TRACE(F("\t\t\tUA Prapor with Anthem playing!!!!"));
      FastLEDShow(500);
      UAAnthem::play(PIN_BUZZ, 0);
      #endif
      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::BG)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_bg_prapor();
      SetStateFromRealLeds();
      
      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::MD)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_md_prapor();
      SetStateFromRealLeds();
      
      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }
}

#ifdef USE_POWER_MONITOR
#define PM_SUPPLIER_VOTAGE 5.30 //Volts
#define PM_LEDS_RESIST 0.17 //Ohms
const float GetLEDVoltageFactor()
{
  if(_settings.Brightness > 2)
  {
    //https://www.reddit.com/r/FastLED/comments/gowuga/fastled_power_consumption_functions/
    const auto& power_mW_unscaled =  calculate_unscaled_power_mW(leds, LED_COUNT);
    // Adjust power consumption for brightness
    float power_mW_actual = (power_mW_unscaled * _settings.Brightness) / 255;

    float ledsA = (power_mW_actual / PM_SUPPLIER_VOTAGE) / 1000;
    float Vdrop = ledsA * PM_LEDS_RESIST;
    float VEffective = PM_SUPPLIER_VOTAGE - Vdrop;

    float factor = PM_SUPPLIER_VOTAGE / VEffective;
    
    PM_TRACE(F("\tLEDS cons: BR: "), _settings.Brightness);
    PM_TRACE(F("\tLEDS cons: "), power_mW_unscaled, F(" mW unscaled"));
    PM_TRACE(F("\tLEDS cons: "), power_mW_actual, F(" mW actual"));
    PM_TRACE(F("\tLEDS cons: "), ledsA, F(" A")); 
    PM_TRACE(F("\tLEDS cons: "), Vdrop, F(" V drop")); 
    PM_TRACE(F("\tLEDS cons: "), VEffective, F(" V effective"));

    PM_TRACE(F("\tLEDS cons: "), factor, F("%"));  
    
    return factor;
  }
  return 1.0;
}
#endif

void PrintFSInfo(String &fsInfo)
{
  #ifdef ESP8266
  FSInfo fs_info;
  SPIFFS.info(fs_info);   
  const auto &total = fs_info.totalBytes;
  const auto &used = fs_info.usedBytes;
  #else //ESP32
  const auto &total = SPIFFS.totalBytes();
  const auto &used = SPIFFS.usedBytes();
  #endif
  fsInfo = String(F("SPIFFS: ")) + F("Total: ") + String(total) + F(" ") + F("Used: ") + String(used);  
}

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  { 
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 130) // Format FS and reset WiFi and restart
  { 
    INFO(F("\t\t\tFormat..."));   
    SPIFFS.format();
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 140)
  {
    LoadSettings();
    PMonitor::LoadSettings();
  }

  if(debugButtonFromSerial == 2) // Reset Settings
  {
    _settings.init();
    SaveSettings();
    api->setBaseUri(_settings.BaseUri);
  }  

  if(debugButtonFromSerial == 100) // Show Network Statistic
  {
    INFO(F(" HEAP: "), ESP.getFreeHeap());
    INFO(F("STACK: "), ESPgetFreeContStack);
    INFO(F(" BR: "), _settings.Brightness);
    INFO(F("Alarmed regions count: "), alarmedLedIdx.size());
    INFO(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus);  
    INFO(F("BOT Channels:"));
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      INFO(F("\t"), channel.first);
    }
    String fsInfo;
    PrintFSInfo(fsInfo);
    INFO(fsInfo);
    PrintNetworkStatToSerial();
  }

  if(debugButtonFromSerial > 2 && debugButtonFromSerial <= 31) // Blink Test
  {
    // ledsState[debugButtonFromSerial].Color = CRGB::Yellow;
    // ledsState[debugButtonFromSerial].StartBlink(50, 20000);
    LedState state;
    state.Color = CRGB::Yellow;    
    state.BlinkPeriod = 50;
    state.BlinkTotalTime = 5000;
    state.IsAlarmed = true;
    state.IsPartialAlarmed = false;
    SetRegionState((UARegion)debugButtonFromSerial, state);
  }  

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus);
  } 

  if(debugButtonFromSerial == 102)
  {
    digitalWrite(PIN_RELAY1, !digitalRead(PIN_RELAY1));
    INFO(F("Realy1"), F(": "), digitalRead(PIN_RELAY1));
  }

  if(debugButtonFromSerial == 103)
  {
    digitalWrite(PIN_RELAY2, !digitalRead(PIN_RELAY2));
    INFO(F("Realy2"), F(": "), digitalRead(PIN_RELAY2));
  }

  if(debugButtonFromSerial == 104)
  {
    BUZZ_INFO(F("AlarmStart"));
    Buzz::AlarmStart(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 105)
  {
    BUZZ_INFO(F("AlarmEnd"));
    Buzz::AlarmEnd(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 106)
  { 
    BUZZ_INFO(F("Siren"));
    Buzz::Siren(PIN_BUZZ, 5000);
  }

  #ifdef USE_BUZZER_MELODIES    
  if(debugButtonFromSerial == 107)
  { 
    BUZZ_INFO(F("Pitches"));
    Buzz::PlayMelody(PIN_BUZZ, pitchesMelodyStr);
    //BUZZ_TRACE(Buzz::GetMelodyString(Buzz::pitchesMelody));
  }

  if(debugButtonFromSerial == 108)
  {  
    BUZZ_INFO(F("Nokia"));
    Buzz::PlayMelody(PIN_BUZZ, nokiaMelodyStr); 
    //BUZZ_TRACE(Buzz::GetMelodyString(Buzz::nokiaMelody));  
  }

  if(debugButtonFromSerial == 109)
  {  
    BUZZ_INFO(F("UAAnthemMelody"));
    /*Buzz::PlayMelody(PIN_BUZZ, Buzz::UAAnthemMelody); 
    BUZZ_TRACE(Buzz::GetMelodyString(Buzz::UAAnthemMelody));*/
    BUZZ_TRACE(UAAnthem::play(PIN_BUZZ, 2));
  }

  if(debugButtonFromSerial == 110)
  {    
    BUZZ_INFO(F("Happy Birthday"));
    Buzz::PlayMelody(PIN_BUZZ, happyBirthdayMelodyStr); 
    //BUZZ_TRACE(Buzz::GetMelodyString(Buzz::happyBirthdayMelody));
  }

  if(debugButtonFromSerial == 111)
  {     
    BUZZ_INFO(F("Xmas"));
    Buzz::PlayMelody(PIN_BUZZ, xmasMelodyStr); 
    //BUZZ_TRACE(Buzz::GetMelodyString(Buzz::xmasMelody));
  }

  if(debugButtonFromSerial == 112)
  {  
    BUZZ_INFO(F("Star War"));    
    Buzz::PlayMelody(PIN_BUZZ, starWarMelodyStr);   
    //BUZZ_TRACE(Buzz::GetMelodyString(Buzz::starWarMelody));
  }
  #endif

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();
    INFO(F("Input: "), readFromSerial);

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
  INFO(F(" BR: "), _settings.Brightness);
  FastLED.setBrightness(_settings.Brightness);  
  RecalculateBrightness(); 
  FastLEDShow(true); 
  SaveSettings();
}

void FastLEDShow(const bool &showTrace)
{
  if(showTrace)
    INFO(F("FastLEDShow()"));
  FastLED.show();  
}

void FastLEDShow(const int &retryCount)
{
  for(int i = 0; i < retryCount; i++)
  {
    FastLED.show();   
    yield(); // watchdog
  }
}

void SetRegionColor(const UARegion &region, const CRGB &color)
{
  for(const auto &idx : getLedIndexesByRegion(region))
  {    
    ledsState[idx].Color = color;
    leds[idx] = color;
  }
}

void SetStateFromRealLeds()
{
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;
    if(led.Idx < 0 && led.Idx >= LED_COUNT) continue;

    led.Color = leds[led.Idx];
  }
}

void SetRegionState(const UARegion &region, LedState &state)
{
  for(const auto &idx : getLedIndexesByRegion(region))
  {
    state.Idx = idx;
    // state.IsAlarmed = ledsState[idx].IsAlarmed;
    // state.IsPartialAlarmed = ledsState[idx].IsPartialAlarmed;
    ledsState[idx] = state;
  }
}

void SetWhiteColorSchema()
{
  TRACE(F("Color Schema: "), F("White"));
  _settings.AlarmedColor = CRGB::Red;
  _settings.NotAlarmedColor = CRGB::White;
  _settings.PartialAlarmedColor = CRGB::Yellow;
}

void SetBlueDefaultColorSchema()
{
  TRACE(F("Color Schema: "), F("Blue"));
  _settings.AlarmedColor = LED_ALARMED_COLOR;
  _settings.NotAlarmedColor = LED_NOT_ALARMED_COLOR;
  _settings.PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;
}

void SetYellowColorSchema()
{
  TRACE(F("Color Schema: "), F("Yellow"));
  _settings.AlarmedColor = CRGB::Red;
  _settings.NotAlarmedColor = CRGB::Yellow;
  _settings.PartialAlarmedColor = CRGB::Orange;
}
