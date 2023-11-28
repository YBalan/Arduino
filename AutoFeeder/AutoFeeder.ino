#include <EEPROM.h>
#include <avr/wdt.h>
#include <DS323x.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define RELEASE
//#define TRACE
//#define INFO

#ifdef DEBUG
  #define INFO
  #define TRACE
#endif

#include "DEBUGHelper.h"

#include "../../Shares/Button.h"
#include "FeedStatusInfo.h"
#include "FeedSettings.h"
#include "FeedScheduler.h"
#include "FeedMotor.h"
#include "LcdProgressBar.h"
#include "AutoFeederMenuHelper.h"

//DebounceTime
#define DEBOUNCE_TIME 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

#define LCD_COLS 16
#define LCD_ROWS 2
#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

DS323x rtc;

Helpers::LcdProgressBar progress(lcd, 1, 0, LCD_COLS, Helpers::LcdProgressSettings::NUMBERS_CENTER);

#define SERVO_PIN 9
#define MOTOR_SHOW_PROGRESS true
Feed::FeedMotor servo(SERVO_PIN, progress);

Feed::Settings settings;

#define OK_PIN 10
#define UP_PIN 11
#define DW_PIN 12
#define RT_PIN 13

#define MANUAL_FEED_PIN 8
#define REMOTE_FEED_PIN 7
#define PAW_FEED_PIN 6
#define PAW_LED_PIN 2
#ifdef RELEASE
#define PAW_BTN_AVALIABLE_AFTER 900000//(15 * 60 * 1000)
#else
#define PAW_BTN_AVALIABLE_AFTER 30000//(15 * 60 * 1000)
#endif
#define PAW_BTN_AVAILABLE_AFTER_LAST_FEEDING false
unsigned long pawBtnAvaliabilityTicks = 0;

Button btnOK(OK_PIN);
ezButton btnUp(UP_PIN);
ezButton btnDw(DW_PIN);
ezButton btnRt(RT_PIN);

Button btnManualFeed(MANUAL_FEED_PIN);
ezButton btnRemoteFeed(REMOTE_FEED_PIN);
ezButton btnPawFeed(PAW_FEED_PIN);

//Menus Functions
short &ShowHistory(short &pos, const short &minPositions = 0, const short &maxPositions = FEEDS_STATUS_HISTORY_COUNT);
short &ShowSchedule(short &pos, const short &minPositions = 0, const short &maxPositions = FEEDS_SCHEDULER_SETTINGS_COUNT);
short &ShowFeedCount(short &pos, const short &minPositions = 0, const short &maxPositions = MAX_FEED_COUNT);

void setup() 
{
  Serial.begin(9600);  
  while (!Serial);

  Serial.println();
  Serial.println();
  Serial.println("!!!! Start Auto Feeder !!!!");
  S_INFO3(__DATE__, " ", __TIME__);  

  lcd.init();
  lcd.init();

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");    

  Wire.begin();
  //delay(2000);  
  rtc.attach(Wire);
  
  ShowLcdTime(1000, rtc.now());

  btnOK.setDebounceTime(DEBOUNCE_TIME);
  btnUp.setDebounceTime(DEBOUNCE_TIME);
  btnDw.setDebounceTime(DEBOUNCE_TIME);
  btnRt.setDebounceTime(DEBOUNCE_TIME);

  btnManualFeed.setDebounceTime(DEBOUNCE_TIME);
  btnRemoteFeed.setDebounceTime(DEBOUNCE_TIME);
  btnPawFeed.setDebounceTime(DEBOUNCE_TIME);

  pinMode(SERVO_PIN, INPUT_PULLUP);

  pinMode(PAW_LED_PIN, OUTPUT);
  digitalWrite(PAW_LED_PIN, HIGH);

  servo.Init();

  LoadSettings();
  PrintToSerialStatus();
  EnableWatchDog();

  ShowLastAction();
}

