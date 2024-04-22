#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <map>

#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#include <ezButton.h>

#define VER 1.2
//#define RELEASE
#define DEBUG

#ifdef DEBUG
#define NETWORK_STATISTIC
#define ENABLE_TRACE

#define ENABLE_INFO_MAIN
//#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
//#define ENABLE_TRACE_SETTINGS

//#define ENABLE_INFO_BOT
//#define ENABLE_TRACE_BOT

//#define ENABLE_INFO_ALARMS
//#define ENABLE_TRACE_ALARMS

//#define ENABLE_INFO_WIFI
//#define ENABLE_TRACE_WIFI
#endif

#define USE_BOT

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "LedState.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBot.h"

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

#ifdef NETWORK_STATISTIC
struct NetworkStatInfo{ int code; int count; String description; };
std::map<int, NetworkStatInfo> networkStat;
#endif

#define RELAY_OFF HIGH
#define RELAY_ON  LOW

#define PIN_RELAY1    D1
#define PIN_RELAY2    D2
#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 26
#define BRIGHTNESS_STEP 10

//DebounceTime
#define DebounceTime 50

std::unique_ptr<AlarmsApi> api(new AlarmsApi());
CRGBArray<LED_COUNT> leds;

std::map<uint16_t, RegionInfo> alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

int32_t menuID = 0;
byte depth = 0;

ezButton resetBtn(PIN_RESET_BTN);

void SetLedState(const UARegion &region, LedState &state)
{
  for(const auto &idx : AlarmsApi::getLedIndexesByRegion(region))
  {
    state.Idx = idx;
    state.IsAlarmed = ledsState[idx].IsAlarmed;
    ledsState[idx] = state;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);   

  pinMode(PIN_RELAY1, INPUT_PULLUP);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, INPUT_PULLUP);
  pinMode(PIN_RELAY2, OUTPUT);

  digitalWrite(PIN_RELAY1, RELAY_OFF);
  digitalWrite(PIN_RELAY2, RELAY_OFF);

  resetBtn.setDebounceTime(DebounceTime);

  WiFiOps::WiFiOps<3> wifiOps("UAMap WiFi Manager", "UAMapAP", "password");

  Serial.println();
  Serial.println("!!!! Start UA Map !!!!");
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);
  Serial.print(" HEAP: "); Serial.println(ESP.getFreeHeap());
  Serial.print("STACK: "); Serial.println(ESP.getFreeContStack());  

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, _settings.PortalModeColor};

  wifiOps
  .AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN", 42)  
  .AddParameter("telegramToken", "Telegram Bot Token", "telegram_token", "TELEGRAM_TOKEN", 47)
  .AddParameter("telegramName", "Telegram Bot Name", "telegram_name", "@telegram_bot", 50)
  .AddParameter("telegramSec", "Telegram Bot Security", "telegram_sec", "SECURE_STRING", 30)
  ;  

  LoadSettings();
  //_settings.reset();

  auto resetButtonState = resetBtn.getState();
  INFO("ResetBtn: ", resetButtonState);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  _settings.resetFlag = 200;
  api->setApiKey(wifiOps.GetParameterValueById("apiToken"));  

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.clear();  

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 1000, -1, false, false, _settings.NoConnectionColor};
  ledsState[0] = {0 /*Crymea*/, 0, 0, true, false, _settings.AlarmedColor};
  ledsState[4] = {4 /*Luh*/, 0, 0, true, false, _settings.AlarmedColor};

  SetAlarmedLED(alarmedLedIdx);
  CheckAndUpdateRealLeds(millis());
  SetBrightness(); 

  #ifdef USE_BOT
  //bot.setChatID(CHAT_ID);
  LoadChannelIDs();
  bot->setToken(wifiOps.GetParameterValueById("telegramToken"));  
  _botSettings.SetBotName(wifiOps.GetParameterValueById("telegramName"));  
  _botSettings.botSecure = wifiOps.GetParameterValueById("telegramSec");
  bot->attach(HangleBotMessages);
  bot->setTextMode(FB_MARKDOWN); 
  // String menu1 = F("Alarms \n Max Br \t Min Br \n Dark \t Light \n Strobe \t Rainbow");
  // String call1 = F("/alarms, /br 255, /br 2, /schema 0, /schema 1, /strobe, /rainbow");
  // bot->inlineMenuCallback("Menu", menu1, call1);
  // menuID = bot->lastBotMsg();
  bot->skipUpdates();
  #endif
}

