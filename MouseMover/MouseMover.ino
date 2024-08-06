#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#ifdef ESP8266
  #define VER F("1.2")
#else //ESP32
  #define VER F("1.2")
#endif

#define AVOID_FLICKERING

//#define RELEASE
#define DEBUG

#define NETWORK_STATISTIC
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN

#ifdef DEBUG

#define VER_POSTFIX F("D")

#define WM_DEBUG_LEVEL WM_DEBUG_NOTIFY

#define ENABLE_TRACE_MAIN

#define ENABLE_INFO_SETTINGS
#define ENABLE_TRACE_SETTINGS

#define ENABLE_INFO_WIFI
#define ENABLE_TRACE_WIFI

#define ENABLE_TRACE_SERVO

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
#define DebounceTime 50

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
  #define ESPgetFreeContStack F("Not supported")
  #define ESPresetHeap {}
  #define ESPresetFreeContStack {}
#endif

#include "Config.h"

//#include <ezButton.h>
#include <LiquidCrystal_I2C.h>
#include "DEBUGHelper.h"
#include "NSTAT.h"
#include "ESPServo.h"
#include "Settings.h"
#include "WiFiOps.h"
#include "RndApi.h"
#ifdef USE_BOT
#include "TelegramBotHelper.h"
#include "TBotMenu.h"
#endif

#define LONG_PRESS_VALUE_MS 2000
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

#define LCD_COLS 16
#define LCD_ROWS 2
#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

//DebounceTime
#define DEBOUNCE_TIME 50
Button btnOK(OK_PIN);
Button btnUp(UP_PIN);
Button btnDw(DW_PIN);
Button btnRt(RT_PIN);

ESPServo servo(SERVO_PIN);

unsigned long lastMovementTicks = 0;
#define CURRENT_STATUS_UPDATE 1000
unsigned long updateTicks = 0;