void loop() 
{
  const auto dtNow = rtc.now();
  const auto current = millis();
  
  ShowLcdTime(current, dtNow);
  
  btnOK.loop();
  btnUp.loop();
  btnDw.loop();
  btnRt.loop();

  btnManualFeed.loop();
  btnRemoteFeed.loop();
  btnPawFeed.loop();  

  if(btnOK.isPressed())
  {
    S_INFO2("Ok ", BUTTON_IS_PRESSED_MSG);
    BacklightOn();
  }

  if(btnRt.isPressed())
  {
    S_INFO4("BACK ", BUTTON_IS_PRESSED_MSG, " menu: ", currentMenu);
    BacklightOn();

    if(currentMenu != Menu::Main)
    {
      SaveSettings();
    }
    currentMenu = Menu::Main;
    settings.FeedScheduler.SetNextAlarm(dtNow);
    ShowLastAction();
  }

  if(btnOK.isReleased())
  {
    S_INFO4("Ok ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);
    if(btnOK.isLongPress())
    {
      S_INFO2("Ok ", BUTTON_IS_LONGPRESSED_MSG);
      if(currentMenu == Menu::Main)
      {
        currentMenu = Menu::History;
        ShowHistory(historyMenuPos = 0);
      }else      
      if(currentMenu == Menu::History)
      {
        currentMenu = Menu::Schedule;
        ShowSchedule(scheduleMenuPos = settings.FeedScheduler.Set);
      }else
      if(currentMenu == Menu::Schedule)
      {
        currentMenu = Menu::FeedCount;
        ShowFeedCount(feedCountMenuPos = settings.RotateCount - 1);
      }
      else currentMenu == Menu::Main;
      btnOK.resetTicks();
    }
  }

  if(btnUp.isReleased())
  {
    S_INFO4("UP ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);    
    BacklightOn();

    if(currentMenu == Menu::History)
    { 
      ShowHistory(++historyMenuPos);      
    }else
    if(currentMenu == Menu::Schedule)
    {
      ShowSchedule(++scheduleMenuPos);
    }else
    if(currentMenu == Menu::FeedCount)
    {
      ShowFeedCount(++feedCountMenuPos);
    }
    else
    {
      #ifdef DEBUG
      if(DoFeed(settings.RotateCount, Feed::Status::TEST, MOTOR_SHOW_PROGRESS))
      {
        settings.SetLastStatus(Feed::StatusInfo(Feed::Status::TEST, dtNow));
      }
      ShowLastAction();
      #endif
    }
  }

  if(btnDw.isReleased())
  {
    S_INFO4("DOWN ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);    
    BacklightOn();

    if(currentMenu == Menu::History)
    {           
      ShowHistory(--historyMenuPos);      
    }else
    if(currentMenu == Menu::Schedule)
    {
      ShowSchedule(--scheduleMenuPos);
    }else
    if(currentMenu == Menu::FeedCount)
    {
      ShowFeedCount(--feedCountMenuPos);
    }
    else
    {
      #ifdef DEBUG
      if(DoFeed(settings.RotateCount, Feed::Status::TEST, MOTOR_SHOW_PROGRESS))
      {
        settings.SetLastStatus(Feed::StatusInfo(Feed::Status::TEST, dtNow));
      }
      ShowLastAction();
      #endif
    }
  }

  if(currentMenu == Menu::Main)
  {
  if(btnManualFeed.isReleased())
  {
    //S_INFO2("Manual at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));    
    BacklightOn();

    if(DoFeed(settings.RotateCount, Feed::Status::MANUAL, MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::MANUAL, dtNow));
    }

    SaveSettings();
    ShowLastAction();    
  }
  else
  if(btnRemoteFeed.isReleased())
  {    
    //S_INFO2("Remoute at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    if(DoFeed(settings.RotateCount, Feed::Status::REMOUTE, MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::REMOUTE, dtNow));
    }

    SaveSettings();
    ShowLastAction();    
  }
  else
  if(btnPawFeed.isReleased())
  {
    //S_INFO2("Paw at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    if(pawBtnAvaliabilityTicks == 0)
    {      
      BacklightOn();

      if(DoFeed(settings.RotateCount, Feed::Status::PAW, MOTOR_SHOW_PROGRESS))
      {
        settings.SetLastStatus(Feed::StatusInfo(Feed::Status::PAW, dtNow));
      }

      pawBtnAvaliabilityTicks = current;

      digitalWrite(PAW_LED_PIN, LOW);
      
      SaveSettings();
      ShowLastAction();      
    }
    else
    {
      //ClearNextTime(); 
      ClearRow(1);      
      
      lcd.print("Wait ");       
      PrintTime(lcd, (PAW_BTN_AVALIABLE_AFTER  - (current - pawBtnAvaliabilityTicks)) / 1000);
      lcd.print("...");

      delay(700);
    }
  }
  else
  if(settings.FeedScheduler.IsTimeToAlarm(dtNow))
  {
    //S_INFO2("Schedule at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    if(DoFeed(settings.RotateCount, Feed::Status::SCHEDULE, MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::SCHEDULE, dtNow));
    }    

    SaveSettings();
    ShowLastAction();    
  }  
  }
 
  CheckPawButtonAvaliable(current);
  CheckBacklightDelayAndReturnToMainMenu(millis()); 
  HandleDebugSerialCommands();
}

