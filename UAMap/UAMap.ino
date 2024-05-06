//Estimates: https://docs.google.com/spreadsheets/d/1mYA_Bc687Y8no1yJDxv83fimtd0WU4nvcGI80_jnJME/edit?usp=sharing

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

#define VER F("1.11")
//#define RELEASE
#define DEBUG

#define USE_BOT
#define USE_BUZZER
#define USE_BOT_MAIN_MENU_INLINE
#define BOT_MAX_INCOME_MSG_SIZE 2000 //should not be less because of menu action takes a lot

//#define USE_BUZZER_MELODIES 

#define HTTP_TIMEOUT 500

//#define LANGUAGE_UA
#define LANGUAGE_EN

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_BOT
#define ENABLE_TRACE_BOT

#define ENABLE_INFO_ALARMS
#define ENABLE_TRACE_ALARMS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

#define ENABLE_INFO_BUZZ
#define ENABLE_TRACE_BUZZ
#endif

#include "DEBUGHelper.h"
#include "AlarmsApi.h"
#include "LedState.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "TelegramBotHelper.h"
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
#define BRIGHTNESS_STEP 25

//DebounceTime
#define DebounceTime 50

std::unique_ptr<AlarmsApi> api(new AlarmsApi());
CRGBArray<LED_COUNT> leds;

typedef std::map<uint8_t, RegionInfo*> LedIndexMappedToRegionInfo;
LedIndexMappedToRegionInfo alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

ezButton resetBtn(PIN_RESET_BTN);

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
  digitalWrite(PIN_BUZZ, LOW);

  digitalWrite(PIN_RELAY1, RELAY_OFF);
  digitalWrite(PIN_RELAY2, RELAY_OFF);  

  resetBtn.setDebounceTime(DebounceTime);  

  Serial.println();
  Serial.println(F("!!!! Start UA Map !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(VER);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESP.getFreeContStack());    

  LoadSettings();
  //_settings.reset();

  FastLED.addLeds<WS2811, PIN_LED_STRIP, GRB>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);

  FastLED.clear();   
  fill_ua_prapor2();
  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);

  FastLEDShow(1000);
  
  WiFiOps::WiFiOps wifiOps(F("UAMap WiFi Manager"), F("UAMapAP"), F("password"));

  wifiOps
  .AddParameter("apiToken", "Alarms API Token", "api_token", "YOUR_ALARMS_API_TOKEN", 47)  
  #ifdef USE_BOT
  .AddParameter("telegramToken", "Telegram Bot Token", "telegram_token", "TELEGRAM_TOKEN", 47)
  .AddParameter("telegramName", "Telegram Bot Name", "telegram_name", "@telegram_bot", 50)
  .AddParameter("telegramSec", "Telegram Bot Security", "telegram_sec", "SECURE_STRING", 30)
  #endif
  ;    

  auto resetButtonState = resetBtn.getState();
  INFO(F("ResetBtn: "), resetButtonState);
  wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
  _settings.resetFlag = 200;
  api->setApiKey(wifiOps.GetParameterValueById("apiToken")); 
  api->setBaseUri(_settings.BaseUri); 
  INFO(F("Base Uri: "), _settings.BaseUri);  
 
  //ledsState[LED_STATUS_IDX] = {LED_STATUS_IDX /*Kyivska*/, 1000, -1, false, false, false, _settings.NoConnectionColor};
  //ledsState[0] = {0 /*Crimea*/, 0, 0, true, false, false, _settings.AlarmedColor};
  //ledsState[4] = {4 /*Luh*/, 0, 0, true, false, false, _settings.AlarmedColor};
  
  SetAlarmedLED(alarmedLedIdx);
  CheckAndUpdateRealLeds(millis(), /*effectStarted:*/false);
  SetBrightness(); 

  #ifdef USE_BOT
  //bot.setChatID(CHAT_ID);
  LoadChannelIDs();
  bot->setToken(wifiOps.GetParameterValueById("telegramToken"));  
  _botSettings.SetBotName(wifiOps.GetParameterValueById("telegramName"));  
  _botSettings.botSecure = wifiOps.GetParameterValueById("telegramSec");
  bot->attach(HangleBotMessages);
  bot->setTextMode(FB_TEXT); 
  //bot->setPeriod(_settings.alarmsUpdateTimeout);
  bot->setLimit(1);
  bot->skipUpdates();
  #endif
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("AP Started Call Back"));
  FastLED.clear(); 
  leds[LED_STATUS_IDX] = _settings.PortalModeColor;

  FastLED.setBrightness(_settings.Brightness > 1 ? _settings.Brightness : 2);  
  FastLEDShow(1000);    
}

