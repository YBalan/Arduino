#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H
//#include <math.h>
#include "DEBUGHelper.h"
#include "Settings.h"

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
nstat - Network Statistic
rssi - WiFi Quality rssi db
ver - Version Info
fs - File System Info
changeconfig - change configuration WiFi, tokens...
chid - List of registered channels
notify - Notify saved http code result
notify0 - Notifications Off
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
#define BOT_COMMAND_FILLRGB F("/fillrgb")
#define BOT_COMMAND_PALETTE F("/palette")

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
#endif

#ifdef USE_LEARN
#define BOT_COMMAND_LEARN F("/learn")
#endif 

#ifdef USE_RELAY_EXT
static int32_t relay1MenuMessageId = 0;
static int32_t relay2MenuMessageId = 0;
#endif 

void SetBrightness();
void SetAlarmedLED();
const int GetAlarmedLedIdxSize();
void SetAlarmedLedRegionInfo(const int &regionId, RegionInfo *const regionPtr);
void SetRegionState(const UARegion &region, LedState &state);
void SetRelayStatus();
void PrintNetworkStatistic(String &str, const int& codeFilter);
#ifdef USE_RELAY_EXT
const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const uint8_t &relayNumber, int32_t &relayMenuMessageId, const String& chatID);
#else
const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const uint8_t &relayNumber, const String& chatID);
#endif
void SendInlineRelayMenu(const String &relayName, const String &relayCommand, const uint8_t &relayNumber, const String& chatID, const int32_t &messageId, const bool &learn = false);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);
void SetWhiteColorSchema();
void SetBlueDefaultColorSchema();
void SetYellowColorSchema();

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
    if(value.length() > 0)
    {
      const auto &newMode = value.toInt(); 
      if(newMode >= 0 && newMode < (uint8_t)ExtMode::MAX)  
      { 
        _settingsExt.Mode = (ExtMode)newMode;
        SaveSettingsExt();
      }
    }
    value = String(F("Mode: ")) + GetExtModeStr(_settingsExt.Mode);    
    effectStarted = false;
    _effect = Effect::Normal;
  }else
  if(GetCommandValue(BOT_COMMAND_MODESOUVENIR, filtered, value))
  {
    bot->sendTyping(msg.chatID);     
    if(value.length() > 0)
    {
      const auto &newMode = value.toInt();
      if(newMode >= 0 && newMode < (uint8_t)ExtSouvenirMode::MAX)  
      { 
        _settingsExt.Mode = ExtMode::Souvenir;
        _settingsExt.SouvenirMode = (ExtSouvenirMode)newMode;
        SaveSettingsExt();
      }      
    }    
    value = String(F("Souvenir mode: ")) + (_settingsExt.Mode == ExtMode::Souvenir ? GetExtSouvenirModeStr(_settingsExt.SouvenirMode) : String(F("Off")));    
    effectStarted = false;
    _effect = Effect::Normal;
  }else
  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    

    #ifdef USE_BOT_INLINE_MENU
      #ifdef ESP8266    
        BOT_MENU_INFO(F("Inline Menu"));
        #ifdef USE_BUZZER
        static const String BotInlineMenu = F("Alarmed \t All \n Min Br \t Mid Br \t Max Br \n Blue \t Yellow \t White \n Strobe \t Rainbow \n Relay 1 \t Relay 2 \n Buzzer Off \t Buzzer 3sec");
        static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema2, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu, /buzztime0, /buzztime3000");
        #else
        static const String BotInlineMenu = F("Alarmed \t All \n Mix Br \t Mid Br \t Max Br \n Blue \t Yellow \t White \n Strobe \t Rainbow \n Relay 1 \t Relay 2");
        static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema2, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu");
        #endif
      #else //ESP32
        BOT_MENU_INFO(F("Inline Menu"));
        static const String BotInlineMenu = F("Alarmed \t All \n Min Br \t Mid Br \t Max Br \n Blue \t Yellow \t White \n Strobe \t Rainbow \n Relay 1 \t Relay 2 \n Buzzer Off \t Buzzer 3sec \n Alarms Mode \n Alarms (Only Custom) Mode \n Souvenir UA Mode");
        static const String BotInlineMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema2, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu, /buzztime0, /buzztime3000, /modealarms0, /modealarms2, /modesouvenir0");
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
    _settingsExt.setNotifyChatId(msg.chatID);
    if(value.length() > 0)
    {
      bool negate = value.startsWith(F("not"));
      if(negate) value.replace(F("not"), F(""));
      int newValue = negate ? -value.toInt() : value.toInt();
      _settings.notifyHttpCode = newValue;
      SaveSettings();
      SaveSettingsExt();
    }
    value = String(F("NotifyHttpCode: ")) + String(_settings.notifyHttpCode) + F(" ChatId: ") + msg.chatID;
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
    BOT_MENU_TRACE(F("\t"), menu, F(" ChatId: "), msg.chatID);

    bot->inlineMenuCallback(_botSettings.botNameForMenu + PM_MENU_NAME, menu, call, msg.chatID);
    pmChatIds[msg.chatID].MsgID = bot->lastBotMsg(); 
      
  } else
  if(GetCommandValue(BOT_COMMAND_PMALARM, filtered, value))
  { 
    bot->sendTyping(msg.chatID);    
    auto &chatIdInfo = pmChatIds[msg.chatID];
    if(value.length() > 0)
    {      
      chatIdInfo.AlarmValue = value.toFloat();      
      SetPMMenu(msg.chatID, chatIdInfo.MsgID, chatIdInfo.CurrentValue);      
      //noAnswerIfFromMenu = true;
    }    
    value = String(F("PM Alarm set: <= ")) + String(chatIdInfo.AlarmValue, 2) + PM_MENU_VOLTAGE_UNIT;
    BOT_MENU_TRACE(value);
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
    #ifndef USE_RELAY_EXT
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay1"), F("/relay1"), value, _settings.Relay1Region, 1, msg.chatID);
    #else
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay1"), F("/relay1"), value, _settings.Relay1Region, 1, relay1MenuMessageId, msg.chatID);
    #endif
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    #ifndef USE_RELAY_EXT
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay2"), F("/relay2"), value, _settings.Relay2Region, 2, msg.chatID);
    #else
    noAnswerIfFromMenu = !HandleRelayMenu(F("Relay2"), F("/relay2"), value, _settings.Relay2Region, 2, relay2MenuMessageId, msg.chatID);
    #endif
  }else
  if(GetCommandValue(BOT_COMMAND_TOKEN, filtered, value))
  {
    value = String(F("Token: ")) + api->GetApiKey();
  }else
  if(GetCommandValue(BOT_COMMAND_NSTAT, filtered, value))
  {    
    PrintNetworkStatistic(value, value.toInt());
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
    
    if(value.length() > 0)
    {
      const uint8_t &schema = value.toInt();
      switch(schema)
      {
        case ColorSchema::White:
          SetWhiteColorSchema();
          //value = String(F("White"));
        break;
        case ColorSchema::Yellow:
          SetYellowColorSchema();
          //value = String(F("Yellow"));
        break;
        case ColorSchema::Blue: 
        default:
          SetBlueDefaultColorSchema();
          //value = String(F("Blue"));
        break;
      }
    }
    SetAlarmedLED();
    SetBrightness();

    value = String(F("Color Schema: ")) + String(_settings.NotAlarmedColor.red) + F(".") + String(_settings.NotAlarmedColor.green) + F(".") + String(_settings.NotAlarmedColor.blue);        
  }else
  if(GetCommandValue(BOT_COMMAND_PLAY, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto melodySizeMs = Buzz::PlayMelody(PIN_BUZZ, value);
    value = String(melodySizeMs) + F("ms...");
  }else
  if(GetCommandValue(BOT_COMMAND_FILLRGB, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    const auto &tokens = CommonHelper::splitToInt(value, ',', '_');    
    if(tokens.size() >= 3)
    {
      _settings.NotAlarmedColor = CRGB(tokens[0], tokens[1], tokens[2]);
      _effect = Effect::FillRGB;
      effectStartTicks = millis();
      effectStarted = false;
      SaveSettings();
      BOT_MENU_TRACE(F("RGB: "), tokens[0], F(" "), tokens[1], F(" "), tokens[2]);
    }
    else
    {
      //_settings.NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
      value = String(F("Wrong RGB: ")) + value;
      BOT_MENU_TRACE(value);
    }
  }
  else
  if(GetCommandValue(BOT_COMMAND_PALETTE, filtered, value))
  {    
    //#ifdef ESP32
    bot->sendTyping(msg.chatID);
    static const String PaletteInlineMenu = F("🔴 \t 🟠 \t 🟡 \t 🟢 \t 🔵 \t 🟣 \t ⚪️");
    static const String PaletteInlineMenuCall = F("/fillrgb255_0_0, /fillrgb255_153_51 , /fillrgb255_255_50 , /fillrgb0_255_0 , /fillrgb0_0_255, /fillrgb153_51_255 , /fillrgb255_255_255");     
    
    bot->inlineMenuCallback(_botSettings.botNameForMenu + F("Palette"), PaletteInlineMenu, PaletteInlineMenuCall, msg.chatID);  
    value.clear();  
    //#endif
  }
  #ifdef USE_LEARN
  else if(GetCommandValue(BOT_COMMAND_LEARN, filtered, value))
  {
    SendInlineRelayMenu(F("Learn"), BOT_COMMAND_TEST, 1, msg.chatID, /*messageId:*/0, /*learn:*/true); 
    //relay1MenuMessageId = bot->lastBotMsg();
    value.clear();
  }
  #endif

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

static const String GetRegionNameById(const uint8_t &id)
{
  return api->GetRegionNameById(id);
}

static const String GetRegionTestCommandById(const uint8_t &id)
{
  return String(F(" (")) + BOT_COMMAND_TEST + String(id) + F(")");
}

#ifdef USE_RELAY_EXT
const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const uint8_t &relayNumber, int32_t &relayMenuMessageId, const String& chatID)
#else
const bool HandleRelayMenu(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const uint8_t &relayNumber, const String& chatID)
#endif
{
  if(value == F("menu"))
  {
    BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
    BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack); 

    ESPresetHeap;
    ESPresetFreeContStack;

    BOT_MENU_INFO(F(" HEAP: "), ESP.getFreeHeap());
    BOT_MENU_INFO(F("STACK: "), ESPgetFreeContStack);

    SendInlineRelayMenu(relayName, relayCommand, relayNumber, chatID, /*messageId:*/0);  

    #ifdef USE_RELAY_EXT    
    relayMenuMessageId = bot->lastBotMsg();
    #endif 

    value.clear();   
    return false;   
  }
  else
  { 
    if(value.length() > 0 && isDigit(value.charAt(0)))
    {
      const auto &regionId = value.toInt();    
      if(regionId == 0 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
      {
        relaySetting = regionId;
        relayNumber == 1 
          ? SetRelay1(regionId, /*removeIfExist:*/true) 
          : SetRelay2(regionId, /*removeIfExist:*/true);
        
        SaveSettings();
        SaveSettingsRelayExt();
      }
    }
    #ifndef USE_RELAY_EXT
      value = relayName + F(": ") + (relaySetting == 0 ? String(F("Off")) : GetRegionNameById(relaySetting) + GetRegionTestCommandById(relaySetting));
    #else  
      bool answerInChat = value.length() == 0 || value == F("test");
      if(answerInChat)
      {  
        relayMenuMessageId = 0;
        const auto &regionsStr = (relayNumber == 1 ? GetRelay1Str(GetRegionNameById) : GetRelay2Str(GetRegionNameById));
        const auto &testStr = (relayNumber == 1 ? GetRelay1Str(GetRegionTestCommandById) : GetRelay2Str(GetRegionTestCommandById));        
        value = relayName + F(": ") + F("\n") + (relaySetting == 0 ? String(F("Off")) : (regionsStr + F("\n") + testStr));        
      }
      else
      {
        BOT_MENU_TRACE(F("Menu messageId: "), relayMenuMessageId);
        SendInlineRelayMenu(relayName, relayCommand, relayNumber, chatID, relayMenuMessageId);
        relayMenuMessageId = bot->lastBotMsg();
      }

      BOT_MENU_TRACE(value);   
      BOT_MENU_TRACE(GetRelay1Str(nullptr));
      BOT_MENU_TRACE(GetRelay2Str(nullptr));
      return answerInChat;
    #endif
    BOT_MENU_TRACE(value);       
    return true;
  }
}

void SendInlineRelayMenu(const String &relayName, const String &relayCommand, const uint8_t &relayNumber, const String& chatID, const int32_t &messageId, const bool &learn)
{
  #ifdef USE_RELAY_EXT
  static const String checkSymbol = "✅";
  #else
  static const String checkSymbol = "[v]";
  #endif

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

    String addCheck = learn ? String(F("")) : ((relayNumber == 1 ? IsRelay1Contains(region.Id) : IsRelay2Contains(region.Id)) ? checkSymbol : F(""));
    menu += addCheck + region.Name + (regionPlace != 0 && regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t "));//(isEndOfGoup ? F("") : (regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t ")));
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

    #ifdef USE_RELAY_EXT
    BOT_MENU_TRACE(F("Menu messageId: "), messageId);
    if(messageId > 0)
    {        
      bot->editMenuCallback(messageId, menu, call, chatID);
    }
    else
    #endif
    {
      bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);
    }
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
  #ifdef SHOW_PM_TIME
    + String(F(" (")) + bot->getTime(3).timeString() + String(F(")"))
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