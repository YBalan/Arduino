#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
//#include <EEPROM.h>

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include <map>

#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>

#define VER 1.1
//#define RELEASE
#define DEBUG

#ifdef DEBUG
#define NETWORK_STATISTIC
#define ENABLE_TRACE

#define ENABLE_INFO_MAIN
//#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_BOT
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

#define PIN_RESET_BTN D5
#define PIN_LED_STRIP D6
#define LED_COUNT 25
#define BRIGHTNESS_STEP 10

UAMap::Settings _settings;

std::unique_ptr<AlarmsApi> api(new AlarmsApi());
CRGBArray<LED_COUNT> leds;

std::map<uint16_t, RegionInfo> alarmedLedIdx;
std::map<uint8_t, LedState> ledsState;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial); 

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

  wifiOps.TryToConnectOrOpenConfigPortal();

  api->setApiKey(wifiOps.GetParameterValueById("apiToken"));

  //LoadSettings();

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
  _botSettings.botName = wifiOps.GetParameterValueById("telegramName");
  _botSettings.botSecure = wifiOps.GetParameterValueById("telegramSec");
  bot->attach(HangleBotMessages);
  bot->setTextMode(FB_MARKDOWN); 
  #endif
}

#ifdef USE_BOT
#define BOT_COMMAND_BR "/br"
#define BOT_COMMAND_RESET "/reset"
#define BOT_COMMAND_RAINBOW "/rainbow"
#define BOT_COMMAND_SCHEMA "/schema"
const std::vector<String> HandleBotMenu(FB_msg& msg, const String &filtered)
{ 
  std::vector<String> messages;
  String value;
  if(GetCommandValue(BOT_COMMAND_BR, filtered, value))
  {
    auto br = value.toInt();
    if(br > 0 && br <= 255)
       _settings.Brightness = br;

    SetBrightness();      
    value = String("Brightness changed to: ") + String(br);
  }else
  if(GetCommandValue(BOT_COMMAND_RESET, filtered, value))
  {
    ESP.resetHeap();
    ESP.resetFreeContStack();

    INFO(" HEAP: ", ESP.getFreeHeap());
    INFO("STACK: ", ESP.getFreeContStack());

    _settings.alarmsCheckWithoutStatus = true;

    int status;
    String statusMsg;
    auto regions = api->getAlarmedRegions2(status, statusMsg, ALARMS_API_REGIONS);    
    INFO("HTTP response regions count: ", regions.size(), " status: ", status, " msg: ", statusMsg);

    value = String("Reseted: Heap: ") + String(ESP.getFreeHeap()) + " Stack: " + String(ESP.getFreeContStack());
  }else
  if(GetCommandValue(BOT_COMMAND_RAINBOW, filtered, value))
  {    
    value = "Rainbow started";
    _effect = Effect::Rainbow;   
    effectStrtTicks = millis(); 
  }else
  if(GetCommandValue(BOT_COMMAND_SCHEMA, filtered, value))
  {    
    uint8_t schema = value.toInt();
    switch(schema)
    {
      case ColorSchema::Light:
        _settings.AlarmedColor = CRGB::LightGreen;
        _settings.NotAlarmedColor = CRGB::Green;
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

    value = String("Color Chema changed to: ") + value;
  }


  String answer = String("Alarmed regions count: ") + String(alarmedLedIdx.size());

  if(value != "")
    messages.push_back(value); 
  messages.push_back(answer);   
  
  for(const auto &regionKvp : alarmedLedIdx)
  {
    const auto &region = regionKvp.second;
    String regionMsg = region.Name + ": [" + String(region.Id) + "]";
    messages.push_back(regionMsg);
  } 

  return std::move(messages);
}
#endif

void loop() 
{
  static bool firstRun = true;
  static uint32_t currentTicks = millis();
  currentTicks = millis();

  if(effectStrtTicks > 0 && currentTicks - effectStrtTicks >= EFFECT_TIMEOUT)
  {
    effectStrtTicks = 0;
    _effect = Effect::Normal;
  }

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
  }else
  if(_effect == Effect::Rainbow)
  {    
    //fill_rainbow(leds, LED_COUNT, 0, 1);
    fill_rainbow_circular(leds, LED_COUNT, 0, 1);
    //FastLEDShow(false);
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
      ESP.resetHeap();
      ESP.resetFreeContStack();
      INFO(" HEAP: ", ESP.getFreeHeap());
      INFO("STACK: ", ESP.getFreeContStack());

      bool statusChanged = api->IsStatusChanged(status, statusMsg);
      if(status == AlarmsApiStatus::API_OK)
      {        
        INFO("IsStatusChanged: ", statusChanged ? "true" : "false");
        INFO("AlarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus ? "true" : "false");

        if(statusChanged || _settings.alarmsCheckWithoutStatus)
        {              
          auto alarmedRegions = api->getAlarmedRegions2(status, statusMsg);    
          INFO("HTTP response Alarmed regions count: ", alarmedRegions.size());
          if(status == AlarmsApiStatus::API_OK)
          { 
            _settings.alarmsCheckWithoutStatus = false;           
            alarmedLedIdx.clear();
            for(const auto &rId : alarmedRegions)
            {
              auto ledIdx = api->getLedIndexByRegionId(rId.first);
              INFO("regionId:", rId.first, "\tled index: ", ledIdx);
              alarmedLedIdx[ledIdx] = std::move(rId.second);
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
      if(_settings.alarmScaleDown)
      {
        if(!led.IsAlarmed || led.FixedBrightnessIfAlarmed)
        {
          auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
          if(showTrace)
            TRACE("Led: ", led.Idx, " Br: ", br);
        }
      }
      else
      {
        if(led.IsAlarmed && !led.FixedBrightnessIfAlarmed)
        {
          auto br = led.setBrightness(GetScaledBrightness(_settings.alarmedScale, _settings.alarmScaleDown));
          if(showTrace)
            TRACE("Led: ", led.Idx, " Br: ", br);
        } 
      }
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
        TRACE("STATUS JSON ERROR START BLINK");
        ledsState[LED_STATUS_IDX].Color = led.Color;
        ledsState[LED_STATUS_IDX].StartBlink(200, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
      default:
        TRACE("STATUS NOT OK START BLINK");
        ledsState[LED_STATUS_IDX].Color = _settings.NoConnectionColor;
        ledsState[LED_STATUS_IDX].StartBlink(LED_STATUS_NO_CONNECTION_PERIOD, LED_STATUS_NO_CONNECTION_TOTALTIME);
      break;
    }
  }
  else
  {
    TRACE("STATUS OK STOP BLINK");    
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
  for(auto de : networkStat)
  {
    auto &info = std::get<1>(de);
    if(info.code != AlarmsApiStatus::API_OK)
      PrintNetworkStatInfoToSerial(info);
  }
  Serial.println();
  #endif
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
    //wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/true);   
    //api->setApiKey(wifiOps.GetParameterValueById("apiToken"));
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

  if(debugButtonFromSerial == 15) // Blink Test
  {
    ledsState[LED_STATUS_IDX].Color = CRGB::Yellow;
    ledsState[LED_STATUS_IDX].StartBlink(200, 20000);
  }  

  if(debugButtonFromSerial == 101)
  {    
    _settings.alarmsCheckWithoutStatus = !_settings.alarmsCheckWithoutStatus;
    INFO("alarmsCheckWithoutStatus: ", _settings.alarmsCheckWithoutStatus);
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
}

void FastLEDShow(const bool &showTrace)
{
  if(showTrace)
    INFO("FastLEDShow()");
  FastLED.show();  
}

void SaveChannelIDs()
{
  TRACE("SaveChannelIDs");
  File configFile = SPIFFS.open("/channelIDs.json", "w");
  if (configFile) 
  {
    TRACE("Write channelIDs file");

    String store;
    for(const auto &v : _botSettings.toStore.registeredChannelIDs)
      store += v.first + ',';

    configFile.write(store.c_str());
    configFile.close();
        //end save
  }
  else
  {
    TRACE("failed to open channelIDs file for writing");    
  }
}

std::vector<String> split(const String &s, char delimiter) {
    std::vector<String> tokens;
    int startIndex = 0; // Index where the current token starts

    // Loop through each character in the string
    for (int i = 0; i < s.length(); i++) {
        // If the current character is the delimiter or it's the last character in the string
        if (s.charAt(i) == delimiter || i == s.length() - 1) {
            // Extract the substring from startIndex to the current position
            String token = s.substring(startIndex, i);
            token.trim();
            tokens.push_back(token);
            startIndex = i + 1; // Update startIndex for the next token
        }
    }
    return tokens;
}

void LoadChannelIDs()
{
  TRACE("LoadChannelIDs");
  File configFile = SPIFFS.open("/channelIDs.json", "r");
  if (configFile) 
  {
    TRACE("Read channelIDs file");    

    auto read = configFile.readString();

    TRACE("ChannelIDs in file: ", read);

    for(const auto &r : split(read, ','))
    {
      TRACE("\tChannelID: ", r);
      if(r != "")
      {
        _botSettings.toStore.registeredChannelIDs[r] = 1;
      }      
    }

    configFile.close();
  }
  else
  {
    TRACE("failed to open channelIDs file for reading");    
  }
}

void SaveSettings()
{
  INFO("Save Settings...");
  //EEPROM.put(0, _settings);
  //EEPROM.put(sizeof(_settings), _botSettings.toStore);
}

void LoadSettings()
{
  INFO("Load Settings...");
  //EEPROM.get(0, _settings);    
  //EEPROM.get(sizeof(_settings), _botSettings.toStore);  
}