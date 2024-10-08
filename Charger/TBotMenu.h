#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H

#include <math.h>
#include "DEBUGHelper.h"
#include "CommonHelper.h"
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
ls - List all stored data
get - Download stored data
get0 - Current status
rem - Remove stored data
sync - Sync time (if WiFi is on)
sync3 - Sync time-zone +3 (if WiFi is on)
cmd - Send command to XYDJ device
cmdget - Get XYDJ device status
cmdup27_4 - Up to 27.4V (LiFePO4 25.6V batt)
cmddw26_0 - Down to 26.0V (LiFePO4 25.6V batt)
cmdup14_3 - Up to 14.3V (LiFePO4 12.8V batt)
cmddw12_9 - Down to 12.9V (LiFePO4 12.8V batt)
cmdup13_3 - Up to 13.3V (ACID 12V batt)
cmddw12_0 - Down to 12.0V (ACID 12V batt)
cmdon - Relay On (if possible)
cmdoff - Relay Off (if possible)
cmdstart - Start pulling data from XYDJ
cmdstop - Stop pulling data from XYDJ (Pause)
cmdop000 - Set relay time in minutes (000-999)
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
#define BOT_COMMAND_SYNC          F("/sync")


//Fast Menu

// From .ino file
const uint32_t &SyncTime();
void PrintNetworkStatistic(String &str, const int& codeFilter);
void PrintFSInfo(String &fsInfo);
void SendCommand(const String &command);
const String DeviceReceive();