const bool DoFeed(const short &feedCount, const Feed::Status &source, const bool &showProgress)
{
  S_INFO2("DoFead count: ", feedCount);
  ClearRow(0);
  lcd.print(" Feeding... -"); lcd.print(Feed::GetFeedStatusString(source, /*shortView:*/false));
  return servo.DoFeed(settings.CurrentPosition, feedCount, showProgress, btnRt);
}

//Menu
void ShowLastAction()
{  
  const Feed::StatusInfo &lastStatus = settings.GetLastStatus();  
  S_TRACE2("LAST: ", lastStatus.Status != Feed::Status::Unknown ? lastStatus.ToString() : NOT_FED_YET_MSG);

  ClearRow(0);  
  if(lastStatus.Status != Feed::Status::Unknown)
  {
    lcd.print("LAST: "); lcd.print(lastStatus.ToString());    
  }
  else
  {
    lcd.print(NOT_FED_YET_MSG);
  }

  ShowNextFeedTime();
}

void ShowNextFeedTime()
{
  ClearNextTime();
  if(settings.FeedScheduler.Set != Feed::ScheduleSet::NotSet)
  {    
    const auto &nextDt = settings.FeedScheduler.GetNextAlarm();
    lcd.print("N:"); nextDt.hour() != 255 ? lcd.print(nextDt.GetTimeWithoutSeconds()) : lcd.print(0);    
  }
  else
  {
    lcd.print(settings.FeedScheduler.SetToString(/*shortView:*/false));
  } 
}

void ClearNextTime()
{
  ClearRow(/*row:*/1, /*startColumn:*/0, /*endColumn:*/8, /*gotoX:*/0); 
}

short &ShowHistory(short &pos, const short &minPositions, const short &maxPositions)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;

  const short idx = maxPositions - pos - 1;
  const Feed::StatusInfo &status = settings.GetStatusByIndex(idx);
  S_TRACE4("Hist: ", idx + 1, ": ", status.ToString());

  ClearRow(0);
  if(status.Status != Feed::Status::Unknown)
  {    
    lcd.print('#'); lcd.print(idx + 1); lcd.print(": "); lcd.print(status.ToString());
    ClearNextTime();
    lcd.print(status.GetDateString());
  } 
  else
  {
    lcd.print('#'); lcd.print(idx + 1); lcd.print(": "); lcd.print(NO_VALUES_MSG);
  } 

  return pos;
}

short &ShowSchedule(short &pos, const short &minPositions, const short &maxPositions)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;  

  ClearRow(0);
  lcd.print("Set: "); lcd.print(Feed::GetSchedulerSetString(pos, /*shortView:*/false));

  settings.FeedScheduler.Set = pos;

  S_TRACE4("Sched: ", pos, ": ", settings.FeedScheduler.SetToString());

  settings.FeedScheduler.SetNextAlarm(rtc.now());

  return pos;
}

short &ShowFeedCount(short &pos, const short &minPositions, const short &maxPositions)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;
  S_TRACE2("Feed Count: ", pos + 1);

  ClearRow(0);
  lcd.print("Feed Count: "); lcd.print(pos + 1);

  settings.RotateCount = pos + 1;

  return pos;
}

short debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // SHOW DateTime
  {
    PrintToSerialDateTime();   
  }

  if(debugButtonFromSerial == 2) //RESET Settings
  {
    settings.Reset();
    SaveSettings();
    ShowLastAction();
  }

  if(debugButtonFromSerial == 3)
  {
    PrintToSerialStatus();
  }
  
  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == 11)
  {
    S_INFO("Reset in 8s...");
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();

    S_INFO2("Input: ", readFromSerial);

    if(SetCurrentDateTime(readFromSerial, rtc))
    {      
      PrintToSerialDateTime();
    }
    else
    {    
      debugButtonFromSerial = readFromSerial.toInt();      
    }
  }
  
  wdt_reset();
}

