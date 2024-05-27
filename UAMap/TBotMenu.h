#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H
//#include <math.h>
#include "DEBUGHelper.h"
#include "Settings.h"
#include "LedState.h"
#include "AlarmsApi.h"

#ifdef USE_BOT

#ifdef ENABLE_INFO_BOT_MENU
#define BOT_MENU_INFO(...) SS_TRACE(F("[BOT MENU INFO] "), __VA_ARGS__)
#else
#define BOT_MENU_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_BOT_MENU
#define BOT_MENU_TRACE(...) SS_TRACE(F("[BOT MENU TRACE] "), __VA_ARGS__)
#else
#define BOT_MENU_TRACE(...) {}
#endif

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
fs - File System Info
modealarms - Alarms Mode
modesouvenir - Souvenir Mode
modesouvenir0 - Souvenir Mode UA Prapor
modesouvenir2 - Souvenir Mode BG Prapor
modesouvenir3 - Souvenir Mode MD Prapor
changeconfig - change configuration WiFi, tokens...
chid - List of registered channels
notify - Notify saved http code result
notify1 - Notify All http code result
notifynot200 - Notify all http code exclude OK(200)
notify429 - Notify To-many Requests (429)

!!!!!!!!!!!!!!!! - Bot Commands for Power Monitor
pm - Show Power monitor
powerupdate - Power Monitor update period in milliseconds
alarmpm - Alarm - send message when voltage <= value
adjpm - Adjust voltage 0.50-1.0

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
#define BOT_COMMAND_MODEALARMS F("/modealarms")
#define BOT_COMMAND_MODESOUVENIR F("/modesouvenir")
#define BOT_COMMAND_FS F("/fs")

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
const float GetLEDVoltageFactor();
#endif

#ifdef USE_NOTIFY
#define BOT_COMMAND_NOTIFY F("/notify")
String notifyChatId;
#endif

void SetBrightness();
void SetAlarmedLED();
const int GetAlarmedLedIdxSize();
void SetAlarmedLedRegionInfo(const int &regionId, RegionInfo *const regionPtr);
void SetRegionState(const UARegion &region, LedState &state);
void SetRelayStatus();
void PrintNetworkStatistic(String &str);
void SendInlineRelayMenu(const String &relayName, const String &relayCommand, const String& chatID);
const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const String& chatID);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);

