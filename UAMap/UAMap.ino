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

//#define LANGUAGE_UA
#define LANGUAGE_EN

#ifdef DEBUG
#define NETWORK_STATISTIC
#define ENABLE_TRACE

#define ENABLE_INFO_MAIN
#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_ALARMS
#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI
#endif

#define USE_BOT
#define BOT_MAX_INCOME_MSG_SIZE 2000

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "LedState.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBot.h"
#include "BuzzHelper.h"

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
#define PIN_BUZZ      D3
#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 26
#define BRIGHTNESS_STEP 10

//DebounceTime
#define DebounceTime 50

std::unique_ptr<AlarmsApi> api(new AlarmsApi());
CRGBArray<LED_COUNT> leds;

typedef std::map<uint8_t, RegionInfo*> LedIndexMappedToRegionInfo;
LedIndexMappedToRegionInfo alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

ezButton resetBtn(PIN_RESET_BTN);

void SetLedState(const UARegion &region, LedState &state)
{
  for(const auto &idx : AlarmsApi::getLedIndexesByRegion(region))
  {
    state.Idx = idx;
    state.IsAlarmed = ledsState[idx].IsAlarmed;
    state.IsPartialAlarmed = ledsState[idx].IsPartialAlarmed;
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

  pinMode(PIN_BUZZ, INPUT_PULLUP);
  pinMode(PIN_BUZZ, OUTPUT);

  digitalWrite(PIN_RELAY1, RELAY_OFF);
  digitalWrite(PIN_RELAY2, RELAY_OFF);

  digitalWrite(PIN_BUZZ, LOW);

  resetBtn.setDebounceTime(DebounceTime);

  WiFiOps::WiFiOps<3> wifiOps(F("UAMap WiFi Manager"), F("UAMapAP"), F("password"));

  Serial.println();
  Serial.println(F("!!!! Start UA Map !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESP.getFreeContStack());  

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, false, _settings.PortalModeColor};

  wifiOps
  .AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN", 47)  
  .AddParameter("telegramToken", "Telegram Bot Token", "telegram_token", "TELEGRAM_TOKEN", 47)
  .AddParameter("telegramName", "Telegram Bot Name", "telegram_name", "@telegram_bot", 50)
  .AddParameter("telegramSec", "Telegram Bot Security", "telegram_sec", "SECURE_STRING", 30)
  ;  

  LoadSettings();
  //_settings.reset();

  auto resetButtonState = resetBtn.getState();
  INFO(F("ResetBtn: "), resetButtonState);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  _settings.resetFlag = 200;
  api->setApiKey(wifiOps.GetParameterValueById("apiToken")); 
  api->setBaseUri(_settings.BaseUri); 
  INFO(F("Base Uri: "), _settings.BaseUri);

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
  FastLED.clear();  

  ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 1000, -1, false, false, false, _settings.NoConnectionColor};
  ledsState[0] = {0 /*Crymea*/, 0, 0, true, false, false, _settings.AlarmedColor};
  ledsState[4] = {4 /*Luh*/, 0, 0, true, false, false, _settings.AlarmedColor};

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
  
  bot->setLimit(1);
  bot->skipUpdates();
  #endif
}

#ifdef USE_BOT

// int32_t menuID = 0;
// byte depth = 0;

#define BOT_COMMAND_BR F("/br")
#define BOT_COMMAND_RESET F("/reset")
#define BOT_COMMAND_RAINBOW F("/rainbow")
#define BOT_COMMAND_STROBE F("/strobe")
#define BOT_COMMAND_SCHEMA F("/schema")
#define BOT_COMMAND_BASEURI F("/baseuri")
#define BOT_COMMAND_ALARMS F("/alarms")
#define BOT_COMMAND_MENU F("/menu")
#define BOT_COMMAND_RELAY1 F("/relay1")
#define BOT_COMMAND_RELAY2 F("/relay2")
#define BOT_COMMAND_UPDATE F("/update")
#define BOT_COMMAND_BUZZTIME F("/buzztime")
const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered)
{   
  std::vector<String> messages;
  String value;
  bool answerCurrentAlarms = false;

  bool hasData = msg.data.length() > 0 && filtered.startsWith(_botSettings.botNameForMenu);
  BOT_TRACE(F("Filtered: "), filtered);
  filtered = hasData ? msg.data : filtered;
  BOT_TRACE(F("Filtered: "), filtered);

  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  { 
    static const String BotMainMenu = F("Alarms \n Brightness Max \t Brightness Mid \t Brightness Min \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2");
    static const String BotMainMenuCall = F("/alarms, /br 255, /br 120, /br 2, /schema 0, /schema 1, /strobe, /rainbow, /relay1 menu, /relay2 menu");
    bot->inlineMenuCallback(_botSettings.botNameForMenu, BotMainMenu, BotMainMenuCall, msg.chatID);
    //menuID = bot->lastBotMsg();
  } else
  if(GetCommandValue(BOT_COMMAND_UPDATE, filtered, value))
  { 
    if(value.length() > 0)
    {
      auto update = value.toInt();
      if(update >= 2000 && update <= 120000 )
      {
        _settings.alarmsUpdateTimeout = update;
        SaveSettings();
      }
    }
    value = String(F("Update Timeout: ")) + String(_settings.alarmsUpdateTimeout) + F("ms...");
  } else
  if(GetCommandValue(BOT_COMMAND_BUZZTIME, filtered, value))
  { 
    if(value.length() > 0)
    {
      auto update = value.toInt();
      if(update == -1 || (update >= 2000 && update <= 120000))
      {
        _settings.BuzzTime = update;
        SaveSettings();
      }
    }
    value = String(F("Buzz Time: ")) + String(_settings.BuzzTime) + F("ms...");
  } else
  if(GetCommandValue(BOT_COMMAND_BR, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto br = value.toInt();
    br = br <= 0 ? 1 : (br > 255 ? 255 : br);
    //if(br > 0 && br <= 255)
       _settings.Brightness = br;

    SetBrightness();      
    value = String(F("Brightness changed to: ")) + String(br);
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

    value = String(F("Reseted: Heap: ")) + String(ESP.getFreeHeap()) + F(" Stack: ") + String(ESP.getFreeContStack());
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY1, filtered, value))
  {
    if(value == "menu")
    {
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack()); 

      ESP.resetHeap();
      ESP.resetFreeContStack();

      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack());

      SendInlineRelayMenu("Relay1", "/relay1", msg.chatID);   

      value = "";
      answerCurrentAlarms = false;
    }
    else
    {
      auto regionId = value.toInt();    
      if(regionId == -1 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
      {
        _settings.Relay1Region = regionId;
      }
      value = "Relay1: " + String(_settings.Relay1Region);
      SaveSettings();
    }

  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    if(value == "menu")
    {
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack()); 

      ESP.resetHeap();
      ESP.resetFreeContStack();

      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack());

      SendInlineRelayMenu("Relay2", "/relay2", msg.chatID);   

      value = "";
      answerCurrentAlarms = false;
    }
    else
    {
      auto regionId = value.toInt();
      if(regionId == -1 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
      {
        _settings.Relay2Region = regionId;
      }
      value = "Relay2: " + String(_settings.Relay2Region);
      SaveSettings();
    }
  }else
  if(GetCommandValue(BOT_COMMAND_RAINBOW, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    value = F("Rainbow started...");
    _effect = Effect::Rainbow;   
    effectStrtTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_STROBE, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    value = F("Strobe started...");
    _effect = Effect::Strobe;   
    effectStrtTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_BASEURI, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    if(value.startsWith(F("https://")) && value.length() < MAX_BASE_URI_LENGTH)
    {
      strcpy(_settings.BaseUri, value.c_str());
      api->setBaseUri(value);
      SaveSettings();
    }
    value = _settings.BaseUri;
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
        _settings.NotAlarmedColor = CRGB::White;
        _settings.PartialAlarmedColor = CRGB::Yellow;
        value = "Light";
      break;
      case ColorSchema::Dark: 
      default:
        _settings.AlarmedColor = LED_ALARMED_COLOR;
        _settings.NotAlarmedColor = LED_NOT_ALARMED_COLOR;
        _settings.PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;
        value = "Dark";
      break;
    }

    SetAlarmedLED(alarmedLedIdx);
    SetBrightness();

    value = String(F("Color Chema changed to: ")) + value;
    answerCurrentAlarms = false;
  }

  if(value != "" && !hasData)
      messages.push_back(value);

  if(answerCurrentAlarms)
  {
    String answer = String(F("Alarmed regions count: ")) + String(alarmedLedIdx.size());
     
    messages.push_back(answer);   
    
    for(const auto &regionKvp : alarmedLedIdx)
    {
      const auto &region = regionKvp.second;
      String regionMsg = region->Name + ": [" + String((uint8_t)region->Id) + "]";
      messages.push_back(regionMsg);
    } 
  }

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

