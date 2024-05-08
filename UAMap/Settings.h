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

uint32_t effectStartTicks = 0;
bool effectStarted = false;
enum Effect : uint8_t
{
  Normal,
  Rainbow,
  Strobe,
  Gay,
  UA,
  UAWithAnthem,

} _effect;

enum ColorSchema : uint8_t
{
  Dark,
  Light,
};

namespace UAMap
{
  class Settings
  {
    public:

    Settings(){ init(); }

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
    int reserved = 0;

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
      reserved = 0;

      Relay1Region = 0;
      Relay2Region = 0;

      BuzzTime = 0;

      strcpy(BaseUri, ALARMS_API_IOT_BASE_URI);
    }
  };
};

UAMap::Settings _settings;

void SaveSettings()
{
  SETTINGS_INFO("Save Settings...");
  File configFile = SPIFFS.open("/config.bin", "w");
  if (configFile) 
  {
    SETTINGS_INFO("Write config.bin file");
    configFile.write((byte *)&_settings, sizeof(_settings));
    configFile.close();
    return;
  }
  SETTINGS_INFO("failed to open config.bin file for writing");
}

void LoadSettings()
{
  SETTINGS_INFO("Load Settings...");
  if (SPIFFS.begin()) {
        SETTINGS_TRACE("mounted file system");
        if (SPIFFS.exists("/config.bin")) {
        File configFile = SPIFFS.open("/config.bin", "r");
        if (configFile) 
        {
          SETTINGS_INFO("Read config.bin file");    
          configFile.read((byte *)&_settings, sizeof(_settings));
          configFile.close();
        }
        else
          SETTINGS_INFO("failed to open config.bin file for reading");
    }
    else
          SETTINGS_INFO("File config.bin does not exist");
  }

  SETTINGS_INFO("BR: ", _settings.Brightness);  
  SETTINGS_INFO("resetFlag: ", _settings.resetFlag);
  SETTINGS_INFO("reserved: ", _settings.reserved);
  if(_settings.reserved != 0)
  {
    SETTINGS_INFO("Reset Settings...");
    _settings.init();
  }
  SETTINGS_INFO("BR: ", _settings.Brightness);  
  SETTINGS_INFO("resetFlag: ", _settings.resetFlag);
}


#endif //SETTINGS_H