#include <memory>
#pragma once
#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <map>
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

#define USER_CHAT_ID_LENGTH 9
#define REGISTER_RETRY_COUNT 5
#define BOT_MENU_NAME "UAMapMenu"
#include <FastBot.h>

#ifdef USE_BOT
std::unique_ptr<FastBot> bot(new FastBot());
#else
std::unique_ptr<FastBot> bot;
#endif

// показать юзер меню (\t - горизонтальное разделение кнопок, \n - вертикальное
//bot.showMenu("Menu1 \t Menu2 \t Menu3 \n Close", CHAT_ID);  
struct BotSettings
{
  String botToken;
  String botName;
  String botSecure;
  struct ToStore
  {
    std::map<String, uint8_t> registeredChannelIDs;
  } toStore;
} _botSettings;

extern const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered);
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

#define REGISTRATION_MSG "Replay on this message with Bot secret to register channel id, please..."
void HangleBotMessages(FB_msg& msg) 
{  
  BOT_TRACE("MessageID: ", msg.messageID);
  BOT_TRACE("ChatID: ", msg.chatID);
  BOT_TRACE("UserID: ", msg.userID);
  BOT_TRACE("IsBot: ", msg.isBot ? "true" : "false")
  BOT_TRACE("UserName: ", msg.username);   
  BOT_TRACE("LastName: ", msg.last_name);  
  BOT_TRACE("ReplayText: ", msg.replyText);  
  BOT_TRACE("Query: ", msg.query); 
  BOT_TRACE("Data: ", msg.data); 

  if(msg.text == "Close") bot->closeMenu();

  BOT_INFO("MESSAGE: ", msg.text);

  auto botNameIdx = -1;
  if(!msg.chatID.startsWith("-") //In private chat
    || (botNameIdx = (_botSettings.botName.length() == 0 ? 0 : msg.text.indexOf(_botSettings.botName))) >= 0 //In Groups only if bot tagged
    || msg.replyText.indexOf(REGISTRATION_MSG) >= 0 //In registration
    || (msg.text == BOT_MENU_NAME && msg.data.length() > 0) //From BOT menu
    )
  {
    botNameIdx = botNameIdx == -1 ? 0 : (botNameIdx + _botSettings.botName.length());

    BOT_TRACE("Checking authorization: ", msg.chatID);
    if(_botSettings.toStore.registeredChannelIDs.count(msg.chatID) > 0) //REGISTERED
    {
      BOT_TRACE("Authorized: ", msg.chatID);
      // if(_registeredChannelIDs[msg.chatID].second >= REGISTER_RETRY_COUNT)
      // {
      //   BOT_TRACE(msg.chatID, "Banned");
      // }
      // else
      if(msg.text.indexOf("/unregister", botNameIdx) >= 0)
      {
        BOT_TRACE("Unregistered: ", msg.chatID);
        _botSettings.toStore.registeredChannelIDs.erase(msg.chatID);
        SaveChannelIDs();
      }else
      if(msg.text.indexOf("/unregisterall", botNameIdx) >= 0)
      {
        BOT_TRACE("Unregistered: ", msg.chatID);
        _botSettings.toStore.registeredChannelIDs.clear();
        SaveChannelIDs();
      }else
      {
        //MENU
        auto filtered = msg.text.substring(botNameIdx, msg.text.length());
        auto result = HandleBotMenu(msg, filtered);
        if(result.size() > 0)
        {
          if(result.size() == 1)
          {
            bot->replyMessage(result[0], msg.messageID, msg.chatID);
          }
          else
          {
            bot->setLimit(result.size());
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
      BOT_TRACE("NOT Authorized: ", msg.chatID);
    }
    
    //REGISTRATION
    {      
      if(msg.text.indexOf("/register", botNameIdx) >= 0)
      {
        BOT_TRACE("Start registration: ", msg.chatID);
        bot->replyMessage(F(REGISTRATION_MSG), msg.messageID, msg.chatID);      
      }else      
      if(msg.replyText.indexOf(REGISTRATION_MSG) >= 0)
      {
        BOT_TRACE("Try registration: ", msg.chatID);
        if(msg.text == _botSettings.botSecure)
        {
          _botSettings.toStore.registeredChannelIDs[msg.chatID] = 1;
          SaveChannelIDs();
          BOT_TRACE("Registration succeed: ", msg.chatID);
          bot->replyMessage(F("Registration succeed: ") + msg.username, msg.messageID, msg.chatID);
        }
        else
        {
          BOT_TRACE("Registration failed: ", msg.chatID);
          bot->replyMessage(F("Registration failed: ") + msg.username, msg.messageID, msg.chatID);
        }
      }
    }    
  }
}

#endif //TELEGRAM_BOT_H