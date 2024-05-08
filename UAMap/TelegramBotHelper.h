#pragma once
#ifndef TELEGRAM_BOT_HELPER_H
#define TELEGRAM_BOT_HELPER_H

//#include <memory>
//#include <map>
#include "DEBUGHelper.h"

#ifdef ENABLE_INFO_BOT
#define BOT_INFO(...) SS_TRACE("[BOT INFO] ", __VA_ARGS__)
#else
#define BOT_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_BOT
#define BOT_TRACE(...) SS_TRACE("[BOT TRACE] ", __VA_ARGS__)
#else
#define BOT_TRACE(...) {}
#endif

#define BOT_MENU_NAME F("Menu")
#define BOT_CONNECTION_ISSUES_MSG F(" faced with connection issues")

#include "TelegramBot.h"
#include "CommonHelper.h"

#ifdef USE_BOT
std::unique_ptr<TelegramBot> bot(new TelegramBot());
#else
std::unique_ptr<TelegramBot> bot;
#endif

//https://stackoverflow.com/questions/72594564/how-can-i-add-menu-button-in-telegram-bot
struct BotSettings
{
  String botNameForMenu;
  String botName;
  String botSecure;

  void SetBotName(const String &value)
  {
    botName = value;
    botNameForMenu = value;
    botNameForMenu.replace("@", "");
    botNameForMenu.replace("_Bot", "");
    botNameForMenu.replace("_bot", "");
    botNameForMenu.replace("_", "");
    botNameForMenu += BOT_MENU_NAME;
  }

  struct ToStore
  {
    std::map<String, uint8_t> registeredChannelIDs;
  } toStore;
} _botSettings;

extern const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered, const bool &isGroup);
extern void SaveChannelIDs();

const bool GetCommandValue(const String &command, const String &filteredMsg, String &value)
{
  int idx = -1;
  if((idx = filteredMsg.indexOf(command)) >= 0)
  {
    idx = idx + command.length();
    value = filteredMsg.substring(idx, filteredMsg.length());
    value.trim();    
    BOT_TRACE("\tCommand: '", command, "' Value: '", value, "' valIdx: ", idx);
    return true;
  }
  return false;
}

