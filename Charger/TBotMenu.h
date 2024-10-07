#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H

#include <math.h>
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
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM

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

//Notify
#ifdef USE_NOTIFY
#define BOT_COMMAND_NOTIFY F("/notify")
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM
#define BOT_COMMAND_DOWNLOAD      F("/get")
#define BOT_COMMAND_REMOVE        F("/rem")
#define BOT_COMMAND_LIST          F("/ls")
#define BOT_COMMAND_CMD          F("/cmd")


//Fast Menu

void PrintNetworkStatistic(String &str, const int& codeFilter);
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void PrintFSInfo(String &fsInfo);
void SendCommand(const String &command);
const String DeviceReceive();


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
    BOT_MENU_INFO(F("BOT Channels:"));
    value.clear();
    value = String(F("BOT Channels:")) + F(" ") + String(_botSettings.toStore.registeredChannelIDs.size()) + F(" -> ");
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      BOT_MENU_INFO(F("\t"), channel.first);
      value += String(F("[")) + channel.first + F("]") + F("; ");
    }
  }
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  else if(GetCommandValue(BOT_COMMAND_CMD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT CMD:"));
    bot->sendTyping(msg.chatID);

    const float &intValue = value.toFloat();
    String command = value.isEmpty() || intValue > 0 ? F("get") : value;

    SendCommand(command);

    value.clear();
    
    const String &receive = DeviceReceive();
    BOT_MENU_INFO(receive);
    
    value = receive;      

    if(value.isEmpty())
    {
      value = F("Device does not respond...");
    }
  }
  else 
  if(GetCommandValue(BOT_COMMAND_LIST, filtered, value))
  {
    BOT_MENU_INFO(F("BOT LIST:"));
    bot->sendTyping(msg.chatID);

    const size_t &last = value.toInt();

    String filter;
    int totalRecordsCount = 0;
    uint32_t totalSize = 0;

    const auto &total = SPIFFS.totalBytes();
    const auto &used = SPIFFS.usedBytes();
    
    uint32_t sw = millis();
    const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalSize); 
    BOT_MENU_TRACE(F("Records: "), totalRecordsCount, F(" "), F("Total size: "), totalSize, F(" "), F("SW:"), millis() - sw, F("ms."));

    if(filesInfo.size() > 0)
    {
      std::vector<String> files;
      for(const auto& fi : filesInfo)
        files.push_back(fi.first);

      // Sort vector in descending order using a lambda function as comparator
      std::sort(files.begin(), files.end(), [](const String& a, const String& b) {
          return a > b; // Descending order
      });     

      value.clear();

      value += String(ds->lastRecord.voltage) + F("V") + F(" ") + F("(") + ds->lastRecord.dateTimeToString() + F(")");      
      messages.push_back(value); value.clear();      
      value += F("\n");                                                       // NewLine

      value += ds->startDate + F(" - ") + ds->endDate;      
      messages.push_back(value); value.clear();      
      value += F("\n");                                                       // NewLine

      value += String(F("Files: ")) + String(ds->getFilesCount()) + F(" ") + F("Records: ") + String(totalRecordsCount) + F(" ") + F("Total size: ") + String(totalSize) + F(" ") + F("Size Left: ") + String(total - used);
      messages.push_back(value); value.clear();
      value += F("\n");                                                       // NewLine

      String endDateCmd = ds->endDate; endDateCmd.replace('-', '_');

      value += String(F("Download All: ")) + BOT_COMMAND_DOWNLOAD + endDateCmd.substring(0, 4);
      messages.push_back(value); value.clear();
      value += F("\n");                                                       // NewLine

      value += String(F("Download Last Month: ")) + BOT_COMMAND_DOWNLOAD + endDateCmd.substring(0, 7);
      messages.push_back(value); value.clear();
      value += F("\n");                                                       // NewLine

      const size_t &num = last > 0 ? min(files.size(), last) : files.size();
      for(uint8_t idx = 0; idx < num; idx++)
      {
        const String &fileName = files[idx].substring(0, fileName.length() - FILE_EXT_LEN);
        if(fileName.startsWith(HEADER_FILE_NAME)) continue;
        String fileNameCmd = fileName; fileNameCmd.replace('-', '_');
        const auto &fileInfo = filesInfo.at(fileName);

        value +=
                fileName                                                        // fileName without extension
                + F("(") + fileInfo.linesCount + F(")")                         // + lines count
                + F(" ")                                                        // delimeter
                + F("(") + BOT_COMMAND_DOWNLOAD + fileNameCmd + F(")")          // /download or /get
                + F(" ")                                                        // delimeter
                + F("(") + BOT_COMMAND_REMOVE + fileNameCmd + F(")")            // /remove or /rem                
        ;
        messages.push_back(value); value.clear();
        value += F("\n");                                                       // NewLine        
      }

    }else
      value = F("File(s) not found");
  }
  else
  if(GetCommandValue(BOT_COMMAND_REMOVE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT REMOVE:"));
    bot->sendTyping(msg.chatID);

    String filter = value; 
    if(!filter.isEmpty())
    {
      const auto &filesRemoved = ds->removeData(filter);
      value = (filesRemoved == 1 ? filter : String(filesRemoved)) + F(" ") + F("File(s) removed");
    }else
      value = F("File(s) not found");
  }
  else
  if(GetCommandValue(BOT_COMMAND_DOWNLOAD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT DOWNLOAD:"));
    bot->sendTyping(msg.chatID);
    
    String filter = value.isEmpty() ? ds->endDate : value;
    
    int totalRecordsCount = 0;
    uint32_t totalSize = 0;

    uint32_t sw = millis();
    const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalSize); 
    BOT_MENU_TRACE(F("Records: "), totalRecordsCount, F(" "), F("Total size: "), totalSize, F(" "), F("SW:"), millis() - sw, F("ms."));

    if(filesInfo.size() > 0)
    {
      filter = String(FILE_PATH) + F("/") + filter;      

      std::vector<String> files;
      for(const auto& fi : filesInfo)
        files.push_back(String(FILE_PATH) + F("/") + fi.first);

      // Sort vector in descending order using a lambda function as comparator
      std::sort(files.begin(), files.end(), [](const String& a, const String& b) {
          return a < b; // Ascending order
      });     
      
      //const auto &totalRecordsCount = TelegramBot::getFilesLineCount(filesInfo);
      String outerFileName = filter + F("(") + totalRecordsCount + F(")") + FILE_EXT; 
      BOT_MENU_TRACE(filter, F(" -> "), outerFileName);

      if(bot->sendFile(files, totalSize, outerFileName, msg.chatID) != 1)
      {
        value = F("Telegram error");
      }
      else
        value.clear();
    }else
      value = F("File(s) not found");
  }
  

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