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

void PrintNetworkStatistic(String &str, const int& codeFilter);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);


const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered, const bool &isGroup)
{  
  std::vector<String> messages;    
  
  String value;
  bool answerCurrentAlarms = false;
  bool answerAll = false;

  bool noAnswerIfFromMenu = msg.data.length() > 0;// && filtered.startsWith(_botSettings.botNameForMenu);
  BOT_MENU_TRACE(F("Filtered: "), filtered);
  filtered = noAnswerIfFromMenu ? msg.data : filtered;
  BOT_MENU_TRACE(F("Filtered: "), filtered);

  
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

  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);  

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
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