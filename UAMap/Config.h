#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#define USE_BOT
#define USE_BUZZER
#define USE_BOT_FAST_MENU 
#define USE_BOT_INLINE_MENU

#define PRODUCT_NAME  F("UAMap")
#define PORTAL_TITLE  String(PRODUCT_NAME) + F(" ") + F("WiFi Manager")
#define AP_NAME       String(PRODUCT_NAME) + F("_AP")
#define AP_PASS       F("password")

#define BRIGHTNESS_STEP 25
//#define LARGE_MAP

#ifdef ESP32  
  #ifdef USE_BOT
    //#define USE_POWER_MONITOR
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

#define API_TOKEN_LENGTH 47
#define API_TOKEN_ID F("apiToken")

#ifndef LARGE_MAP
#define LED_STATUS_IDX 14 //UARegion::Kyivska//
#else
#define LED_STATUS_IDX 27 //UARegion::Kyivska//
#endif
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
  #define PIN_CLOCK         D0
  #define PIN_RELAY1        D1
  #define PIN_RELAY2        D2
  #define PIN_BUZZ          D3
  #define PIN_RESET_BTN     D5
  #define PIN_LED_STRIP     D6 
  #define PIN_PMONITOR_SDA  D8
  #define PIN_PMONITOR_SCL  D9 
#else //ESP32
  #define PIN_CLOCK         13
  #define PIN_RELAY1        18
  #define PIN_RELAY2        19
  #define PIN_BUZZ          21
  #define PIN_RESET_BTN     22
  #define PIN_LED_STRIP     23 
  #define PIN_PMONITOR_SDA  4
  #define PIN_PMONITOR_SCL  5  
#endif

#include <vector>
#include <map>

enum UARegion : uint8_t
{
  Khmelnitska = 3,
  Vinnytska = 4,
  Rivnenska = 5,
  Volynska = 8,
  Dnipropetrovska = 9,
  Zhytomyrska = 10,
  Zakarpatska = 11,
  Zaporizka = 12,
  Ivano_Frankivska = 13,
  Kyivska = 14,
  Kirovohradska = 15,
  Luhanska = 16,
  Mykolaivska = 17,
  Odeska = 18,
  Poltavska = 19,
  Sumska = 20,
  Ternopilska =21,
  Kharkivska = 22,
  Khersonska = 23,
  Cherkaska = 24,
  Chernihivska = 25,
  Chernivetska = 26,
  Lvivska = 27,
  Donetska = 28,
  Crimea = 29,
  Sevastopol = 30,
  Kyiv = 31,  
};

#ifndef LARGE_MAP
#define MAX_LEDS_FOR_REGION 2
typedef std::array<uint8_t, MAX_LEDS_FOR_REGION> LedRange;
typedef std::map<UARegion, LedRange> AlarmsLedIndexesMap;
static AlarmsLedIndexesMap alarmsLedIndexesMap =
{
  { UARegion::Crimea,               {0} },
  { UARegion::Khersonska,           {1} },
  { UARegion::Zaporizka,            {2} },
  { UARegion::Donetska,             {3} },
  { UARegion::Luhanska,             {4} },
  { UARegion::Kharkivska,           {5} },
  { UARegion::Dnipropetrovska,      {6} },
  { UARegion::Mykolaivska,          {7} },
  { UARegion::Odeska,               {8, 9} },
  { UARegion::Kirovohradska,        {10} },  
  { UARegion::Poltavska,            {11} },
  { UARegion::Sumska,               {12} },
  { UARegion::Chernihivska,         {13} },  
  { UARegion::Kyivska,              {14} },  
  { UARegion::Kyiv,                 {14} },
  { UARegion::Cherkaska,            {15} },
  { UARegion::Vinnytska,            {16} },  
  { UARegion::Zhytomyrska,          {17} },
  { UARegion::Rivnenska,            {18} },  
  { UARegion::Khmelnitska,          {19} },  
  { UARegion::Chernivetska,         {20} },
  { UARegion::Ivano_Frankivska,     {21} },
  { UARegion::Ternopilska,          {22} }, 
  { UARegion::Volynska,             {23} },
  { UARegion::Lvivska,              {24} },  
  { UARegion::Zakarpatska,          {25} },
};
#endif

#ifdef LARGE_MAP
#define MAX_LEDS_FOR_REGION 3
typedef std::array<uint8_t, MAX_LEDS_FOR_REGION> LedRange;
typedef std::map<UARegion, LedRange> AlarmsLedIndexesMap;
static AlarmsLedIndexesMap alarmsLedIndexesMap =
{
  { UARegion::Sevastopol,           {0} },
  { UARegion::Crimea,               {1} },  
  { UARegion::Khersonska,           {2, 3} },
  { UARegion::Zaporizka,            {4, 5} },
  { UARegion::Donetska,             {6, 7} },
  { UARegion::Luhanska,             {8, 9} },
  { UARegion::Kharkivska,           {10, 11} },
  { UARegion::Dnipropetrovska,      {12, 13} },
  { UARegion::Mykolaivska,          {14, 15} },
  { UARegion::Odeska,               {16, 17, 18} },
  { UARegion::Kirovohradska,        {19, 20} },  
  { UARegion::Poltavska,            {21, 22} },
  { UARegion::Sumska,               {23, 24} },
  { UARegion::Chernihivska,         {25, 26} },  
  { UARegion::Kyiv,                 {27} },
  { UARegion::Kyivska,              {28} },    
  { UARegion::Cherkaska,            {29, 30} },
  { UARegion::Vinnytska,            {31, 32} },  
  { UARegion::Zhytomyrska,          {33, 34} },
  { UARegion::Rivnenska,            {35, 36} },  
  { UARegion::Khmelnitska,          {37, 38} },  
  { UARegion::Chernivetska,         {39} },
  { UARegion::Ivano_Frankivska,     {40} },
  { UARegion::Ternopilska,          {41} }, 
  { UARegion::Volynska,             {42} },
  { UARegion::Lvivska,              {43, 44} },  
  { UARegion::Zakarpatska,          {45, 46} },
};
#endif

static const int getLedsCount()
{
  int res = 0;
  for(const auto &l : alarmsLedIndexesMap)
  {
    const auto &ledRange = l.second;
    for(uint8_t i = 0; i < ledRange.size(); i++)
    {
      if(i == 0 || ledRange[i] > 0)
      {
        res++;
      }
    }      
  }

  return res;
}

static const int LED_COUNT = getLedsCount();

const std::vector<uint8_t> getLedIndexesByRegion(const UARegion &region) 
{
  std::vector<uint8_t> res;
  //ALARMS_INFO(F("LED Map Count: "), alarmsLedIndexesMap.size());

  if(alarmsLedIndexesMap.count(region) > 0)
  {
    const auto &ledRange = alarmsLedIndexesMap[region];        
    for(uint8_t i = 0; i < ledRange.size(); i++)
    {
      if(i == 0 || ledRange[i] > 0)
      {
        res.push_back(ledRange[i]);
        //ALARMS_INFO(F(" Region: "), (uint8_t)region, F(" mapped to idx: "), ledRange[i]);      
      }
    }        
  }
  return std::move(res);
}

const std::vector<uint8_t> getLedIndexesByRegionId(const uint16_t &regionId)
{       
  auto region = (UARegion)(regionId == 9999 ? (uint16_t)UARegion::Crimea : regionId);

  return getLedIndexesByRegion(region);
}   

#endif //CONFIG_H