// internal
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void sendList(const int &last, const bool &showGet, const bool &showRem, String &value, std::vector<String> &messages);
void sendStatus(String &value, std::vector<String> &messages, const int &totalRecordsCount = 0, const uint32_t &totalSize = 0);

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
  if(GetCommandValue(BOT_COMMAND_SYNC, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    const int &intValue = value.toInt();

    if(filtered.length() > String(BOT_COMMAND_SYNC).length() && intValue >= -12 && intValue <= 14)
    {
      _settings.timeZone = intValue;
      SaveSettings();    
    }
    const auto &now = SyncTime();    
    value = String(F("Synced time: ")) + epochToDateTime(now);
  }
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  else if(GetCommandValue(BOT_COMMAND_CMD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT CMD:"));
    bot->sendTyping(msg.chatID);

    const int &intValue = value.toInt();
    String command = value.isEmpty() || intValue > 0 ? String(F("get")) : value;
    command.toLowerCase();

    if(command.startsWith(F("up")) || command.startsWith(F("dw")))
    {
      const String &cmd = command.substring(0, 2);
      command.replace('_', '.');
      const float &fVal = command.substring(2).toFloat();
      if(fVal > 0 && fVal <= 80.0)
      {
        command = cmd + (fVal < 10 ? F("0") : F("")) + String(fVal, 1);
      }else
      {
        command.clear();      
        value = String(F("'")) + value + F("'") + F(" ") + F(" Wrong command");
      }
    }

    if(!command.isEmpty())
    {
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
  }
  else 
  if(GetCommandValue(BOT_COMMAND_LIST, filtered, value))
  {
    BOT_MENU_INFO(F("BOT LIST:"));
    bot->sendTyping(msg.chatID);

    const int &last = value.toInt();
    value.clear();
    sendList(last, /*showGet:*/true, /*showRem:*/false, value, messages);
  }
  else
  if(GetCommandValue(BOT_COMMAND_REMOVE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT REMOVE:"));
    bot->sendTyping(msg.chatID);

    const int &last = value.toInt();    
    if(last >= 0 && last < 2024){
      value.clear();
      sendList(last, /*showGet:*/false, /*showRem:*/true, value, messages);
    }
    else{
      String filter = value; 
      if(!filter.isEmpty())
      {
        const auto &filesRemoved = ds->removeData(filter);
        value = (filesRemoved == 1 ? filter : String(filesRemoved)) + F(" ") + F("File(s) removed");
        sendList(last, /*showGet:*/false, /*showRem:*/true, value, messages);
      }else
        value = F("File(s) not found");
    }
  }
  else
  if(GetCommandValue(BOT_COMMAND_DOWNLOAD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT DOWNLOAD:"));
    bot->sendTyping(msg.chatID);
    
    const int &last = value.toInt(); 
    if(filtered.length() > String(BOT_COMMAND_DOWNLOAD).length() && last == 0){
      value.clear();
      sendStatus(value, messages);
    }else
    if(last >= 0 && last < 2024){
      value.clear();
      sendList(last, /*showGet:*/true, /*showRem:*/false, value, messages);
    }
    else{
      String filter = value.isEmpty() ? ds->endDate : value;
      
      int totalRecordsCount = 0;
      uint32_t totalSize = 0;

      uint32_t sw = millis();
      const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalSize); 
      BOT_MENU_TRACE(F("Records: "), totalRecordsCount, F(" "), F("Size: "), totalSize, F(" "), F("SW:"), millis() - sw, F("ms."));

      if(filesInfo.size() > 0)
      {
        filter = String(FILE_PATH) + F("/") + filter;      

        std::vector<String> files;
        for(const auto& fi : filesInfo)
          files.push_back(String(FILE_PATH) + F("/") + fi.first);

        // Sort vector in Ascending order using a lambda function as comparator
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
  }
  

  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);  

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

void sendStatus(String &value, std::vector<String> &messages, const int &totalRecordsCount, const uint32_t &totalSize)
{  
    const auto &total = SPIFFS.totalBytes();
    const auto &used = SPIFFS.usedBytes();

    if(!value.isEmpty()){
      messages.push_back(value); value.clear();}

    value += String(ds->lastRecord.voltage, 1) + F("V") 
          + F(" ") + F("[") + (ds->lastRecord.relayOn ? F("On") : F("Off")) + F("]")
          + F("\n") + F("(") + ds->lastRecord.dateTimeToString() + F(")")
    ;      
    messages.push_back(value); value.clear();      
    //value += F("\n");                                                       // NewLine

    value += ds->startDate + F(" - ") + ds->endDate;      
    messages.push_back(value); value.clear();      
    //value += F("\n");                                                       // NewLine

    value += String(F("Files: ")) + String(ds->getFilesCount()) 
          + (totalRecordsCount > 0 ? String(F("\n")) + F("Recs: ") + String(totalRecordsCount) : String(F("")))
          + (totalSize > 0 ? String(F("\n")) + F("Size: ") + String(totalSize) : String(F("")))
          + F("\n") + F("Left: ") + String(total - used);
    messages.push_back(value); value.clear();
    //value += F("\n");                                                       // NewLine
}

void sendList(const int &last, const bool &showGet, const bool &showRem, String &value, std::vector<String> &messages)
{
    BOT_MENU_INFO(F("BOT sendList:"));       

    String filter;
    int totalRecordsCount = 0;
    uint32_t totalSize = 0;

    uint32_t sw = millis();
    const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalSize); 
    BOT_MENU_TRACE(F("Recs: "), totalRecordsCount, F(" "), F("Size: "), totalSize, F(" "), F("SW:"), millis() - sw, F("ms."));

    sendStatus(value, messages, totalRecordsCount, totalSize);

    if(filesInfo.size() > 0)
    {
      std::vector<String> files;
      for(const auto& fi : filesInfo)
        files.push_back(fi.first);

      // Sort vector in descending order using a lambda function as comparator
      std::sort(files.begin(), files.end(), [](const String& a, const String& b) {
          return a > b; // Descending order
      });      

      String endDateCmd = ds->endDate; endDateCmd.replace('-', '_');

      if(showGet){
        value += String(F("Download")) + F(" Year: ") + BOT_COMMAND_DOWNLOAD + endDateCmd.substring(0, 4);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine

        value += String(F("Download")) + F(" Last Month: ") + BOT_COMMAND_DOWNLOAD + endDateCmd.substring(0, 7);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine
      }

      const int &num = last > 0 ? min((int)files.size(), last) : files.size();
      for(uint8_t idx = 0; idx < num; idx++)
      {
        const String &fileName = files[idx];
        const auto &fileInfo = filesInfo.at(fileName);
        const String &fileNameWithoutExtension = fileName.substring(0, fileName.length() - FILE_EXT_LEN);                     // fileName without extension
        if(fileName.startsWith(HEADER_FILE_NAME)) continue;
        String fileNameCmd = fileNameWithoutExtension; fileNameCmd.replace('-', '_');        

        value +=
                fileNameWithoutExtension                                                                                      // fileName without extension
                + F(" ") + F("(") + CommonHelper::toString(fileInfo.linesCount, 4) + F(")")                                   // + lines count                
                + (showGet ? String(F(" ")) + F("[") + BOT_COMMAND_DOWNLOAD + fileNameCmd + F("]") : String(F("")) )          // /download or /get                
                + (showRem ? String(F(" ")) + F("[") + BOT_COMMAND_REMOVE + fileNameCmd + F("]") : String(F("")) )            // /remove or /rem                
        ;
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine        
      }

      if(showRem){
        value += String(F("Remove")) + F(" Year: ") + BOT_COMMAND_REMOVE + endDateCmd.substring(0, 4);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine

        value += String(F("Remove")) + F(" Last Month: ") + BOT_COMMAND_REMOVE + endDateCmd.substring(0, 7);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine
      }
    }else
    {
      value = F("File(s) not found");
      messages.push_back(value);
    }
}

#endif //USE_BOT

#endif //TELEGRAM_BOT_MENU_H