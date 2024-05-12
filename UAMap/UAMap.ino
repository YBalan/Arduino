//Estimates: https://docs.google.com/spreadsheets/d/1mYA_Bc687Y8no1yJDxv83fimtd0WU4nvcGI80_jnJME/edit?usp=sharing

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include "Config.h"

#ifdef ESP8266
  #define VER F("1.20")
#else //ESP32
  #define VER F("1.22")
#endif

//#define RELEASE
#define DEBUG

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define USE_BUZZER_MELODIES 

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_ALARMS
#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

#define ENABLE_INFO_PMONITOR
#define ENABLE_TRACE_PMONITOR

#define ENABLE_INFO_BUZZ
#define ENABLE_TRACE_BUZZ
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

#ifdef ESP8266 
  #define FASTLED_ESP8266_RAW_PIN_ORDER
  ///#define FASTLED_ALL_PINS_HARDWARE_SPI
  //#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
  //#define FASTLED_ESP8266_D1_PIN_ORDER  
#else //ESP32
  //#define FASTLED_ESP32_SPI_BUS HSPI
#endif
#include <FastLED.h>  

#include <map>
#include <set>
#include <ezButton.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "LedState.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBotHelper.h"
#include "BuzzHelper.h"
#include "UAAnthem.h"
#include "PMonitor.h"

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

std::unique_ptr<AlarmsApi> api(new AlarmsApi());
//CRGBArray<LED_COUNT> leds;
CRGB *leds = new CRGB[LED_COUNT];

typedef std::map<uint8_t, RegionInfo*> LedIndexMappedToRegionInfo;
LedIndexMappedToRegionInfo alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

ezButton resetBtn(PIN_RESET_BTN);

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
  //_settings.reset();

  PMonitor::LoadSettings();
  PMonitor::Init(PIN_PMONITOR_SDA, PIN_PMONITOR_SCL);

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);

  FastLED.clear();   
  fill_ua_prapor2();
  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);

  FastLEDShow(1000);
  
  WiFiOps::WiFiOps wifiOps(F("UAMap WiFi Manager"), F("UAMapAP"), F("password"));

  wifiOps
  .AddParameter(F("apiToken"), F("Alarms API Token"), F("api_token"), F("YOUR_ALARMS_API_TOKEN"), 47)  
  #ifdef USE_BOT
  .AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;    

  auto resetButtonState = resetBtn.getState();
  INFO(F("ResetBtn: "), resetButtonState);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  _settings.resetFlag = 200;
  api->setApiKey(wifiOps.GetParameterValueById(F("apiToken"))); 
  api->setBaseUri(_settings.BaseUri); 
  INFO(F("Base Uri: "), _settings.BaseUri);  
 
  //ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyivska*/, 1000, -1, false, false, false, _settings.NoConnectionColor};
  //ledsState[0] = {0 /*Crimea*/, 0, 0, true, false, false, _settings.AlarmedColor};
  //ledsState[4] = {4 /*Luh*/, 0, 0, true, false, false, _settings.AlarmedColor};
  
  SetAlarmedLED(alarmedLedIdx);
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
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("AP Started Call Back"));
  FastLED.clear(); 
  leds[LED_STATUS_IDX] = _settings.PortalModeColor;

  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);  
  FastLEDShow(1000);    
}

#ifdef USE_BOT

// int32_t menuID = 0;
// byte depth = 0;

/*
!!!!!!!!!!!!!!!! - Bot Additional Commands for Admin
register - Register chat
unregister - Unregister chat
unregisterall - Unregister all chat(s)
update - Current period of update in milliseconds
update10000 - Set period of update to 10secs
baseuri - Current alerts.api uri
relay1menu - Relay1 Menu to choose region
relay2menu - Relay2 Menu to choose region
token - Current Alerts.Api token
nstat - Network Statistic
rssi - WiFi Quality rssi db
test - test by regionId
ver - Version Info
changeconfig - change configuration WiFi, tokens...
chid - List of registered channels
adjpm - Adjust voltage 0.50-1.0
notify - Notify http code result

!!!!!!!!!!!!!!!! - Bot Commands for Users
gay - trolololo
ua - Ukraine Prapor
ua1 - Ukraine Parpor with Anthem
menu - Simple menu
br - Current brightness
br255 - Max brightness
br2 - Min brightness
br1 - Min brightness only alarmed visible
alarmed - List of currently alarmed regions
all - List of All regions
relay1 - Region Id set for Relay1
relay10 - Switch Off Relay1
relay2 - Region Id set for Relay2
relay20 - Switch Off Relay2
buzztime - Current buzzer time in milliseconds
buzztime3000 - Set buzzer time to 3secs
buzztime0 - Switch off buzzer
schema - Current Color schema
schema0 - Set Color schema to Dark
schema1 - Set Color schema to Light
strobe - Stroboscope with current Br & Schema
rainbow - Rainbow with current Br
play - Play tones 500,800
pm - Show Power monitor
*/