/*String BotRelayMenu1("Odeska \n Kharkivska \t Mykolaivska \t Vinnytska \t Kyivska \n Kirovohradska \t Poltavska \t Sumska \t Ternopilska");
String BotRelayMenuCall1("{0} 18, {0} 22, {0} 17, {0} 4, {0} 14, {0} 15, {0} 19, {0} 20, {0} 21");*/
void SendInlineRelayMenu(const String &relayName, const String &relayCommand, const String& chatID)
{
  // String call1 = BotRelayMenuCall1;
  // call1.replace("{0}", relayCommand);
  // bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, BotRelayMenu1, call1, chatID);

  String menu;
  String call;

  uint8_t regionsCount = MAX_REGIONS_COUNT;
  for(uint8_t i = 0; i < regionsCount; i++)
  {
    const auto &region = api->iotApiRegions[i];
    menu += region.Name + (i < regionsCount - 1 ? ( i % 4 == 0 ? " \n " : " \t ") : "");
    call += relayCommand + " " + region.Id + (i < regionsCount - 1 ? ", " : "");  

    if(i % 8 == 0)      
    {
      menu += String(" \n ") + "Disable";
      call += String(", ") + relayCommand + " -1";

      INFO(menu);
      INFO(call);      

      bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);

      delay(100);

      menu = "";
      call = "";
    }
  }
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
    INFO(BUTTON_IS_PRESSED_MSG, F(" BR: "), _settings.Brightness);
  }
  if(resetBtn.isReleased())
  {    
    _settings.Brightness -= BRIGHTNESS_STEP;
    INFO(BUTTON_IS_RELEASED_MSG, F(" BR: "), _settings.Brightness);
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
  uint8_t botStatus = bot->tick();
  if(botStatus == 2)
  {
    BOT_INFO(F("Bot overloaded!"));
    bot->skipUpdates();
  }
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

    //TRACE(F("Led idx: "), led.Idx, F(" Total timeout: "), led.BlinkTotalTime, F(" Period timeout: "), led.BlinkPeriod, F(" current: "), currentTicks);

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
      TRACE(F("Led idx: "), led.Idx, F(" Total timeout: "), led.BlinkTotalTime, F(" FINISHED"));
    }   

    if(led.BlinkPeriod != 0 && (led.PeriodTicks > 0 && currentTicks - led.PeriodTicks >= abs(led.BlinkPeriod)))
    { 
      leds[led.Idx] = led.BlinkPeriod > 0 ? 0 : led.Color;     
      led.BlinkPeriod *= -1;
      led.PeriodTicks = currentTicks;
      changed = true;
      TRACE(F("Led idx: "), led.Idx, F(" BLINK! : period: "), led.BlinkPeriod, F(" Total time: "), led.BlinkTotalTime);
    }   

    if(led.BlinkTotalTime == 0)
    {
      bool colorChanges = leds[led.Idx] == led.Color;
      if(!colorChanges)
        TRACE(F("Led idx: "), led.Idx, F(" Color Changed"));
      changed = (changed || !colorChanges);

      led.Color = led.IsAlarmed ? (led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor) : _settings.NotAlarmedColor;       
      RecalculateBrightness(led, false);
      leds[led.Idx] = led.Color;         
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
    int status = ApiStatusCode::NO_WIFI;
    String statusMsg;
   
    if ((WiFi.status() == WL_CONNECTED)) 
    {     
      status = ApiStatusCode::UNKNOWN;
      //INFO("WiFi - CONNECTED");
      //ESP.resetHeap();
      //ESP.resetFreeContStack();
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack());

      bool statusChanged = api->IsStatusChanged(status, statusMsg);
      if(status == ApiStatusCode::API_OK || status == HTTP_CODE_METHOD_NOT_ALLOWED)
      {        
        INFO(F("IsStatusChanged: "), statusChanged ? F("true") : F("false"));
        TRACE(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus ? F("true") : F("false"));

        if(statusChanged || _settings.alarmsCheckWithoutStatus || ALARMS_CHECK_WITHOUT_STATUS)
        {              
          auto alarmedRegions = api->getAlarmedRegions2(status, statusMsg);    
          TRACE(F("HTTP "), F("Alarmed regions: "), alarmedRegions.size());
          if(status == ApiStatusCode::API_OK)
          { 
            _settings.alarmsCheckWithoutStatus = false;           
            alarmedLedIdx.clear();
            for(uint8_t idx = 0; idx < alarmedRegions.size(); idx++)
            {
              RegionInfo * const alarmedRegion = alarmedRegions[idx];
              const auto &ledRange = AlarmsApi::getLedIndexesByRegion(alarmedRegion->Id);
              for(auto &ledIdx : ledRange)
              {
                TRACE(F(" Region: "), alarmedRegion->Id, F("\t"), F("Led idx: "), ledIdx, F("\tName: "), alarmedRegion->Name);
                alarmedLedIdx[ledIdx] = alarmedRegion;
              }
            }

            SetAlarmedLED(alarmedLedIdx);            
            changed = true;
          } 

          if(status == ApiStatusCode::JSON_ERROR)
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

    INFO(F(""));
    INFO(F("Alarmed regions: "), alarmedLedIdx.size());
    INFO(F("Waiting "), _settings.alarmsUpdateTimeout, F("ms..."));
    PrintNetworkStatToSerial();
  }
  return changed;
}

void SetAlarmedLED(LedIndexMappedToRegionInfo &alarmedLedIdx)
{
  TRACE(F("SetAlarmedLED: "), alarmedLedIdx.size());
  for(uint8_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
  {   
    auto &led = ledsState[ledIdx];
    led.Idx = ledIdx;
    if(alarmedLedIdx.count(ledIdx) > 0)
    {
      if(!led.IsAlarmed)
      {
        led.Color = led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor;
        led.BlinkPeriod = LED_NEW_ALARMED_PERIOD;
        led.BlinkTotalTime = LED_NEW_ALARMED_TOTALTIME;
      }       
      led.IsAlarmed = true;  
      led.IsPartialAlarmed = alarmedLedIdx[ledIdx]->AlarmStatus == ApiAlarmStatus::PartialAlarmed;
    }
    else
    {
      led.IsAlarmed = false;
      led.IsPartialAlarmed = false;
      led.Color = _settings.NotAlarmedColor;      
      led.StopBlink();
    }       

    RecalculateBrightness(led, false);    
  }  
}

void SetRelayStatus(const LedIndexMappedToRegionInfo &alarmedLedIdx)
{
  INFO(F("SetRelayStatus: "), F(" Relay1: "), _settings.Relay1Region, F(" Relay2: "), _settings.Relay2Region); 
  if(_settings.Relay1Region == -1 && _settings.Relay2Region == -1) return;
  
  bool found1 = false;
  bool found2 = false;  

  for(const auto &idx : alarmedLedIdx)
  {
    TRACE(F("Led idx: "), idx.first, F(" Region: "), idx.second->Id);
  
    if(!found1 && idx.second->Id == (UARegion)_settings.Relay1Region)
    {
      TRACE(F("Found1: "), idx.second->Id, F(" Name: "), idx.second->Name);
      found1 = true;        
    }      
    if(!found2 && idx.second->Id == (UARegion)_settings.Relay2Region)
    {
      TRACE(F("Found2: "), idx.second->Id, F(" Name: "), idx.second->Name);
      found2 = true;        
    }
  }

  if(found1)
  {
    if(digitalRead(PIN_RELAY1) == RELAY_OFF)
    {
      DoStrobe();
      Buzz::AlarmStart(PIN_BUZZ, _settings.BuzzTime);      
    }

    digitalWrite(PIN_RELAY1, RELAY_ON);    
    TRACE(F(" Relay 1: "), F("ON"), F(" Region: "), _settings.Relay1Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY1) == RELAY_ON)
    { 
      digitalWrite(PIN_RELAY1, RELAY_OFF);
      Buzz::AlarmEnd(PIN_BUZZ, _settings.BuzzTime);
      TRACE(F(" Relay 1: "), F("OFF"), F(" Region: "), _settings.Relay1Region);
    }
  }

  if(found2)
  {
    digitalWrite(PIN_RELAY2, RELAY_ON);
    TRACE(F(" Relay 2: "), F("ON"), F(" Region: "), _settings.Relay2Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY2) == RELAY_ON)
    {      
      digitalWrite(PIN_RELAY2, RELAY_OFF);
      TRACE(F(" Relay 2: "), F("OFF"), F(" Region: "), _settings.Relay2Region);
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
         INFO(F("Led idx: "), led.Idx, F(" BR: "), br);
    }
      // if(_settings.alarmScaleDown)
      // {
      //   if(!led.IsAlarmed || led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE(F("Led: "), led.Idx, F(" BR: "), br);
      //   }
      // }
      // else
      // {
      //   if(led.IsAlarmed && !led.FixedBrightnessIfAlarmed)
      //   {
      //     auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
      //     if(showTrace)
      //       TRACE(F("Led: "), led.Idx, F(" BR: "), br);
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
  static int prevStatus = ApiStatusCode::UNKNOWN;
  bool changed = prevStatus != status;
  prevStatus = status;

  FillNetworkStat(status, status != ApiStatusCode::API_OK ? msg : F("OK (200)"));
  auto &led = ledsState[LED_STATUS_IDX];
  if(status != ApiStatusCode::API_OK)
  {
    TRACE(F("Status: "), status == ApiStatusCode::WRONG_API_KEY ? F("Unauthorized") : (status == ApiStatusCode::NO_WIFI ? F("No WiFi") : F("No Connection")), F(" | "), msg);
    switch(status)
    {
      case ApiStatusCode::JSON_ERROR:
        TRACE(F("STATUS JSON ERROR START BLINK"));
        ledsState[LED_STATUS_IDX].Color = led.Color;
        ledsState[LED_STATUS_IDX].StartBlink(200, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
      default:
        INFO(F("STATUS NOT OK START BLINK"));
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
    TRACE(F("STATUS OK STOP BLINK"));    
    led.Color = led.IsAlarmed ? (led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor) : _settings.NotAlarmedColor; ;
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
  Serial.print(F("Network Statistic: "));
  if(networkStat.count(ApiStatusCode::API_OK) > 0)
    PrintNetworkStatInfoToSerial(networkStat[ApiStatusCode::API_OK]);
  for(const auto &de : networkStat)
  {
    auto &info = de.second;
    if(info.code != ApiStatusCode::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
}

void DoStrobe()
{
  for(uint8_t i = 0; i < LED_COUNT; i++)
  {
    auto &led = ledsState[i];
    led.StartBlink(70, 15000);
    led.Color = led.IsAlarmed ? (led.IsPartialAlarmed ? _settings.PartialAlarmedColor : _settings.AlarmedColor) : _settings.NotAlarmedColor; ;
  }
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
      DoStrobe();
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks);  
  }
}

#ifdef NETWORK_STATISTIC  
void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{  
  Serial.print(F("[\"")); Serial.print(info.description); Serial.print(F("\": ")); Serial.print(info.count); Serial.print(F("]; "));   
}
#endif

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 2) // Reset Settings
  {
    _settings.reset();
    SaveSettings();
    api->setBaseUri(_settings.BaseUri);
  }

  if(debugButtonFromSerial == 1) // Reset WiFi
  {
    ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyiv*/, 0, 0, false, false, false, _settings.PortalModeColor};
    FastLEDShow(true);
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 100) // Show Network Statistic
  {
    INFO(F(" HEAP: "), ESP.getFreeHeap());
    INFO(F("STACK: "), ESP.getFreeContStack());
    INFO(F(" BR: "), _settings.Brightness);
    INFO(F("Alarmed regions count: "), alarmedLedIdx.size());
    INFO(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus);  
    INFO(F("BOT Channels:"));
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      INFO(F("\t"), channel.first);
    }
    PrintNetworkStatToSerial();
  }

  if(debugButtonFromSerial > 2 && debugButtonFromSerial <= 31) // Blink Test
  {
    // ledsState[debugButtonFromSerial].Color = CRGB::Yellow;
    // ledsState[debugButtonFromSerial].StartBlink(50, 20000);
    LedState state;
    state.Color = CRGB::Yellow;    
    state.BlinkPeriod = 50;
    state.BlinkTotalTime = 5000;
    state.IsAlarmed = false;
    state.IsPartialAlarmed = false;
    SetLedState((UARegion)debugButtonFromSerial, state);
  }  

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus);
  }

  // if(debugButtonFromSerial == 102)
  // {
  //   INFO(F(" HEAP: "), ESP.getFreeHeap());
  //   INFO(F("STACK: "), ESP.getFreeContStack());

  //   int status;
  //   String statusMsg;
  //   auto regions = api->getAlarmedRegions2(status, statusMsg, ALARMS_API_OFFICIAL_REGIONS);
  //   Serial.println();
  //   INFO(F("HTTP response regions count: "), regions.size(), F(" status: "), status, F(" msg: "), statusMsg);

  //   INFO(F(" HEAP: "), ESP.getFreeHeap());
  //   INFO(F("STACK: "), ESP.getFreeContStack());
  // }

  if(debugButtonFromSerial == 102)
  {
    digitalWrite(PIN_RELAY1, !digitalRead(PIN_RELAY1));
    INFO(F(" Realy 1: "), digitalRead(PIN_RELAY1));
  }

  if(debugButtonFromSerial == 103)
  {
    digitalWrite(PIN_RELAY2, !digitalRead(PIN_RELAY2));
    INFO(F(" Realy 2: "), digitalRead(PIN_RELAY2));
  }

  if(debugButtonFromSerial == 104)
  {
    //digitalWrite(PIN_BUZZ, !digitalRead(PIN_BUZZ));
    INFO(F(" BUZZ: "), digitalRead(PIN_BUZZ));    

    Buzz::AlarmStart(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 105)
  {
    //digitalWrite(PIN_BUZZ, !digitalRead(PIN_BUZZ));
    INFO(F(" BUZZ: "), digitalRead(PIN_BUZZ));    

    Buzz::AlarmEnd(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 106)
  {
    //digitalWrite(PIN_BUZZ, !digitalRead(PIN_BUZZ));
    INFO(F(" BUZZ: "), digitalRead(PIN_BUZZ));    

    Buzz::Siren(PIN_BUZZ, 5000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();
    INFO(F("Input: "), readFromSerial);

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
  INFO(F(" BR: "), _settings.Brightness);
  FastLED.setBrightness(_settings.Brightness);  
  RecalculateBrightness(); 
  FastLEDShow(true); 
  SaveSettings();
}

void FastLEDShow(const bool &showTrace)
{
  if(showTrace)
    INFO(F("FastLEDShow()"));
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
