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

#define ALARMS_UPDATE_TIMEOUT 25000
#define ALARMS_CHECK_WITHOUT_STATUS false

#define LED_STATUS_IDX 15 //Kyivska
#define LED_STATUS_NO_CONNECTION_COLOR CRGB::Orange
#define LED_STATUS_NO_CONNECTION_PERIOD 1000
#define LED_STATUS_NO_CONNECTION_TOTALTIME -1

#define LED_PORTAL_MODE_COLOR CRGB::Green
#define LED_ALARMED_COLOR CRGB::Red
#define LED_NOT_ALARMED_COLOR CRGB::Blue

#define LED_NEW_ALARMED_PERIOD 500
#define LED_NEW_ALARMED_TOTALTIME 15000

#define LED_ALARMED_SCALE_FACTOR 0//50% 

#define EFFECT_TIMEOUT 15000
uint32_t effectStrtTicks = 0;
bool effectStarted = false;
enum Effect : uint8_t
{
  Normal,
  Rainbow,
  Strobe,

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
    CRGB PortalModeColor = LED_PORTAL_MODE_COLOR;
    CRGB NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
    CRGB NotAlarmedColor = LED_NOT_ALARMED_COLOR;
    CRGB AlarmedColor = LED_ALARMED_COLOR;
    uint8_t Brightness = 2;

    bool alarmsCheckWithoutStatus = ALARMS_CHECK_WITHOUT_STATUS;
    uint32_t alarmsUpdateTimeout = ALARMS_UPDATE_TIMEOUT;
    uint8_t alarmedScale = LED_ALARMED_SCALE_FACTOR;
    bool alarmScaleDown = true;

    int16_t resetFlag = 200;
    int reserved = 0;

    int8_t Relay1Region = -1;
    int8_t Relay2Region = -1;

    void reset()
    {
      PortalModeColor = LED_PORTAL_MODE_COLOR;
      NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
      NotAlarmedColor = LED_NOT_ALARMED_COLOR;
      AlarmedColor = LED_ALARMED_COLOR;
      Brightness = 2;

      alarmsCheckWithoutStatus = ALARMS_CHECK_WITHOUT_STATUS;
      alarmsUpdateTimeout = ALARMS_UPDATE_TIMEOUT;
      alarmedScale = LED_ALARMED_SCALE_FACTOR;
      alarmScaleDown = true;
      resetFlag = 200;
      reserved = 0;

      Relay1Region = -1;
      Relay2Region = -1;
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
    _settings.reset();
  }
  SETTINGS_INFO("BR: ", _settings.Brightness);  
  SETTINGS_INFO("resetFlag: ", _settings.resetFlag);
}


#endif //SETTINGS_H