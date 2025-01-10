#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.0")
#else //ESP32
  #define VER F("1.10")
#endif

//#define RELEASE
#define DEBUG
#define VER_POSTFIX F("D")

//#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_DS
#define ENABLE_TRACE_DS

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_BOT_MENU
#define ENABLE_TRACE_BOT_MENU

#define ENABLE_INFO_WIFI
//#define ENABLE_TRACE_WIFI

#define ENABLE_INFO_API
#define ENABLE_TRACE_API

#define ENABLE_INFO_BUZZ
#define ENABLE_TRACE_BUZZ
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
#include "BuzzHelper.h"

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

std::unique_ptr<WiFiOps::WiFiOps> wifiOps(new WiFiOps::WiFiOps(PORTAL_TITLE, AP_NAME, AP_PASS));

static uint8_t debugCommandFromSerial = 0;

Button wifiBtn(PIN_WIFI_BTN);
ezButton notifyBtn(PIN_NOTIFY);

uint32_t storeDataTicks = 0;
uint32_t syncTimeTicks = 0;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);     

  //pinMode(PIN_NOTIFY, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  wifiBtn.setDebounceTime(DEBOUNCE_TIME);  
  notifyBtn.setDebounceTime(DEBOUNCE_TIME);  

  Serial.println();
  Serial.print(F("!!!! Start ")); Serial.print(PRODUCT_NAME); Serial.println(F(" !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(String(VER) + VER_POSTFIX);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  MountFS();
  //listDir(MFS, "/", 3);  

  LoadSettings();
  LoadSettingsExt();    
  loadSubscribedChats(SUBSCRIBED_CHATS_FILE_NAME);

  //listDir(MFS, "/", 3); 

  #ifdef WM_DEBUG_LEVEL
    INFO(F("WM_DEBUG_LEVEL: "), WM_DEBUG_LEVEL);    
  #else
    INFO(F("WM_DEBUG_LEVEL: "), F("Off"));
  #endif

  wifiOps  
  #ifdef USE_BOT
  ->AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;

  //listDir(MFS, "/", 3);
  
  if(IsWiFiOn())
  {    
    INFO(F("ResetFlag: "), _settings.resetFlag);
    wifiOps->TryToConnectOrOpenConfigPortal(CONFIG_PORTAL_TIMEOUT, /*restartAfterPortalTimeOut*/false, /*resetSettings:*/_settings.resetFlag == RESET_WIFI_FLAG /*|| resetButtonState == LOW*/);
    if(_settings.resetFlag == RESET_WIFI_FLAG)
    {
      _settings.resetFlag = 200;
      SaveSettings();
    }

    const auto &now = SyncTime();
    TRACE(F("Current epochTime: "), now);     

    InitBot();
  } 

  //listDir(MFS, "/", 3);

  StartTimers();

  if(_settings.notifyOnline){
    if(IsWiFiOn() && WiFi.status() == WL_CONNECTED){
      #ifdef USE_BOT
      sendToAllSubscribers(ONLINE_MSG);
      #endif
    }
  }
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("Portal Started: "), manager->getConfigPortalSSID());  
}

const bool IsWiFiOn()
{
  const auto &wifiBtnState = wifiBtn.getState(); //digitalRead(PIN_WIFI_BTN);  
  return wifiBtnState == HIGH;  
}

void InitBot()
{
  #ifdef USE_BOT  
  LoadChannelIDs();
  if(WiFi.status() == WL_CONNECTED){
    bot->setToken(wifiOps->GetParameterValueById(F("telegramToken")));  
    _botSettings.SetBotName(wifiOps->GetParameterValueById(F("telegramName")));  
    _botSettings.botSecure = wifiOps->GetParameterValueById(F("telegramSec"));
    bot->attach(HangleBotMessages);
    bot->setTextMode(FB_TEXT); 
    //bot->setPeriod(_settings.alarmsUpdateTimeout);
    bot->setLimit(1);
    bot->skipUpdates();
  }
  #endif
}

