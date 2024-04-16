#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#define ALARMS_UPDATE_TIMEOUT 15000
#define ALARMS_CHECK_WITHOUT_STATUS false

#define LED_STATUS_IDX 15 //Kyivska
#define LED_STATUS_NO_CONNECTION_COLOR CRGB::Orange
#define LED_STATUS_NO_CONNECTION_PERIOD 1000
#define LED_STATUS_NO_CONNECTION_TOTALTIME -1

#define LED_PORTAL_MODE_COLOR CRGB::Green
#define LED_ALARMED_COLOR CRGB::Red
#define LED_NOT_ALARMED_COLOR CRGB::Blue

#define LED_NEW_ALARMED_PERIOD 500
#define LED_NEW_ALARMED_TOTALTIME 5500

#define LED_ALARMED_SCALE_FACTOR 50//50% 

namespace UAMap
{
  class Settings
  {
    public:
    CRGB PortalModeColor = LED_PORTAL_MODE_COLOR;
    CRGB NoConnectionColor = LED_STATUS_NO_CONNECTION_COLOR;
    CRGB NotAlarmedColor = LED_NOT_ALARMED_COLOR;
    CRGB AlarmedColor = LED_ALARMED_COLOR;
    uint8_t Brightness = 55;

    bool alarmsCheckWithoutStatus = ALARMS_CHECK_WITHOUT_STATUS;
    uint32_t alarmsUpdateTimeout = ALARMS_UPDATE_TIMEOUT;
  };
};


#endif //SETTINGS_H