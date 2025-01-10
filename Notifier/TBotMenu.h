#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H
//#include <math.h>
#include "DEBUGHelper.h"
#include "Settings.h"
#include "BuzzHelper.h"

#define SUBSCRIBED_CHATS_FILE_NAME F("/subscribedChats")

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

#define BOT_COMMAND_RESTART       F("/restart")
#define BOT_COMMAND_TEST          F("/test")
#define BOT_COMMAND_VER           F("/ver")
#define BOT_COMMAND_CHANGE_CONFIG F("/changeconfig")
#define BOT_COMMAND_MENU          F("/menu")
#define BOT_COMMAND_UPDATE        F("/update")
#define BOT_COMMAND_TOKEN         F("/token")
#define BOT_COMMAND_NSTAT         F("/nstat")
#define BOT_COMMAND_RSSI          F("/rssi")
#define BOT_COMMAND_CHID          F("/chid")
#define BOT_COMMAND_FS            F("/fs")
#define BOT_COMMAND_SYNC          F("/sync")

//Fast Menu

//Notify
#ifdef USE_NOTIFY
#define BOT_COMMAND_NOTIFY        F("/notify")
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM
#define BOT_COMMAND_SUBSCRIBE     F("/subscribe")
#define BOT_COMMAND_UNSUBSCRIBE   F("/unsubscribe")
#define BOT_COMMAND_MESSAGE       F("/msg")
#define BOT_COMMAND_BUZZ          F("/buzz")
#define BOT_COMMAND_ONLINE        F("/online")
#define BOT_COMMAND_STATUS        F("/status")

// From .ino file
const uint32_t SyncTime();
void PrintNetworkStatistic(String &str, const int& codeFilter);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);
void SendCommand(const String &command);
const String getStatus();

struct PMChatInfo { int32_t msgId = 0; };
static std::map<String, PMChatInfo> pmChatIds;

void loadSubscribedChats(const String &fileName);
void saveSubscribedChats(const String &fileName);