const bool isNotifyPinChanged(){
  const auto val = notifyBtn.getState();//digitalRead(PIN_NOTIFY);
  
  if(_settings.notifyPinPrevValue >= 0){
    if(_settings.notifyPinPrevValue != val){
      _settings.notifyPinPrevValue = val;
      _settings.notifyPinCounter += 1;
      return true;
    }
  }
  _settings.notifyPinPrevValue = val;  
  return false;
}

void loop()
{
  //TRACE(TRACE_TAB, F("Loop!!!"));

  static uint32_t currentTicks = millis(); 
  currentTicks = millis(); 

  wifiBtn.loop();
  notifyBtn.loop();

  if(wifiBtn.isPressed())
  {
    TRACE(BUTTON_IS_PRESSED_MSG, TRACE_TAB, F("WiFi"), F("Switch"));
    wifiOps->TryToConnectOrOpenConfigPortal(CONFIG_PORTAL_TIMEOUT, /*restartAfterPortalTimeOut*/false);
    const auto &now = SyncTime();
    TRACE(F("Current epochTime: "), now);
     
    InitBot();
  }

  if(wifiBtn.isReleased())
  {
    TRACE(BUTTON_IS_RELEASED_MSG, TRACE_TAB, F("WiFi"), F("Switch"));
    WiFi.disconnect();
  }     

  int httpCode = 200;
  String statusMsg = F("OK");
  RunAndHandleHttpApi(currentTicks, httpCode, statusMsg);

  const uint32_t &ticks = currentTicks - storeDataTicks;  
  if(storeDataTicks > 0 && ticks >= (_settings.storeDataTimeout == 0 ? STORE_DATA_TIMEOUT : _settings.storeDataTimeout)){
    storeDataTicks = currentTicks;

    //INFO(F("Store data timeout: "), String(_settings.storeDataTimeout), F(" "), F("WiFi is: "), IsWiFiOn() ? F("On") : F("Off"));    
    //StoreData(ticks);        

    //TRACE(TRACE_TAB, F("220V Status: "), getStatus());    
  }

  if(isNotifyPinChanged()){      
      if(_settings.notifyPinCountBefore == _settings.notifyPinCounter){
        _settings.notifyPinCounter = 0;
        _settings.notifyPinPrevValue = -1;
        //SendMessages        

        const auto &status = getStatus();
        TRACE(TRACE_TAB, F("220V Status: "), status);

        if(IsWiFiOn() && WiFi.status() == WL_CONNECTED){        
          sendToAllSubscribers(status);
        }        
        playNotify();
      }else{
        //SendMessages
      } 
      SaveSettings();     
    }

  if(IsWiFiOn() && WiFi.status() == WL_CONNECTED){
    const uint32_t &syncTicks = currentTicks - syncTimeTicks;
    if(syncTimeTicks > 0 && syncTicks >= SYNC_TIME_TIMEOUT){
      syncTimeTicks = currentTicks;      
      const auto &now = SyncTime();      
      StartTimers();      
    }    
  }

  #ifdef USE_BOT
  if(IsWiFiOn()){
    const uint8_t &botStatus = bot->tick();  
    yield(); // watchdog
    if(botStatus == 0){
      // BOT_INFO(F("BOT UPDATE MANUAL: millis: "), millis(), F(" current: "), currentTicks, F(" "));
      // botStatus = bot->tickManual();
    }
    if(botStatus == 2){
      BOT_INFO(F("Bot overloaded!"));
      bot->skipUpdates();
      //bot->answer(F("Bot overloaded!"), /**alert:*/ true); 
    }  
  }
  #endif   

  HandleDebugSerialCommands();
}

const String getStatus(){
  auto pinValue = notifyBtn.getState();//digitalRead(PIN_NOTIFY);
  TRACE(F("pinValue: "), pinValue);
  return pinValue == LOW ? F("ON") : F("OFF");
}

void playNotify(){  
  Buzz::PlayMelody(PIN_BUZZER, _settingsExt.Buzz);
}

void StoreData(const uint32_t &ticks)
{
  TRACE(F("Store data..."));  
  String fsInfo;
  PrintFSInfo(fsInfo); 
  TRACE(fsInfo);
}