#define BOT_COMMAND_BR F("/br")
#define BOT_COMMAND_RESET F("/reset")
#define BOT_COMMAND_TEST F("/test")
#define BOT_COMMAND_VER F("/ver")
#define BOT_COMMAND_CHANGE_CONFIG F("/changeconfig")
#define BOT_COMMAND_RAINBOW F("/rainbow")
#define BOT_COMMAND_STROBE F("/strobe")
#define BOT_COMMAND_SCHEMA F("/schema")
#define BOT_COMMAND_BASEURI F("/baseuri")
#define BOT_COMMAND_ALARMED F("/alarmed")
#define BOT_COMMAND_ALL F("/all")
#define BOT_COMMAND_MENU F("/menu")
#define BOT_COMMAND_RELAY1 F("/relay1")
#define BOT_COMMAND_RELAY2 F("/relay2")
#define BOT_COMMAND_UPDATE F("/update")
#define BOT_COMMAND_BUZZTIME F("/buzztime")
#define BOT_COMMAND_TOKEN F("/token")
#define BOT_COMMAND_NSTAT F("/nstat")
#define BOT_COMMAND_RSSI F("/rssi")
#define BOT_COMMAND_GAY F("/gay")
#define BOT_COMMAND_UA F("/ua")
#define BOT_COMMAND_CHID F("/chid")
#define BOT_COMMAND_FRMW_UPDATE F("frmwupdate")
#define BOT_COMMAND_PLAY F("/play")
#define BOT_COMMAND_SAVEMELODY F("/savemelody")

//Fast Menu
#define BOT_MENU_UA_PRAPOR F("UA Prapor")
#define BOT_MENU_ALARMED F("Alarmed")
#define BOT_MENU_ALL F("All")
#define BOT_MENU_MIN_BR F("Min Br")
#define BOT_MENU_MID_BR F("Mid Br")
#define BOT_MENU_MAX_BR F("Max Br")
#define BOT_MENU_NIGHT_BR F("Night Br")

#ifdef USE_POWER_MONITOR
#define BOT_COMMAND_PM F("/pm")
#define BOT_COMMAND_PMALARM F("/alarmpm")
#define BOT_COMMAND_PMUPDATE F("/powerupdate")
#define BOT_COMMAND_PMADJ F("/adjpm")
struct PMChatInfo { int32_t MsgID = -1; float AlarmValue = 0.0; float CurrentValue = 0.0; };
static std::map<String, PMChatInfo> pmChatIds; // = new (std::map<String, PMChatInfo>());
static uint32_t pmUpdatePeriod = PM_UPDATE_PERIOD;
static uint32_t pmUpdateTicks = 0;
#endif

#ifdef USE_NOTIFY
#define BOT_COMMANDS_NOTIFY F("/notify")
String notifyChatId;
#endif

