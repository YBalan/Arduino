#pragma once
#ifndef TELEGRAM_BOT_MENU_H
#define TELEGRAM_BOT_MENU_H

#include <math.h>
#include "DEBUGHelper.h"
#include "CommonHelper.h"
#include "Settings.h"

#define MONITOR_CHATS_FILE_NAME F("/monitorChats")

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
name - Set/Get Device Name
monitor - Show updatable monitor menu
short - Get short record status
short0 - Use long record format (Less days in storage)
short1 - Use short record format (More days in storage)
interval - Interval to store in Mins (1-15)
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
#define BOT_COMMAND_NOTIFY        F("/notify")
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM
#define BOT_COMMAND_DOWNLOAD      F("/get")
#define BOT_COMMAND_REMOVE        F("/rem")
#define BOT_COMMAND_LIST          F("/ls")
#define BOT_COMMAND_CMD           F("/cmd")
#define BOT_COMMAND_SYNC          F("/sync")
#define BOT_COMMAND_NAME          F("/name")
#define BOT_COMMAND_MONITOR       F("/monitor")
#define BOT_COMMAND_SHORT         F("/short")
#define BOT_COMMAND_INTERVAL      F("/interval")
//Service Commands
#define BOT_COMMAND_UPDOWN_MENU   F("/updw")

//Fast Menu

//SHORT_MONITOR_IN_GROUP
#ifdef SHORT_MONITOR_IN_GROUP
const bool ShortMonitorInGroup = SHORT_MONITOR_IN_GROUP == true;
#else
const bool ShortMonitorInGroup = false;
#endif

// From .ino file
const uint32_t SyncTime();
void Restart();
void MountFS();
void PrintNetworkStatistic(String &str, const int& codeFilter);
void PrintFSInfo(String &fsInfo);
void SendCommand(const String &command);
const String DeviceReceive(const int &minDelay, const String &whileNotStartWith);

// internal
const String GetPMMenu(const float &voltage, const String &chatId, const float& led_consumption_voltage_factor = 0.0);
const String GetPMMenuCall(const float &voltage, const String &chatId);
void SetPMMenu(const String &chatId, const int32_t &msgId, const float &voltage, const float& led_consumption_voltage_factor = 0.0);
void sendList(const int &last, const bool &showGet, const bool &showRem, String &value, std::vector<String> &messages, const bool &recalculateRecords = false);
void sendStatus(String &value, std::vector<String> &messages, const int &totalRecordsCount = 0, const uint32_t &totalRecordsSize = 0);