#ifdef USE_BOT

// int32_t menuID = 0;
// byte depth = 0;

/*
!!!!!!!!!!!!!!!! - Bot Additional Commands for Admin
register - Register chat
unregister - Unregister chat
unregisterall - Unregister all chat(s)
update - Current period of update in milliseconds
update10000 - Set period of update to 10secs
baseuri - Current alerts.api uri
token - Current Alerts.Api token
nstat - Network Statistic
rssi - WiFi Quality rssi db
test - test by regionId
ver - Version Info
changeconfig - change configuration WiFi, tokens...
chid - Registered ChannelIDs
relay1menu - Relay1 Menu to choose region
relay2menu - Relay2 Menu to choose region
gay - trolololo

!!!!!!!!!!!!!!!! - Bot Commands for Users
ua - Ukrain Prapor
menu - Simple menu
br - Current brightness
br255 - Max brightness
br2 - Min brightness
br1 - Min brightness only alarmed visible
alarmed - List of currently alarmed regions
all - List of All regions
relay1 - Region Id set for Relay1
relay10 - Switch Off Relay1
relay2 - Region Id set for Relay2
relay20 - Switch Off Relay2
buzztime - Current buzzer time in milliseconds
buzztime3000 - Set buzzer time to 3secs
buzztime0 - Switch off buzzer
schema - Current Color schema
schema0 - Set Color schema to Dark
schema1 - Set Color schema to Light
strobe - Stroboscope with current Br & Schema
rainbow - Rainbow with current Br

*/

#define BOT_COMMAND_BR F("/br")
#define BOT_COMMAND_RESET F("/reset")
#define BOT_COMMAND_TEST F("/test")
#define BOT_COMMAND_VER F("/ver")
#define BOT_COMMAND_CHANGE_CONFIG F("/changeconfig")
#define BOT_COMMAND_RAINBOW F("/rainbow")
#define BOT_COMMAND_STROBE F("/strobe")
#define BOT_COMMAND_SCHEMA F("/schema")
#define BOT_COMMAND_BASEURI F("/baseuri")
#define BOT_COMMAND_ALARMED F("/alarmed")
#define BOT_COMMAND_ALL F("/all")
#define BOT_COMMAND_MENU F("/menu")
#define BOT_COMMAND_RELAY1 F("/relay1")
#define BOT_COMMAND_RELAY2 F("/relay2")
#define BOT_COMMAND_UPDATE F("/update")
#define BOT_COMMAND_BUZZTIME F("/buzztime")
#define BOT_COMMAND_TOKEN F("/token")
#define BOT_COMMAND_NSTAT F("/nstat")
#define BOT_COMMAND_RSSI F("/rssi")
#define BOT_COMMAND_GAY F("/gay")
#define BOT_COMMAND_UA F("/ua")
#define BOT_COMMAND_CHID F("/chid")
#define BOT_COMMAND_FRMW_UPDATE F("frmwupdate")