const uint32_t SyncTime()
{
  if(WiFi.status() == WL_CONNECTED){
    const auto &now = GetCurrentTime(_settings.timeZone);
    TRACE(F("Synced time: "), F(" "), F(" epoch:"), now, F(" "), F("dateTime: "), epochToDateTime(now));
    return now;
  }
  return 0;
}

void StartTimers(){
  storeDataTicks = millis();
  syncTimeTicks = millis();
}

void HandleDebugSerialCommands()
{
  if(debugCommandFromSerial == 1) // restart
  { 
    // _settings.resetFlag = 1985;
    // SaveSettings();
    Restart();
  }

  if(debugCommandFromSerial == 140) // Reset WiFi
  { 
    _settings.resetFlag = RESET_WIFI_FLAG;
    SaveSettings();
    Restart();
  }

  if(debugCommandFromSerial == 2) // Show current status
  { 
    TRACE(TRACE_TAB, F("220V Status: "), getStatus());
    _settings.trace();
    _settingsExt.trace();

    String fsInfo; PrintFSInfo(fsInfo); TRACE(fsInfo);
  }

  if(debugCommandFromSerial == 3) // Init all
  { 
    _settings.init();
    _settingsExt.init();
    
    _settings.trace();
    _settingsExt.trace();

    SaveSettings();
    SaveSettingsExt();
  }

  if(debugCommandFromSerial == 4) // Show FS tree
  { 
    MountFS();
    listDir(MFS, "/", 3);
  }

  if(debugCommandFromSerial == 5) // Store 
  { 
   
  }

  if(debugCommandFromSerial == 130) // Format FS and reset WiFi and restart
  { 
    INFO(TRACE_TAB, F("Format..."));   
    MFS.format();
    _settings.resetFlag = RESET_WIFI_FLAG;
    SaveSettings();
    Restart();
  }

  debugCommandFromSerial = 0;
  if(Serial.available() > 0)
  {
    const auto &readFromSerial = Serial.readString();
    INFO(F("Input: "), readFromSerial);
    debugCommandFromSerial = readFromSerial.toInt(); 
  }  
}

void MountFS(){
  #ifdef LITTLEFS
  INFO(F("Mount FS"), F(": "), F("LittleFS"));
  #else
  INFO(F("Mount FS"), F(": "), F("SPIFFS"));
  #endif
  if(!MFS.begin(/*formatOnFail:*/true)){
    #ifdef LITTLEFS
    INFO(F("Failed to mount FS"), F(": "), F("LittleFS"));
    #else
    INFO(F("Failed to mount FS"), F(": "), F("SPIFFS"));
    #endif
    delay(5000);
    Restart();
  }
}

void Restart(){
  #ifdef ESP32
  MFS.end();
  #endif
  ESP.restart();
}

const bool RunHttp(const unsigned long &currentTicks, int &httpCode, String &statusMsg)
{
  httpCode = 200;
  int status = httpCode;
  statusMsg = F("OK"); 
  #ifdef USE_API
  HandleErrors(httpCode, status, statusMsg);
  #endif //USE_API
  return true;
}

void RunAndHandleHttpApi(const unsigned long &currentTicks, int &httpCode, String &statusMsg)
{  
  httpCode = 1002;
  statusMsg = F("NoWiFi");  
  
  if(IsWiFiOn())
  {    
    if ((WiFi.status() == WL_CONNECTED)) 
    {
      httpCode = 200;
      statusMsg = F("OK"); 
      #ifdef USE_API      
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
        #endif //USE_NOTIFY
      #endif //USE_BOT

      #ifdef USE_STOPWATCH
        TRACE(F("API Stop watch: "), sw, F("ms..."));
      #endif
      #endif //USE_API
    }
    else
    {  
      #ifdef ESP32     
      //WiFiStatusLED();
      //TRACE(TRACE_TAB, F("RECONNECT"), F(" "), F("Status: "), WiFi.status());       
      if(WiFi.status() == WL_CONNECT_FAILED  ||  WiFi.status() == WL_CONNECTION_LOST)
      {        
        TRACE(TRACE_TAB, F("RECONNECT"));
        WiFi.reconnect();
        delay(200);
      }
      #endif
    }   
  }
  FillNetworkStat(httpCode, statusMsg);
  PrintNetworkStatToSerial();   
}