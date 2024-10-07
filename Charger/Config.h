#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#define PRODUCT_NAME  F("Charger")
#define PORTAL_TITLE  String(PRODUCT_NAME) + F("WiFi Manager")
#define AP_NAME       String(PRODUCT_NAME) + F("_AP")
#define AP_PASS       F("password")

#define STORE_DATA_TIMEOUT 60000

#define USE_BOT
#define USE_BOT_FAST_MENU 
#define USE_BOT_INLINE_MENU

#ifdef ESP32  
  #ifdef USE_BOT
    
  #endif
  //#define USE_API
  //#define HTTP_TIMEOUT 3000
  #define LANGUAGE_UA
  #define BOT_MAX_INCOME_MSG_SIZE 5000 //should not be less because of menu action takes a lot
  #define USE_BOT_ONE_MSG_ANSWER true
  #define USE_STOPWATCH
  #define USE_NOTIFY
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

// PINS
#ifdef ESP8266    
  #define RXD2 D8
  #define TXD2 D7
  #define PIN_WIFI_BTN  D2
#else //ESP32
  #define RXD2 16
  #define TXD2 17
  #define PIN_WIFI_BTN  15
  #define PIN_WIFI_LED_BTN  12
#endif



#endif //CONFIG_H