// Monitor
struct PMChatInfo { int32_t msgId = -1; float alarmValue = 0.0; float currentValue = 0.0; };
static std::map<String, PMChatInfo> pmChatIds;
void loadMonitorChats(const String &fileName);
void saveMonitorChats(const String &fileName);
void sendUpdateMonitorAllMenu(const String &deviceName);
const int32_t sendUpdateMonitorMenu(const String &deviceName, const String &chatId, const int32_t &messageId);
void handleUpDownMenuValues(String &value, const String &chatId);
void updateAllMonitorsFromDeviceSettings(String &value, const String &waitWhile);


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
    Restart();
  }else
  if(GetCommandValue(BOT_COMMAND_CHANGE_CONFIG, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    _settings.resetFlag = 1985;
    SaveSettings();
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    Restart();
  }else  
  if(GetCommandValue(BOT_COMMAND_VER, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    value = String(F("Flash Date: ")) + String(__DATE__) + F(" ") + String(__TIME__) + F(" ") + F("V:") + VER + VER_POSTFIX;
  }else
  if(GetCommandValue(BOT_COMMAND_FS, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    if(value.startsWith(F("format"))){
      MFS.format();
      bot->sendMessage(F("Wait for restart..."), msg.chatID);
      Restart();
    }else
    if(value.startsWith(F("end"))){
      MFS.end();
      MountFS();
    }
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
  }else
  if(GetCommandValue(BOT_COMMAND_SYNC, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    const int &intValue = value.toInt();

    if(value.length() > 0 && intValue >= -12 && intValue <= 14)
    {
      _settings.timeZone = intValue;
      SaveSettings();    
    }
    const auto &now = SyncTime();
    ds->setDateTime(now); 
    value = String(F("Synced time: ")) + epochToDateTime(now);
  }
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CUSTOM  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  else if(GetCommandValue(BOT_COMMAND_SHORT, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    const int &intValue = value.toInt();

    if(value.length() > 0 && intValue >= 0)
    {
      _settings.shortRecord = intValue > 0;
      SaveSettings();    
    }    
    ds->setShortRecord(_settings.shortRecord); 
    value = String(F("Short record: ")) + (_settings.shortRecord ? F("true") : F("false"));
  }
  else if(GetCommandValue(BOT_COMMAND_INTERVAL, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    const int &intValue = value.toInt();

    if(value.length() > 0 && intValue >= 1 && intValue <= 15)
    {
      _settings.storeDataTimeout = intValue * 60000;
      SaveSettings();    
    }
    value = String(F("Interval: ")) + String((int)(_settings.storeDataTimeout / 60000)) + F("min.");
  }
  else if(GetCommandValue(BOT_COMMAND_NAME, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("NAME:"));
    bot->sendTyping(msg.chatID);

    if(!value.isEmpty() && value.length() < MAX_DEVICE_NAME_LENGTH){
      strcpy(_settings.DeviceName, value.c_str());
      SaveSettings();
    }
    value = _settings.DeviceName;
    if(value.isEmpty()){ value = F("Name is empty now..."); }
  }
  else if(GetCommandValue(BOT_COMMAND_MONITOR, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("MONITOR:"));
    bot->sendTyping(msg.chatID);

    #ifdef DEBUG
    loadMonitorChats(MONITOR_CHATS_FILE_NAME);
    #endif

    auto &chatInfo = pmChatIds[msg.chatID];
    const auto &msgId = sendUpdateMonitorMenu(_settings.DeviceName, msg.chatID, -1);
    if(msgId != -1){
      chatInfo.msgId = msgId;
      saveMonitorChats(MONITOR_CHATS_FILE_NAME);
    }

    value.clear();
  }
  else if(GetCommandValue(BOT_COMMAND_CMD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("CMD:"));
    bot->sendTyping(msg.chatID);

    bool isOPTimeChanged = false;
    float voltageValue = 0.0;
    const int &intValue = value.toInt();
    String command = value.isEmpty() || intValue > 0 ? String(F("get")) : value;
    command.toLowerCase();

    if(command.startsWith(F("up")) || command.startsWith(F("dw"))){
      const String &cmd = command.substring(0, 2);
      command.replace('_', '.');
      const float &fVal = command.substring(2).toFloat();
      if(fVal > 0.0 && fVal <= MAX_VOLTAGE){
        command = cmd + (fVal < 10 ? F("0") : F("")) + String(fVal, 1);
        voltageValue = fVal;
      }else{
        command.clear();      
        value = String(F("'")) + value + F("'") + F(" ") + F("Wrong command");
      }
    }else
    if(command.startsWith(F("op"))){
      const String &cmd = command.substring(0, 2);
      const int &iVal = command.substring(2).toInt();
      if(iVal >= 0 && iVal <= MAX_OPTIME){
        char buff[4];
        snprintf(buff, sizeof(buff), "%03d", iVal);
        command = String(buff);
        voltageValue = iVal;
        isOPTimeChanged = true;
      }else{
        command.clear();      
        value = String(F("'")) + value + F("'") + F(" ") + F("Wrong command");
      }
    }else
    if(command.startsWith(F("on"))){

    }else
    if(command.startsWith(F("off"))){

    }

    if(!command.isEmpty()){
      SendCommand(command);

      value.clear();
      
      const String &waitWhile = command.startsWith(F("get")) ? String(F("U-")) : String(F(""));
      updateAllMonitorsFromDeviceSettings(value, waitWhile);      

      if(voltageValue >= 0.0 && !value.startsWith(F("FALL"))) {
        if(command.startsWith(F("up"))){
          ds->params.upVoltage = voltageValue;
        }else
        if(command.startsWith(F("dw"))){
          ds->params.dwVoltage = voltageValue;
        }else
        if(isOPTimeChanged){
          ds->params.opTime = (int)voltageValue;
        }
        sendUpdateMonitorAllMenu(_settings.DeviceName);
      }

      if(value.isEmpty()){
        value = F("Device does not respond...");
      }
    }
  }
  else 
  if(GetCommandValue(BOT_COMMAND_LIST, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("LIST:"));
    bot->sendTyping(msg.chatID);

    const int &last = value.toInt();
    value.clear();
    sendList(last, /*showGet:*/true, /*showRem:*/false, value, messages, /*recalculateRecords:*/true);
  }
  else
  if(GetCommandValue(BOT_COMMAND_REMOVE, filtered, value))
  {
    BOT_MENU_INFO(F("BOT"), F("REMOVE:"));
    bot->sendTyping(msg.chatID);

    const int &last = value.toInt();    
    if(last >= 0 && last < 2024 && !value.endsWith(FILE_EXT)){
      value.clear();
      sendList(last, /*showGet:*/false, /*showRem:*/true, value, messages);
    }
    else{
      String filter = value; 
      if(!filter.isEmpty())
      {
        const auto &filesRemoved = ds->removeData(filter);
        value = (filesRemoved == 1 ? filter : String(filesRemoved)) + F(" ") + F("File(s) removed");
        sendList(last, /*showGet:*/true, /*showRem:*/false, value, messages);
      }else
        value = F("File(s) not found");
    }
  }
  else
  if(GetCommandValue(BOT_COMMAND_DOWNLOAD, filtered, value))
  {
    BOT_MENU_INFO(F("BOT "), F("DOWNLOAD:"));
    bot->sendTyping(msg.chatID);
    
    const int &last = value.toInt(); 
    if(value.length() > 0 && last == 0){
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
      uint32_t totalRecordsSize = 0;

      uint32_t sw = millis();
      const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalRecordsSize); 
      BOT_MENU_TRACE(F("Records: "), totalRecordsCount, F(" "), F("Size: "), totalRecordsSize, F(" "), F("SW:"), millis() - sw, F("ms."));

      if(filesInfo.size() > 0)
      {
        String outerFileName = _botSettings.botNameForMenu + F("_") + filter;        

        std::vector<String> files;
        for(const auto& fi : filesInfo)
          files.push_back(String(FILE_PATH) + F("/") + fi.first);

        // Sort vector in Ascending order using a lambda function as comparator
        std::sort(files.begin(), files.end(), [](const String& a, const String& b) {
            return a < b; // Ascending order
        });     
        
        //const auto &totalRecordsCount = TelegramBot::getFilesLineCount(filesInfo);
        outerFileName += String(F("(")) + totalRecordsCount + F(")") + FILE_EXT; 
        BOT_MENU_TRACE(filter, F(" -> "), outerFileName);

        if(bot->sendFile(files, totalRecordsSize, outerFileName, msg.chatID) != 1)
        {
          value = F("Telegram error");
        }
        else
          value.clear();
      }else
        value = F("File(s) not found");
    }
  }else
  if(GetCommandValue(BOT_COMMAND_UPDOWN_MENU, filtered, value)){ 
    BOT_MENU_INFO(F("BOT "), F("UP DOWN MENU:"));
    //bot->sendTyping(msg.chatID);  
    handleUpDownMenuValues(value, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_TEST, filtered, value)){ 
    BOT_MENU_INFO(F("BOT "), F("TEST:"));
    bot->sendTyping(msg.chatID);    
    #ifdef DEBUG
      int days = value.toInt();
      days = days > 0 ? days : 1;
      value = String(days) + F(" felt");
      value.clear();
      dsTest.setDateTime(1440 * 60);
      String removedFile;        
      for(int i = 0; i < 1440 * days; i++){
        //if(i % 1440 == 0) bot->sendTyping(msg.chatID);
        BOT_MENU_TRACE(i);
        if(!dsTest.appendData(60, removedFile)){
          value += String(F("FALL"));
          break;
        }else
        if(!removedFile.isEmpty()) { value += removedFile + F(" removed!") + '\n'; removedFile.clear(); }
        //delay(100);
      }
    #endif
    PrintFSInfo(value);    
  }
  

  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);  

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

void updateAllMonitorsFromDeviceSettings(String &value, const String &waitWhile){
  const String &receive = DeviceReceive(100, waitWhile);
  BOT_MENU_INFO(receive);

  if(receive.startsWith(F("U-"))){
    if(ds->params.readFromXYDJ(receive)){
      sendUpdateMonitorAllMenu(_settings.DeviceName);
    }
  }      
  value = receive;  
}

void sendStatus(String &value, std::vector<String> &messages, const int &totalRecordsCount, const uint32_t &totalRecordsSize){  
    const auto &total = MFS.totalBytes();
    const auto &used = MFS.usedBytes();

    if(!value.isEmpty()){
      messages.push_back(value); value.clear();}

    value += String(ds->getVoltage(), 1) + F("V") 
          + F(" ") + F("[") + (ds->getRelayOn() ? F("On") : F("Off")) + F("]")
          + F("\n") + F("(") + ds->getCurrentDateTimeStr() + F(")")
    ;      
    messages.push_back(value); value.clear();      
    //value += F("\n");                                                       // NewLine

    value += String(F("On")) + F(": ") + ds->getLastRelayOnStatus()
            + F("\n")
            + F("Off") + F(": ") + ds->getLastRelayOffStatus()
    ;      
    messages.push_back(value); value.clear();      
    //value += F("\n");                                                       // NewLine

    value += ds->startDate + F(" - ") + ds->endDate;      
    messages.push_back(value); value.clear();      
    //value += F("\n");                                                       // NewLine

    int percentage = 100;
    float daysLeft = DataStorage::getDaysLeft(_settings.storeDataTimeout / 60000, _settings.shortRecord, total, used, percentage);

    value += String(F("Day(s): ")) + String(ds->getFilesCount()) 
          + (totalRecordsCount > 0 ? String(F("\n")) + F("Recs: ") + String(totalRecordsCount) : String(F("")))
          + (totalRecordsSize > 0 ? String(F("\n")) + F("Size: ") + String(totalRecordsSize) + F("bytes") : String(F("")))
          + F("\n") + F("Left: ") + String(daysLeft, 1) + F("Day(s)") + F(" ") + F("(") + String(percentage) + F("%") + F(")");
    messages.push_back(value); value.clear();
    //value += F("\n");                                                       // NewLine
}

void sendList(const int &last, const bool &showGet, const bool &showRem, String &value, std::vector<String> &messages, const bool &recalculateRecords){
    BOT_MENU_INFO(F("BOT "), F("sendList:"));       

    String filter;
    int totalRecordsCount = 0;
    uint32_t totalRecordsSize = 0;

    uint32_t sw = millis();
    const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalRecordsSize, recalculateRecords); 
    BOT_MENU_TRACE(F("Recs: "), totalRecordsCount, F(" "), F("Size: "), totalRecordsSize, F(" "), F("SW:"), millis() - sw, F("ms."));

    sendStatus(value, messages, totalRecordsCount, totalRecordsSize);

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
        const String &yearCmd = endDateCmd.substring(0, 4);
        const String &monthCmd = endDateCmd.substring(0, 7);
        const int &yearRecords = ds->getRecordsCount(ds->endDate.substring(0, 4));
        const int &monthRecords = ds->getRecordsCount(ds->endDate.substring(0, 7));

        value += String(F("Year: ")) + F("(") + yearRecords + F(")") + F(" ") + F("[") + BOT_COMMAND_DOWNLOAD + yearCmd + F("]");
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine

        value += String(F("Month: ")) + F("(") + monthRecords + F(")") + F(" ") +  F("[") + BOT_COMMAND_DOWNLOAD + monthCmd + F("]");
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
                + F(" ") + F("(") + CommonHelper::toString(fileInfo.linesCount, 4, '0') + F(")")                              // + lines count                
                + (showGet ? String(F(" ")) + F("[") + BOT_COMMAND_DOWNLOAD + fileNameCmd + F("]") : String(F("")) )          // /download or /get                
                + (showRem ? String(F(" ")) + F("[") + BOT_COMMAND_REMOVE + fileNameCmd + F("]") : String(F("")) )            // /remove or /rem                
        ;
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine        
      }

      if(showRem){
        value += String(F("Remove")) + F(" ") + F("Year: ") + BOT_COMMAND_REMOVE + endDateCmd.substring(0, 4);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine

        value += String(F("Remove")) + F(" ") + F("Month: ") + BOT_COMMAND_REMOVE + endDateCmd.substring(0, 7);
        messages.push_back(value); value.clear();
        //value += F("\n");                                                       // NewLine
      }
    }else
    {
      value = F("File(s) not found");
      messages.push_back(value);
    }
}

const String getMonitorMenu(const bool &isInGroup){  
  bool showRelayStatus = !isInGroup || !(isInGroup && ShortMonitorInGroup);
  String menu = String(ds->getVoltage(), 1) + F("V")
              + F("\t") + (ds->getRelayOn() ? F("🔋") : F("⚡️"))
              + F("\n") + F("🕐") + ds->getLastRecordDateTimeStr()
              + F("\t") + F("📊") //F("📉") F("📉")
              + F("\n") + ds->params.toString()
              + (showRelayStatus ? String(F("\n")) + F("On") + F(": ") + ds->getLastRelayOnStatus() : String(F("")))
              + (showRelayStatus ? String(F("\n")) + F("Off") + F(": ") + ds->getLastRelayOffStatus() : String(F("")))
      ;
  BOT_MENU_TRACE(menu);
  return std::move(menu);
}

const String getMonitorMenuCallback(const bool &isInGroup){
  bool showRelayStatus = !isInGroup || !(isInGroup && ShortMonitorInGroup);
  String currentDate = ds->endDate;
  currentDate.replace('-', '_');

  String commandCmdGet = String(BOT_COMMAND_CMD) + F("get");

  String call = String(BOT_COMMAND_MONITOR)                                                         // Voltage
              + F(",") + (String(BOT_COMMAND_CMD) + (ds->getRelayOn() ? F("off") : F("on")) )       // Relay On/Off
              + F(",") + BOT_COMMAND_DOWNLOAD                                                       // Last record DateTime
              + F(",") + BOT_COMMAND_DOWNLOAD + currentDate                                         // Download current date .csv
              + F(",") + BOT_COMMAND_UPDOWN_MENU                                                    // Mode U-1 dw:12.0 up: 13.0 op: 0 min
              + (showRelayStatus ? String(F(",")) + commandCmdGet : String(F("")))                  // On relay status
              + (showRelayStatus ? String(F(",")) + commandCmdGet : String(F("")))                  // Off relay status
      ;
  BOT_MENU_TRACE(call);
  return std::move(call);
}

static std::map<String, int32_t> menuIds;
static float newUp = 0.0;
static float newDw = 0.0;
void sendUpdateUpDownMenu(const float &dwDelta, const float &upDelta, const bool &submit, bool &cancel, const String &deviceName, const String &chatId){
  BOT_MENU_INFO(F("BOT "), F("sendUpdateUpDownMenu: "), dwDelta, F(" "), upDelta, F(" "), submit, F(" "), cancel);

  float nUp = newUp == 0.0 && upDelta == 0.0 ? ds->params.upVoltage : (newUp + upDelta);
  float nDw = newDw == 0.0 && dwDelta == 0.0 ? ds->params.dwVoltage : (newDw + dwDelta);

  BOT_MENU_INFO(nDw, F(" "), nUp);

  String menu;
  String call;

  bool isNewValuesValid = false;
  if(true || nDw > 0.0 && nUp <= MAX_VOLTAGE && newUp > newDw){
    isNewValuesValid = true;
    newUp = nUp;
    newDw = nDw;
    BOT_MENU_INFO(newDw, F(" "), newUp);
  }

  if(!cancel){
    menu =    String(F("DW")) + F("\t") + F("UP")
         + F("\n") + F("⤴️") + F("\t") + F("⬆️") + F("\t") + F("⬆️") + F("\t") + F("⤴️") //↖️↗️⬆️↘️↙️⤴️⤵️
         + F("\n") + String(newDw, 1) + F("V") + F("\t") + String(newUp, 1) + F("V")
         + F("\n") + F("⤵️") + F("\t") + F("⬇️") + F("\t") + F("⬇️") + F("\t") + F("⤵️") //↖️↗️⬆️↘️↙️⤴️⤵️
         + F("\n") + F("Submit") + F("\t") + F("Cancel")
    ;

    // menu =    String(F("DW")) + F("\t") + F("UP")
    //      + F("\n") + F("+1.0") + F("\t") + F("+0.1") + F("\t") + F("+1.0") + F("\t") + F("+0.1")
    //      + F("\n") + String(newDw, 1) + F("V") + F("\t") + String(newUp, 1) + F("V")
    //      + F("\n") + F("-1.0") + F("\t") + F("-0.1") + F("\t") + F("-1.0") + F("\t") + F("-0.1")
    //      + F("\n") + F("Submit") + F("\t") + F("Cancel")
    // ;

    String cdw = String(BOT_COMMAND_UPDOWN_MENU) + F("dw");
    String cup = String(BOT_COMMAND_UPDOWN_MENU) + F("up");

    call = String(F("/0")) + F(",") + F("/0")         //First line DW UP - nothing
         + F(",") + cdw + F("10") + F(",") + cdw + F("01") // DW - up
         + F(",") + cup + F("10") + F(",") + cup + F("01") // UP - up
         + F(",") + F("/0") + F(",") + F("/0")               //3rd line - nothing
         + F(",") + cdw + F("10_") + F(",") + cdw + F("01_") // DW - down
         + F(",") + cup + F("10_") + F(",") + cup + F("01_") // UP - down
         + F(",") + BOT_COMMAND_UPDOWN_MENU + F("Submit") + F(",") + BOT_COMMAND_UPDOWN_MENU + F("Cancel")
    ;
  }

  auto &messageId = menuIds[chatId];
  BOT_MENU_INFO(F("messageId: "), messageId, F(" "), F("chatId: "), chatId);
  BOT_MENU_INFO(menu);
  BOT_MENU_INFO(call); 

  if(messageId > 0){      
    if(submit && isNewValuesValid){
      ds->params.upVoltage = newUp;
      ds->params.dwVoltage = newDw;
      //sendUpdateMonitorAllMenu(_settings.DeviceName); 
      String cmd = String(F("dw")) + String(newDw, 1) + F(",") + F("up") + String(newUp);
      BOT_MENU_INFO(cmd);
      SendCommand(cmd);
      delay(1000);
      SendCommand(F("get"));
      updateAllMonitorsFromDeviceSettings(cmd, F("U-"));
      BOT_MENU_INFO(cmd);
      cancel = true;   
    }
    if(cancel){
      bot->deleteMessage(messageId, chatId);
      menuIds.erase(chatId);
      newUp = 0.0; newDw = 0.0;
    }else
      bot->editMenuCallback(messageId, menu, call, chatId); 
  }else
  if(messageId == 0){        
    bot->inlineMenuCallback(_botSettings.botNameForMenu + F("Params"), menu, call, chatId);
    messageId = bot->lastBotMsg(); 
  }  
}

void handleUpDownMenuValues(String &value, const String &chatId){
  float upDelta = 0.0;
  float dwDelta = 0.0;
  bool submit = false;
  bool cancel = false;
  if(!value.isEmpty()){
    submit = value.startsWith(F("Submit"));
    cancel = submit || value.startsWith(F("Cancel"));

    if(!cancel){
      float val = (float)value.substring(2).toInt();
      bool minus = value.endsWith(F("_"));
      if(value.startsWith(F("dw"))){
        dwDelta = (val / 10) * (minus ? -1 : 1);
      }else
      if(value.startsWith(F("up"))){
        upDelta = (val / 10) * (minus ? -1 : 1);
      }
      BOT_MENU_INFO(dwDelta, F(" "), upDelta);
    }
  }
  sendUpdateUpDownMenu(dwDelta, upDelta, submit, cancel, _settings.DeviceName, chatId);
}

const int32_t sendUpdateMonitorMenu(const String &deviceName, const String &chatId, const int32_t &messageId){
  bool isInGroup = chatId.startsWith(F("-"));
  const String &menu = getMonitorMenu(isInGroup);
  const String &call = getMonitorMenuCallback(isInGroup);
  int32_t resMsgId = -1;

  if(messageId == -1){
    bot->inlineMenuCallback(_botSettings.botNameForMenu + deviceName, menu, call, chatId);
    resMsgId = bot->lastBotMsg(); 
  }
  else{
    bot->editMenuCallback(messageId, menu, call, chatId);    
  }
  return resMsgId;
}

void sendUpdateMonitorAllMenu(const String &deviceName){
  for(const auto &[key, value] : pmChatIds){
    sendUpdateMonitorMenu(deviceName, key, value.msgId);
  }
}

void saveMonitorChats(const String &fileName){
  BOT_MENU_TRACE(F("saveMonitorChats"));
  File file = MFS.open(fileName.c_str(), FILE_WRITE);
  CommonHelper::saveMap(file, pmChatIds);  
  file.close();

  #ifdef DEBUG
  file = MFS.open(fileName.c_str(), FILE_READ);  
  BOT_MENU_INFO(file.readString());
  file.close();
  #endif
}

void loadMonitorChats(const String &fileName){
  BOT_MENU_TRACE(F("loadMonitorChats"));  

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
  BOT_MENU_TRACE(F("loadMonitorChats"), F("size: "), map.size());
  for (const auto& [key, value] : map) {
    BOT_MENU_TRACE(F("ChatId: "), key);
    BOT_MENU_TRACE(F("\t"), F("MessageId: "), value.msgId);
    BOT_MENU_TRACE(F("\t"), F("alarm: "), value.alarmValue);
    BOT_MENU_TRACE(F("\t"), F("current: "), value.currentValue);
  }
  #endif
}

#endif //USE_BOT

#endif //TELEGRAM_BOT_MENU_H