enum class Menu : uint8_t
{
  Main,
  Move,
  BoundMin,
  BoundMax,
  PeriodMin,
  PeriodMax,
  MoveStyle,
  Pause,
} currentMenu;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);   

  btnOK.setDebounceTime(DEBOUNCE_TIME);
  btnUp.setDebounceTime(DEBOUNCE_TIME);
  btnDw.setDebounceTime(DEBOUNCE_TIME);
  btnRt.setDebounceTime(DEBOUNCE_TIME); 

  Wire.begin();   // Default GPIO_22 (SCL) & GPIO_21 (SDA).
  // Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL); // Custom GPIO.
  lcd.init();
  BacklightOn(0);
  lcd.setCursor(0, 0);
  lcd.print(String(F("V")) + VER + VER_POSTFIX + F(" ") + F("MouseMover"));
  lcd.setCursor(0, 1);
  lcd.print(__DATE__);
  delay(3000);
  lcd.clear();

  Serial.println();
  Serial.print(F("!!!! Start ")); Serial.println(F("MouseMover"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(String(VER) + VER_POSTFIX);
  Serial.print(F(" HEAP: ")); Serial.println(ESP.getFreeHeap());
  Serial.print(F("STACK: ")); Serial.println(ESPgetFreeContStack);    

  LoadSettings();
  LoadSettingsExt();  
  //_settings.reset();
  DebugPrintSettingsValues();

  WiFiOps::WiFiOps wifiOps(F("MouseMover WiFi Manager"), F("MMAP"), F("password"));

  #ifdef WM_DEBUG_LEVEL
    INFO(F("WM_DEBUG_LEVEL: "), WM_DEBUG_LEVEL);    
  #else
    INFO(F("WM_DEBUG_LEVEL: "), F("Off"));
  #endif

  wifiOps  
  #ifdef USE_API_KEY
  .AddParameter(F("apiToken"), F("API Token"), F("api_token"), F("YOUR_API_TOKEN"), 47)
  #endif
  #ifdef USE_BOT
  .AddParameter(F("telegramToken"), F("Telegram Bot Token"), F("telegram_token"), F("TELEGRAM_TOKEN"), 47)
  .AddParameter(F("telegramName"), F("Telegram Bot Name"), F("telegram_name"), F("@telegram_bot"), 50)
  .AddParameter(F("telegramSec"), F("Telegram Bot Security"), F("telegram_sec"), F("SECURE_STRING"), 30)
  #endif
  ;    

  const auto &okButtonState = btnOK.getState();
  INFO(F("Without WiFi Btn: "), okButtonState == HIGH ? F("Off") : F("On"));
  if(okButtonState == HIGH)
  {
    const auto &resetButtonState = btnRt.getState();
    INFO(F("ResetBtn: "), resetButtonState == HIGH ? F("Off") : F("On"));
    INFO(F("ResetFlag: "), _settings.resetFlag);
    wifiOps.TryToConnectOrOpenConfigPortal(/*resetSettings:*/_settings.resetFlag == 1985 || resetButtonState == LOW);
    if(_settings.resetFlag == 1985)
    {
      _settings.resetFlag = 200;
      SaveSettings();
    }
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

  servo.attach();
  //servo.init();
}

void WiFiOps::WiFiManagerCallBacks::whenAPStarted(WiFiManager *manager)
{
  INFO(F("WiFi Portal: "), manager->getConfigPortalSSID());  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("WiFi Portal: "));
  lcd.setCursor(0, 1);
  lcd.print(manager->getConfigPortalSSID());
}

void loop()
{
  static uint32_t current = millis();
  current = millis();  

  int status =  ApiStatusCode::NO_WIFI; 
  String statusMsg = F("No WiFi");

  btnOK.loop();
  btnUp.loop();
  btnDw.loop();
  btnRt.loop();

  if(btnOK.isPressed())
  {
    INFO(F("Ok "), BUTTON_IS_PRESSED_MSG);
    BacklightOn(current);    
  }

  if(btnOK.isReleased())
  {
    INFO(F("Ok "), BUTTON_IS_RELEASED_MSG);
    if(btnOK.isLongPress())
    {
      INFO(F("Ok "), BUTTON_IS_LONGPRESSED_MSG);
      if(currentMenu == Menu::Main)
      {        
        FillRandomValues(status, statusMsg);
      }
    }
    else
    {
      if(currentMenu == Menu::Main)
      {
        Move(MoveStyle::Normal);
      }
    }
  }

  if(btnUp.isPressed())
  {
    INFO(F("Up "), BUTTON_IS_PRESSED_MSG);
    BacklightOn(current);
  }

  if(btnUp.isReleased())
  {
    INFO(F("Up "), BUTTON_IS_RELEASED_MSG);
    BacklightOn(current);

    if(currentMenu == Menu::Main)
    {
      currentMenu = Menu::BoundMax;
      if(servo.attach())
      {
        servo.init();  
      }
    }else
    if(currentMenu == Menu::BoundMax || currentMenu == Menu::BoundMin)
    {
      if(btnUp.isLongPress())
      {
        INFO(F("Up "), BUTTON_IS_LONGPRESSED_MSG);
        const auto &startPos = servo.read();
        TRACE(F("startAngle: "), startPos);
        if(startPos < _settings.endAngle)
        {
          _settings.startAngle = startPos;
          SaveChanges();
        }else
        {
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Error:Min>=Max");
          delay(3000);
        }        
        btnUp.resetTicks();
      }else
      {      
        if(servo.attach())
        {       
          const auto &currentPos = servo.move(-5, DEFAULT_MOVE_SPEED_DELAY);
          LCDPrintBoundMenu(currentPos);
        }       
      }
    }        
  }

  if(btnDw.isPressed())
  {
    INFO(F("Dw "), BUTTON_IS_PRESSED_MSG);
    BacklightOn(current);
  }

  if(btnDw.isReleased())
  {
    INFO(F("Dw "), BUTTON_IS_RELEASED_MSG);
    BacklightOn(current);
    
    if(currentMenu == Menu::Main)
    {
      currentMenu = Menu::BoundMax;
      if(servo.attach())
      {
        servo.init();  
      }
    }else
    if(currentMenu == Menu::BoundMax || currentMenu == Menu::BoundMin)
    {
      if(btnDw.isLongPress())
      {
        INFO(F("Dw "), BUTTON_IS_LONGPRESSED_MSG);
        const auto &endPos = servo.read();
        TRACE(F("endAngle: "), endPos);
        if(endPos > _settings.startAngle)
        {
          _settings.endAngle = endPos;
          SaveChanges();
        }else
        {
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Error:Max>=Min");
          delay(3000);
        }
        btnDw.resetTicks();
      }else
      {      
        if(servo.attach())
        {       
          const auto &currentPos = servo.move(+5, DEFAULT_MOVE_SPEED_DELAY);
          LCDPrintBoundMenu(currentPos);
        }       
      }
    }        
  }

  if(btnRt.isPressed())
  {
    INFO(F("Rt "), BUTTON_IS_PRESSED_MSG);
    BacklightOn(current);

    if(currentMenu != Menu::Main)
    {
      currentMenu = Menu::Main;
    }
  }

  HandleMenuAndActions(current, status, statusMsg); 

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

  CheckBacklightDelayAndReturnToMainMenu(millis()); 
  HandleDebugSerialCommands();
}

void HandleMenuAndActions(const unsigned long &currentTicks, int &status, String &statusMsg)
{
  if(currentMenu == Menu::Main)
  {    
    uint16_t currentInSec = (currentTicks - lastMovementTicks) * 0.001;    
    if(currentInSec >= _settings.periodTimeoutSecR)
    {
      Move(MoveStyle::Normal);      
      lastMovementTicks = millis();  

      #ifdef USE_API      
      TRACE(F("Start get Randoms"));
      if ((WiFi.status() == WL_CONNECTED)) 
      { 
        FillRandomValues(status, statusMsg);

        SaveChanges();
      }
      TRACE(F("http Status: "), status, F(" "), statusMsg);
      #endif
    }
    if((currentTicks - updateTicks) >= CURRENT_STATUS_UPDATE) 
    {
      //TRACE(F("HandleMenuAndActions: "), currentInSec);
      MainMenuStatus(currentInSec);
      updateTicks = currentTicks;
    }
  } else
  if(currentMenu == Menu::BoundMax || currentMenu == Menu::BoundMin)
  {
    if((currentTicks - updateTicks) >= CURRENT_STATUS_UPDATE) 
    {
      LCDPrintBoundMenu(servo.read());
      updateTicks = currentTicks;
    }
  }
}

const bool FillRandomValues(int &status, String &statusMsg)
{
  TRACE(F("Random..."));
  lcd.clear();
  lcd.print(F("Random..."));
  
  const int &moveAngleR = GetRandomNumber(_settings.startAngle + 10, _settings.endAngle, status, statusMsg);
  if(status != ApiStatusCode::API_OK) return false;
  if(moveAngleR > 0)
    _settings.moveAngleR = moveAngleR;  

  yield(); // watchdog

  const int &moveSpeedDelayR = GetRandomNumber(DEFAULT_MOVE_SPEED_DELAY_MIN, DEFAULT_MOVE_SPEED_DELAY_MAX, status, statusMsg);
  if(status != ApiStatusCode::API_OK) return false;
  if(moveSpeedDelayR > 0)
    _settings.moveSpeedDelayR = moveSpeedDelayR;

  yield(); // watchdog

  const int &moveStepR = GetRandomNumber(DEFAULT_MOVE_STEP_MIN, DEFAULT_MOVE_STEP_MAX, status, statusMsg);
  if(status != ApiStatusCode::API_OK) return false;
  if(moveStepR > 0)
    _settings.moveStepR = moveStepR;

  yield(); // watchdog

  const int &periodTimeoutSecR = GetRandomNumber(_settings.periodTimeoutSecMin, _settings.periodTimeoutSecMax, status, statusMsg);
  if(status != ApiStatusCode::API_OK) return false;
  if(periodTimeoutSecR > 0)
    _settings.periodTimeoutSecR = periodTimeoutSecR;

  yield(); // watchdog  

  DebugPrintSettingsValues();

  return true;
}

void DebugPrintSettingsValues()
{
  TRACE(F("moveAngleR: "), _settings.moveAngleR, F(" "), F("startAngle: "), _settings.startAngle, F(" "), F("endAngle: "), _settings.endAngle);
  TRACE(F("movmoveSpeedDelayR: "), _settings.moveSpeedDelayR);
  TRACE(F("moveStepR: "), _settings.moveStepR);
  TRACE(F("periodTimeoutSecR: "), _settings.periodTimeoutSecR, F(" "), F("Min: "), _settings.periodTimeoutSecMin, F(" "), F("Max: "), _settings.periodTimeoutSecMax);
}

void Move(const MoveStyle &style)
{
  currentMenu = Menu::Move;
  lcd.clear();
  lcd.print(F("Move..."));  

  LCDPrintWiFiStatus();
  LCDPrintRandomValues();

  servo.attach();

  if(style == MoveStyle::Normal)
  {
    //servo.pos(_settings.startAngle);
    delay(_settings.moveSpeedDelayR);

    TRACE(F("Move Forward"));
    for(int pos = _settings.startAngle; pos <= _settings.moveAngleR; pos += _settings.moveStepR)
    {
      servo.pos(pos, _settings.moveSpeedDelayR);
      yield(); // watchdog
    }

    delay(_settings.moveSpeedDelayR);

    TRACE(F("Move back"));
    for(int pos = _settings.moveAngleR; pos >= _settings.startAngle; pos -= _settings.moveStepR)
    {
      servo.pos(pos, _settings.moveSpeedDelayR);
      yield(); // watchdog
    }
  }  
  currentMenu = Menu::Main;  
}

void BacklightOn(const unsigned long &currentTicks)
{
  lcd.backlight();
  backlightStartTicks = currentTicks == 0 ? millis() : currentTicks;
}

const bool &CheckBacklightDelayAndReturnToMainMenu(const unsigned long &currentTicks)
{
  if(backlightStartTicks > 0 && currentTicks - backlightStartTicks >= BACKLIGHT_DELAY)
  {      
    backlightStartTicks = 0;
    currentMenu = Menu::Main;
    lcd.noBacklight();
    btnOK.resetTicks();    
    return true;
  }
  return false;
}

void SaveChanges()
{
  TRACE(F("Save..."));
  SaveSettings();
  lcd.clear();
  lcd.print(F("Save..."));
  DebugPrintSettingsValues();
  delay(700); 
}

void MainMenuStatus(const unsigned long &currentInSec)
{
  if(currentMenu == Menu::Main)
  {    
    uint16_t remainSec = _settings.periodTimeoutSecR - currentInSec;
    String remain = String(remainSec);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("N:")); lcd.print(remain); lcd.print(F("s."));
    lcd.print(F("/")); lcd.print(_settings.periodTimeoutSecR); //lcd.print(F("s."));

    LCDPrintWiFiStatus();

    LCDPrintRandomValues();
  }
}

