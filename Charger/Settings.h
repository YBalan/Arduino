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


    int16_t resetFlag = 200;
    int notifyHttpCode = 0;   
    

    void init()
    {      
      resetFlag = 200;
      notifyHttpCode = 0;      
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

  #define SETTINGS_RELAY_EXT_REGIONS_COUNT 26  
  class SettingsRelayExt
  {
    #ifdef USE_RELAY_EXT
    uint8_t Relay1[SETTINGS_RELAY_EXT_REGIONS_COUNT];
    uint8_t Relay2[SETTINGS_RELAY_EXT_REGIONS_COUNT];
    #endif
    public:
      const bool IsRelay1Contains(const uint8_t &regionId) const;
      const bool IsRelay2Contains(const uint8_t &regionId) const;
      const bool IsRelay1Off() const;
      const bool IsRelay2Off() const;
      const String GetRelay1Str(std::function<const String(const uint8_t&)> toStr) const;
      const String GetRelay2Str(std::function<const String(const uint8_t&)> toStr) const;
      const bool ClearRelay1();
      const bool ClearRelay2();
      const bool SetRelay1(const uint8_t &regionId, const bool &removeIfExist);
      const bool SetRelay2(const uint8_t &regionId, const bool &removeIfExist);
      void Init();
    private:
      const bool ClearRelaySet(uint8_t *const relaySet);
      const bool IsRelaySetContains(const uint8_t *const relaySet, const uint8_t &regionId, const bool &forAll = false) const;
      const bool AddRelay(uint8_t *const relaySet, const uint8_t &regionId, const bool &removeIfExist);
      const String GetRelayStr(const uint8_t *const relaySet, std::function<const String(const uint8_t&)> toStr) const;
  };
};

UAMap::Settings _settings;
UAMap::SettingsExt _settingsExt;
UAMap::SettingsRelayExt _settingsRelayExt;

#ifdef USE_RELAY_EXT
bool useRelayExt = true;
#else
bool useRelayExt = false;
#endif

const bool IsRelay1Off()
{
  return !useRelayExt ? _settings.Relay1Region == 0 : _settingsRelayExt.IsRelay1Off();    
}

const bool IsRelay1Contains(const uint8_t &regionId)
{
  return !useRelayExt ? _settings.Relay1Region == regionId : _settingsRelayExt.IsRelay1Contains(regionId);    
}

const bool SetRelay1(const uint8_t &regionId, const bool &removeIfExist)
{
  bool res = _settings.Relay1Region == regionId;
  _settings.Relay1Region = regionId;
  return !useRelayExt ? res : _settingsRelayExt.SetRelay1(regionId, removeIfExist);
}

const String GetRelay1Str(std::function<const String(const uint8_t&)> toStr)
{
  return !useRelayExt 
        ? toStr != nullptr ? toStr(_settings.Relay1Region) : String(_settings.Relay1Region) 
        : _settingsRelayExt.GetRelay1Str(toStr);
}

const bool IsRelay2Off()
{
  return !useRelayExt ? _settings.Relay2Region == 0 : _settingsRelayExt.IsRelay2Off();    
}

const bool IsRelay2Contains(const uint8_t &regionId)
{
  return !useRelayExt ? _settings.Relay2Region == regionId : _settingsRelayExt.IsRelay2Contains(regionId);    
}

const bool SetRelay2(const uint8_t &regionId, const bool &removeIfExist)
{
  bool res = _settings.Relay2Region == regionId;
  _settings.Relay2Region = regionId;
  return !useRelayExt ? res : _settingsRelayExt.SetRelay2(regionId, removeIfExist);
}

const String GetRelay2Str(std::function<const String(const uint8_t&)> toStr)
{
  return !useRelayExt 
    ? toStr != nullptr ? toStr(_settings.Relay2Region) : String(_settings.Relay2Region) 
    : _settingsRelayExt.GetRelay2Str(toStr);
}

const bool IsRelaysContains(const uint8_t &regionId)
{
  return IsRelay1Contains(regionId) || IsRelay2Contains(regionId);
}

