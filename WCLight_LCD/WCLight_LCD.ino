
#include <EEPROM.h>
#include <avr/wdt.h>

#include <LiquidCrystal_I2C.h>
#include "../../Shares/Button.h"
#include "Light.h"

#define RELEASE

#define WC_IN_PIN 13
#define WC_SW_PIN 3 /*White*/

#define BATH_IN_PIN 12
#define BATH_SW_PIN 2 /*Brown*/

#define RESET_BTN_PIN 11

#define BACKLIGHT_PIN 10

#define SHOW_STATUS_DELAY 1000u
#define BACKLIGHT_DELAY 40000u

#define WC_DOOR_OPEN_MORE_THEN 10000u
#define BT_DOOR_OPEN_MORE_THEN 20000u

//EEPROM
#define EEPROM_SETTINGS_ADDR 10

//DebounceTime
#define DebounceTime 50

const unsigned long ULONG_MAX = 0UL - 1UL;

enum StatPage
{
  Visits = 0,
  Max,  
  Avg,
  Med,
  Filled,
  End = STATISTIC_DATA_LENGHT + 5,
  Min,
};

short currentPage = End;

Light wcLight;
Light btLight;

Button WCBtn(WC_IN_PIN);
Button BathBtn(BATH_IN_PIN);
Button BacklightBtn(BACKLIGHT_PIN);
Button ResetBtn(RESET_BTN_PIN);

unsigned long backlightStart = 0;
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

  wcLight.CalculateTimeAndStatistics(/*fromStart:*/true);
  btLight.CalculateTimeAndStatistics(/*fromStart:*/true);

  Backlight();
  Switch(); 
  
  PrintToSerial(millis(), "", "");

  #ifdef DEBUG
  PrintStatisticDataAll();  
  #endif

  ClearRow(0);
  ClearRow(1);
  PrintStatus();

  EnableWatchDog();  
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

  const auto currentTicks = millis();

  if(!(currentTicks % SHOW_STATUS_DELAY))
  {      
    PrintStatus(/*fromTimer:*/true);    
  }

  if(backlightStart > 0 && currentTicks - backlightStart >= BACKLIGHT_DELAY)
  {      
    lcd.noBacklight();
    backlightStart = 0;
  }

  if(BacklightBtn.isPressed() || IsDebugPressed(BACKLIGHT_PIN, BKLPressed))
  {
    Serial.println("The ""BacklightBtn"" is pressed: ");
    //if(backlightStart == 0)
    {
      Backlight();
    }
  }

  if(BacklightBtn.isReleased() || IsDebugPressed(BACKLIGHT_PIN, BKLPressed))
  {
    if(BacklightBtn.isLongPress())
    {
      BacklightBtn.resetTicks();
      if(currentPage != End)
      {
        currentPage = End;
        PrintStatus();
      }
      else
      {
        currentPage = Visits;
        PrintStatistics(currentPage);
      }
    }
    else
    {     
      if(currentPage >= Visits && currentPage < End)
      {        
        currentPage += 1;
        PrintStatistics(currentPage);        
      }
      else
      {
        PrintStatus();
      }      
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
    BKLPressed = false;
    #endif

    Backlight(); 
    Switch();
    PrintStatus(true);
    SaveSettings();
  }

  if(ResetBtn.isReleased() || IsDebugReleased(RESET_BTN_PIN, RSTPressed))
  {
    Serial.println("The ""ResetBtn"" is released: ");
    if(ResetBtn.isLongPress())
    {
      ResetBtn.resetTicks();
      Serial.println("The ""ResetBtn"" is long pressed: ");
      Serial.println("Reset Time...");
      ResetTime();

      ResetStatistic();

      Backlight();
      Switch();
      PrintStatus(true);
      SaveSettings();
    }
  }

  if(WCBtn.isPressed() || IsDebugPressed(WC_IN_PIN, WCPressed))
  {    
    Serial.println("The ""WCBtn"" is pressed: ");
    
    if(wcLight.Pressed())
    {
       SaveSettings();
    }

    Backlight();
    Switch();
    PrintStatus();    
  }

  if(WCBtn.isReleased() || IsDebugReleased(WC_IN_PIN, WCPressed))
  {    
    Serial.println("The ""WCBtn"" is released: ");

    if(WCBtn.getTicks() >= WC_DOOR_OPEN_MORE_THEN)
    {
      wcLight.setToOff();
      WCBtn.resetTicks();
    }

    if(wcLight.Released())
    {
      SaveSettings();
    }

    Backlight();
    Switch();    
    PrintStatus();
  }

  if(BathBtn.isPressed() || IsDebugPressed(BATH_IN_PIN, BTPressed))
  {    
    Serial.println("The ""BathBtn"" is pressed: ");
        
    if(btLight.Pressed())
    {
      SaveSettings();
    }

    Backlight();
    Switch();    
    PrintStatus();
  }

  if(BathBtn.isReleased() || IsDebugReleased(BATH_IN_PIN, BTPressed))
  {    
    Serial.println("The ""BathBtn"" is released: ");

    if(BathBtn.getTicks() >= BT_DOOR_OPEN_MORE_THEN)
    {
      btLight.setToOff();
      BathBtn.resetTicks();
    }

    if(btLight.Released())
    {
      SaveSettings();
    }

    Backlight();
    Switch();    
    PrintStatus();
  }

  HandleDebugSerialCommands();  
}