void LCDPrintWiFiStatus()
{
  String W = (WiFi.status() == WL_CONNECTED) ? F("W") : F("");
  lcd.setCursor(LCD_COLS - W.length(), 0);
  lcd.print(W);
}

void LCDPrintBoundMenu(const int& currentPos)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(_settings.startAngle); 
  lcd.print(F(" > ")); lcd.print(currentPos); lcd.print(F(" < "));
  lcd.print(_settings.endAngle); 

  lcd.setCursor(0, 1);
  lcd.print(F("SaveMin")); 
  lcd.print(F("|"));
  lcd.print(F("|"));
  lcd.print(F("MaxSave"));
}

void LCDPrintRandomValues()
{
  lcd.setCursor(0, 1);    
  lcd.print(F("A:")); lcd.print(_settings.moveAngleR); lcd.print(F(" "));    
  lcd.print(F("S:")); lcd.print(_settings.moveStepR); lcd.print(F(" "));
  lcd.print(F("T:")); lcd.print(_settings.moveSpeedDelayR); lcd.print(F(" "));
}

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // Reset WiFi
  { 
    _settings.resetFlag = 1985;
    SaveSettings();
    ESP.restart();
  }

  if(debugButtonFromSerial == 2) // Reset Settings
  { 
    _settings.init();
    SaveSettings();  
    DebugPrintSettingsValues();  
  }

  if(debugButtonFromSerial == 3) // Print Settings
  {     
    DebugPrintSettingsValues();  
  }

  if(debugButtonFromSerial == 130) // Format FS and reset WiFi and restart
  { 
    INFO(F("\t\t\tFormat..."));   
    SPIFFS.format();    
    ESP.restart();
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();
    INFO(F("Input: "), readFromSerial);
    debugButtonFromSerial = readFromSerial.toInt(); 
  }
}
