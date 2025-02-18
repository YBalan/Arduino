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
#define MAX_DEVICE_NAME_LENGTH 20

namespace UAMap
{
  class Settings
  {
    public:
    Settings(){ SETTINGS_INFO(F("Reset Settings...")); init(); }   


    int16_t resetFlag = 200;
    int notifyHttpCode = 0;   
    
    char NotifyChatId[MAX_CHAT_ID_LENGTH];
    int8_t timeZone = DEFAULT_TIME_ZONE;
    uint32_t storeDataTimeout = STORE_DATA_TIMEOUT;

    char DeviceName[MAX_DEVICE_NAME_LENGTH];
    bool shortRecord = false;
    int useUdp = 0;
    float batteryUp = 60.0f;
    float batteryDw = 7.0f;
    float batteryNotify = 40.0f;

    void init()
    {      
      memset(NotifyChatId, 0, MAX_CHAT_ID_LENGTH);
      memset(DeviceName, 0, MAX_DEVICE_NAME_LENGTH);
      resetFlag = 200;
      notifyHttpCode = 0;      
      timeZone = DEFAULT_TIME_ZONE;
      storeDataTimeout = STORE_DATA_TIMEOUT;
      shortRecord = false;
      useUdp = 0;
      batteryUp = 60.0f;
      batteryDw = 7.0f;
      batteryNotify = 40.0f;
    }

    void setNotifyChatId(const String &str){ if(str.length() > MAX_CHAT_ID_LENGTH) SETTINGS_TRACE(F("\tChatID length exceed maximum!")); strcpy(NotifyChatId, str.c_str()); }
  };

  class SettingsExt
  {
    public:    
    
    public:
    void init()
    {
      SETTINGS_INFO(F("EXT: "), F("Reset Settings..."));     
      
    }    
  };  
} 

UAMap::Settings _settings;
UAMap::SettingsExt _settingsExt;


const bool SaveFile(const char* const fileName, const byte* const data, const size_t& size)
{  
  File configFile = MFS.open(fileName, FILE_WRITE);
  if (configFile) 
  { 
    configFile.write(data, size);
    configFile.close();
    return true;
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
  
  SETTINGS_INFO(F("ResetFlag: "), _settings.resetFlag);
  SETTINGS_INFO(F("NotifyHttpCode: "), _settings.notifyHttpCode);
  SETTINGS_INFO(F("NotifyChatId: "), _settings.NotifyChatId);
  if(!res)
  {
    //SETTINGS_INFO(F("Reset Settings..."));
    _settings.init();
  }
  
  SETTINGS_INFO(F("ResetFlag: "), _settings.resetFlag);
  SETTINGS_INFO(F("NotifyHttpCode: "), _settings.notifyHttpCode);
  SETTINGS_INFO(F("NotifyChatId: "), _settings.NotifyChatId);

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
 
  return res;
}


#endif //SETTINGS_H