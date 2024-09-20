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
*/

#define BOT_COMMAND_RESTART F("/restart")
#define BOT_COMMAND_TEST F("/test")
#define BOT_COMMAND_VER F("/ver")
#define BOT_COMMAND_CHANGE_CONFIG F("/changeconfig")
#define BOT_COMMAND_MENU F("/menu")
#define BOT_COMMAND_UPDATE F("/update")
#define BOT_COMMAND_TOKEN F("/token")
#define BOT_COMMAND_NSTAT F("/nstat")
#define BOT_COMMAND_RSSI F("/rssi")
#define BOT_COMMAND_CHID F("/chid")
#define BOT_COMMAND_FS F("/fs")

//Fast Menu

//Notify
#ifdef USE_NOTIFY
#define BOT_COMMAND_NOTIFY F("/notify")
#endif

void PrintNetworkStatistic(String &str, const int& codeFilter);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);
void SendCommand(const String &command);


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

    
    
  } else
  #ifdef USE_NOTIFY
  if(GetCommandValue(BOT_COMMAND_NOTIFY, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    _settings.setNotifyChatId(msg.chatID);
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
  if(GetCommandValue(BOT_COMMAND_RESTART, filtered, value))
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



#endif //USE_BOT

#endif //TELEGRAM_BOT_MENU_H