#ifdef USE_BOT
#define BOT_COMMAND_BR "/br"
#define BOT_COMMAND_RESET "/reset"
#define BOT_COMMAND_RAINBOW "/rainbow"
#define BOT_COMMAND_STROBE "/strobe"
#define BOT_COMMAND_SCHEMA "/schema"
#define BOT_COMMAND_TEST "/test"
#define BOT_COMMAND_ALARMS "/alarms"
#define BOT_COMMAND_MENU "/menu"
#define BOT_COMMAND_RELAY1 "/relay1"
#define BOT_COMMAND_RELAY2 "/relay2"
const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered)
{   
  std::vector<String> messages;
  String value;
  bool answerCurrentAlarms = false;

  bool hasData = msg.data.length() > 0 && filtered == _botSettings.botNameForMenu;
  BOT_TRACE("Filtered: ", filtered);
  filtered = hasData ? msg.data : filtered;
  BOT_TRACE("Filtered: ", filtered);

  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  {    
    String menu1 = F("Alarms \n Brightness Max \t Brightness Mid \t Brightness Min \n Dark \t Light \n Strobe \t Rainbow");
    String call1 = F("/alarms, /br 255, /br 120, /br 2, /schema 0, /schema 1, /strobe, /rainbow"); 
    bot->inlineMenuCallback(_botSettings.botNameForMenu, menu1, call1, msg.chatID);
    menuID = bot->lastBotMsg();
  } else
  if(GetCommandValue(BOT_COMMAND_BR, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto br = value.toInt();
    br = br <= 0 ? 1 : (br > 255 ? 255 : br);
    //if(br > 0 && br <= 255)
       _settings.Brightness = br;

    SetBrightness();      
    value = String("Brightness changed to: ") + String(br);
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_RESET, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    ESP.resetHeap();
    ESP.resetFreeContStack();

    INFO(" HEAP: ", ESP.getFreeHeap());
    INFO("STACK: ", ESP.getFreeContStack());

    _settings.reset();
    _settings.alarmsCheckWithoutStatus = true;    

    value = String("Reseted: Heap: ") + String(ESP.getFreeHeap()) + " Stack: " + String(ESP.getFreeContStack());
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY1, filtered, value))
  {
    auto regionId = value.toInt();
    if(regionId == -1 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
    {
      _settings.Relay1Region = regionId;
    }
    value = "Relay1: " + String(_settings.Relay1Region);
    SaveSettings();
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    auto regionId = value.toInt();
    if(regionId == -1 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
    {
      _settings.Relay2Region = regionId;
    }
    value = "Relay2: " + String(_settings.Relay2Region);
    SaveSettings();
  }else
  if(GetCommandValue(BOT_COMMAND_RAINBOW, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    value = "Rainbow started...";
    _effect = Effect::Rainbow;   
    effectStrtTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_STROBE, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    value = "Strobe started...";
    _effect = Effect::Strobe;   
    effectStrtTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_TEST, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    bot->sendMessage("Working...", msg.chatID);
    int status;
    String statusMsg;
    auto regions = api->getAlarmedRegions2(status, statusMsg, ALARMS_API_REGIONS);
    Serial.println();
    value = String("All regions count: ") + String(regions.size());// + " status: " + String(status) + " msg: " + statusMsg;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_ALARMS, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    answerCurrentAlarms = true;
  }else
  if(GetCommandValue(BOT_COMMAND_SCHEMA, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    uint8_t schema = value.toInt();
    switch(schema)
    {
      case ColorSchema::Light:
        _settings.AlarmedColor = CRGB::Red;
        _settings.NotAlarmedColor = CRGB::Yellow;
        value = "Light";
      break;
      case ColorSchema::Dark: 
      default:
        _settings.AlarmedColor = LED_ALARMED_COLOR;
        _settings.NotAlarmedColor = LED_NOT_ALARMED_COLOR;
        value = "Dark";
      break;
    }

    SetAlarmedLED(alarmedLedIdx);
    SetBrightness();

    value = String("Color Chema changed to: ") + value;
    answerCurrentAlarms = false;
  }

  if(value != "" && !hasData)
      messages.push_back(value);

  if(answerCurrentAlarms)
  {
    String answer = String("Alarmed regions count: ") + String(alarmedLedIdx.size());
     
    messages.push_back(answer);   
    
    for(const auto &regionKvp : alarmedLedIdx)
    {
      const auto &region = regionKvp.second;
      String regionMsg = region.Name + ": [" + String(region.Id) + "]";
      messages.push_back(regionMsg);
    } 
  }

  if(messages.size() == 0)
  {
    bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}
#endif

void loop() 
{
  static bool firstRun = true;
  static uint32_t currentTicks = millis();
  currentTicks = millis();

  resetBtn.loop();

  if(resetBtn.isPressed())
  {
    INFO(BUTTON_IS_PRESSED_MSG, " BR: ", _settings.Brightness);
  }
  if(resetBtn.isReleased())
  {    
    _settings.Brightness -= BRIGHTNESS_STEP;
    INFO(BUTTON_IS_RELEASED_MSG, " BR: ", _settings.Brightness);
    SetBrightness();    
  }

  HandleEffects(currentTicks);
 
  if(_effect == Effect::Normal)
  {    
    if(CheckAndUpdateAlarms(currentTicks))
    {
      //FastLEDShow(true);
    }    
    if(CheckAndUpdateRealLeds(currentTicks))
    {
      //FastLEDShow(false);
    }
  }  
  
  FastLEDShow(false); 

  HandleDebugSerialCommands();    

  #ifdef USE_BOT
  bot->tick();
  #endif

  firstRun = false;
}

const bool CheckAndUpdateRealLeds(const unsigned long &currentTicks)
{
  //INFO("CheckStatuses");

  bool changed = false;
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;

    if(led.Idx < 0 && led.Idx >= LED_COUNT) break;

    //TRACE("Led idx: ", led.Idx, " Total timeout: ", led.BlinkTotalTime, " Period timeout: ", led.BlinkPeriod, " current: ", currentTicks);

    if(led.TotalTimeTicks == 0 && led.BlinkTotalTime > 0)    
      led.TotalTimeTicks = currentTicks;      
    if(led.PeriodTicks == 0 && led.BlinkPeriod != 0)
      led.PeriodTicks = currentTicks;

    if(led.BlinkTotalTime > 0 && (led.TotalTimeTicks > 0 && currentTicks - led.TotalTimeTicks >= led.BlinkTotalTime))
    { 
      led.PeriodTicks = 0;
      led.TotalTimeTicks = 0;    
      led.BlinkPeriod = 0;
      led.BlinkTotalTime = 0;
      TRACE("Led idx: ", led.Idx, " Total timeout: ", led.BlinkTotalTime, " FINISHED");
    }   

    if(led.BlinkPeriod != 0 && (led.PeriodTicks > 0 && currentTicks - led.PeriodTicks >= abs(led.BlinkPeriod)))
    { 
      leds[led.Idx] = led.BlinkPeriod > 0 ? 0 : led.Color;     
      led.BlinkPeriod *= -1;
      led.PeriodTicks = currentTicks;
      changed = true;
      TRACE("Led idx: ", led.Idx, " BLINK! : period: ", led.BlinkPeriod, " Total time: ", led.BlinkTotalTime);
    }   

    if(led.BlinkTotalTime == 0)
    {
      bool colorChanges = leds[led.Idx] == led.Color;
      if(!colorChanges)
        TRACE("Led idx: ", led.Idx, " Color Changed");
      changed = (changed || !colorChanges);
      leds[led.Idx] = led.IsAlarmed ? _settings.AlarmedColor : _settings.NotAlarmedColor;   
      //changed = true;      
    }
  }
  return changed;
}

//uint32_t alarmsTicks = 0;
const bool CheckAndUpdateAlarms(const unsigned long &currentTicks)
{  
  //INFO("CheckAndUpdateAlarms");  
  static uint32_t alarmsTicks = 0;
  bool changed = false;
  if(alarmsTicks == 0 || currentTicks - alarmsTicks >= _settings.alarmsUpdateTimeout)
  {      
    alarmsTicks = currentTicks;

    // wait for WiFi connection
    int status = AlarmsApiStatus::NO_WIFI;
    String statusMsg;
   
    if ((WiFi.status() == WL_CONNECTED)) 
    {     
      //INFO("WiFi - CONNECTED");
      //ESP.resetHeap();
      //ESP.resetFreeContStack();
      INFO(" HEAP: ", ESP.getFreeHeap());
      INFO("STACK: ", ESP.getFreeContStack());

      bool statusChanged = api->IsStatusChanged(status, statusMsg);
      if(status == AlarmsApiStatus::API_OK)
      {        
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");
        INFO("AlarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus ? "true" : "false");

        if(statusChanged || _settings.alarmsCheckWithoutStatus || ALARMS_CHECK_WITHOUT_STATUS)
        {              
          auto alarmedRegions = api->getAlarmedRegions2(status, statusMsg);    
          INFO("HTTP response Alarmed regions count: ", alarmedRegions.size());
          if(status == AlarmsApiStatus::API_OK)
          { 
            _settings.alarmsCheckWithoutStatus = false;           
            alarmedLedIdx.clear();
            for(const auto &rId : alarmedRegions)
            {
              // auto ledIdx = api->getLedIndexByRegionId(rId.first);
              // INFO("regionId:", rId.first, "\tled index: ", ledIdx);
              // alarmedLedIdx[ledIdx] = std::move(rId.second);

              auto ledRange = AlarmsApi::getLedIndexesByRegionId(rId.first);
              for(const auto &ledIdx : ledRange)
              {
                INFO("regionId:", rId.first, "\tled index: ", ledIdx);
                alarmedLedIdx[ledIdx] = std::move(rId.second);
              }
            }

            SetAlarmedLED(alarmedLedIdx);            
            changed = true;
          } 

          if(status == AlarmsApiStatus::JSON_ERROR)
          {
            _settings.alarmsCheckWithoutStatus = true;
            ESP.resetHeap();
            ESP.resetFreeContStack();
          }         
        } 

        SetRelayStatus(alarmedLedIdx);           
      }                       
    }   
    
    bool statusChanged = SetStatusLED(status, statusMsg);
    changed = (changed || statusChanged);

    INFO("");
    INFO("Alarmed regions count: ", alarmedLedIdx.size());
    INFO("Waiting ", ALARMS_UPDATE_TIMEOUT, "ms. before the next round...");
    PrintNetworkStatToSerial();
  }
  return changed;
}

void SetAlarmedLED(const std::map<uint16_t, RegionInfo> &alarmedLedIdx)
{
  TRACE("SetAlarmedLED: ", alarmedLedIdx.size());
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = ledsState[ledIdx];
    led.Idx = ledIdx;
    if(alarmedLedIdx.count(ledIdx) > 0)
    {
      if(!led.IsAlarmed)
      {
        led.Color = _settings.AlarmedColor;
        led.BlinkPeriod = LED_NEW_ALARMED_PERIOD;
        led.BlinkTotalTime = LED_NEW_ALARMED_TOTALTIME;
      }       
      led.IsAlarmed = true;      
    }
    else
    {
      led.IsAlarmed = false;
      led.Color = _settings.NotAlarmedColor;      
      led.StopBlink();
    }       

    RecalculateBrightness(led, false);    
  }  
}

void SetRelayStatus(const std::map<uint16_t, RegionInfo> &alarmedLedIdx)
{
  INFO("SetRelayStatus: Relay1: ", _settings.Relay1Region, " Relay2: ", _settings.Relay2Region); 
  
  bool found1 = false;
  bool found2 = false;
  for(const auto &idx : alarmedLedIdx)
  {
    INFO(idx.first, ": ", idx.second.Id);
  
    if(!found1 && idx.second.Id == (uint8_t)_settings.Relay1Region)
    {
      INFO("Found1: ", idx.second.Id);
      found1 = true;        
    }      
    if(!found2 && idx.second.Id == (uint8_t)_settings.Relay2Region)
    {
      INFO("Found2: ", idx.second.Id);
      found2 = true;        
    }
  }

  if(found1)
  {
    digitalWrite(PIN_RELAY1, RELAY_ON);
    INFO("Relay 1: ", "ON", " Region: ", _settings.Relay1Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY1) == RELAY_ON)
    {
      INFO("Relay 1: ", "OFF", " Region: ", _settings.Relay1Region);
      digitalWrite(PIN_RELAY1, RELAY_OFF);
    }
  }

  if(found2)
  {
    digitalWrite(PIN_RELAY2, RELAY_ON);
    INFO("Relay 2: ", "ON", " Region: ", _settings.Relay2Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY2) == RELAY_ON)
    {
      INFO("Relay 2: ", "OFF", " Region: ", _settings.Relay2Region);
      digitalWrite(PIN_RELAY2, RELAY_OFF);
    }
  }    
  
}

const uint8_t GetScaledBrightness(const uint8_t& brScale, const bool& scaleDown)
{
  auto scale = (uint8_t)(float(_settings.Brightness / 100.0) * (float)brScale);
  //int res = _settings.Brightness + (scaleDown ? -scale : scale);
  int res = _settings.Brightness + (scaleDown ? -scale : scale);
  return res < 0 ? 0 : (res > 255 ? 255 : res);
}

void RecalculateBrightness(LedState &led, const bool &showTrace)
{
  if(_settings.alarmedScale > 0)
  {
    if(led.IsAlarmed && led.FixedBrightnessIfAlarmed)
    {
      auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      if(showTrace)
         INFO("Led: ", led.Idx, " Br: ", br);
    }
      // if(_settings.alarmScaleDown)
      // {
      //   if(!led.IsAlarmed || led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE("Led: ", led.Idx, " Br: ", br);
      //   }
      // }
      // else
      // {
      //   if(led.IsAlarmed && !led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE("Led: ", led.Idx, " Br: ", br);
      //   } 
      // }
    }
}

void RecalculateBrightness()
{
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;
    RecalculateBrightness(led, true);
  }
}

//int prevStatus = AlarmsApiStatus::UNKNOWN;
const bool SetStatusLED(const int &status, const String &msg)
{
  static int prevStatus = AlarmsApiStatus::UNKNOWN;
  bool changed = prevStatus != status;
  prevStatus = status;

  FillNetworkStat(status, status != AlarmsApiStatus::API_OK ? msg : "OK (200)");
  auto &led = ledsState[LED_STATUS_IDX];
  if(status != AlarmsApiStatus::API_OK)
  {
    INFO("Status: ", status == AlarmsApiStatus::WRONG_API_KEY ? "Unauthorized" : (status == AlarmsApiStatus::NO_WIFI ? "No WiFi" : "No Connection"), " | ", msg);
    switch(status)
    {
      case AlarmsApiStatus::JSON_ERROR:
        INFO("STATUS JSON ERROR START BLINK");
        ledsState[LED_STATUS_IDX].Color = led.Color;
        ledsState[LED_STATUS_IDX].StartBlink(200, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
      default:
        INFO("STATUS NOT OK START BLINK");
        ledsState[LED_STATUS_IDX].Color = _settings.NoConnectionColor;
        ledsState[LED_STATUS_IDX].StartBlink(LED_STATUS_NO_CONNECTION_PERIOD, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
    }
    #ifdef USE_BOT
    //SendMessageToAllRegisteredChannels(BOT_CONNECTION_ISSUES_MSG);
    #endif
    //FastLEDShow(1000);
  }
  else
  {
    INFO("STATUS OK STOP BLINK");    
    led.Color = led.IsAlarmed ? _settings.AlarmedColor : _settings.NotAlarmedColor;
    RecalculateBrightness(led, false);
    led.StopBlink();   
  }

  return changed;
}

void FillNetworkStat(const int& code, const String &desc)
{
  #ifdef NETWORK_STATISTIC
  networkStat[code].count++;
  networkStat[code].code = code;
  if(networkStat[code].description == "")
    networkStat[code].description = desc;
  #endif
}

void PrintNetworkStatToSerial()
{
  #ifdef NETWORK_STATISTIC
  Serial.print("Network Statistic: ");
  if(networkStat.count(AlarmsApiStatus::API_OK) > 0)
    PrintNetworkStatInfoToSerial(networkStat[AlarmsApiStatus::API_OK]);
  for(const auto &de : networkStat)
  {
    auto &info = de.second;
    if(info.code != AlarmsApiStatus::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
}

void HandleEffects(const unsigned long &currentTicks)
{
   if(effectStrtTicks > 0 && currentTicks - effectStrtTicks >= EFFECT_TIMEOUT)
  {
    effectStrtTicks = 0;
    _effect = Effect::Normal;
  }

  if(_effect == Effect::Rainbow)
  {    
    //fill_rainbow(leds, LED_COUNT, 0, 1);
    fill_rainbow_circular(leds, LED_COUNT, 0, 1);
    effectStarted = true;
  }else
  if(_effect == Effect::Strobe)
  {    
    if(!effectStarted)
    {
      for(uint8_t i = 0; i < LED_COUNT; i++)
      {
        auto &led = ledsState[i];
        led.StartBlink(70, 15000);
        led.Color = led.IsAlarmed ? _settings.AlarmedColor : _settings.NotAlarmedColor;
      }
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks);  
  }
}

#ifdef NETWORK_STATISTIC  
void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{  
  Serial.print("[\""); Serial.print(info.description); Serial.print("\": "); Serial.print(info.count); Serial.print("]; ");   
}
#endif

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  {
    ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, _settings.PortalModeColor};
    FastLEDShow(true);
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 2) // Show Network Statistic
  {
    INFO(" HEAP: ", ESP.getFreeHeap());
    INFO("STACK: ", ESP.getFreeContStack());
    INFO("BR: ", _settings.Brightness);
    INFO("Alarmed regions count: ", alarmedLedIdx.size());
    INFO("alarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus);    
    PrintNetworkStatToSerial();
  }

  if(debugButtonFromSerial > 2 && debugButtonFromSerial <= 127) // Blink Test
  {
    // ledsState[debugButtonFromSerial].Color = CRGB::Yellow;
    // ledsState[debugButtonFromSerial].StartBlink(50, 20000);
    LedState state;
    state.Color = CRGB::Yellow;    
    state.BlinkPeriod = 50;
    state.BlinkTotalTime = 5000;
    state.IsAlarmed = false;
    SetLedState((UARegion)debugButtonFromSerial, state);
  }  

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO("alarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus);
  }

  if(debugButtonFromSerial == 102)
  {
    INFO(" HEAP: ", ESP.getFreeHeap());
    INFO("STACK: ", ESP.getFreeContStack());

    int status;
    String statusMsg;
    auto regions = api->getAlarmedRegions2(status, statusMsg, ALARMS_API_REGIONS);
    Serial.println();
    INFO("HTTP response regions count: ", regions.size(), " status: ", status, " msg: ", statusMsg);

    INFO(" HEAP: ", ESP.getFreeHeap());
    INFO("STACK: ", ESP.getFreeContStack());
  }

  if(debugButtonFromSerial == 103)
  {
    digitalWrite(PIN_RELAY1, !digitalRead(PIN_RELAY1));
    INFO("Realy 1: ", digitalRead(PIN_RELAY1));
  }

  if(debugButtonFromSerial == 104)
  {
    digitalWrite(PIN_RELAY2, !digitalRead(PIN_RELAY2));
    INFO("Realy 2: ", digitalRead(PIN_RELAY2));
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();
    INFO("Input: ", readFromSerial);

    uint8_t minusCount = std::count(readFromSerial.begin(), readFromSerial.end(), '-');
    if(minusCount > 0)
    {
      auto step = minusCount * BRIGHTNESS_STEP;
      if(_settings.Brightness - step > 0)
        _settings.Brightness -= step;
      else
        _settings.Brightness = 5;

      SetBrightness();     
    }

    uint8_t plusCount = std::count(readFromSerial.begin(), readFromSerial.end(), '+');
    if(plusCount > 0)
    {
      auto step = plusCount * BRIGHTNESS_STEP;
      if(_settings.Brightness + step < 255)
        _settings.Brightness += step;
      else
        _settings.Brightness = 255;
      
      SetBrightness();      
    }    

    debugButtonFromSerial = readFromSerial.toInt(); 
  }
}

void SetBrightness()
{
  INFO("BR: ", _settings.Brightness);
  FastLED.setBrightness(_settings.Brightness);  
  RecalculateBrightness(); 
  FastLEDShow(true); 
  SaveSettings();
}

void FastLEDShow(const bool &showTrace)
{
  if(showTrace)
    INFO("FastLEDShow()");
  FastLED.show();  
}

void FastLEDShow(const int &retryCount)
{
  for(int i = 0; i < retryCount; i++)
  {
    FastLED.show();   
    delay(2);
  }
}
