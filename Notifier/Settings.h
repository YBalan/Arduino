#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include "DEBUGHelper.h"
#include <functional>

#ifdef ENABLE_INFO_SETTINGS
#define SETTINGS_INFO(...) SS_TRACE(__VA_ARGS__)
#else
#define SETTINGS_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_SETTINGS
#define SETTINGS_TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define SETTINGS_TRACE(...) {}
#endif

#define MAX_BASE_URI_LENGTH 50
#define MAX_CHAT_ID_LENGTH  20
#define MAX_MESSAGE_LENGTH 50
#define MAX_BUZZ_LENGTH 500

namespace UAMap
{
  class Settings
  {
    public:
    Settings(){ SETTINGS_INFO(F("Reset Settings...")); init(); }   

    int16_t resetFlag = 200;
    int notifyHttpCode = 0;   
    
    char NotifyChatId[MAX_CHAT_ID_LENGTH];
    int8_t timeZone = 3;
    uint32_t storeDataTimeout = STORE_DATA_TIMEOUT;

    uint16_t notifyPinCountBefore = 1;
    uint16_t notifyPinCounter = 0;
    int8_t notifyPinPrevValue = -1;

    bool notifyOnline = true;

    bool inversePinLogic = true;

    uint32_t dtOn = START_EPOCH_TIMESTAMP; 
    uint32_t dtOff = START_EPOCH_TIMESTAMP;
    uint32_t dtLast = START_EPOCH_TIMESTAMP;

    void init()
    {      
      memset(NotifyChatId, 0, MAX_CHAT_ID_LENGTH);
      
      resetFlag = 200;
      notifyHttpCode = 0;      
      timeZone = 3;
      storeDataTimeout = STORE_DATA_TIMEOUT;
      notifyPinCountBefore = 1;
      notifyPinCounter = 0;
      notifyPinPrevValue = -1;
      notifyOnline = true;
      inversePinLogic = true;
      dtOn = START_EPOCH_TIMESTAMP;
      dtOff = START_EPOCH_TIMESTAMP;
      dtLast = START_EPOCH_TIMESTAMP;
    }
    
    const bool setNotifyChatId(const String &str){ return setStringValue(NotifyChatId, str, MAX_CHAT_ID_LENGTH, F("NotifyChatId: "), /*setIfExceeds:*/true, /*trace:*/true); }   
    void trace(){
      SETTINGS_TRACE(F("resetFlag: "), String(resetFlag));
      SETTINGS_TRACE(F("notifyHttpCode: "), String(notifyHttpCode));
      SETTINGS_TRACE(F("timeZone: "), String(timeZone));
      SETTINGS_TRACE(F("storeDataTimeout: "), String(storeDataTimeout));
      SETTINGS_TRACE(F("notifyPinCountBefore: "), String(notifyPinCountBefore));
      SETTINGS_TRACE(F("notifyPinCounter: "), String(notifyPinCounter));
      SETTINGS_TRACE(F("notifyPinPrevValue: "), String(notifyPinPrevValue), F(" "), F("["), notifyPinPrevValue < 0 ? F("NA") : (notifyPinPrevValue == LOW ? F("LOW") : F("HIGH")), F("]"));
      SETTINGS_TRACE(F("notifyOnline: "), notifyOnline ? F("true") : F("false"));
      SETTINGS_TRACE(F("inversePinLogic: "), inversePinLogic ? F("true") : F("false"));
      SETTINGS_TRACE(F("DateTimeOn: "), epochToDateTime(dtOn));
      SETTINGS_TRACE(F("DateTimeOff: "), epochToDateTime(dtOff));
      SETTINGS_TRACE(F("DateTimeLast: "), epochToDateTime(dtLast));
    } 

    static const bool setStringValue(char *buff, const String &value, int16_t maxLength, const String &name, const bool &setIfExceeds = true, const bool &trace = true){
      bool res = true; 
      if(value.length() > maxLength) { 
        if(trace) SETTINGS_TRACE(name, F("length exceeds maximum!")); 
        res = false; 
      }
      if(setIfExceeds) { strcpy(buff, value.c_str()); buff[maxLength - 1] = '\0'; }
      return res;
    }
  };

  class SettingsExt
  {
    public:    
    char Message[MAX_MESSAGE_LENGTH];
    char BuzzOn[MAX_BUZZ_LENGTH];
    char BuzzOff[MAX_BUZZ_LENGTH];
    
