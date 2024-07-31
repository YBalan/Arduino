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

#define DEFAULT_START_ANGLE 0
#define DEFAULT_END_ANGLE 270
#define DEFAULT_MOVE_ANGLE DEFAULT_END_ANGLE
#define DEFAULT_MOVE_SPEED 10
#define DEFAULT_PERIOD_TIMEPUT_SEC 1200

enum MoveStyle : uint8_t
{
  Normal,
};

namespace UAMap
{
  class Settings
  {
    public:
    Settings(){ SETTINGS_INFO(F("Reset Settings...")); init(); }   


    int16_t resetFlag = 200;
    int16_t notifyHttpCode = 0; 
    uint16_t startAngle = DEFAULT_START_ANGLE;
    uint16_t endAngle = DEFAULT_END_ANGLE; 
    uint16_t moveAngle = DEFAULT_MOVE_ANGLE;
    uint8_t moveSpeed = DEFAULT_MOVE_SPEED;
    uint32_t periodTimeoutSec = DEFAULT_PERIOD_TIMEPUT_SEC;
    

    MoveStyle moveStyle = MoveStyle::Normal;

    void init()
    {      
      resetFlag = 200;
      notifyHttpCode = 0;      
    }
  };

  class SettingsExt
  {
    public:    
    char NotifyChatId[MAX_CHAT_ID_LENGTH];
    public:
    void init()
    {
      SETTINGS_INFO(F("EXT: "), F("Reset Settings..."));     
      memset(NotifyChatId, 0, MAX_CHAT_ID_LENGTH);
    }
    void setNotifyChatId(const String &str){ if(str.length() > MAX_CHAT_ID_LENGTH) SETTINGS_TRACE(F("\tChatID length exceed maximum!")); strcpy(NotifyChatId, str.c_str()); }
  };  
} 

UAMap::Settings _settings;
UAMap::SettingsExt _settingsExt;


const bool SaveFile(const char* const fileName, const byte* const data, const size_t& size)
{  
  File configFile = SPIFFS.open(fileName, "w");
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
  if (SPIFFS.begin()) 
  {
    SETTINGS_TRACE(F("File system mounted"));
    if (SPIFFS.exists(fileName)) 
    {
      File configFile = SPIFFS.open(fileName, "r");
      if (configFile) 
      {
        SETTINGS_INFO(F("Read file "), fileName);    
        configFile.read(data, size);
        configFile.close();
        return true;
      }
      else
      {
        SETTINGS_INFO(F("Failed to open "), fileName);
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
    Serial.println(F("\t\t\tFormat..."));
    SPIFFS.format();
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
  if(!res)
  {
    //SETTINGS_INFO(F("Reset Settings..."));
    _settings.init();
  }
  
  SETTINGS_INFO(F("ResetFlag: "), _settings.resetFlag);
  SETTINGS_INFO(F("NotifyHttpCode: "), _settings.notifyHttpCode);

  return res;
}

const bool LoadSettingsExt()
{
  String fileName = F("/configExt.bin");
  SETTINGS_INFO(F("EXT: "), F("Load Settings from: "), fileName);

  const bool &res = LoadFile(fileName.c_str(), (byte *)&_settingsExt, sizeof(_settingsExt));  
  
  SETTINGS_INFO(F("EXT NotifyChatId: "), _settingsExt.NotifyChatId);
 
  if(!res)
  {    
    _settingsExt.init();
  }
 
  return res;
}


#endif //SETTINGS_H