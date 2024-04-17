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
#include <FastBot.h>
FastBot bot;

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


#define REGISTRATION_MSG "Replay on this message with Bot secret to register channel id, please..."
void newMsg(FB_msg& msg) 
{  
  BOT_TRACE("MessageID: ", msg.messageID);
  BOT_TRACE("ChatID: ", msg.chatID);
  BOT_TRACE("UserID: ", msg.userID);
  BOT_TRACE("IsBot: ", msg.isBot ? "true" : "false")
  BOT_TRACE("UserName: ", msg.username);   
  BOT_TRACE("LastName: ", msg.last_name);  
  BOT_TRACE("ReplayText: ", msg.replyText);  
  BOT_TRACE("Query: ", msg.query); 

  if(msg.text == "Close") bot.closeMenu();

  BOT_TRACE("MESSAGE: ", msg.text);

  auto botNameIdx = msg.text.indexOf(_botSettings.botName);
  if(msg.chatID.length() == USER_CHAT_ID_LENGTH || botNameIdx >= 0 || msg.replyText.indexOf(REGISTRATION_MSG) >= 0)
  {
    botNameIdx = botNameIdx == -1 ? 0 : botNameIdx;

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
      }else
      {
        //MENU
        //bot.sendMessage(F("Alarmed regions count: ") + String(alarmedLedIdx.size()), msg.chatID);
        bot.replyMessage(F("Alarmed regions count: "), msg.messageID, msg.chatID);
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
        bot.replyMessage(F(REGISTRATION_MSG), msg.messageID, msg.chatID);      
      }else      
      if(msg.replyText.indexOf(REGISTRATION_MSG) >= 0)
      {
        BOT_TRACE("Try registration: ", msg.chatID);
        if(msg.text == _botSettings.botSecure)
        {
          _botSettings.toStore.registeredChannelIDs[msg.chatID] = 1;
          BOT_TRACE("Registration succeed: ", msg.chatID);
          bot.replyMessage(F("Registration succeed: ") + msg.username, msg.messageID, msg.chatID);
        }
        else
        {
          BOT_TRACE("Registration failed: ", msg.chatID);
          bot.replyMessage(F("Registration failed: ") + msg.username, msg.messageID, msg.chatID);
        }
      }
    }    
  }
}

#endif //TELEGRAM_BOT_H