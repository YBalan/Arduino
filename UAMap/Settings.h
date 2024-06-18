#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include "DEBUGHelper.h"

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

uint32_t effectStartTicks = 0;
bool effectStarted = false;
enum class Effect : uint8_t
{
  Normal,
  Rainbow,
  Strobe,
  Gay,
  FillRGB,
  UA,
  UAWithAnthem,
  BG,
  MD,
  MAX,

} _effect;

enum class ExtSouvenirMode : uint8_t
{
  UAPrapor,
  Reserved,    //NotUsed
  BGPrapor,    
  MDPrapor,  
  MAX,  
};

const String GetExtSouvenirModeStr(const ExtSouvenirMode &mode)
{
  String res;
  switch(mode)
  {    
    case ExtSouvenirMode::UAPrapor: res = String(F("UAPrapor")); break;
    case ExtSouvenirMode::Reserved: res = String(F("Reserved")); break;
    case ExtSouvenirMode::BGPrapor: res = String(F("BGPrapor")); break;
    case ExtSouvenirMode::MDPrapor: res = String(F("MDPrapor")); break;
    default: res = String(F("Unknown: ")) + String((uint8_t)mode); break;
  }

  return res;
}

enum class ExtMode : uint8_t
{
  Alarms,
  Souvenir,
  AlarmsOnlyCustomRegions,
  MAX,
};

const String GetExtModeStr(const ExtMode &mode)
{
  String res;
  switch(mode)
  {    
    case ExtMode::Alarms: res = String(F("Alarms")); break;
    case ExtMode::Souvenir: res = String(F("Souvenir")); break;
    case ExtMode::AlarmsOnlyCustomRegions: res = String(F("AlarmsOnlyCustomRegions")); break;    
    default: res = String(F("Unknown: ")) + String((uint8_t)mode); break;
  }

  return res;
}

enum ColorSchema : uint8_t
{
  Blue,
  White,
  Yellow,
};

namespace UAMap
{
  class Settings
  {
    public:

    Settings(){ SETTINGS_INFO(F("Reset Settings...")); init(); }

    CRGB PortalModeColor = LED_PORTAL_MODE_COLOR;
    CRGB NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
    CRGB NotAlarmedColor = LED_NOT_ALARMED_COLOR;
    CRGB PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;
    CRGB AlarmedColor = LED_ALARMED_COLOR;
    uint8_t Brightness = 2;

    bool alarmsCheckWithoutStatus = ALARMS_CHECK_WITHOUT_STATUS;
    uint32_t alarmsUpdateTimeout = ALARMS_UPDATE_DEFAULT_TIMEOUT;
    uint8_t alarmedScale = LED_ALARMED_SCALE_FACTOR;
    bool alarmScaleDown = true;

    int16_t resetFlag = 200;
    int notifyHttpCode = 0;

    uint8_t Relay1Region = 0;
    uint8_t Relay2Region = 0;

    uint16_t BuzzTime = 0;

    char BaseUri[MAX_BASE_URI_LENGTH]; // = ALARMS_API_IOT_BASE_URI;

    void init()
    {
      PortalModeColor = LED_PORTAL_MODE_COLOR;
      NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
      NotAlarmedColor = LED_NOT_ALARMED_COLOR;
      AlarmedColor = LED_ALARMED_COLOR;
      PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;

      Brightness = 2;

      alarmsCheckWithoutStatus = ALARMS_CHECK_WITHOUT_STATUS;
      alarmsUpdateTimeout = ALARMS_UPDATE_DEFAULT_TIMEOUT;
      alarmedScale = LED_ALARMED_SCALE_FACTOR;
      alarmScaleDown = true;
      resetFlag = 200;
      notifyHttpCode = 0;

      Relay1Region = 0;
      Relay2Region = 0;

      BuzzTime = 0;

      strcpy(BaseUri, ALARMS_API_IOT_BASE_URI);
    }
  };

  class SettingsExt
  {
    public:
    ExtMode Mode;
    ExtSouvenirMode SouvenirMode;
    char NotifyChatId[MAX_CHAT_ID_LENGTH];
    public:
    void init()
    {
      SETTINGS_INFO(F("EXT: "), F("Reset Settings..."));
      Mode = ExtMode::Alarms;
      SouvenirMode = ExtSouvenirMode::UAPrapor;
      memset(NotifyChatId, 0, MAX_CHAT_ID_LENGTH);
    }
    void setNotifyChatId(const String &str){ if(str.length() > MAX_CHAT_ID_LENGTH) SETTINGS_TRACE(F("\tChatID length exceed maximum!")); strcpy(NotifyChatId, str.c_str()); }
  };
};

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
  
  SETTINGS_INFO(F("BR: "), _settings.Brightness);  
  SETTINGS_INFO(F("ResetFlag: "), _settings.resetFlag);
  SETTINGS_INFO(F("NotifyHttpCode: "), _settings.notifyHttpCode);
  if(!res)
  {
    //SETTINGS_INFO(F("Reset Settings..."));
    _settings.init();
  }
  SETTINGS_INFO(F("BR: "), _settings.Brightness);  
  SETTINGS_INFO(F("ResetFlag: "), _settings.resetFlag);
  SETTINGS_INFO(F("NotifyHttpCode: "), _settings.notifyHttpCode);

  return res;
}

const bool LoadSettingsExt()
{
  String fileName = F("/configExt.bin");
  SETTINGS_INFO(F("EXT: "), F("Load Settings from: "), fileName);

  const bool &res = LoadFile(fileName.c_str(), (byte *)&_settingsExt, sizeof(_settingsExt));
  
  SETTINGS_INFO(F("EXT MODE: "), (uint8_t)_settingsExt.Mode);  
  SETTINGS_INFO(F("EXT Souvenir MODE: "), (uint8_t)_settingsExt.SouvenirMode);
  SETTINGS_INFO(F("EXT NotifyChatId: "), _settingsExt.NotifyChatId);
 
  if(!res)
  {    
    _settingsExt.init();
  }
 
  return res;
}


#endif //SETTINGS_H