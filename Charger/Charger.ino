#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.0")
#else //ESP32
  #define VER F("1.28")
#endif

//#define RELEASE
//#define DEBUG

//#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG
#define VER_POSTFIX F("D")

//#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY
#undef WM_DEBUG_LEVEL

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
#else

#define VER_POSTFIX F("R")
#define WM_NODEBUG
//#define WM_DEBUG_LEVEL WM_DEBUG_SILENT

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
#include "DataStorage.h"
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

std::unique_ptr<WiFiOps::WiFiOps> wifiOps(new WiFiOps::WiFiOps(PORTAL_TITLE, AP_NAME, AP_PASS));

static uint8_t debugCommandFromSerial = 0;

#define XYDJ Serial2
Button wifiBtn(PIN_WIFI_BTN);

uint32_t storeDataTicks = 0;
uint32_t syncTimeTicks = 0;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);     

  XYDJ.begin(9600, SERIAL_8N1, RXD2, TXD2);
  while (!Serial2); 

  pinMode(PIN_WIFI_LED, OUTPUT);
  pinMode(PIN_WIFI_LED2, OUTPUT);
  setLEDs(LOW);
  wifiBtn.setDebounceTime(DEBOUNCE_TIME);  

  Serial.println();
  Serial.print(F("!!!! Start ")); Serial.print(PRODUCT_NAME); Serial.println(F(" !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(String(VER) + VER_POSTFIX);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  MountFS();

  LoadSettings();
  LoadSettingsExt();  
  //_settings.reset();
  loadMonitorChats(MONITOR_CHATS_FILE_NAME);

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
  
  uint32_t now = 0;
  if(IsWiFiOn())
  {
    INFO(F("ResetFlag: "), _settings.resetFlag);
    setLEDs(HIGH);
    wifiOps->TryToConnectOrOpenConfigPortal(CONFIG_PORTAL_TIMEOUT, /*restartAfterPortalTimeOut*/false, /*resetSettings:*/_settings.resetFlag == RESET_WIFI_FLAG /*|| resetButtonState == LOW*/);
    if(_settings.resetFlag == RESET_WIFI_FLAG)
    {
      _settings.resetFlag = 200;
      SaveSettings();
    }

    now = SyncTime();
    StartTimers();
    InitBot();
  }    

  ds->begin(_settings.shortRecord);  
  ds->setResetReason(GetResetReason(/*shortView:*/true));

  if(now > 0) ds->setDateTime(now);
  StartTimers();
  SendCommand(F("start")); 

  SendCommand(F("get"));
  ds->params.readFromXYDJ(DeviceReceive(100, F("U-")));
}

void StartTimers(){
  storeDataTicks = millis();
  syncTimeTicks = millis();
}

void setLEDs(const uint8_t &value){
  digitalWrite(PIN_WIFI_LED, value); 
  digitalWrite(PIN_WIFI_LED2, value); 
}

void toogleLEDs(){
  digitalWrite(PIN_WIFI_LED, !digitalRead(PIN_WIFI_LED)); 
  digitalWrite(PIN_WIFI_LED2, !digitalRead(PIN_WIFI_LED2)); 
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager){
  INFO(F("Portal Started: "), manager->getConfigPortalSSID()); 
  setLEDs(HIGH); 
}

const bool IsWiFiOn(){
  const auto &wifiBtnState = wifiBtn.getState(); //digitalRead(PIN_WIFI_BTN);  
  return wifiBtnState == LOW;  
}

void WiFiStatusLED(){
  setLEDs(WiFi.status() == WL_CONNECTED ? HIGH : LOW);
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

void InitBot(){
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

void loop(){
  static uint32_t currentTicks = millis(); 
  currentTicks = millis(); 

  wifiBtn.loop();

  if(wifiBtn.isPressed())
  {
    TRACE(BUTTON_IS_PRESSED_MSG, TRACE_TAB, F("WiFi"), F("Switch"));
    wifiOps->TryToConnectOrOpenConfigPortal(CONFIG_PORTAL_TIMEOUT, /*restartAfterPortalTimeOut*/false);
    const auto &now = SyncTime();   
    ds->setDateTime(now); 
    StartTimers();
    InitBot();
  }

  int httpCode = 200;
  String statusMsg = F("OK");
  RunAndHandleHttpApi(currentTicks, httpCode, statusMsg);

  if(wifiBtn.isReleased())
  {
    TRACE(BUTTON_IS_RELEASED_MSG, TRACE_TAB, F("WiFi"), F("Switch"));   
    WiFi.disconnect();
  }
  static bool saveRequired = false;
  if(XYDJ.available() > 0)  {     
    toogleLEDs();
    const auto &received = XYDJ.readStringUntil('\n');    

    if(received.startsWith(F("U"))) {
      INFO("EXT Device = ", F("'"), received, F("'"));
    }
    else if(received.startsWith(F("FALL"))) {
      INFO("EXT Device = ", F("'"), received, F("'"));
    }
    else if(received.startsWith(F("DOWN"))) {
      INFO("EXT Device = ", F("'"), received, F("'"));
    }
    else {
      saveRequired = true;      
      const bool isRelayStatusChanged = ds->updateCurrentData(received, (millis() - storeDataTicks) / 1000);            
      INFO("      XYDJ = ", F("'"), ds->writeToCsv(), F("'"), F(" "), F("WiFi"), F("Switch: "), IsWiFiOn() ? F("On") : F("Off"), F(" "), F("WiFi"), F("Status: "), statusMsg);
      if(isRelayStatusChanged){
        sendUpdateMonitorAllMenu(_settings.DeviceName);
        StartTimers();
      }
    }
  }
  
  if(Serial.available() > 0) {
    const auto &sendCommand = Serial.readString();
    TRACE(F("STR "), F("Serial = "), F("'"), sendCommand, F("'"));
    debugCommandFromSerial = sendCommand.toInt();    
    if(debugCommandFromSerial == 0)
    {    
      SendCommand(sendCommand);
    }    
  }

  //ds->currentData.setWiFiStatus(statusMsg.length() > 6 ? String(httpCode) : statusMsg);
  ds->setWiFiStatus(statusMsg.length() > 6 ? String(httpCode) : statusMsg);

  currentTicks = millis();
  const uint32_t &storeTicks = currentTicks - storeDataTicks;  
  //TRACE(TRACE_TAB, storeTicks, F(" "), currentTicks, F(" "), storeDataTicks, F(" "), _settings.storeDataTimeout);
  if(storeDataTicks > 0 && storeTicks >= _settings.storeDataTimeout)
  {    
    storeDataTicks = currentTicks;

    INFO(F("Is Save Data Required: "), saveRequired ? F("true") : F("false"));
    INFO(F("WiFi"), F("Switch: "), IsWiFiOn() ? F("On") : F("Off"), F(" "), F("WiFi"), F("Status: "), statusMsg);

    if(saveRequired)
    {      
      StoreData(storeTicks);    
      //ds->currentData.setResetReason(F("Normal"));
      ds->setResetReason(F("Normal"));
    }
    else{
      //ds->currentData.setResetReason(F("Paused")); 
      ds->setResetReason(F("Paused")); 
    }
    saveRequired = false;

    if(IsWiFiOn() && WiFi.status() == WL_CONNECTED)
      sendUpdateMonitorAllMenu(_settings.DeviceName);

    String nstatTrace;
    PrintNetworkStatistic(nstatTrace, 0);
    TRACE(nstatTrace);
  }

  if(IsWiFiOn() && WiFi.status() == WL_CONNECTED)
  {
    const uint32_t &syncTicks = currentTicks - syncTimeTicks;
    if(syncTimeTicks > 0 && syncTicks >= SYNC_TIME_TIMEOUT)
    {
      syncTimeTicks = currentTicks;      
      const auto &now = SyncTime();      
      ds->setDateTime(now); 
      StartTimers();
    }
  }

  #ifdef USE_BOT
  if(IsWiFiOn())
  {
    const uint8_t &botStatus = bot->tick();  
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
  }
  #endif   

  HandleDebugSerialCommands();
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

void StoreData(const uint32_t &storeTicks)
{
  INFO(TRACE_TAB, F("Store currentData..."));
  setLEDs(HIGH);
  String removedFile;
  ds->appendData((int)(storeTicks / 1000), removedFile);
  ds->traceToSerial();   
  String fsInfo;
  PrintFSInfo(fsInfo); 
  TRACE(fsInfo);
  if(!removedFile.isEmpty()){
    TRACE(TRACE_TAB, removedFile, F(" removed!"));
  }
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
      WiFiStatusLED();
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
      WiFiStatusLED();
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
}

void HandleDebugSerialCommands()
{
  if(debugCommandFromSerial == 1) // restart
  {
    Restart();
  }

  if(debugCommandFromSerial == 140) // Reset WiFi
  { 
    _settings.resetFlag = RESET_WIFI_FLAG;
    SaveSettings();
    Restart();
  }

  if(debugCommandFromSerial == 2) // Show currentData
  { 
    ds->traceToSerial();
    ds->traceFilesInfoToSerial();
  }

  if(debugCommandFromSerial == 3) // Extract all
  { 
    TRACE(F("Download All..."))
    String filter;

    uint32_t totalSize = 0;
    int totalRecordsCount = 0;

    uint32_t sw = millis();
    const auto &filesInfo = ds->downloadData(filter, totalRecordsCount, totalSize);    
    
    BOT_MENU_TRACE(F("Records: "), totalRecordsCount, F(" "), F("Total size: "), totalSize, F(" "), F("SW:"), millis() - sw, F("ms."));
  }

  if(debugCommandFromSerial == 4) // Remove all
  { 
    TRACE(F("Remove All..."))
    String filter;
    const auto &files = ds->removeData(filter);
    TRACE(files);
  }

  if(debugCommandFromSerial == 5) // Store currentData
  { 
    //ds->currentData.setResetReason(F("Test"));  
    //ds->currentData.setWiFiStatus(WiFi.status() == WL_CONNECTED ? F("OK") : F("NoWiFi"));
    ds->setResetReason(F("Test"));  
    ds->setWiFiStatus(WiFi.status() == WL_CONNECTED ? F("OK") : F("NoWiFi"));
    StoreData(millis() - storeDataTicks);      
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
  // if(debugCommandFromSerial == 0)
  // {    
  //   if(Serial.available() > 0)
  //   {
  //     debugCommandFromSerial = Serial.readString().toInt();
  //     TRACE(F("INT "), F("Serial = "), debugCommandFromSerial);
  //   }
  // }  
}

void SendCommand(const String &command)
{
  INFO(F("Send command: "), command);
  XYDJ.print(command);    
}

const String DeviceReceive(const int &minDelay, const String &whileNotStartWith)
{
  INFO(F("Waiting for device..."));  
  String result;  
  
  delay(minDelay);
  while(XYDJ.available() > 0)
  {     
     const auto &read = XYDJ.readStringUntil('\n');
     if(!whileNotStartWith.isEmpty() && read.startsWith(whileNotStartWith)){      
      result = read; break;            
     }else result += read + '\n';
  }  
  TRACE(result);
  return std::move(result);
}

void Restart(){
  #ifdef ESP32
  MFS.end();
  #endif
  ESP.restart();
}
