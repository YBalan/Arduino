#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.0")
#else //ESP32
  #define VER F("1.0")
#endif

//#define RELEASE
#define DEBUG
#define VER_POSTFIX F("D")

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_BOT_MENU
#define ENABLE_TRACE_BOT_MENU

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

#define ENABLE_INFO_API
#define ENABLE_TRACE_API
#else

#define VER_POSTFIX F("R")
#define WM_NODEBUG
//#define WM_DEBUG_LEVEL 0

#endif

#define RELAY_OFF HIGH
#define RELAY_ON  LOW

//DebounceTime
#define DEBOUNCE_TIME 50

#ifdef ESP32
  #define IsESP32 true
  #include <SPIFFS.h>
#else
  #define IsESP32 false
#endif

// Platform specific
#ifdef ESP8266 
  #define ESPgetFreeContStack ESP.getFreeContStack()
  #define ESPresetHeap ESP.resetHeap()
  #define ESPresetFreeContStack ESP.resetFreeContStack()
#else //ESP32
  #define ESPgetFreeContStack F("NA")
  #define ESPresetHeap {}
  #define ESPresetFreeContStack {}
#endif

#include "Config.h"

#include <map>
#include <set>
//#include <ezButton.h>

#include "DEBUGHelper.h"
#include "ESPHelper.h"
#include "NSTAT.h"
#include "HttpApi.h"
#include "Settings.h"
#include "WiFiOps.h"
#ifdef USE_BOT
#include "TelegramBotHelper.h"
#include "TBotMenu.h"
#endif

#define LONG_PRESS_VALUE_MS 1000
#include "Button.h"

#ifdef ENABLE_INFO_MAIN
#define INFO(...) SS_TRACE(__VA_ARGS__)
#else
#define INFO(...) {}
#endif

#ifdef ENABLE_TRACE_MAIN
#define TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define TRACE(...) {}
#endif

static uint8_t debugButtonFromSerial = 0;

#define XYDJ Serial2

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);     

  XYDJ.begin(9600, SERIAL_8N1, RXD2, TXD2);
  while (!Serial2); 

  Serial.println();
  Serial.print(F("!!!! Start ")); Serial.print(PRODUCT_NAME); Serial.println(F(" !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(String(VER) + VER_POSTFIX);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  LoadSettings();
  LoadSettingsExt();  
  //_settings.reset(); 
  //return;

  WiFiOps::WiFiOps wifiOps(PORTAL_TITLE, AP_NAME, AP_PASS);

  #ifdef WM_DEBUG_LEVEL
    INFO(F("WM_DEBUG_LEVEL: "), WM_DEBUG_LEVEL);    
  #else
    INFO(F("WM_DEBUG_LEVEL: "), F("Off"));
  #endif

  wifiOps  
  #ifdef USE_BOT
  .AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;    
  
  // auto resetButtonState = resetBtn.getState();
  // INFO(F("ResetBtn: "), resetButtonState == HIGH ? F("Off") : F("On"));
   INFO(F("ResetFlag: "), _settings.resetFlag);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 /*|| resetButtonState == LOW*/);
  if(_settings.resetFlag == 1985)
  {
    _settings.resetFlag = 200;
    SaveSettings();
  }

  #ifdef USE_BOT  
  LoadChannelIDs();
  bot->setToken(wifiOps.GetParameterValueById(F("telegramToken")));  
  _botSettings.SetBotName(wifiOps.GetParameterValueById(F("telegramName")));  
  _botSettings.botSecure = wifiOps.GetParameterValueById(F("telegramSec"));
  bot->attach(HangleBotMessages);
  bot->setTextMode(FB_TEXT); 
  //bot->setPeriod(_settings.alarmsUpdateTimeout);
  bot->setLimit(1);
  bot->skipUpdates();
  #endif 
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("Portal Started: "), manager->getConfigPortalSSID());  
}

void loop()
{
  static uint32_t currentTicks = millis();
  currentTicks = millis();  

  #ifdef DEBUG
  if(XYDJ.available() > 0)
  { 
    const auto &received = XYDJ.readStringUntil('\n');
    INFO("XYDJ = ", received);     
  }

  //delay(1000);  

  if(Serial.available() > 0)
  {
    const auto &sendCommand = Serial.readString();
    debugButtonFromSerial = sendCommand.toInt();
    if(debugButtonFromSerial == 0)
    {    
      SendCommand(sendCommand);
    }
    else
    {
      TRACE(F("Serial = "), debugButtonFromSerial);
    }
  }
  #endif

  RunAndHandleHttpApi(currentTicks);

  #ifdef USE_BOT
  uint8_t botStatus = bot->tick();  
  yield(); // watchdog
  if(botStatus == 0)
  {
    // BOT_INFO(F("BOT UPDATE MANUAL: millis: "), millis(), F(" current: "), currentTicks, F(" "));
    // botStatus = bot->tickManual();
  }
  if(botStatus == 2)
  {
    BOT_INFO(F("Bot overloaded!"));
    bot->skipUpdates();
    //bot->answer(F("Bot overloaded!"), /**alert:*/ true); 
  }  
  #endif   

  HandleDebugSerialCommands();
}

const bool RunHttp(const unsigned long &currentTicks, int &httpCode, String &statusMsg)
{
  httpCode = 200;
  int status = httpCode;
  HandleErrors(httpCode, status, statusMsg);
  return true;
}

void RunAndHandleHttpApi(const unsigned long &currentTicks)
{
  // wait for WiFi connection
  int httpCode = ApiStatusCode::NO_WIFI;
  String statusMsg = F("No WiFi");  
  
  if ((WiFi.status() == WL_CONNECTED)) 
  {  
    #ifdef USE_STOPWATCH
    uint32_t sw = millis();
    #endif

    RunHttp(currentTicks, httpCode, statusMsg);    

    #ifdef USE_STOPWATCH
    sw = millis() - sw;
    #endif

    #ifdef USE_BOT
      #ifdef USE_NOTIFY
        if(_settings.notifyHttpCode == 1 || //All http codes
            (_settings.notifyHttpCode != 0  // Notify is ON
              &&(
                    (_settings.notifyHttpCode < 0 && httpCode != abs(_settings.notifyHttpCode)) //-200 means - All except 200
                  || (_settings.notifyHttpCode == httpCode)
                )
            )
          )
        {
          String notifyMessage = String(F("Notification: ")) + statusMsg + F(" ") + String(httpCode)
          #ifdef USE_STOPWATCH
          + F(" (") + String(sw) + F("ms") + F(")")
          #endif
          ;

          if(strlen(_settings.NotifyChatId) > 0)
          {
            TRACE(notifyMessage, F(" ChatId: "), _settings.NotifyChatId);
            bot->sendMessage(notifyMessage, _settings.NotifyChatId);
          }
        }
      #endif
    #endif

    #ifdef USE_STOPWATCH
      TRACE(F("API Stop watch: "), sw, F("ms..."));
    #endif
  }
  else
  {  
    #ifdef ESP32      
    WiFi.disconnect();
    WiFi.reconnect();
    delay(200);
    #endif
  }   

  FillNetworkStat(httpCode, statusMsg);
  PrintNetworkStatToSerial();   
}

void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  { 
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 130) // Format FS and reset WiFi and restart
  { 
    INFO(F("\t\t\tFormat..."));   
    SPIFFS.format();
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    debugButtonFromSerial = Serial.readString().toInt();
    TRACE(F("Serial = "), debugButtonFromSerial);
  }
}

void SendCommand(const String &command)
{
  INFO(F("Send command: "), command);
  XYDJ.print(command);    
}