void HandleDebugSerialCommands()
{  
  if(debugButtonFromSerial == 1)
  {
    PrintToSerial(millis(), "", "");
    PrintStatisticDataAll();
  }

  if(debugButtonFromSerial == 2)
  {    
    ResetTime();
    SaveSettings();
    PrintStatus();
  }

  if(debugButtonFromSerial == 3)
  {    
    ResetStatistic();
    PrintStatisticDataAll();
    SaveSettings();
    PrintStatus();
  }

  //Unit Test Helpers::Time::HumanizeTime
  if(debugButtonFromSerial == 10)
  {
    char buff[10];
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + MonthSecs * 2, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + MonthSecs * 2, buff, true));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + WeekSecs * 3, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + DaySecs * 4, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + HourSecs * 5, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + MinuteSecs * 6, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(YearSecs * 2 + 31, buff, false));

    Serial.println(Helpers::Time::HumanizeTime(WeekSecs * 2 + DaySecs * 4, buff, false));
    
    Serial.println(Helpers::Time::HumanizeTime(HourSecs * 2 + 30, buff, false));


    Serial.println(Helpers::Time::HumanizeTime(5, buff, false));
    Serial.println(Helpers::Time::HumanizeTime(5, buff, true));
    Serial.println(Helpers::Time::HumanizeTime(59, buff, false));
  }

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

void ResetTime()
{
  wcLight.resetTime();
  btLight.resetTime();
}

void ResetStatistic()
{
  Serial.println("Reset Statistic...");

  wcLight.resetStatistic();
  btLight.resetStatistic();
}

void PrintStatus(){PrintStatus(false);}
void PrintStatus(const bool &fromTimer)
{ 
  const auto &wcStart = wcLight.GetStartTicks();
  const auto &btStart = btLight.GetStartTicks();

  if((wcStart > 0 || btStart > 0) || !fromTimer)
  {
    unsigned long current = 0;

    char wcBuff[20];
    char btBuff[20];

    sprintf(wcBuff, "%s:%s|%s", wcLight.settings.State == ON ? "W" : "w", wcLight.GetTotalTime(), wcLight.GetLastTime(current));
    sprintf(btBuff, "%s:%s|%s", btLight.settings.State == ON ? "B" : "b", btLight.GetTotalTime(), btLight.GetLastTime(current));

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
        sprintf(btBuff, "%s:%s|%s", btLight.settings.State == ON ? "B" : "b", btLight.GetTotalTime(), btLight.GetLastTime(current));
        ClearRow(1);
        lcd.print(btBuff);
      }
    }    

    #ifdef DEBUG
    current = millis();
    PrintToSerial(current, wcBuff, btBuff);
    #endif
  }
}