const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered, const bool &isGroup)
{  
  std::vector<String> messages;

  if(msg.OTA && msg.text == BOT_COMMAND_FRMW_UPDATE)
  { 
    INFO(F("Update check..."));
    String fileName = msg.fileName;
    fileName.replace(F(".bin"), F(""));
    fileName.replace(F(".gz"), F(""));
    auto uidx = fileName.lastIndexOf(F("_"));
    bool isEsp32Frmw = fileName.lastIndexOf(F("esp32")) >= 0;

    if(uidx >= 0 && uidx < fileName.length() - 1)
    {    
      if((IsESP32 && isEsp32Frmw) || (!IsESP32 && !isEsp32Frmw))
      {
        bot->OTAVersion = fileName.substring(uidx + 1);
        String currentVersion = String(VER);
        if(bot->OTAVersion.toFloat() > currentVersion.toFloat())
        {
          messages.push_back(String(F("Updates OK")) + F(": ") + currentVersion + F(" -> ") + bot->OTAVersion);
          INFO(messages[0]);
          bot->update();
        }
        else
        {        
          messages.push_back(bot->OTAVersion + F(" <= ") + currentVersion + F(". NO Updates..."));        
          bot->OTAVersion.clear();   
          INFO(messages[0]);     
        }
      }
      else
      {
        messages.push_back(String(F("Wrong firmware")) + F(". NO Updates..."));
        INFO(messages[0]);
      }
    }
    else
    {      
      messages.push_back(String(F("Unknown version")) + F(". NO Updates..."));
      INFO(messages[0]);
    }    
    return std::move(messages);
  }
  
  String value;
  bool answerCurrentAlarms = false;
  bool answerAll = false;

  bool noAnswerIfFromMenu = msg.data.length() > 0;// && filtered.startsWith(_botSettings.botNameForMenu);
  BOT_TRACE(F("Filtered: "), filtered);
  filtered = noAnswerIfFromMenu ? msg.data : filtered;
  BOT_TRACE(F("Filtered: "), filtered);

  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    

    #ifdef USE_BOT_INLINE_MENU
      BOT_INFO(F("Inline Menu"));
      #ifdef USE_BUZZER
      static const String BotInlineMenu = F("Alarmed \t All \n Min Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2 \n Buzzer Off \t Buzzer 3sec");
      static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu, /buzztime0, /buzztime3000");
      #else
      static const String BotInlineMenu = F("Alarmed \t All \n Mix Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2");
      static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu");
      #endif

      ESPresetHeap;
      ESPresetFreeContStack;

      BOT_INFO(F(" HEAP: "), ESP.getFreeHeap());
      BOT_INFO(F("STACK: "), ESPgetFreeContStack);
      
      bot->inlineMenuCallback(_botSettings.botNameForMenu, BotInlineMenu, BotInlineMenuCall, msg.chatID);    
    #endif
    
    #ifdef USE_BOT_FAST_MENU    
      BOT_INFO(F("Fast Menu"));        
      static const String BotFastMenu = String(BOT_MENU_UA_PRAPOR)    
        + F(" \n ") + BOT_MENU_MIN_BR + F(" \t ") + BOT_MENU_MID_BR + F(" \t ") + BOT_MENU_MAX_BR + F(" \t ") + BOT_MENU_NIGHT_BR
        + F(" \n ") + BOT_MENU_ALARMED + F(" \t ") + BOT_MENU_ALL
      ;     
      bot->showMenuText(_botSettings.botNameForMenu, BotFastMenu, msg.chatID, true);
    #endif  
    
  } else
  #ifdef USE_NOTIFY
  if(GetCommandValue(BOT_COMMANDS_NOTIFY, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    notifyChatId = msg.chatID;
    if(value.length() > 0)
    {
      _settings.notifyHttpCode = value.toInt();      
      SaveSettings();
    }
    value = String(F("NotifyHttpCode: ")) + String(_settings.notifyHttpCode);
    TRACE(value);
  }else
  #endif
  #ifdef USE_POWER_MONITOR
  if(GetCommandValue(BOT_COMMAND_PM, filtered, value))
  { 
    bot->sendTyping(msg.chatID);

    pmUpdatePeriod = pmUpdatePeriod == 0 ? PM_UPDATE_PERIOD : pmUpdatePeriod;
    pmUpdateTicks = pmUpdatePeriod == 0 ? 0 : millis();

    PM_TRACE(F("PM Update period: "), pmUpdatePeriod);
    
    const float &voltage = PMonitor::GetVoltage();
    pmChatIds[msg.chatID].CurrentValue = voltage;

    const String &menu = GetPMMenu(voltage, msg.chatID);
    const String &call = GetPMMenuCall(voltage, msg.chatID);
    PM_TRACE(F("\t"), menu, F(" -> "), msg.chatID);

    bot->inlineMenuCallback(_botSettings.botNameForMenu + PM_MENU_NAME, menu, call, msg.chatID);
    pmChatIds[msg.chatID].MsgID = bot->lastBotMsg(); 
      
  } else
  if(GetCommandValue(BOT_COMMAND_PMALARM, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    
    if(value.length() > 0)
    {
      auto &chatIdInfo = pmChatIds[msg.chatID];
      chatIdInfo.AlarmValue = value.toFloat();      
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue);
      value = String(F("PM Alarm set: <= ")) + String(chatIdInfo.AlarmValue, 2) + PM_MENU_VOLTAGE_UNIT;
      PM_TRACE(value);
      noAnswerIfFromMenu = true;
    }    
  } else    
  if(GetCommandValue(BOT_COMMAND_PMADJ, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    
    if(value.length() > 0)
    {
      auto &chatIdInfo = pmChatIds[msg.chatID];

      PMonitor::AdjCalibration(value.toFloat());

      const float &voltage = PMonitor::GetVoltage();
      chatIdInfo.CurrentValue = voltage;
      
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue);
      
      PMonitor::SaveSettings();      
    }    
    const auto &adjValue = PMonitor::GetCalibration();
    value = String(F("PM Adjust set: ")) + String(adjValue, 3);
    PM_TRACE(value);    
  } else    
  if(GetCommandValue(BOT_COMMAND_PMUPDATE, filtered, value))
  { 
    bot->sendTyping(msg.chatID);      
    auto &chatIdInfo = pmChatIds[msg.chatID];
    auto newValue = value.toInt();
    if(newValue == 0 || (newValue >= PM_MIN_UPDATE_PERIOD && newValue <= PM_UPDATE_PERIOD))
    {
      pmUpdatePeriod = newValue;      
      pmUpdateTicks = pmUpdatePeriod == 0 ? 0 : millis();
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue);
      value = String(F("PM Update set: ")) + String(pmUpdatePeriod) + F("ms...");
      PM_TRACE(value);
      noAnswerIfFromMenu = true;
    }

    if(pmUpdatePeriod == 0)
    {
      for(const auto &chatIDkv : pmChatIds)
      {
        const auto &chatID = chatIDkv.first;
        const auto &chatIDInfo = chatIDkv.second;        
        SetPMMenu(chatID, chatIDInfo.MsgID, chatIdInfo.CurrentValue);
      }
    }
    
    PM_TRACE(F("PM Update period: "), pmUpdatePeriod);
  } else  
  #endif
  #ifdef USE_BOT_FAST_MENU 
  if(GetCommandValue(BOT_MENU_ALARMED, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    
    answerCurrentAlarms = true;
  } else
  if(GetCommandValue(BOT_MENU_ALL, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    
    answerAll = true;
  } else
  if(GetCommandValue(BOT_MENU_MIN_BR, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    _settings.Brightness = 2;
    SetBrightness();    
    value = String(F("Brightness: ")) + String(_settings.Brightness);
  } else
  if(GetCommandValue(BOT_MENU_MID_BR, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    _settings.Brightness = 30;
    SetBrightness();    
    value = String(F("Brightness: ")) + String(_settings.Brightness);
  } else  
  if(GetCommandValue(BOT_MENU_MAX_BR, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    _settings.Brightness = 255;
    SetBrightness();    
    value = String(F("Brightness: ")) + String(_settings.Brightness);
  } else
  if(GetCommandValue(BOT_MENU_NIGHT_BR, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    _settings.Brightness = 1;
    SetBrightness();    
    value = String(F("Brightness: ")) + String(_settings.Brightness);
  } else
  #endif
  if(GetCommandValue(BOT_COMMAND_UPDATE, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    if(value.length() > 0)
    {
      auto update = value.toInt();
      if(update >= 2000 && update <= 120000 )
      {
        _settings.alarmsUpdateTimeout = update;
        SaveSettings();
      }
    }
    value = String(F("Update Timeout: ")) + String(_settings.alarmsUpdateTimeout) + F("ms...");
  } else
  if(GetCommandValue(BOT_COMMAND_BUZZTIME, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    if(value.length() > 0)
    {
      auto update = value.toInt();
      if(update == 0 || (update >= 2000 && update <= 120000))
      {
        _settings.BuzzTime = update;
        SaveSettings();
      }
    }
    value = String(F("Buzz Time: ")) + String(_settings.BuzzTime) + F("ms...");
    noAnswerIfFromMenu = false;
  } else
  if(GetCommandValue(BOT_COMMAND_BR, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto br = value.toInt();
    if(value.length() > 0)
    {
      br = br <= 0 ? 1 : (br > 255 ? 255 : br);
      //if(br > 0 && br <= 255)
      _settings.Brightness = br;

      SetBrightness();      
    }
    value = String(F("Brightness: ")) + String(_settings.Brightness);
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_RESET, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    ESP.restart();
  }else
  if(GetCommandValue(BOT_COMMAND_CHANGE_CONFIG, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    _settings.resetFlag = 1985;
    SaveSettings();
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    ESP.restart();
  }else
  if(GetCommandValue(BOT_COMMAND_TEST, filtered, value))
  {
    bot->sendTyping(msg.chatID);    

    auto regionId = value.toInt();
    String answer = String(F(" Region: ")) + String(regionId);
    if(value.length() > 0 && regionId > 2 && regionId < 31)
    {      
      LedState state;
      state.Color = CRGB::Yellow;    
      state.BlinkPeriod = 50;
      state.BlinkTotalTime = 5000;
      state.IsAlarmed = true;
      state.IsPartialAlarmed = false;
      SetRegionState((UARegion)regionId, state);

      RegionInfo *const regionPtr = api->GetRegionById((UARegion)regionId);
      if(regionPtr != nullptr)
      {
        regionPtr->AlarmStatus = ApiAlarmStatus::Alarmed;
        alarmedLedIdx[(UARegion)regionId] = regionPtr;
        answer += F(" Test started...");
        SetRelayStatus(alarmedLedIdx);
      }      
    } 
    value = answer; 

  }else
  if(GetCommandValue(BOT_COMMAND_VER, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    value = String(F("Flash Date: ")) + String(__DATE__) + F(" ") + String(__TIME__) + F(" ") + F("V:") + VER;
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY1, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelay(F("Relay1"), F("/relay1"), value, _settings.Relay1Region, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelay(F("Relay2"), F("/relay2"), value, _settings.Relay2Region, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_TOKEN, filtered, value))
  {
    value = String(F("Token: ")) + api->GetApiKey();
  }else
  if(GetCommandValue(BOT_COMMAND_NSTAT, filtered, value))
  {
    PrintNetworkStatistic(value);
  }else
  if(GetCommandValue(BOT_COMMAND_RSSI, filtered, value))
  {
    value = String(F("SSID: ")) + WiFi.SSID() + F(" ") /*+ F("EncryptionType: ") + String(WiFi.encryptionType()) + F(" ")*/ 
          + F("RSSI: ") + String(WiFi.RSSI()) + F("db") + F(" ")
          + F("Channel: ") + WiFi.channel() + F(" ")
          + F("IP: ") + WiFi.localIP().toString() + F(" ")
          + F("GatewayIP: ") + WiFi.gatewayIP().toString() + F(" ")
          + F("MAC: ") + WiFi.macAddress()          
    ;
  }else
  if(GetCommandValue(BOT_COMMAND_CHID, filtered, value))
  {
    INFO(F("BOT Channels:"));
    value.clear();
    value = String(F("BOT Channels:")) + F(" ") + String(_botSettings.toStore.registeredChannelIDs.size()) + F(" -> ");
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      INFO(F("\t"), channel.first);
      value += String(F("[")) + channel.first + F("]") + F("; ");
    }
  }else
  if(GetCommandValue(BOT_COMMAND_RAINBOW, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    //value = F("Rainbow started...");
    value.clear();
    _effect = Effect::Rainbow;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_STROBE, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("Strobe started...");
    value.clear();
    _effect = Effect::Strobe;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_GAY, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("Gay started...");
    value.clear();
    _effect = Effect::Gay;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_UA, filtered, value) || GetCommandValue(BOT_MENU_UA_PRAPOR, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("UA started...");    
    value.clear();
    _effect = value.toInt() > 0 ? Effect::UAWithAnthem : Effect::UA;   
    effectStartTicks = millis();
    effectStarted = false;          
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_BASEURI, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    if(value.startsWith(F("https://")) && value.length() < MAX_BASE_URI_LENGTH)
    {
      strcpy(_settings.BaseUri, value.c_str());
      api->setBaseUri(value);
      SaveSettings();
    }
    value = _settings.BaseUri;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_ALARMED, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    answerCurrentAlarms = true;
    answerAll = false;
  }else
  if(GetCommandValue(BOT_COMMAND_ALL, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    answerCurrentAlarms = true;
    answerAll = true;
  }else
  if(GetCommandValue(BOT_COMMAND_SCHEMA, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    
    uint8_t schema = value.toInt();
    switch(schema)
    {
      case ColorSchema::Light:
        _settings.AlarmedColor = CRGB::Red;
        _settings.NotAlarmedColor = CRGB::White;
        _settings.PartialAlarmedColor = CRGB::Yellow;
        value = String(F("Light"));
      break;
      case ColorSchema::Dark: 
      default:
        _settings.AlarmedColor = LED_ALARMED_COLOR;
        _settings.NotAlarmedColor = LED_NOT_ALARMED_COLOR;
        _settings.PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;
        value = String(F("Dark"));
      break;
    }

    SetAlarmedLED(alarmedLedIdx);
    SetBrightness();

    value = String(F("Color Schema: ")) + value;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_PLAY, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto melodySizeMs = Buzz::PlayMelody(PIN_BUZZ, value);
    value = String(melodySizeMs) + F("ms...");
  }

  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);

  if(answerCurrentAlarms || answerAll)
  {
    if(answerCurrentAlarms)
    {
      String answerAlarmed = String(F("Alarmed regions count: ")) + String(alarmedLedIdx.size());     
      messages.push_back(answerAlarmed);
    }

    if(answerAll)  
    {  
      String answerAllMsg = String(F("All regions count: ")) + String(MAX_REGIONS_COUNT);
      messages.push_back(answerAllMsg);
    }    
    
    for(const auto &region : api->iotApiRegions)
    {      
      if(region.AlarmStatus == ApiAlarmStatus::Alarmed || answerAll)
      {
        if(USE_BOT_ONE_MSG_ANSWER)
        {
          messages[0] += String(F("\n")) + region.Name + F(": ") + F("[") + String((uint8_t)region.Id) + F("]");
        }
        else
        {
          String regionMsg = region.Name + F(": ") + F("[") + String((uint8_t)region.Id) + F("]") + F(": ") 
                            // + F("[") 
                            // + (region.AlarmStatus == ApiAlarmStatus::Alarmed ? F("A") : (region.AlarmStatus == ApiAlarmStatus::PartialAlarmed ? F("P") : F("N"))) 
                            // + F("]")
                            ;
          messages.push_back(regionMsg);
        }
      }
    } 
  }

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

const bool HandleRelay(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const String& chatID)
{
  if(value == F("menu"))
    {
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESPgetFreeContStack); 

      ESPresetHeap;
      ESPresetFreeContStack;

      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESPgetFreeContStack);

      SendInlineRelayMenu(relayName, relayCommand, chatID);   

      value.clear();   
      return false;   
    }
    else
    {
      if(value.length() > 0)
      {
        auto regionId = value.toInt();    
        if(regionId == 0 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
        {
          relaySetting = regionId;
          SaveSettings();
        }
      }
      value = relayName + F(": ") + (relaySetting == 0 ? F("Off") : api->GetRegionNameById((UARegion)relaySetting));
      BOT_TRACE(F("Bot answer: "), value);
      return true;
    }
}

/*String BotRelayMenu1("Odeska \n Kharkivska \t Mykolaivska \t Vinnytska \t Kyivska \n Kirovohradska \t Poltavska \t Sumska \t Ternopilska");
String BotRelayMenuCall1("{0} 18, {0} 22, {0} 17, {0} 4, {0} 14, {0} 15, {0} 19, {0} 20, {0} 21");*/
void SendInlineRelayMenu(const String &relayName, const String &relayCommand, const String& chatID)
{
  // String call1 = BotRelayMenuCall1;
  // call1.replace("{0}", relayCommand);
  // bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, BotRelayMenu1, call1, chatID);

  String menu;
  String call;
  
  bool sendWholeMenu = IsESP32;
  
  static const uint8_t RegionsInLine = 2;
  static const uint8_t RegionsInGroup = 6;

  uint8_t regionsCount = MAX_REGIONS_COUNT;
  uint8_t regionPlace = 1;
  bool isEndOfGoup = false;
  for(uint8_t regionIdx = 0; regionIdx < regionsCount; regionIdx++)
  {
    const auto &region = api->iotApiRegions[regionIdx];
    if(region.Id == UARegion::Kyiv || region.Id == UARegion::Sevastopol) continue;

    menu += region.Name + (regionPlace != 0 && regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t "));//(isEndOfGoup ? F("") : (regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t ")));
    call += relayCommand + region.Id + F(", ");//(isEndOfGoup ? F("") : F(", "));  

    regionPlace++;
    isEndOfGoup = (regionPlace == RegionsInGroup || regionIdx == regionsCount - 1);

    if(!sendWholeMenu && isEndOfGoup)      
    {
      menu += String(F(" \n ")) + relayName + F(" ") + F("Off");
      call += String(F(", ")) + relayCommand + F("0");

      regionPlace = 1;      

      ESPresetHeap;
      ESPresetFreeContStack;

      BOT_INFO(F(" HEAP: "), ESP.getFreeHeap());
      BOT_INFO(F("STACK: "), ESPgetFreeContStack);  

      BOT_INFO(menu);
      BOT_INFO(call);    

      bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);

      delay(100);

      menu.clear();
      call.clear();
    }
  }

  if(sendWholeMenu)
  {
    menu += String(F(" \n ")) + relayName + F(" ") + F("Off");
    call += String(F(", ")) + relayCommand + F("0");

    INFO(menu);
    INFO(call);   

    bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);
  }
}
#endif

void loop() 
{
  static bool firstRun = true;
  static uint32_t currentTicks = millis();
  currentTicks = millis();

  resetBtn.loop();

  if(resetBtn.isPressed())
  {
    INFO(BUTTON_IS_PRESSED_MSG, F(" BR: "), _settings.Brightness);
  }
  if(resetBtn.isReleased())
  {    
    static bool brBtnChangeDirectionUp = false;
    auto nextBr = _settings.Brightness + (brBtnChangeDirectionUp ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP);
    if(nextBr < 0)
    {
      nextBr = 1;
      brBtnChangeDirectionUp = true;
    }else
    if(nextBr > 255)
    {
      nextBr = 255;
      brBtnChangeDirectionUp = false;
    }

    _settings.Brightness = nextBr;

    INFO(BUTTON_IS_RELEASED_MSG, F(" BR: "), _settings.Brightness);
    SetBrightness();    
  }  

  HandleEffects(currentTicks);
 
  int httpCode;
  String statusMsg;
  if(_effect == Effect::Normal)
  { 
    if(CheckAndUpdateAlarms(currentTicks, httpCode, statusMsg))
    {
      //when updated
      //FastLEDShow(true);
      #ifdef USE_BOT
        #ifdef USE_NOTIFY
          if(_settings.notifyHttpCode != 0 
            &&(
                  (_settings.notifyHttpCode == -200 && httpCode != 200)
                || (_settings.notifyHttpCode == httpCode)
              )
           )
          {
            String notifyMessage = String(F("Notification: ")) + statusMsg + F(" ") + String(httpCode);
            TRACE(notifyMessage);
            bot->sendMessage(notifyMessage, notifyChatId);
          }
        #endif
      #endif
    }

    if(CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/false))
    {
      //FastLEDShow(false);
    }        
  }  
  
  FastLEDShow(false); 

  HandleDebugSerialCommands();     

  #ifdef USE_BOT
  uint8_t botStatus = bot->tick();  
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
      const auto &voltage = PMonitor::GetVoltage();      

      for(auto &chatIDkv : pmChatIds)
      {
        const auto &chatID = chatIDkv.first;
        auto &chatIDInfo = chatIDkv.second;
        chatIDInfo.CurrentValue = voltage;
        SetPMMenu(chatID, chatIDInfo.MsgID, voltage);

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
  #endif
  #endif

  firstRun = false;
}

#ifdef USE_POWER_MONITOR
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage)
{
  const String &menu = GetPMMenu(voltage, chatId);
  const String &call = GetPMMenuCall(voltage, chatId);

  PM_TRACE(F("\t"), menu, F(" -> "), chatId);
  
  bot->editMenuCallback(msgId, menu, call, chatId);
}

const String GetPMMenu(const float &voltage, const String &chatId)
{
  const auto &chatIdInfo = pmChatIds[chatId];
  const auto periodStr = String(pmUpdatePeriod / 1000) + F("s.");
  String voltageMenu = String(voltage, 2) + PM_MENU_VOLTAGE_UNIT 
    + F(" \n ") + F("Set ") + PM_MENU_ALARM_NAME + F(" <= ") + String(voltage - PM_MENU_ALARM_DECREMENT, 2) + PM_MENU_VOLTAGE_UNIT
    + (chatIdInfo.AlarmValue > 0 ? String(F(" \t ")) + PM_MENU_ALARM_NAME + F(" <= ") + String(chatIdInfo.AlarmValue, 2) : String(F("")))
    + (chatIdInfo.AlarmValue > 0 ? String(F(" \n ")) + PM_MENU_ALARM_NAME + F(" ") + F("Off") : String(F("")))
    + F(" \n ") + (pmUpdatePeriod == 0 ? String(F("Stoped")) : (pmUpdatePeriod > PM_MIN_UPDATE_PERIOD ? String(F("Timeout: ")) + periodStr : String(F("Stop")) + F(" ") + periodStr))     
  ;
  
  return std::move(voltageMenu);
}

const String GetPMMenuCall(const float &voltage, const String &chatId)
{  
  const auto &chatIdInfo = pmChatIds[chatId];
  const auto newpmPeriod = pmUpdatePeriod / 2;
  String call = String(BOT_COMMAND_PM) 
        + F(",") + BOT_COMMAND_PMALARM + String(voltage - PM_MENU_ALARM_DECREMENT, 2)  
        + (chatIdInfo.AlarmValue > 0 ? String(F(",")) + BOT_COMMAND_PMALARM : String(F(""))) //Fake
        + (chatIdInfo.AlarmValue > 0 ? String(F(",")) + BOT_COMMAND_PMALARM + F("0") : String(F(""))) 
        + F(",") + BOT_COMMAND_PMUPDATE + String(newpmPeriod < PM_MIN_UPDATE_PERIOD ? 0 : newpmPeriod)
    ;
  return std::move(call);
}
#endif

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

            SetAlarmedLED(alarmedLedIdx);            
            changed = true;
          } 

          if(httpCode == ApiStatusCode::JSON_ERROR)
          {
            _settings.alarmsCheckWithoutStatus = true;
            ESPresetHeap;
            ESPresetFreeContStack;
          }         
        } 

        SetRelayStatus(alarmedLedIdx);           
      }                       
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

void SetAlarmedLED(LedIndexMappedToRegionInfo &alarmedLedIdx)
{
  TRACE(F("SetAlarmedLED: "), alarmedLedIdx.size());
  FastLED.setBrightness(_settings.Brightness);
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = ledsState[ledIdx];
    led.Idx = ledIdx;
    if(alarmedLedIdx.count(ledIdx) > 0)
    {
      if(!led.IsAlarmed)
      {
        led.Color = led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor;
        led.BlinkPeriod = LED_NEW_ALARMED_PERIOD;
        led.BlinkTotalTime = LED_NEW_ALARMED_TOTALTIME;
      }       
      led.IsAlarmed = true;  
      led.IsPartialAlarmed = alarmedLedIdx[ledIdx]->AlarmStatus == ApiAlarmStatus::PartialAlarmed;
    }
    else
    {
      led.IsAlarmed = false;
      led.IsPartialAlarmed = false;
      led.Color = _settings.NotAlarmedColor;      
      led.StopBlink();
    }       

    RecalculateBrightness(led, false);    
  }  
}

void SetRelayStatus(const LedIndexMappedToRegionInfo &alarmedLedIdx)
{
  INFO(F("SetRelayStatus: "), F("Relay1"), F(": "), _settings.Relay1Region, F(" "), F("Relay2"), F(": "), _settings.Relay2Region); 
  if(_settings.Relay1Region == 0 && _settings.Relay2Region == 0) return;
  
  bool found1 = false;
  bool found2 = false;  

  for(const auto &idx : alarmedLedIdx)
  {
    TRACE(F("Led idx: "), idx.first, F(" Region: "), idx.second->Id);
  
    if(!found1 && idx.second->Id == (UARegion)_settings.Relay1Region)
    {
      TRACE(F("Found1: "), idx.second->Id, F(" Name: "), idx.second->Name);
      found1 = true;        
    }      
    if(!found2 && idx.second->Id == (UARegion)_settings.Relay2Region)
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
    TRACE(F("Relay1"), F(": "), F("ON"), F(" Region: "), _settings.Relay1Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY1) == RELAY_ON)
    { 
      digitalWrite(PIN_RELAY1, RELAY_OFF);
      Buzz::AlarmEnd(PIN_BUZZ, _settings.BuzzTime);
      TRACE(F("Relay1"), F(": "), F("Off"), F(" Region: "), _settings.Relay1Region);
    }
  }

  if(found2)
  {
    digitalWrite(PIN_RELAY2, RELAY_ON);
    TRACE(F("Relay2"), F(": "), F("ON"), F(" Region: "), _settings.Relay2Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY2) == RELAY_ON)
    {      
      digitalWrite(PIN_RELAY2, RELAY_OFF);
      TRACE(F("Relay2"), F(": "), F("Off"), F(" Region: "), _settings.Relay2Region);
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
  if(networkStat.count(ApiStatusCode::API_OK) > 0)
    PrintNetworkStatInfoToSerial(networkStat[ApiStatusCode::API_OK]);
  for(const auto &de : networkStat)
  {
    const auto &info = de.second;
    if(info.code != ApiStatusCode::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
}

void PrintNetworkStatInfo(const NetworkStatInfo &info, String &str)
{  
  str += String(F("[\"")) + info.description + F("\": ") + String(info.count) + F("]; ");
}

void PrintNetworkStatistic(String &str)
{
  str = F("Network Statistic: ");
  #ifdef NETWORK_STATISTIC  
  if(networkStat.count(ApiStatusCode::API_OK) > 0)
    PrintNetworkStatInfo(networkStat[ApiStatusCode::API_OK], str);
  for(const auto &de : networkStat)
  {
    const auto &info = de.second;
    if(info.code != ApiStatusCode::API_OK)
      PrintNetworkStatInfo(info, str);
  }
  //str.replace("(", "");
  //str.replace(")", "");
  #else
  str += F("Off");
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
  }  else
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
  }
  else
  if(_effect == Effect::UAWithAnthem)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_ua_prapor2();
      SetStateFromRealLeds();

      #ifdef USE_BUZZER
      FastLEDShow(500);
      UAAnthem::play(PIN_BUZZ, 0);
      #endif
      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }
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

void fill_ua_prapor2()
{ 
  SetRegionColor(UARegion::Crimea,            CRGB::Yellow);
  SetRegionColor(UARegion::Khersonska,        CRGB::Yellow);
  SetRegionColor(UARegion::Zaporizka,         CRGB::Yellow);
  SetRegionColor(UARegion::Donetska,          CRGB::Yellow);
  SetRegionColor(UARegion::Dnipropetrovska,   CRGB::Yellow);
  SetRegionColor(UARegion::Mykolaivska,       CRGB::Yellow);
  SetRegionColor(UARegion::Odeska,            CRGB::Yellow);
  SetRegionColor(UARegion::Kirovohradska,     CRGB::Yellow);
  SetRegionColor(UARegion::Vinnytska,         CRGB::Yellow);
  SetRegionColor(UARegion::Khmelnitska,       CRGB::Yellow);
  SetRegionColor(UARegion::Chernivetska,      CRGB::Yellow);
  SetRegionColor(UARegion::Ivano_Frankivska,  CRGB::Yellow);
  SetRegionColor(UARegion::Ternopilska,       CRGB::Yellow);
  SetRegionColor(UARegion::Lvivska,           CRGB::Yellow);
  SetRegionColor(UARegion::Zakarpatska,       CRGB::Yellow);


  SetRegionColor(UARegion::Luhanska,          CRGB::Blue);
  SetRegionColor(UARegion::Kharkivska,        CRGB::Blue);
  SetRegionColor(UARegion::Poltavska,         CRGB::Blue);
  SetRegionColor(UARegion::Sumska,            CRGB::Blue);
  SetRegionColor(UARegion::Chernihivska,      CRGB::Blue);
  SetRegionColor(UARegion::Kyivska,           CRGB::Blue);
  SetRegionColor(UARegion::Cherkaska,         CRGB::Blue);
  SetRegionColor(UARegion::Zhytomyrska,       CRGB::Blue);
  SetRegionColor(UARegion::Rivnenska,         CRGB::Blue);
  SetRegionColor(UARegion::Volynska,          CRGB::Blue);

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