#define REGISTRATION_MSG F("Replay on this with secret")
void HangleBotMessages(FB_msg& msg) 
{  
  BOT_TRACE(F("MessageID: "), msg.messageID);
  BOT_TRACE(F("ChatID: "), msg.chatID);
  BOT_TRACE(F("UserID: "), msg.userID);
  BOT_TRACE(F("IsBot: "), msg.isBot ? F("true") : F("false"))
  BOT_TRACE(F("UserName: "), msg.username);   
  BOT_TRACE(F("LastName: "), msg.last_name);  
  BOT_TRACE(F("ReplayText: "), msg.replyText);  
  BOT_TRACE(F("Query: "), msg.query); 
  BOT_TRACE(F("Data: "), msg.data); 
  BOT_TRACE(F("OTA: "), msg.OTA); 
  BOT_TRACE(F("FileName: "), msg.fileName);  

  BOT_INFO(F("MESSAGE: "), msg.text);

  auto botNameIdx = -1;
  bool isGroup = msg.chatID.startsWith("-");
  if(
    (!isGroup) //In private chat
    || (msg.text == F("/nstat") || msg.text == F("/rssi") || msg.text == F("/ver") || msg.text == F("frmwupdate")) //Only /nstat or /rssi or /ver command for all bots in group
    || (botNameIdx = (_botSettings.botName.length() == 0 ? 0 : msg.text.indexOf(_botSettings.botName))) >= 0 //In Groups only if bot tagged
    || (msg.replyText.indexOf(_botSettings.botName) == 0 && msg.replyText.indexOf(REGISTRATION_MSG, botNameIdx + _botSettings.botName.length()) > 0) //In registration
    || (msg.data.length() > 0 && msg.text.indexOf(_botSettings.botNameForMenu) >= 0) //From BOT inline menu
    || (msg.replyText == _botSettings.botNameForMenu) //From BOT fast menu
    )
  {
    //botNameIdx = botNameIdx == -1 ? 0 : botNameIdx; // for register unregister

    BOT_TRACE(F("Checking authorization: "), msg.chatID);
    if(_botSettings.toStore.registeredChannelIDs.count(msg.chatID) > 0) //REGISTERED
    {
      BOT_TRACE(F("Authorized: "), msg.chatID);
      // if(_registeredChannelIDs[msg.chatID].second >= REGISTER_RETRY_COUNT)
      // {
      //   BOT_TRACE(msg.chatID, "Banned");
      // }
      // else
      if(msg.text.indexOf(F("/unregister")) >= 0)
      {
        BOT_TRACE(F("Unregistered: "), msg.chatID);
        _botSettings.toStore.registeredChannelIDs.erase(msg.chatID);
        SaveChannelIDs();
      }else
      if(msg.text.indexOf(F("/unregisterall")) >= 0)
      {
        BOT_TRACE(F("Unregistered: "), msg.chatID);
        _botSettings.toStore.registeredChannelIDs.clear();
        SaveChannelIDs();
      }else
      {
        //MENU
        //auto filtered = msg.text.substring(botNameIdx, msg.text.length());
        auto filtered = msg.text;
        filtered.replace(_botSettings.botName, F(""));
        auto result = HandleBotMenu(msg, filtered, isGroup);
        if(result.size() > 0)
        {
          if(result.size() == 1)
          {
            bot->replyMessage(result[0], msg.messageID, msg.chatID);
          }
          else
          {            
            for(const auto &r : result)
            {
              bot->sendMessage(r, msg.chatID);
            }
          }          
        }
      }
    }
    else
    {
      BOT_TRACE(F("NOT Authorized: "), msg.chatID);
    }
    
    //REGISTRATION
    {      
      if(msg.text.indexOf(F("/register")) >= 0)
      {
        BOT_TRACE(F("Start registration: "), msg.chatID);
        bot->replyMessage(_botSettings.botName + F(" ") + REGISTRATION_MSG, msg.messageID, msg.chatID);      
      }else      
      if(msg.replyText.indexOf(REGISTRATION_MSG) >= 0)
      {
        BOT_TRACE(F("Try registration: "), msg.chatID);
        if(msg.text == _botSettings.botSecure)
        {
          _botSettings.toStore.registeredChannelIDs[msg.chatID] = 1;
          SaveChannelIDs();
          BOT_TRACE(F("Registration succeed: "), msg.chatID);
          bot->replyMessage(String(F("Registration succeed: ")) + msg.username, msg.messageID, msg.chatID);
        }
        else
        {
          BOT_TRACE(F("Registration failed: "), msg.chatID);
          bot->replyMessage(String(F("Registration failed: ")) + msg.username, msg.messageID, msg.chatID);
        }
      }
    }    
  }
}

void SendMessageToAllRegisteredChannels(const String &msg, const bool &useBotName = true)
{
  for(const auto &channelID : _botSettings.toStore.registeredChannelIDs)
  {
    bot->sendMessage( useBotName ? _botSettings.botNameForMenu + msg : msg, channelID.first);
  }
}

void SaveChannelIDs()
{
  BOT_INFO(F("SaveChannelIDs"));
  File configFile = SPIFFS.open("/channelIDs.json", "w");
  if (configFile) 
  {
    BOT_TRACE(F("Write channelIDs file"));

    String store;
    for(const auto &v : _botSettings.toStore.registeredChannelIDs)
      store += v.first + ',';

    #ifdef ESP8266
    configFile.write(store.c_str());
    #else
    configFile.write((uint8_t*)store.c_str(), store.length());
    #endif
    configFile.close();
        //end save
  }
  else
  {
    BOT_TRACE(F("failed to open channelIDs file for writing"));    
  }
}

void LoadChannelIDs()
{
  BOT_INFO(F("LoadChannelIDs"));
  File configFile = SPIFFS.open("/channelIDs.json", "r");
  if (configFile) 
  {
    BOT_TRACE(F("Read channelIDs file"));    

    auto read = configFile.readString();

    BOT_TRACE(F("ChannelIDs in file: "), read);

    for(const auto &r : CommonHelper::split(read, ','))
    {
      BOT_INFO(F("\tChannelID: "), r);
      if(r.length() > 0)
      {
        _botSettings.toStore.registeredChannelIDs[r] = 1;
      }      
    }

    configFile.close();
  }
  else
  {
    BOT_TRACE(F("failed to open channelIDs file for reading"));    
  }
}

#endif //TELEGRAM_BOT_HELPER_H