const bool UAMap::SettingsRelayExt::IsRelay1Contains(const uint8_t &regionId) const
{
  #ifdef USE_RELAY_EXT
  return IsRelaySetContains(Relay1, regionId);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::IsRelay2Contains(const uint8_t &regionId) const
{
  #ifdef USE_RELAY_EXT
  return IsRelaySetContains(Relay2, regionId);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::IsRelay1Off() const
{
  #ifdef USE_RELAY_EXT
  return IsRelaySetContains(Relay1, 0, /*forAll:*/true);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::IsRelay2Off() const
{
  #ifdef USE_RELAY_EXT
  return IsRelaySetContains(Relay2, 0, /*forAll:*/true);
  #endif
  return false;
}

const String UAMap::SettingsRelayExt::GetRelay1Str(std::function<const String(const uint8_t&)> toStr) const
{
  #ifdef USE_RELAY_EXT
  return GetRelayStr(Relay1, toStr);
  #endif
  return F("");
}

const String UAMap::SettingsRelayExt::GetRelay2Str(std::function<const String(const uint8_t&)> toStr) const
{
  #ifdef USE_RELAY_EXT
  return GetRelayStr(Relay2, toStr);
  #endif
  return F("");
}

const bool UAMap::SettingsRelayExt::SetRelay1(const uint8_t &regionId, const bool &removeIfExist)
{
  #ifdef USE_RELAY_EXT
  return regionId == 0 ? ClearRelay1() : AddRelay(Relay1, regionId, removeIfExist);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::SetRelay2(const uint8_t &regionId, const bool &removeIfExist)
{
  #ifdef USE_RELAY_EXT
  return regionId == 0 ? ClearRelay2() : AddRelay(Relay2, regionId, removeIfExist);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::ClearRelay1()
{
  #ifdef USE_RELAY_EXT
  return ClearRelaySet(Relay1);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::ClearRelay2()
{
  #ifdef USE_RELAY_EXT
  return ClearRelaySet(Relay2);
  #endif
  return false;
}

const bool UAMap::SettingsRelayExt::ClearRelaySet(uint8_t *relaySet)
{
  for(uint8_t i = 0; i < SETTINGS_RELAY_EXT_REGIONS_COUNT; i++) relaySet[i] = 0;
  return true;
}

void UAMap::SettingsRelayExt::Init()
{
  #ifdef USE_RELAY_EXT
  ClearRelaySet(Relay1);
  ClearRelaySet(Relay2);
  #endif
}

const bool UAMap::SettingsRelayExt::IsRelaySetContains(const uint8_t *const relaySet, const uint8_t &regionId, const bool &forAll) const
{
  for(uint8_t i = 0; i < SETTINGS_RELAY_EXT_REGIONS_COUNT; i++)
  {
    if(forAll)
    {
      if(relaySet[i] != regionId)
        return false;
    }
    else
    {
      if(relaySet[i] == regionId)
        return true;
    }
  }
  return forAll ? true : false;
}

const bool UAMap::SettingsRelayExt::AddRelay(uint8_t *const relaySet, const uint8_t &regionId, const bool &removeIfExist)
{
  bool exists = false;
  int8_t firstZeroIdx = -1;
  for(uint8_t i = 0; i < SETTINGS_RELAY_EXT_REGIONS_COUNT; i++) 
  {
    if(relaySet[i] == 0 && firstZeroIdx < 0)
      firstZeroIdx = i;
    if(relaySet[i] == regionId)
    {
      exists = true;
      if(removeIfExist) relaySet[i] = 0;
      //break;
    }    
  }
  if(!exists)
  {
    relaySet[firstZeroIdx < 0 ? 0 : firstZeroIdx] = regionId;
  }
  return exists;
}

const String UAMap::SettingsRelayExt::GetRelayStr(const uint8_t *const relaySet, std::function<const String(const uint8_t&)> toStr) const
{
  String res;
  for(uint8_t i = 0; i < SETTINGS_RELAY_EXT_REGIONS_COUNT; i++) 
  {
    if(toStr != nullptr && relaySet[i] == 0) continue;
    res += (toStr != nullptr ? toStr(relaySet[i]) : String(relaySet[i])) + (i==SETTINGS_RELAY_EXT_REGIONS_COUNT-1 ? F("") : F(", "));    
  }
  return res;
}

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

const bool SaveSettingsRelayExt()
{
  String fileName = F("/configRelayExt.bin");
  SETTINGS_INFO(F("RELAY EXT: "), F("Save Settings to: "), fileName);
  if(SaveFile(fileName.c_str(), (byte *)&_settingsRelayExt, sizeof(_settingsRelayExt)))
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

const bool LoadSettingsRelayExt()
{
  String fileName = F("/configRelayExt.bin");
  SETTINGS_INFO(F("RELAY EXT: "), F("Load Settings from: "), fileName);

  const bool &res = LoadFile(fileName.c_str(), (byte *)&_settingsRelayExt, sizeof(_settingsRelayExt));
  
  SETTINGS_INFO(F("RELAY EXT: "), F(" 1: "), _settingsRelayExt.GetRelay1Str(nullptr));  
  SETTINGS_INFO(F("RELAY EXT: "), F(" 2: "), _settingsRelayExt.GetRelay2Str(nullptr));  
 
  if(!res)
  {    
    _settingsRelayExt.Init();
  }
 
  return res;
}


#endif //SETTINGS_H