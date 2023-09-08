
#include <EEPROM.h>
#include <avr/wdt.h>
//#include <arduino-timer.h>
#include <LiquidCrystal_I2C.h>
#include "../../Shares/Button.h"


#include "Light.h"

//#define DEBUG

#define WC_IN_PIN 13
#define WC_SW_PIN 3 /*White*/

#define BATH_IN_PIN 12
#define BATH_SW_PIN 2 /*Brown*/

#define RESET_BTN_PIN 11

#define BACKLIGHT_PIN 10

#define SHOW_STATUS_DELAY 1000u
#define BACKLIGHT_DELAY 30000u

//EEPROM
#define EEPROM_SETTINGS_ADDR 10

//DebounceTime
#define DebounceTime 50

Light wcLight;
Light btLight;

ezButton WCBtn(WC_IN_PIN);
ezButton BathBtn(BATH_IN_PIN);
ezButton BacklightBtn(BACKLIGHT_PIN);
Button ResetBtn(RESET_BTN_PIN);

//auto lcdPrintTimer = timer_create_default();

unsigned long backlightStart = 0;

bool UpdateStatus = false;
bool BackLight = false;
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool wcPressed = false;
bool btPressed = false;
bool rstPressed = false;
bool bklPressed = false;

#define WCPressed wcPressed
#define BTPressed btPressed
#define RSTPressed rstPressed
#define BKLPressed bklPressed

int debugButtonFromSerial = 0;

void setup()
{
  lcd.init();
  lcd.init();

  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");
  //lcd.scrollDisplayLeft();

  Serial.begin(9600);                // initialize serial
     
  Serial.println();
  Serial.println();
  Serial.println("!!!! START WC Toilet & Bathroom Auto light !!!!");

  WCBtn.setDebounceTime(DebounceTime);
  BathBtn.setDebounceTime(DebounceTime);
  ResetBtn.setDebounceTime(DebounceTime);
  BacklightBtn.setDebounceTime(DebounceTime);

  pinMode(WC_SW_PIN, INPUT_PULLUP);
  pinMode(WC_SW_PIN, OUTPUT);
  pinMode(BATH_SW_PIN, INPUT_PULLUP);
  pinMode(BATH_SW_PIN, OUTPUT);

  LoadSettings(); 

  wcLight.HandleState(/*fromStart:*/true);
  btLight.HandleState(/*fromStart:*/true);

  Backlight();
  Switch();
  ClearRow(0);
  ClearRow(1);
  PrintStatus();  

  EnableWatchDog();
  //lcdPrintTimer.every(2000, PrintStatus);
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  Serial.println("Watchdog enabled.");
}

void loop()
{
  WCBtn.loop();
  BathBtn.loop();
  ResetBtn.loop();
  BacklightBtn.loop();

  //lcdPrintTimer.tick();

  if(!(millis() % SHOW_STATUS_DELAY))
  {      
    PrintStatus(/*fromTimer:*/true);    
  }

  if(backlightStart > 0 && millis() - backlightStart >= BACKLIGHT_DELAY)
  {      
    lcd.noBacklight();
    backlightStart = 0;
  }

  if(BacklightBtn.isPressed() || IsDebugPressed(BACKLIGHT_PIN, BKLPressed))
  {
    Serial.println("The ""BacklightBtn"" is pressed: ");
    if(backlightStart == 0)
    {
      Backlight();
    }
  }

  if(ResetBtn.isPressed() || IsDebugPressed(RESET_BTN_PIN, RSTPressed))
  {    
    Serial.println("The ""ResetBtn"" is pressed: ");
    Serial.println("Reset State...");
    ResetState();

    #ifdef DEBUG
    WCPressed = false;
    BTPressed = false;
    #endif

    Backlight(); 
    Switch();
    PrintStatus();
    SaveSettings();
  }

  if(ResetBtn.isReleased() || IsDebugReleased(RESET_BTN_PIN, RSTPressed))
  {
    Serial.println("The ""ResetBtn"" is released: ");
    if(ResetBtn.isLongPress())
    {
      ResetBtn.resetTicks();
      Serial.println("The ""ResetBtn"" is long pressed: ");
      Serial.println("Reset Statistic...");
      ResetStatistic();

      Backlight();
      Switch();
      PrintStatus();
      SaveSettings();
    }
  }

  if(WCBtn.isPressed() || IsDebugPressed(WC_IN_PIN, WCPressed))
  {    
    Serial.println("The ""WCBtn"" is pressed: ");
    
    wcLight.Pressed();

    Backlight();

    Switch();
    SaveSettings();
    PrintStatus();    
  }

  if(WCBtn.isReleased() || IsDebugReleased(WC_IN_PIN, WCPressed))
  {    
    Serial.println("The ""WCBtn"" is released: ");

    wcLight.Released();

    Backlight();

    Switch();
    SaveSettings();
    PrintStatus();
  }

  if(BathBtn.isPressed() || IsDebugPressed(BATH_IN_PIN, BTPressed))
  {    
    Serial.println("The ""BathBtn"" is pressed: ");
        
    btLight.Pressed();

    Backlight();

    Switch();
    SaveSettings();
    PrintStatus();
  }

  if(BathBtn.isReleased() || IsDebugReleased(BATH_IN_PIN, BTPressed))
  {    
    Serial.println("The ""BathBtn"" is released: ");

    btLight.Released();

    Backlight();

    Switch();
    SaveSettings();
    PrintStatus();
  }

  HandleDebugSerialCommands();  
}