    public:
    void init()
    {
      SETTINGS_INFO(F("EXT: "), F("Reset Settings..."));      

      memset(Message, 0, MAX_MESSAGE_LENGTH);
      memset(BuzzOn, 0, MAX_BUZZ_LENGTH);
      memset(BuzzOff, 0, MAX_BUZZ_LENGTH);
      setBuzzOn(BUZZER_ON_DEFAULT);
      setBuzzOff(BUZZER_OFF_DEFAULT);
    }   

    const bool setMessage(const String &str){ return Settings::setStringValue(Message, str, MAX_MESSAGE_LENGTH, F("Message: "), /*setIfExceeds:*/true, /*trace:*/true); }
    const bool setBuzzOn(const String &str){ return Settings::setStringValue(BuzzOn, str, MAX_BUZZ_LENGTH, F("BuzzOn: "), /*setIfExceeds:*/true, /*trace:*/true); }     
    const bool setBuzzOff(const String &str){ return Settings::setStringValue(BuzzOff, str, MAX_BUZZ_LENGTH, F("BuzzOff: "), /*setIfExceeds:*/true, /*trace:*/true); }     

    void trace(){
      SETTINGS_TRACE(F("Message: "), Message);
      SETTINGS_TRACE(F("BuzzOn: "), BuzzOn);
      SETTINGS_TRACE(F("BuzzOff: "), BuzzOff);
    }
  };  
} 

UAMap::Settings _settings;
UAMap::SettingsExt _settingsExt;

const bool SaveFile(const char* const fileName, const byte* const data, const size_t& size)
{  
  if (MFS.begin()) 
  {
    File configFile = MFS.open(fileName, FILE_WRITE);
    if (configFile) 
    { 
      configFile.write(data, size);
      configFile.close();
      return true;
    }
  }  
  return false;
}

const bool LoadFile(const char* const fileName, byte* const data, const size_t& size)
{
  if (MFS.begin()) 
  {
    SETTINGS_TRACE(F("File system mounted"));
    if (MFS.exists(fileName)) 
    {
      File configFile = MFS.open(fileName, FILE_READ);
      if (configFile) 
      {
        SETTINGS_INFO(F("Read file "), fileName);    
        configFile.read(data, size);
        configFile.close();
        return true;
      }
      else
      {
        SETTINGS_INFO(F("Failed to open: "), fileName);
        return false;
      }
    }
    else
    {
      SETTINGS_INFO(F("File does not exist "), fileName);
      return false;
    }
  }
  else
  {
    SETTINGS_INFO(F("Failed to mount FS"));    
  }
  return false;
}

const bool SaveSettings()
{
  String fileName = F("/config.bin");
  SETTINGS_INFO(F("Save Settings to: "), fileName);
  if(SaveFile(fileName.c_str(), (byte *)&_settings, sizeof(_settings)))
  {
    SETTINGS_INFO(F("Write to: "), fileName);
    return true;
  }
  SETTINGS_INFO(F("Failed to open: "), fileName);
  return false;
}

const bool SaveSettingsExt()
{
  String fileName = F("/configExt.bin");
  SETTINGS_INFO(F("EXT: "), F("Save Settings to: "), fileName);
  if(SaveFile(fileName.c_str(), (byte *)&_settingsExt, sizeof(_settingsExt)))
  {
    SETTINGS_INFO(F("Write to: "), fileName);
    return true;
  }
  SETTINGS_INFO(F("Failed to open: "), fileName);
  return false;
}

const bool LoadSettings()
{
  String fileName = F("/config.bin");
  SETTINGS_INFO(F("Load Settings from: "), fileName);

  const bool &res = LoadFile(fileName.c_str(), (byte *)&_settings, sizeof(_settings));    
  
  if(!res)
  {
    //SETTINGS_INFO(F("Reset Settings..."));
    _settings.init();
  }
  
  _settings.trace();
  return res;
}

const bool LoadSettingsExt()
{
  String fileName = F("/configExt.bin");
  SETTINGS_INFO(F("EXT: "), F("Load Settings from: "), fileName);

  const bool &res = LoadFile(fileName.c_str(), (byte *)&_settingsExt, sizeof(_settingsExt));  
 
  if(!res)
  {    
    _settingsExt.init();
  }

  _settingsExt.trace();
 
  return res;
}


#endif //SETTINGS_H