#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.0")
#else //ESP32
  #define VER F("1.0")
#endif

#define AVOID_FLICKERING

//#define RELEASE
#define DEBUG

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY

#define USE_BUZZER_MELODIES 

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

#define ENABLE_INFO_PMONITOR
#define ENABLE_TRACE_PMONITOR

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

#include "Config.h"

#include <map>
#include <set>
//#include <ezButton.h>

#include "DEBUGHelper.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBotHelper.h"
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

Button resetBtn(PIN_RESET_BTN);

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);   

  resetBtn.setDebounceTime(DebounceTime);  

  Serial.println();
  Serial.println(F("!!!! Start Charger !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(VER);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  LoadSettings();
  LoadSettingsExt();  
  //_settings.reset();

  WiFiOps::WiFiOps wifiOps(F("Charger WiFi Manager"), F("ChargerAP"), F("password"));

  #ifdef WM_DEBUG_LEVEL
    INFO(F("WM_DEBUG_LEVEL: "), WM_DEBUG_LEVEL);    
  #else
    INFO(F("WM_DEBUG_LEVEL: "), F("Off"));
  #endif

  wifiOps  
  #ifdef USE_BOT
  .AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;    

  auto resetButtonState = resetBtn.getState();
  INFO(F("ResetBtn: "), resetButtonState == HIGH ? F("Off") : F("On"));
  INFO(F("ResetFlag: "), _settings.resetFlag);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  if(_settings.resetFlag == 1985)
  {
    _settings.resetFlag = 200;
    SaveSettings();
  }

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
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("Config Portal Started: "), manager->getConfigPortalSSID());
  
}

void loop()
{

  resetBtn.loop();
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

}

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
    if(networkStat.count(200) > 0)
      PrintNetworkStatInfoToSerial(networkStat[200]);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != 200)
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
    if(networkStat.count(200) > 0 && (codeFilter == 0 || codeFilter == 200))
      PrintNetworkStatInfo(networkStat[200], str);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != 200 && (codeFilter == 0 || codeFilter == info.code))
        PrintNetworkStatInfo(info, str);
    }
  }
  const auto &millisec = millis();
  str += (networkStat.size() > 0 ? String(F(" ")) : String(F("")))
      + (millisec >= 60000 ? String(millisec / 60000) + String(F("min")) : String(millisec / 1000) + String(F("sec")));
      
  //str.replace("(", "");
  //str.replace(")", "");
  #else
  str += F("Off");
  #endif
}