void HandleDebugSerialCommands()
{  
  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == -1)
  {
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    debugButtonFromSerial = Serial.readString().toInt();
  }

  wdt_reset();
}

void Switch()
{
  digitalWrite(WC_SW_PIN, wcLight.settings.State);
  digitalWrite(BATH_SW_PIN, btLight.settings.State);  
}

void ResetState()
{
  wcLight.resetState();
  btLight.resetState();
}

void ResetStatistic()
{
  wcLight.resetStatistic();
  btLight.resetStatistic();
}


void PrintStatus(){PrintStatus(false);}
void PrintStatus(const bool &fromTimer)
{ 
  auto wcStart = wcLight.GetStartTicks();
  auto btStart = btLight.GetStartTicks();

  UpdateStatus = wcStart > 0 || btStart > 0;
  if(UpdateStatus || !fromTimer)
  {
    unsigned long current = 0;

    char wcBuff[20];
    char btBuff[20];

    sprintf(wcBuff, "%s:%s|%s", wcLight.settings.State == ON ? "W" : "w", wcLight.GetTotalTime(), wcLight.GetLastTime(current));
    sprintf(btBuff, "%s:%s|%s", btLight.settings.State == ON ? "B" : "b",btLight.GetTotalTime(), btLight.GetLastTime(current));

    if(!fromTimer)
    {
      ClearRow(0);
      lcd.print(wcBuff);
      ClearRow(1);
      lcd.print(btBuff);
    }
    else
    {
      current = millis();
      if(wcStart > 0)
      {
        sprintf(wcBuff, "%s:%s|%s", wcLight.settings.State == ON ? "W" : "w", wcLight.GetTotalTime(), wcLight.GetLastTime(current));
        ClearRow(0);
        lcd.print(wcBuff);
      }

      if(btStart > 0)
      {
        sprintf(btBuff, "%s:%s|%s", btLight.settings.State == ON ? "B" : "b",btLight.GetTotalTime(), btLight.GetLastTime(current));
        ClearRow(1);
        lcd.print(btBuff);
      }
    }    

    #ifdef DEBUG
    current = millis();
    char buffer[100];
    sprintf(buffer, "WC:[%s][%d][%s]{c:%d}(time:%lu) BT:[%s][%d][%s]{c:%d}(time:%lu)"
          , wcLight.GetStatus(), wcLight.settings.Count, wcBuff, wcLight.settings.Counter, current - wcLight.GetStartTicks()
          , btLight.GetStatus(), btLight.settings.Count, btBuff, btLight.settings.Counter, current - btLight.GetStartTicks());
    Serial.println(buffer);
    #endif
  }
}

void ClearRow(const short &row)
{
  lcd.setCursor(0, row);
  lcd.print("                  ");
  lcd.setCursor(0, row);
}

void Backlight()
{
  lcd.backlight();
  backlightStart = millis();
}

void SaveSettings()
{
  Serial.println("Save Settings...");
  EEPROM.put(EEPROM_SETTINGS_ADDR, wcLight.settings);
  EEPROM.put(EEPROM_SETTINGS_ADDR + sizeof(Light::Settings), btLight.settings);
}

void LoadSettings()
{
  Serial.println("Load Settings...");
  EEPROM.get(EEPROM_SETTINGS_ADDR, wcLight.settings);
  EEPROM.get(EEPROM_SETTINGS_ADDR + sizeof(Light::Settings), btLight.settings);

  if(wcLight.settings.Count <= -1 || wcLight.settings.Counter <= -1 || btLight.settings.Count <= -1 || btLight.settings.Counter <= -1)
  {
    Serial.println("Settings corrupted...");

    wcLight.resetState();
    wcLight.resetStatistic();

    btLight.resetState();
    btLight.resetStatistic();
  }  
}

const bool IsDebugPressed(const short &value, bool &pressedState)
{
  #ifdef DEBUG
  if(debugButtonFromSerial == value && !pressedState)
  {
    pressedState = true;
    debugButtonFromSerial = 0;
    return true;
  }
  #endif
  
  return false;  
}

const bool IsDebugReleased(const short &value, bool &pressedState)
{
  #ifdef DEBUG
  if(debugButtonFromSerial == value && pressedState)
  {
    pressedState = false;
    debugButtonFromSerial = 0;
    return true;
  }
  #endif
  
  return false;  
}