//Format: yyyyMMddhhmmss (20231116163401)
const bool SetCurrentDateTime(const String &value, DS323x &realTimeClock)
{
  if(value.length() >= 12)
  {
    short yyyy = value.substring(0, 4).toInt();
    short MM = value.substring(4, 6).toInt();      
    short dd = value.substring(6, 8).toInt();      

    short HH = value.substring(8, 10).toInt();      
    short mm = value.substring(10, 12).toInt();

    short ss = 0;

    if(yyyy < 2023 || yyyy > 2100)  {S_INFO2("Wrong: ", yyyy);  return false; }
    if(MM < 1 || MM > 12)           {S_INFO2("Wrong: ", MM);    return false; }
    if(dd < 1 || dd > 31)           {S_INFO2("Wrong: ", dd);    return false; }
    if(HH < 0 || HH > 23)           {S_INFO2("Wrong: ", HH);    return false; }
    if(mm < 0 || mm > 59)           {S_INFO2("Wrong: ", mm);    return false; }

    if(value.length() >= 14)
    {
      ss = value.substring(12, 14).toInt();
      if(ss < 0 || ss > 59)         {S_INFO2("Wrong: ", ss); ss = 0;}
    }

    auto dt = DateTime(yyyy, MM, dd, HH, mm, ss);      
    realTimeClock.now(dt);

    return true;
  }
  return false;
}

void PrintToSerialDateTime()
{
  S_INF  ("SYS DT: "); S_INFO3(__DATE__, " ", __TIME__);
  S_INFO2("RTC DT: ", rtc.now().timestamp());  
}

void PrintToSerialStatus()
{
  S_INFO2("CurrentPos: ", settings.CurrentPosition);
  S_INFO2("Sched: ", settings.FeedScheduler.SetToString());
  S_INFO2("Next Alarm: ", settings.FeedScheduler.GetNextAlarm().timestamp());  
  S_INFO2("Feed Count: ", settings.RotateCount);  
}

void BacklightOn()
{
  lcd.backlight();
  backlightStartTicks = millis();
}

const bool &CheckBacklightDelayAndReturnToMainMenu(const unsigned long &currentTicks)
{
  if(backlightStartTicks > 0 && currentTicks - backlightStartTicks >= BACKLIGHT_DELAY)
  {      
    backlightStartTicks = 0;
    currentMenu = Menu::Main;
    lcd.noBacklight();
    ShowLastAction();     
    return true;
  }
  return false;
}

const bool CheckPawButtonAvaliable(const unsigned long &currentTicks)
{
  if(pawBtnAvaliabilityTicks > 0 && currentTicks - pawBtnAvaliabilityTicks >= PAW_BTN_AVALIABLE_AFTER)
  {      
    BacklightOn();
    digitalWrite(PAW_LED_PIN, HIGH);
    pawBtnAvaliabilityTicks = 0;
    return true;
  }
  return false;
}

unsigned long prevTicks = 0;
void ShowLcdTime(const unsigned long &currentTicks, const DateTime &dtNow)
{ 
  if(currentTicks - prevTicks >= 1000)
  {
    if(currentMenu != Menu::History)
    {
      ShowNextFeedTime();
    }
     
    lcd.setCursor(8, 1);
    lcd.print(dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    prevTicks = currentTicks;
  }
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  S_INFO("Watchdog enabled.");
}

void ClearRow(const short &row) { ClearRow(row, 0, LCD_COLS, -1); }
void ClearRow(const short &row, const short &columnStart, const short &columnEnd, const short &gotoX)
{
  lcd.setCursor(columnStart, row);
  for(short ch = columnStart; ch < columnEnd; ch++) lcd.print(' ');
  lcd.setCursor(gotoX == -1 ? columnStart : gotoX, row);
}

void SaveSettings()
{
  S_INFO("Save...");

  ClearRow(1);
  lcd.print("Save...");

  //S_TRACE7("Max:", EEPROM.length(), " Total: ", sizeof(settings), " ", "Hist: ", sizeof(settings.FeedHistory));

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings); 

  delay(500);  
}

void LoadSettings()
{
  S_INFO("Load...");

  ClearRow(1);
  lcd.print("Load...");
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  

  if(settings.CurrentPosition == 65535)
  {
    settings.Reset();    
  }
}