const String getMessage(const String &status);
void sendToAllSubscribers(const String &status);
void sendToSubscriber(const String msg, const String &chatId, const int32_t &msgId);

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
    _settings.resetFlag = RESET_WIFI_FLAG;
    SaveSettings();
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    ESP.restart();
  }else  
  if(GetCommandValue(BOT_COMMAND_VER, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    value = String(F("Flash Date: ")) + String(__DATE__) + F(" ") + String(__TIME__) + F(" ") + F("V:") + VER + VER_POSTFIX;
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
    BOT_MENU_INFO(F("BOT "), F("Channels:"));
    value.clear();
    value = String(F("BOT Channels:")) + F(" ") + String(_botSettings.toStore.registeredChannelIDs.size()) + F(" -> ");
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      BOT_MENU_INFO(F("\t"), channel.first);
      value += String(F("[")) + channel.first + F("]") + F("; ");
    }
  } else
  if(GetCommandValue(BOT_COMMAND_SYNC, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Sync:"));

    bot->sendTyping(msg.chatID);
    const int &intValue = value.toInt();

    if(value.length() > 0 && intValue >= -12 && intValue <= 14)
    {
      _settings.timeZone = intValue;
      SaveSettings();    
    }
    const auto &now = SyncTime();    
    value = String(F("Synced time: ")) + epochToDateTime(now);
  }
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  else
  if(GetCommandValue(BOT_COMMAND_SUBSCRIBE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Subscribe:"));

    bot->sendTyping(msg.chatID);

    #ifdef DEBUG
    loadSubscribedChats(SUBSCRIBED_CHATS_FILE_NAME);
    #endif       

    if(value.startsWith(F("rs"))){
      value.clear();
      for (const auto& [key, chatInfo] : pmChatIds) {
        BOT_MENU_TRACE(F("ChatId: "), key);
        BOT_MENU_TRACE(F("\t"), F("MessageId: "), chatInfo.msgId);    
        value += key + F("; ");
      }      
    }else{
      auto &chatInfo = pmChatIds[msg.chatID]; 

      saveSubscribedChats(SUBSCRIBED_CHATS_FILE_NAME);
      sendToSubscriber(getMessage(getStatus()), msg.chatID, chatInfo.msgId);
    }

    /*if(msgId != chatInfo.msgId){
      chatInfo.msgId = 1;      
    } */   
  }else
  if(GetCommandValue(BOT_COMMAND_UNSUBSCRIBE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("UnSubscribe:"));

    bot->sendTyping(msg.chatID); 

    if(value.startsWith(F("all"))){
      value = String(pmChatIds.size()) + F(" ") + F("chatId") + F(" ") + F("cleared");
      pmChatIds.clear();
      saveSubscribedChats(SUBSCRIBED_CHATS_FILE_NAME);
    }else{
      value = String(msg.chatID) + F(" ") + F("chatId") + F(" ") + F("erased");
      pmChatIds.erase(msg.chatID);      
    }       
    
    saveSubscribedChats(SUBSCRIBED_CHATS_FILE_NAME);
        
  }else
  if(GetCommandValue(BOT_COMMAND_MESSAGE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Message:"));

    bot->sendTyping(msg.chatID);    

    if(value.isEmpty()){    
      value = _settingsExt.Message;
    }else{      
      if(!_settingsExt.setMessage(value)){
        value = String(F("Message: ")) + F("length exceeds maximum!");
      }

      SaveSettingsExt();        
    }
  }else
  if(GetCommandValue(BOT_COMMAND_BUZZ, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Buzz:"));

    bot->sendTyping(msg.chatID);    

    if(value.isEmpty()){    
      value = _settingsExt.Buzz;
    }else{    
      if(value.startsWith(F("def"))){
        value = BUZZER_DEFAULT;
      }

      if(!_settingsExt.setBuzz(value)){
        value = String(F("Buzz: ")) + F("length exceeds maximum!");        
      }

      Buzz::PlayMelody(PIN_BUZZER, _settingsExt.Buzz);
      SaveSettingsExt();        
    }    
  }else
  if(GetCommandValue(BOT_COMMAND_ONLINE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Online:"));

    bot->sendTyping(msg.chatID);       
    
    _settings.notifyOnline = value.toInt() > 0;

    value = String(F("Notify Online: ")) + (_settings.notifyOnline ? F("On") : F("Off"));

    SaveSettings();        
  }else
  if(GetCommandValue(BOT_COMMAND_STATUS, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("Status:"));

    bot->sendTyping(msg.chatID);       

    value = String(F("Status: ")) + String(getStatus());    
  }  


  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);  

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

const String getMessage(const String &status){
  return std::move(String(_settingsExt.Message) + F(" ") + status);
}

void sendToAllSubscribers(const String &status){  
  const auto &msg = getMessage(status);
  for(const auto &[key, value] : pmChatIds){
    sendToSubscriber(msg, key, value.msgId);    
  }  
}

void sendToSubscriber(const String msg, const String &chatId, const int32_t &msgId){  
      bot->replyMessage(msg, msgId < 0 ? 0 : msgId, chatId);
}    

void saveSubscribedChats(const String &fileName){
  BOT_MENU_TRACE(F("saveSubscribedChats"));
  File file = MFS.open(fileName.c_str(), FILE_WRITE);
  CommonHelper::saveMap(file, pmChatIds);  
  file.close();

  #ifdef DEBUG
  file = MFS.open(fileName.c_str(), FILE_READ);  
  BOT_MENU_INFO(file.readString());
  file.close();
  #endif
}

void loadSubscribedChats(const String &fileName){
  BOT_MENU_TRACE(F("loadSubscribedChats"));  

  #ifdef DEBUG
  auto &map = pmChatIds;
  {    
    File file = MFS.open(fileName.c_str(), FILE_READ); 
    file.seek(0);
    BOT_MENU_INFO(file.readString());
    file.close();
  }
  #endif

  File file = MFS.open(fileName.c_str(), FILE_READ);
  CommonHelper::loadMap(file, pmChatIds);  
  file.close();

  #ifdef DEBUG
  BOT_MENU_TRACE(F("loadSubscribedChats"), F(" "), F("size: "), map.size());
  for (const auto& [key, value] : map) {
    BOT_MENU_TRACE(F("ChatId: "), key);
    BOT_MENU_TRACE(F("\t"), F("MessageId: "), value.msgId);    
  }
  #endif
}

#endif //USE_BOT

#endif //TELEGRAM_BOT_MENU_H