void PrintStatistics(const short &page)
{
  char wcBuff[20];
  char btBuff[20];

  switch(page)
  {
    case Visits:
      sprintf(wcBuff, "W:Visits:%d", wcLight.settings.Count);
      sprintf(btBuff, "B:Visits:%d", btLight.settings.Count);
    break;
    case Max:
    {
      char buff1[10];
      char buff2[10];
      sprintf(wcBuff, "W:Max:%s", Light::HumanizeShortTime(wcLight.statistic.Max, buff1));
      sprintf(btBuff, "B:Max:%s", Light::HumanizeShortTime(btLight.statistic.Max, buff2));
      break;
    }
    case Min:
    {
      char buff1[10];
      char buff2[10];
      sprintf(wcBuff, "W:Min:%s", Light::HumanizeShortTime(wcLight.statistic.Min, buff1));
      sprintf(btBuff, "B:Min:%s", Light::HumanizeShortTime(btLight.statistic.Min, buff2));
      break;
    }
    case Avg:
    {
      char buff1[10];
      char buff2[10];
      sprintf(wcBuff, "W:Avg:%s", Light::HumanizeShortTime(wcLight.statistic.Avg, buff1));
      sprintf(btBuff, "B:Avg:%s", Light::HumanizeShortTime(btLight.statistic.Avg, buff2));
      break;
    }
    case Med:
    {
      char buff1[10];
      char buff2[10];
      sprintf(wcBuff, "W:Med:%s", Light::HumanizeShortTime(wcLight.statistic.Med, buff1));
      sprintf(btBuff, "B:Med:%s", Light::HumanizeShortTime(btLight.statistic.Med, buff2));
      break;
    }
    case Filled:
    {      
      sprintf(wcBuff, "W:Filled:%d/%d", wcLight.statistic.GetFilledCount(), STATISTIC_DATA_LENGHT);
      sprintf(btBuff, "B:Filled:%d/%d", btLight.statistic.GetFilledCount(), STATISTIC_DATA_LENGHT);
      break;
    }    
    case End:
    {
      PrintStatus();
      return;    
    }
    default:
    {
      if(page > Filled && page < End)
      {
        char buff1[10];
        char buff2[10];
        short idx = End - page - 1;
        sprintf(wcBuff, "W:#%d=%s", STATISTIC_DATA_LENGHT - idx, Light::HumanizeShortTime(wcLight.statistic.GetByIndex(idx), buff1));
        sprintf(btBuff, "B:#%d=%s", STATISTIC_DATA_LENGHT - idx, Light::HumanizeShortTime(btLight.statistic.GetByIndex(idx), buff2));
        break;
      }
      PrintStatus();
      return;
    }
  }

  ClearRow(0);
  lcd.print(wcBuff);
  ClearRow(1);
  lcd.print(btBuff);

  #ifdef DEBUG
  Serial.println(wcBuff);
  Serial.println(btBuff);  
  #endif
}

void PrintStatisticDataAll()
{
  PrintStatistics(Visits);
  PrintStatistics(Max);
  PrintStatistics(Min);
  PrintStatistics(Avg);
  PrintStatistics(Med);
  PrintStatistics(Filled);
  wcLight.statistic.PrintData();
  Serial.println();
  btLight.statistic.PrintData();  
  Serial.println();
}

void PrintToSerial(const unsigned long &current, const char *wcBuff, const char *btBuff)
{
  char buffer[100];
  sprintf(buffer, "WC:[%s][%d][%s]{c:%d}(time:%lu) BT:[%s][%d][%s]{c:%d}(time:%lu)"
        , wcLight.GetStatus(), wcLight.settings.Count, wcBuff, wcLight.settings.Counter, current - wcLight.GetStartTicks()
        , btLight.GetStatus(), btLight.settings.Count, btBuff, btLight.settings.Counter, current - btLight.GetStartTicks());
  Serial.println(buffer);
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

  short offset = EEPROM_SETTINGS_ADDR;
  
  EEPROM.put(offset, wcLight.settings);  
  offset += sizeof(Light::Settings);
  EEPROM.put(offset, btLight.settings);
  offset += sizeof(Light::Settings);

  EEPROM.put(offset, wcLight.statistic);  
  offset += sizeof(Statistic);
  EEPROM.put(offset, btLight.statistic);
  offset += sizeof(Statistic);  
}

void LoadSettings()
{
  Serial.println("Load Settings...");

  short offset = EEPROM_SETTINGS_ADDR;  
  EEPROM.get(offset, wcLight.settings);  
  offset += sizeof(Light::Settings);  
  EEPROM.get(offset, btLight.settings);  
  offset += sizeof(Light::Settings);  

  EEPROM.get(offset, wcLight.statistic);  
  offset += sizeof(Statistic);
  EEPROM.get(offset, btLight.statistic);
  offset += sizeof(Statistic);  

  if(wcLight.settings.Count <= -1 || wcLight.settings.Counter <= -1 || btLight.settings.Count <= -1 || btLight.settings.Counter <= -1)
  {
    Serial.println("Settings corrupted...");

    wcLight.resetState();
    wcLight.resetTime();

    btLight.resetState();
    btLight.resetTime();
  }  

  if(wcLight.statistic.Max == ULONG_MAX || wcLight.statistic.Avg == ULONG_MAX)
  {
    Serial.println("WC Statistic corrupted...");
    wcLight.resetStatistic();
  }
  if(btLight.statistic.Max == ULONG_MAX || btLight.statistic.Avg == ULONG_MAX)
  {
    Serial.println("BT Statistic corrupted...");
    btLight.resetStatistic();
  }
}

const bool IsDebugPressed(const short &value, bool &pressedState)
{
  //#ifdef DEBUG
  if(debugButtonFromSerial == value && !pressedState)
  {
    pressedState = true;
    debugButtonFromSerial = 0;
    return true;
  }
  //#endif
  
  return false;  
}

const bool IsDebugReleased(const short &value, bool &pressedState)
{
  //#ifdef DEBUG
  if(debugButtonFromSerial == value && pressedState)
  {
    pressedState = false;
    debugButtonFromSerial = 0;
    return true;
  }
  //#endif
  
  return false;  
}