const std::vector<String> HandleBotMenu(FB_msg& msg, String &filtered, const bool &isGroup)
{  
  std::vector<String> messages;

  if(msg.OTA && msg.text == BOT_COMMAND_FRMW_UPDATE)
  { 
    INFO(F("Update check..."));
    String fileName = msg.fileName;
    fileName.replace(F(".bin"), F(""));    
    if(fileName.length() > 0)
    {
      INFO(F("Update..."));
      bot->update();
    }
    return std::move(messages);
  }
  
  String value;
  bool answerCurrentAlarms = false;
  bool answerAll = false;

  bool noAnswerIfFromMenu = msg.data.length() > 0 && filtered.startsWith(_botSettings.botNameForMenu);
  BOT_TRACE(F("Filtered: "), filtered);
  filtered = noAnswerIfFromMenu ? msg.data : filtered;
  BOT_TRACE(F("Filtered: "), filtered);

  if(GetCommandValue(BOT_COMMAND_MENU, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
    INFO(F("Main Menu"));
    #ifdef USE_BUZZER
    static const String BotMainMenu = F("Alarmed \t All \n Min Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2 \n Buzzer Off \t Buzzer 3sec");
    static const String BotMainMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu, /buzztime0, /buzztime3000");
    #else
    static const String BotMainMenu = F("Alarmed \t All \n Mix Br \t Mid Br \t Max Br \n Dark \t Light \n Strobe \t Rainbow \n Relay 1 \t Relay 2");
    static const String BotMainMenuCall = F("/alarmed, /all, /br2, /br128, /br255, /schema0, /schema1, /strobe, /rainbow, /relay1menu, /relay2menu");
    #endif

    ESP.resetHeap();
    ESP.resetFreeContStack();

    INFO(F(" HEAP: "), ESP.getFreeHeap());
    INFO(F("STACK: "), ESP.getFreeContStack());

    #ifdef USE_BOT_MAIN_MENU_INLINE
    bot->inlineMenuCallback(_botSettings.botNameForMenu, BotMainMenu, BotMainMenuCall, msg.chatID);
    #else
    //bot->showMenuText(BotMainMenu, BotMainMenuCall, msg.chatID, true);
    #endif
    
    delay(500);
  } else
  if(GetCommandValue(BOT_COMMAND_UPDATE, filtered, value))
  { 
    bot->sendTyping(msg.chatID);
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
    bot->sendTyping(msg.chatID);
    if(value.length() > 0)
    {
      auto update = value.toInt();
      if(update == 0 || (update >= 2000 && update <= 120000))
      {
        _settings.BuzzTime = update;
        SaveSettings();
      }
    }
    value = String(F("Buzz Time: ")) + String(_settings.BuzzTime) + F("ms...");
    noAnswerIfFromMenu = false;
  } else
  if(GetCommandValue(BOT_COMMAND_BR, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    auto br = value.toInt();
    if(value.length() > 0)
    {
      br = br <= 0 ? 1 : (br > 255 ? 255 : br);
      //if(br > 0 && br <= 255)
      _settings.Brightness = br;

      SetBrightness();      
    }
    value = String(F("Brightness: ")) + String(_settings.Brightness);
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_RESET, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    ESP.restart();
  }else
  if(GetCommandValue(BOT_COMMAND_CHANGE_CONFIG, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    _settings.resetFlag = 1985;
    SaveSettings();
    bot->sendMessage(F("Wait for restart..."), msg.chatID);
    ESP.restart();
  }else
  if(GetCommandValue(BOT_COMMAND_TEST, filtered, value))
  {
    bot->sendTyping(msg.chatID);    

    auto regionId = value.toInt();
    String answer = F(" Region: ") + String(regionId);
    if(value.length() > 0 && regionId > 2 && regionId < 31)
    {      
      LedState state;
      state.Color = CRGB::Yellow;    
      state.BlinkPeriod = 50;
      state.BlinkTotalTime = 5000;
      state.IsAlarmed = true;
      state.IsPartialAlarmed = false;
      SetRegionState((UARegion)regionId, state);

      RegionInfo *const regionPtr = api->GetRegionById((UARegion)regionId);
      if(regionPtr != nullptr)
      {
        regionPtr->AlarmStatus = ApiAlarmStatus::Alarmed;
        alarmedLedIdx[(UARegion)regionId] = regionPtr;
        answer += F(" Test started...");
        SetRelayStatus(alarmedLedIdx);
      }      
    } 
    value = answer; 

  }else
  if(GetCommandValue(BOT_COMMAND_VER, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    value = F("Flash Date: ") + String(__DATE__) + F(" ") + String(__TIME__) + F(" ") + F("V:") + VER;
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY1, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelay(F("Relay1"), F("/relay1"), value, _settings.Relay1Region, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_RELAY2, filtered, value))
  {
    noAnswerIfFromMenu = !HandleRelay(F("Relay2"), F("/relay2"), value, _settings.Relay2Region, msg.chatID);
  }else
  if(GetCommandValue(BOT_COMMAND_TOKEN, filtered, value))
  {
    value = String(F("Token: ")) + api->GetApiKey();
  }else
  if(GetCommandValue(BOT_COMMAND_NSTAT, filtered, value))
  {
    PrintNetworkStatistic(value);
  }else
  if(GetCommandValue(BOT_COMMAND_RSSI, filtered, value))
  {
    value = F("SSID: ") + WiFi.SSID() + F(" ") /*+ F("EncryptionType: ") + String(WiFi.encryptionType()) + F(" ")*/ 
          + F("RSSI: ") + String(WiFi.RSSI()) + F("db") + F(" ")
          + F("Channel: ") + WiFi.channel() + F(" ")
          + F("IP: ") + WiFi.localIP().toString() + F(" ")
          + F("GatewayIP: ") + WiFi.gatewayIP().toString() + F(" ")
          + F("MAC: ") + WiFi.macAddress()          
    ;
  }else
  if(GetCommandValue(BOT_COMMAND_CHID, filtered, value))
  {
    INFO(F("BOT Channels:"));
    value.clear();
    value = String(F("BOT Channels:")) + F(" ") + String(_botSettings.toStore.registeredChannelIDs.size()) + F(" -> ");
    for(const auto &channel : _botSettings.toStore.registeredChannelIDs)  
    {
      INFO(F("\t"), channel.first);
      value += F("[") + channel.first + F("]") + F("; ");
    }
  }else
  if(GetCommandValue(BOT_COMMAND_RAINBOW, filtered, value))
  {    
    bot->sendTyping(msg.chatID);
    //value = F("Rainbow started...");
    value.clear();
    _effect = Effect::Rainbow;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_STROBE, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("Strobe started...");
    value.clear();
    _effect = Effect::Strobe;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_GAY, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("Gay started...");
    value.clear();
    _effect = Effect::Gay;   
    effectStartTicks = millis();
    effectStarted = false;
    answerCurrentAlarms = false;
  }else
  if(GetCommandValue(BOT_COMMAND_UA, filtered, value))
  {   
    bot->sendTyping(msg.chatID);
    //value = F("UA started...");
    value.clear();
    _effect = Effect::UA;   
    effectStartTicks = millis();
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
  if(GetCommandValue(BOT_COMMAND_ALARMED, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    answerCurrentAlarms = true;
    answerAll = false;
  }else
  if(GetCommandValue(BOT_COMMAND_ALL, filtered, value))
  {
    bot->sendTyping(msg.chatID);
    answerCurrentAlarms = true;
    answerAll = true;
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
        value = String(F("Light"));
      break;
      case ColorSchema::Dark: 
      default:
        _settings.AlarmedColor = LED_ALARMED_COLOR;
        _settings.NotAlarmedColor = LED_NOT_ALARMED_COLOR;
        _settings.PartialAlarmedColor = LED_PARTIAL_ALARMED_COLOR;
        value = String(F("Dark"));
      break;
    }

    SetAlarmedLED(alarmedLedIdx);
    SetBrightness();

    value = String(F("Color Schema: ")) + value;
    answerCurrentAlarms = false;
  }

  if(value.length() > 0 && !noAnswerIfFromMenu)
      messages.push_back(value);

  if(answerCurrentAlarms || answerAll)
  {
    if(answerCurrentAlarms)
    {
      String answerAlarmed = String(F("Alarmed regions count: ")) + String(alarmedLedIdx.size());     
      messages.push_back(answerAlarmed);
    }

    if(answerAll)  
    {  
      String answerAllMsg = String(F("All regions count: ")) + String(MAX_REGIONS_COUNT);
      messages.push_back(answerAllMsg);
    }
    
    for(const auto &region : api->iotApiRegions)
    {      
      if(region.AlarmStatus == ApiAlarmStatus::Alarmed || answerAll)
      {
        String regionMsg = region.Name + F(": [") + String((uint8_t)region.Id) + F("]");
        messages.push_back(regionMsg);
      }
    } 
  }

  if(messages.size() == 0)
  {
    //bot->answer("Use menu", FB_NOTIF); 
  }

  return std::move(messages);
}

const bool HandleRelay(const String &relayName, const String &relayCommand, String &value, uint8_t &relaySetting, const String& chatID)
{
  if(value == F("menu"))
    {
      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack()); 

      ESP.resetHeap();
      ESP.resetFreeContStack();

      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack());

      SendInlineRelayMenu(relayName, relayCommand, chatID);   

      value.clear();   
      return false;   
    }
    else
    {
      if(value.length() > 0)
      {
        auto regionId = value.toInt();    
        if(regionId == 0 || alarmsLedIndexesMap.count((UARegion)regionId) > 0)
        {
          relaySetting = regionId;
          SaveSettings();
        }
      }
      value = relayName + F(": ") + (relaySetting == 0 ? F("Off") : api->GetRegionNameById((UARegion)relaySetting));
      BOT_TRACE(F("Bot answer: "), value);
      return true;
    }
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

  bool sendWholeMenu = false;
  static const uint8_t RegionsInLine = 2;
  static const uint8_t RegionsInGroup = 6;

  uint8_t regionsCount = MAX_REGIONS_COUNT;
  uint8_t regionPlace = 1;
  bool isEndOfGoup = false;
  for(uint8_t regionIdx = 0; regionIdx < regionsCount; regionIdx++)
  {
    const auto &region = api->iotApiRegions[regionIdx];
    if(region.Id == UARegion::Kyiv || region.Id == UARegion::Sevastopol) continue;

    menu += region.Name + (regionPlace != 0 && regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t "));//(isEndOfGoup ? F("") : (regionPlace % RegionsInLine == 0 ? F(" \n ") : F(" \t ")));
    call += relayCommand + region.Id + F(", ");//(isEndOfGoup ? F("") : F(", "));  

    regionPlace++;
    isEndOfGoup = (regionPlace == RegionsInGroup || regionIdx == regionsCount - 1);

    if(!sendWholeMenu && isEndOfGoup)      
    {
      menu += String(F(" \n ")) + relayName + F(" ") + F("Off");
      call += String(F(", ")) + relayCommand + F(" 0");

      regionPlace = 1;      

      ESP.resetHeap();
      ESP.resetFreeContStack();

      INFO(F(" HEAP: "), ESP.getFreeHeap());
      INFO(F("STACK: "), ESP.getFreeContStack());  

      INFO(menu);
      INFO(call);    

      bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);

      delay(100);

      menu.clear();
      call.clear();
    }
  }

  if(sendWholeMenu)
  {
    menu += String(F(" \n ")) + F("Disable");
    call += String(F(", ")) + relayCommand + F(" 0");

    INFO(menu);
    INFO(call);   

    bot->inlineMenuCallback(_botSettings.botNameForMenu + relayName, menu, call, chatID);
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
    static bool brBtnChangeDirectionUp = false;
    auto nextBr = _settings.Brightness + (brBtnChangeDirectionUp ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP);
    if(nextBr < 0)
    {
      nextBr = 1;
      brBtnChangeDirectionUp = true;
    }else
    if(nextBr > 255)
    {
      nextBr = 255;
      brBtnChangeDirectionUp = false;
    }

    _settings.Brightness = nextBr;

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

    if(CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/false))
    {
      //FastLEDShow(false);
    }        
  }  
  
  FastLEDShow(false); 

  HandleDebugSerialCommands();   

  #ifdef USE_BOT
  uint8_t botStatus = bot->tick();
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

  firstRun = false;
}

const bool CheckAndUpdateRealLeds(const unsigned long &currentTicks, const bool &effectStarted)
{
  //INFO("CheckStatuses");

  bool changed = false;
  for(auto &ledKvp : ledsState)
  {
    auto &led = ledKvp.second;

    if(led.Idx < 0 && led.Idx >= LED_COUNT) continue;

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
      //TRACE(F("Led idx: "), led.Idx, F(" Total timeout: "), led.BlinkTotalTime, F(" FINISHED"));
    }   

    if(led.BlinkPeriod != 0 && (led.PeriodTicks > 0 && currentTicks - led.PeriodTicks >= abs(led.BlinkPeriod)))
    {       
      leds[led.Idx] = led.BlinkPeriod > 0 ? CRGB::Black : led.Color;      
      //led.BlinkPeriod *= -1;
      led.BlinkPeriod = led.BlinkPeriod > 0 ? (0 - led.BlinkPeriod) : (abs(led.BlinkPeriod));
      led.PeriodTicks = currentTicks;
      changed = true;
      //TRACE(F("Led idx: "), led.Idx, F(" BLINK! : period: "), led.BlinkPeriod, F(" Total time: "), led.BlinkTotalTime);
    }   

    if(led.BlinkTotalTime == 0)
    {
      bool colorChanges = leds[led.Idx] == led.Color;
      if(!colorChanges)
      {
        //TRACE(F("Led idx: "), led.Idx, F(" Color Changed"));
      }
      changed = (changed || !colorChanges);

      if(!effectStarted)
        led.SetColors(_settings);      
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
    String statusMsg = F("No WiFi");
   
    if ((WiFi.status() == WL_CONNECTED)) 
    {     
      status = ApiStatusCode::UNKNOWN;
      statusMsg.clear();
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
          TRACE(F("HTTP "), F("Alarmed regions count: "), alarmedRegions.size());
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
    INFO(F("Alarmed regions count: "), alarmedLedIdx.size());
    INFO(F("Waiting "), _settings.alarmsUpdateTimeout, F("ms..."));
    PrintNetworkStatToSerial();
  }
  return changed;
}

void SetAlarmedLED(LedIndexMappedToRegionInfo &alarmedLedIdx)
{
  TRACE(F("SetAlarmedLED: "), alarmedLedIdx.size());
  FastLED.setBrightness(_settings.Brightness);
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
  INFO(F("SetRelayStatus: "), F("Relay1"), F(": "), _settings.Relay1Region, F(" "), F("Relay2"), F(": "), _settings.Relay2Region); 
  if(_settings.Relay1Region == 0 && _settings.Relay2Region == 0) return;
  
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
      DoStrobe(/*alarmedColorSchema:*/true);
      Buzz::AlarmStart(PIN_BUZZ, _settings.BuzzTime);      
    }

    digitalWrite(PIN_RELAY1, RELAY_ON);    
    TRACE(F("Relay1"), F(": "), F("ON"), F(" Region: "), _settings.Relay1Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY1) == RELAY_ON)
    { 
      digitalWrite(PIN_RELAY1, RELAY_OFF);
      Buzz::AlarmEnd(PIN_BUZZ, _settings.BuzzTime);
      TRACE(F("Relay1"), F(": "), F("Off"), F(" Region: "), _settings.Relay1Region);
    }
  }

  if(found2)
  {
    digitalWrite(PIN_RELAY2, RELAY_ON);
    TRACE(F("Relay2"), F(": "), F("ON"), F(" Region: "), _settings.Relay2Region);      
  }
  else
  {
    if(digitalRead(PIN_RELAY2) == RELAY_ON)
    {      
      digitalWrite(PIN_RELAY2, RELAY_OFF);
      TRACE(F("Relay2"), F(": "), F("Off"), F(" Region: "), _settings.Relay2Region);
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

const bool SetStatusLED(const int &status, const String &msg)
{
  static int prevStatus = ApiStatusCode::UNKNOWN;
  bool changed = prevStatus != status;
  prevStatus = status;

  FillNetworkStat(status, msg);
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
    led.SetColors(_settings);
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

#ifdef NETWORK_STATISTIC  
void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{  
  Serial.print(F("[\"")); Serial.print(info.description); Serial.print(F("\": ")); Serial.print(info.count); Serial.print(F("]; "));   
}
#endif

void PrintNetworkStatToSerial()
{
  #ifdef NETWORK_STATISTIC
  Serial.print(F("Network Statistic: "));
  if(networkStat.count(ApiStatusCode::API_OK) > 0)
    PrintNetworkStatInfoToSerial(networkStat[ApiStatusCode::API_OK]);
  for(const auto &de : networkStat)
  {
    const auto &info = de.second;
    if(info.code != ApiStatusCode::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
}

void PrintNetworkStatInfo(const NetworkStatInfo &info, String &str)
{  
  str += F("[\"") + info.description + F("\": ") + String(info.count) + F("]; ");
}

void PrintNetworkStatistic(String &str)
{
  str = F("Network Statistic: ");
  #ifdef NETWORK_STATISTIC  
  if(networkStat.count(ApiStatusCode::API_OK) > 0)
    PrintNetworkStatInfo(networkStat[ApiStatusCode::API_OK], str);
  for(const auto &de : networkStat)
  {
    const auto &info = de.second;
    if(info.code != ApiStatusCode::API_OK)
      PrintNetworkStatInfo(info, str);
  }
  //str.replace("(", "");
  //str.replace(")", "");
  #else
  str += F("Off");
  #endif
}

void DoStrobe(const bool &alarmedColorSchema)
{
  for(uint8_t i = 0; i < LED_COUNT; i++)
  {
    auto &led = ledsState[i];
    led.StartBlink(70, 15000);
    if(alarmedColorSchema)
      led.SetColors(_settings);
  }
}

void HandleEffects(const unsigned long &currentTicks)
{
   if(effectStartTicks > 0 && currentTicks - effectStartTicks >= EFFECT_TIMEOUT)
  {
    effectStartTicks = 0;
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
      DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }  else
  if(_effect == Effect::Gay)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_rainbow_circular(leds, LED_COUNT, 0, 1);
      for(auto &ledKvp : ledsState)
      {
        auto &led = ledKvp.second;
        if(led.Idx < 0 && led.Idx >= LED_COUNT) continue;

        led.Color = leds[led.Idx];
      }

      DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }else
  if(_effect == Effect::UA)
  {     
    if(!effectStarted)
    {
      FastLED.setBrightness(255);
      fill_ua_prapor2();
      for(auto &ledKvp : ledsState)
      {
        auto &led = ledKvp.second;
        if(led.Idx < 0 && led.Idx >= LED_COUNT) continue;

        led.Color = leds[led.Idx];
      }

      //DoStrobe(/*alarmedColorSchema:*/false);
      effectStarted = true;
    }  
    CheckAndUpdateRealLeds(currentTicks, /*effectStarted:*/true);  
  }
}

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 2) // Reset Settings
  {
    _settings.init();
    SaveSettings();
    api->setBaseUri(_settings.BaseUri);
  }

  if(debugButtonFromSerial == 1) // Reset WiFi
  {    
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
    state.IsAlarmed = true;
    state.IsPartialAlarmed = false;
    SetRegionState((UARegion)debugButtonFromSerial, state);
  }  

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO(F("alarmsCheckWithoutStatus: "), _settings.alarmsCheckWithoutStatus);
  } 

  if(debugButtonFromSerial == 102)
  {
    digitalWrite(PIN_RELAY1, !digitalRead(PIN_RELAY1));
    INFO(F("Realy1"), F(": "), digitalRead(PIN_RELAY1));
  }

  if(debugButtonFromSerial == 103)
  {
    digitalWrite(PIN_RELAY2, !digitalRead(PIN_RELAY2));
    INFO(F("Realy2"), F(": "), digitalRead(PIN_RELAY2));
  }

  if(debugButtonFromSerial == 104)
  {
    INFO(F("AlarmStart"));
    Buzz::AlarmStart(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 105)
  {
    INFO(F("AlarmEnd"));
    Buzz::AlarmEnd(PIN_BUZZ, 5000);
  }

  if(debugButtonFromSerial == 106)
  { 
    INFO(F("Siren"));
    Buzz::Siren(PIN_BUZZ, 5000);
  }

  #ifdef USE_BUZZER_MELODIES    
  if(debugButtonFromSerial == 107)
  { 
    INFO(F("Pitches"));
    Buzz::PlayMelody(PIN_BUZZ, Buzz::pitchesMelody, ARRAY_SIZE(Buzz::pitchesMelody)); 
  }

  if(debugButtonFromSerial == 108)
  {  
    INFO(F("Nokia"));
    Buzz::PlayMelody(PIN_BUZZ, Buzz::nokiaMelody, ARRAY_SIZE(Buzz::nokiaMelody)); 
  }

  if(debugButtonFromSerial == 109)
  {  
    INFO(F("Pacman"));
    Buzz::PlayMelody(PIN_BUZZ, Buzz::pacmanMelody, ARRAY_SIZE(Buzz::pacmanMelody)); 
  }

  if(debugButtonFromSerial == 110)
  {    
    INFO(F("Happy Birthday"));
    Buzz::PlayMelody(PIN_BUZZ, Buzz::happyBirthdayMelody, ARRAY_SIZE(Buzz::happyBirthdayMelody)); 
  }

  if(debugButtonFromSerial == 111)
  {     
    INFO(F("Xmas"));
    Buzz::PlayMelody(PIN_BUZZ, Buzz::xmasMelody, ARRAY_SIZE(Buzz::xmasMelody)); 
  }

  if(debugButtonFromSerial == 112)
  {  
    INFO(F("Star War"));    
    Buzz::PlayMelody(PIN_BUZZ, Buzz::starWarMelody, ARRAY_SIZE(Buzz::starWarMelody));
  }
  #endif

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
    yield(); // watchdog
  }
}

void fill_ua_prapor()
{
  leds[0] = CRGB::Yellow;
  leds[1] = CRGB::Yellow;
  leds[2] = CRGB::Yellow;  
  leds[3] = CRGB::Yellow;

  leds[4] = CRGB::Blue;
  leds[5] = CRGB::Blue;

  leds[6] = CRGB::Yellow;
  leds[7] = CRGB::Yellow;
  leds[8] = CRGB::Yellow;
  leds[9] = CRGB::Yellow;
  leds[10] = CRGB::Yellow;

  leds[11] = CRGB::Blue;
  leds[12] = CRGB::Blue;
  leds[13] = CRGB::Blue;
  leds[14] = CRGB::Blue;
  leds[15] = CRGB::Blue;

  leds[16] = CRGB::Yellow;

  leds[17] = CRGB::Blue;
  leds[18] = CRGB::Blue;

  leds[19] = CRGB::Yellow;
  leds[20] = CRGB::Yellow;
  leds[21] = CRGB::Yellow;
  leds[22] = CRGB::Yellow;

  leds[23] = CRGB::Blue;

  leds[24] = CRGB::Yellow;
  leds[25] = CRGB::Yellow;
}

void fill_ua_prapor2()
{ 
  SetRegionColor(UARegion::Crimea,            CRGB::Yellow);
  SetRegionColor(UARegion::Khersonska,        CRGB::Yellow);
  SetRegionColor(UARegion::Zaporizka,         CRGB::Yellow);
  SetRegionColor(UARegion::Donetska,          CRGB::Yellow);
  SetRegionColor(UARegion::Dnipropetrovska,   CRGB::Yellow);
  SetRegionColor(UARegion::Mykolaivska,       CRGB::Yellow);
  SetRegionColor(UARegion::Odeska,            CRGB::Yellow);
  SetRegionColor(UARegion::Kirovohradska,     CRGB::Yellow);
  SetRegionColor(UARegion::Vinnytska,         CRGB::Yellow);
  SetRegionColor(UARegion::Khmelnitska,       CRGB::Yellow);
  SetRegionColor(UARegion::Chernivetska,      CRGB::Yellow);
  SetRegionColor(UARegion::Ivano_Frankivska,  CRGB::Yellow);
  SetRegionColor(UARegion::Ternopilska,       CRGB::Yellow);
  SetRegionColor(UARegion::Lvivska,           CRGB::Yellow);
  SetRegionColor(UARegion::Zakarpatska,       CRGB::Yellow);


  SetRegionColor(UARegion::Luhanska,          CRGB::Blue);
  SetRegionColor(UARegion::Kharkivska,        CRGB::Blue);
  SetRegionColor(UARegion::Poltavska,         CRGB::Blue);
  SetRegionColor(UARegion::Sumska,            CRGB::Blue);
  SetRegionColor(UARegion::Chernihivska,      CRGB::Blue);
  SetRegionColor(UARegion::Kyivska,           CRGB::Blue);
  SetRegionColor(UARegion::Cherkaska,         CRGB::Blue);
  SetRegionColor(UARegion::Zhytomyrska,       CRGB::Blue);
  SetRegionColor(UARegion::Rivnenska,         CRGB::Blue);
  SetRegionColor(UARegion::Volynska,          CRGB::Blue);

}

void SetRegionColor(const UARegion &region, const CRGB &color)
{
  for(const auto &idx : AlarmsApi::getLedIndexesByRegion(region))
  {    
    ledsState[idx].Color = color;
    leds[idx] = color;
  }
}

void SetRegionState(const UARegion &region, LedState &state)
{
  for(const auto &idx : AlarmsApi::getLedIndexesByRegion(region))
  {
    state.Idx = idx;
    // state.IsAlarmed = ledsState[idx].IsAlarmed;
    // state.IsPartialAlarmed = ledsState[idx].IsPartialAlarmed;
    ledsState[idx] = state;
  }
}