#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PORTAL_TIMEOUT 60

//#define USE_BOT
#define USE_API
//#define USE_API_KEY
#define USE_BOT_FAST_MENU 
#define USE_BOT_INLINE_MENU

#define BRIGHTNESS_STEP 25

#ifdef ESP32  
  #ifdef USE_BOT
    #define USE_POWER_MONITOR
    #define SHOW_PM_FACTOR
    #define SHOW_PM_TIME
    #define USE_NOTIFY
    #define USE_RELAY_EXT
    #define USE_LEARN
  #endif
  //#define HTTP_TIMEOUT 3000
  #define LANGUAGE_UA
  #define BOT_MAX_INCOME_MSG_SIZE 5000 //should not be less because of menu action takes a lot
  #define USE_BOT_ONE_MSG_ANSWER true
  #define USE_STOPWATCH
#else  
  #if defined(DEBUG) && defined(USE_BOT)
    #define USE_NOTIFY
  #endif  
  #define USE_NOTIFY
  //#define HTTP_TIMEOUT 1000
  #define LANGUAGE_EN
  #define BOT_MAX_INCOME_MSG_SIZE 2000 //should not be less because of menu action takes a lot  
  #define USE_BOT_ONE_MSG_ANSWER false
#endif

#ifdef USE_POWER_MONITOR  
  #define PM_MENU_VOLTAGE_FIRST true
  #define PM_MENU_VOLTAGE_UNIT F("V")
  #define PM_MENU_NAME F("PM")
  #define PM_MENU_ALARM_NAME F("Alarm")
  #define PM_MENU_ALARM_DECREMENT 0.2
  #define PM_UPDATE_PERIOD 60000
  #define PM_MIN_UPDATE_PERIOD 15000 
#endif

#define EFFECT_TIMEOUT 15000
#define ALARMS_UPDATE_DEFAULT_TIMEOUT 25000
#define ALARMS_CHECK_WITHOUT_STATUS false

#define LED_STATUS_IDX 14 //Kyivska
#define LED_STATUS_NO_CONNECTION_COLOR CRGB::White
#define LED_STATUS_NO_CONNECTION_PERIOD 1000
#define LED_STATUS_NO_CONNECTION_TOTALTIME -1 //infinite

#define LED_PORTAL_MODE_COLOR CRGB::Green
#define LED_LOAD_MODE_COLOR CRGB::White
#define LED_ALARMED_COLOR CRGB::Red
#define LED_NOT_ALARMED_COLOR CRGB::Blue
#define LED_PARTIAL_ALARMED_COLOR CRGB::Yellow

#define LED_NEW_ALARMED_PERIOD 500
#define LED_NEW_ALARMED_TOTALTIME 15000

#define LED_ALARMED_SCALE_FACTOR 0//50% 

// PINS
#ifdef ESP8266
  // #define PIN_CLOCK         D0
  // #define PIN_RELAY1        D1
  // #define PIN_RELAY2        D2
  // #define PIN_BUZZ          D3
  // #define PIN_RESET_BTN     D5
  // #define PIN_LED_STRIP     D6 
  // #define PIN_PMONITOR_SDA  D8
  // #define PIN_PMONITOR_SCL  D9 
#else //ESP32
  // #define PIN_CLOCK         13
  // #define PIN_RELAY1        18
  // #define PIN_RELAY2        19
  // #define PIN_BUZZ          21
  // #define PIN_RESET_BTN     22
  // #define PIN_LED_STRIP     23 
  // #define PIN_PMONITOR_SDA  4
  // #define PIN_PMONITOR_SCL  5  

  
  #define OK_PIN 27
  #define UP_PIN 14
  #define DW_PIN 12
  #define RT_PIN 13
  #define SERVO_PIN 23
  #define PIN_LCD_SDA  21
  #define PIN_LCD_SCL  22
#endif


#endif //CONFIG_H