const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered, const bool &isGroup)
{  
  std::vector<String> messages;

  if(msg.OTA && msg.text == BOT_COMMAND_FRMW_UPDATE)
  { 
    BOT_MENU_INFO(F("Update check..."));
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
          BOT_MENU_INFO(messages[0]);
          bot->update();
        }
        else
        {        
          messages.push_back(bot->OTAVersion + F(" <= ") + currentVersion + F(". NO Updates..."));        
          bot->OTAVersion.clear();   
          BOT_MENU_INFO(messages[0]);     
        }
      }
      else
      {
        messages.push_back(String(F("Wrong firmware")) + F(". NO Updates..."));
        BOT_MENU_INFO(messages[0]);
      }
    }
    else
    {      
      messages.push_back(String(F("Unknown version")) + F(". NO Updates..."));
      BOT_MENU_INFO(messages[0]);
    }    
    return std::move(messages);
  }
  
  String value;
  bool answerCurrentAlarms = false;
  bool answerAll = false;

  bool noAnswerIfFromMenu = msg.data.length() > 0;// && filtered.startsWith(_botSettings.botNameForMenu);
  BOT_MENU_TRACE(F("Filtered: "), filtered);
  filtered = noAnswerIfFromMenu ? msg.data : filtered;
  BOT_MENU_TRACE(F("Filtered: "), filtered);

  if(GetCommandValue(BOT_COMMAND_MODEALARMS, filtered, value))
  {
    bot->sendTyping(msg.chatID); 
    _settingsExt.Mode = ExtMode::Alarms;
    value = String(F("Ext mode: ")) + String(_settingsExt.Mode);
    SaveSettingsExt();
    effectStarted = false;
    _effect = Effect::Normal;
  }else
  if(GetCommandValue(BOT_COMMAND_MODESOUVENIR, filtered, value))
  {
    bot->sendTyping(msg.chatID); 
    //if(value.length() > 0)
    {
      const auto &newMode = value.toInt();
      switch (newMode)
      {
        case 2:
          _settingsExt.SouvenirMode = ExtSouvenirMode::BGPrapor;
          break;
        case 3:
          _settingsExt.SouvenirMode = ExtSouvenirMode::MDPrapor;
          break;
        case 0:
        default:
          _settingsExt.SouvenirMode = ExtSouvenirMode::UAPrapor;
          break;
      }
    }
    _settingsExt.Mode = ExtMode::Souvenir;
    value = String(F("Souvenir mode: ")) + String(_settingsExt.SouvenirMode);
    SaveSettingsExt();
    effectStarted = false;
    _effect = Effect::Normal;
  }else
  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    

    #ifdef USE_BOT_INLINE_MENU
      BOT_MENU_INFO(F("Inline Menu"));
      #ifdef USE_BUZZER
      static const String BotInlineMenu = F("Alarmed \t All \n Min Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2 \n Buzzer Off \t Buzzer 3sec");
      static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu, /buzztime0, /buzztime3000");
      #else
      static const String BotInlineMenu = F("Alarmed \t All \n Mix Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2");
      static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu");
      #endif

      ESPresetHeap;
      ESPresetFreeContStack;

      BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
      BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack);
      
      bot->inlineMenuCallback(_botSettings.botNameForMenu, BotInlineMenu, BotInlineMenuCall, msg.chatID);    
    #endif
    
    #ifdef USE_BOT_FAST_MENU    
      BOT_MENU_INFO(F("Fast Menu"));        
      static const String BotFastMenu = String(BOT_MENU_UA_PRAPOR)    
        + F(" \n ") + BOT_MENU_MIN_BR + F(" \t ") + BOT_MENU_MID_BR + F(" \t ") + BOT_MENU_MAX_BR + F(" \t ") + BOT_MENU_NIGHT_BR
        + F(" \n ") + BOT_MENU_ALARMED + F(" \t ") + BOT_MENU_ALL
      ;     
      bot->showMenuText(_botSettings.botNameForMenu, BotFastMenu, msg.chatID, true);
    #endif  
    
  } else
  #ifdef USE_NOTIFY
  if(GetCommandValue(BOT_COMMAND_NOTIFY, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    notifyChatId = msg.chatID;
    if(value.length() > 0)
    {
      bool negate = value.startsWith(F("not"));
      if(negate) value.replace(F("not"), F(""));
      int newValue = negate ? -value.toInt() : value.toInt();
      _settings.notifyHttpCode = newValue;
      SaveSettings();
    }
    value = String(F("NotifyHttpCode: ")) + String(_settings.notifyHttpCode);
    BOT_MENU_TRACE(value);
  }else
  #endif
  #ifdef USE_POWER_MONITOR
  if(GetCommandValue(BOT_COMMAND_PM, filtered, value))
  { 
    bot->sendTyping(msg.chatID);

    pmUpdatePeriod = pmUpdatePeriod == 0 ? PM_UPDATE_PERIOD : pmUpdatePeriod;
    pmUpdateTicks = pmUpdatePeriod == 0 ? 0 : millis();

    BOT_MENU_TRACE(F("PM Update period: "), pmUpdatePeriod);
    
    const float &led_consumption_voltage_factor = GetLEDVoltageFactor();
    const float &voltage = PMonitor::GetVoltage(led_consumption_voltage_factor);
    pmChatIds[msg.chatID].CurrentValue = voltage;

    const String &menu = GetPMMenu(voltage, msg.chatID, led_consumption_voltage_factor);
    const String &call = GetPMMenuCall(voltage, msg.chatID);
    BOT_MENU_TRACE(F("\t"), menu, F(" -> "), msg.chatID);

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
      BOT_MENU_TRACE(value);
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

      const float &led_consumption_voltage_factor = GetLEDVoltageFactor();
      const float &voltage = PMonitor::GetVoltage(led_consumption_voltage_factor);
      chatIdInfo.CurrentValue = voltage;
      
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue, led_consumption_voltage_factor);
      
      PMonitor::SaveSettings();      
    }    
    const auto &adjValue = PMonitor::GetCalibration();
    value = String(F("PM Adjust set: ")) + String(adjValue, 3);
    BOT_MENU_TRACE(value);    
  } else    
  if(GetCommandValue(BOT_COMMAND_PMUPDATE, filtered, value))
  { 
    bot->sendTyping(msg.chatID);      
    auto &chatIdInfo = pmChatIds[msg.chatID];
    auto newValue = value.toInt();
    value.clear();
    if(newValue == 0 || (newValue >= PM_MIN_UPDATE_PERIOD && newValue <= PM_UPDATE_PERIOD))
    {
      pmUpdatePeriod = newValue;      
      pmUpdateTicks = pmUpdatePeriod == 0 ? 0 : millis();
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue);
      value = String(F("PM Update set: ")) + String(pmUpdatePeriod) + F("ms...");
      BOT_MENU_TRACE(value);
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
    
    BOT_MENU_TRACE(F("PM Update period: "), pmUpdatePeriod);
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
        //alarmedLedIdx[(UARegion)regionId] = regionPtr;
        SetAlarmedLedRegionInfo(regionId, regionPtr);
        answer += F(" Test started...");
        SetRelayStatus();
      }      
    } 
    value = answer; 

  }else
  if(GetCommandValue(BOT_COMMAND_VER, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    value = String(F("Flash Date: ")) + String(__DATE__) + F(" ") + String(__TIME__) + F(" ") + F("V:") + VER;
  }else
  if(GetCommandValue(BOT_COMMAND_FS, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    PrintFSInfo(value);
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY1, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay1"), F("/relay1"), value, _settings.Relay1Region, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay2"), F("/relay2"), value, _settings.Relay2Region, msg.chatID);
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
    BOT_MENU_INFO(F("BOT Channels:"));
    value.clear();
    value = String(F("BOT Channels:")) + F(" ") + String(_botSettings.toStore.registeredChannelIDs.size()) + F(" -> ");
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      BOT_MENU_INFO(F("\t"), channel.first);
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
    const auto &prapor = value.toInt();
    //_effect = value.toInt() > 0 ? Effect::UAWithAnthem : Effect::UA; 
    switch (prapor)
    {
      case 3:
        _effect = Effect::MD;
      break;
      case 2:
        _effect = Effect::BG;
      break;
      case 1:
        _effect = Effect::UAWithAnthem;
        break;
      case 0:
      default:
        _effect = Effect::UA;
      break;
    }  
    value.clear();
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

    SetAlarmedLED();
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
      String answerAlarmed = String(F("Alarmed regions count: ")) + String(GetAlarmedLedIdxSize());     
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

const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const String& chatID)
{
  if(value == F("menu"))
  {
    BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
    BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack); 

    ESPresetHeap;
    ESPresetFreeContStack;

    BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
    BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack);

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
    value = relayName + F(": ") + (relaySetting == 0 ? F("Off") : api->GetRegionNameById((UARegion)relaySetting)) + F(" (") + BOT_COMMAND_TEST + String(relaySetting) + F(")");
    BOT_MENU_TRACE(F("Bot answer: "), value);
    return true;
  }
}

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

      BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
      BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack);  

      BOT_MENU_INFO(menu);
      BOT_MENU_INFO(call);    

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

    BOT_MENU_INFO(menu);
    BOT_MENU_INFO(call);   

    bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);
  }
}

#ifdef USE_POWER_MONITOR
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float &led_consumption_voltage_factor)
{
  const String &menu = GetPMMenu(voltage, chatId, led_consumption_voltage_factor);
  const String &call = GetPMMenuCall(voltage, chatId);

  BOT_MENU_TRACE(F("\t"), menu, F(" -> "), chatId);
  
  bot->editMenuCallback(msgId, menu, call, chatId);
}

const String GetPMMenu(const float &voltage, const String &chatId, const float &led_consumption_voltage_factor)
{
  const auto &chatIdInfo = pmChatIds[chatId];
  const auto periodStr = String(pmUpdatePeriod / 1000) + F("s.");
  String voltageMainMenu = String(voltage, 2) + PM_MENU_VOLTAGE_UNIT
  #ifdef SHOW_PM_FACTOR
    + (led_consumption_voltage_factor > 0.0 && !isnan(led_consumption_voltage_factor) ? String(F(" (")) + String(led_consumption_voltage_factor, 3) + F(")") : String(F("")))
  #endif
  ;

  String voltageMenu = voltageMainMenu 
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

#endif

#endif //TELEGRAM_BOT_MENU_H