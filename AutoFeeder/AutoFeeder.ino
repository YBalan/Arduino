#include <EEPROM.h>
#include <avr/wdt.h>
#include <DS323x.h>
#include <Servo.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define VER 1.13
#define RELEASE

#define ENABLE_TRACE
//#define ENABLE_TRACE_MAIN
//#define ENABLE_INFO_MAIN
//#define ENABLE_TRACE_FEEDSCHEDULER
//#define ENABLE_TRACE_FEEDDATETIME
//#define ENABLE_TRACE_LCD_PROGRESS
//#define ENABLE_TRACE_MOTOR
#define USE_DHT

#include "DEBUGHelper.h"
#include "Helpers.h"

#include "../../Shares/Button.h"
#include "FeedStatusInfo.h"
#include "FeedSettings.h"
#include "FeedScheduler.h"
#include "FeedMotor.h"
#include "LcdProgressBar.h"
#include "AutoFeederMenuHelper.h"

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

#ifdef USE_DHT
#define HUMADITY_SENSOR_PIN 3
DHT dht(HUMADITY_SENSOR_PIN, DHT11);
#endif

#ifdef RELEASE
#define PAW_BTN_AVALIABLE_AFTER 900000//(15 * 60 * 1000)
#else
#define PAW_BTN_AVALIABLE_AFTER 30000//(30 * 60 * 1000)
#endif
#define PAW_BTN_AVAILABLE_AFTER_LAST_FEEDING false
unsigned long pawBtnAvaliabilityTicks = 0;

Button btnOK(OK_PIN);
ezButton btnUp(UP_PIN);
ezButton btnDw(DW_PIN);
Button btnRt(RT_PIN);

Button btnManualFeed(MANUAL_FEED_PIN);
ezButton btnRemoteFeed(REMOTE_FEED_PIN);
ezButton btnPawFeed(PAW_FEED_PIN);

//Menus Functions
int8_t &ShowHistory(int8_t &pos, const int8_t &minPositions = 0, const int8_t &maxPositions = FEEDS_STATUS_HISTORY_COUNT, const int8_t &step = 1);
int8_t &ShowSchedule(int8_t &pos, const int8_t &minPositions = 0, const int8_t &maxPositions = Feed::ScheduleSet::MAX, const int8_t &step = 1);
int8_t &ShowStartAngle(int8_t &pos, const int8_t &minPositions = 0, const int8_t &maxPositions = MOTOR_MAX_POS / 2, const int8_t &step = MOTOR_START_POS_INCREMENT);
int8_t &ShowRotateCount(int8_t &pos, const int8_t &minPositions = 0, const int8_t &maxPositions = MAX_FEED_COUNT, const int8_t &step = 1);

void setup() 
{
  Serial.begin(9600);  
  while (!Serial);

  Serial.println();
  Serial.println();
  Serial.println("!!!! Start Auto Feeder !!!!");
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);

  lcd.init();
  lcd.init();

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");    

  Wire.begin();  
  rtc.attach(Wire);

#ifdef USE_DHT
  dht.begin();
#endif

  DateTime dtNow = rtc.now();
  ShowLcdTime(1000, dtNow);
  Serial.println(rtc.now().timestamp());

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
  
  settings.FeedScheduler.SetNextAlarm(rtc);

  PrintToSerialStatus();
  EnableWatchDog();

  ShowLastAction();
}

void loop() 
{  
  static uint32_t current = millis();
  current = millis();
  
  ShowLcdTime(current, rtc.now());
  
  btnOK.loop();
  btnUp.loop();
  btnDw.loop();
  btnRt.loop();

  btnManualFeed.loop();
  btnRemoteFeed.loop();
  btnPawFeed.loop();  

  if(btnOK.isPressed())
  {
    INFO("Ok ", BUTTON_IS_PRESSED_MSG);
    BacklightOn();
  }

  if(btnRt.isPressed())
  {
    INFO("BACK ", BUTTON_IS_PRESSED_MSG, " menu: ", currentMenu);
    BacklightOn();

    if(currentMenu != Menu::Main && currentMenu != Menu::Dht && currentMenu != Menu::History)
    {
      SaveSettings();
    }
    currentMenu = Menu::Main;
    settings.FeedScheduler.SetNextAlarm(rtc);
    ShowLastAction();
  }  

  if(btnOK.isReleased())
  {
    INFO("Ok ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);
    if(btnOK.isLongPress())
    {
      btnOK.resetTicks();
      if(btnRt.isLongPress())
      {
        btnRt.resetTicks();

        INFO("Ok ", " ", "BACK ", BUTTON_IS_LONGPRESSED_MSG);
        ChangeTimeMenu(rtc, lcd, btnOK, btnUp, btnDw, btnRt);
        BacklightOn();
        ShowLastAction();        
        return;
      }

      btnRt.resetTicks();

      INFO("Ok ", BUTTON_IS_LONGPRESSED_MSG);
      if(currentMenu == Menu::Main)
      {
        currentMenu = Menu::Dht;
        if(!ShowDht())
        {
          currentMenu = Menu::History;
          ShowHistory(historyMenuPos = 0);
        }
      }else
      if(currentMenu == Menu::Dht)
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
        currentMenu = Menu::StartAngle;
        ShowStartAngle(startAngleMenuPos = settings.StartAngle);
      }else
      if(currentMenu == Menu::StartAngle)
      {
        currentMenu = Menu::RotateCount;
        ShowRotateCount(rotateCountMenuPos = settings.RotateCount - 1);
      }
      else currentMenu == Menu::Main;      
    }
  }
  else
  if(btnRt.isReleased())
  {
    INFO("BACK ", BUTTON_IS_RELEASED_MSG);

    if(btnRt.isLongPress())
    {
      INFO("BACK ", BUTTON_IS_LONGPRESSED_MSG_MSG);
      ClearRow(0);
      lcd.print("V:"); lcd.print(VER); lcd.print("   "); lcd.print(__DATE__);      
    }    
    btnRt.resetTicks();
  }

  if(btnUp.isReleased())
  {
    INFO("UP ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);    
    BacklightOn();

    if(currentMenu == Menu::History)
    { 
      ShowHistory(++historyMenuPos);      
    }else
    if(currentMenu == Menu::Schedule)
    {
      ShowSchedule(++scheduleMenuPos);
    }else
    if(currentMenu == Menu::StartAngle)
    {
      ShowStartAngle(startAngleMenuPos += MOTOR_START_POS_INCREMENT);
    }else
    if(currentMenu == Menu::RotateCount)
    {
      ShowRotateCount(++rotateCountMenuPos);
    }    
  }

  if(btnDw.isReleased())
  {
    INFO("DOWN ", BUTTON_IS_RELEASED_MSG, " menu: ", currentMenu);    
    BacklightOn();

    if(currentMenu == Menu::History)
    {           
      ShowHistory(--historyMenuPos);      
    }else
    if(currentMenu == Menu::Schedule)
    {
      ShowSchedule(--scheduleMenuPos);
    }else
    if(currentMenu == Menu::StartAngle)
    {
      ShowStartAngle(startAngleMenuPos -= MOTOR_START_POS_INCREMENT);
    }else
    if(currentMenu == Menu::RotateCount)
    {
      ShowRotateCount(--rotateCountMenuPos);
    }    
  } 

  if(currentMenu == Menu::Main)
  {  
    if(btnManualFeed.isPressed())
    {
      INFO("Manual ", BUTTON_IS_PRESSED_MSG);
    }
    if(btnManualFeed.isReleased())
    {
      //S_INFO2("Manual at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));    
      INFO("Manual ", BUTTON_IS_RELEASED_MSG);

      BacklightOn();
      if(btnManualFeed.isLongPress())
      {
        INFO("Manual ", BUTTON_IS_LONGPRESSED_MSG);

        btnManualFeed.resetTicks();
        if(DoFeed(settings.RotateCount, Feed::Status::TEST, MOTOR_SHOW_PROGRESS))
        {
          //settings.SetLastStatus(Feed::StatusInfo(Feed::Status::TEST, dtNow));
        }
        ShowLastAction();      
      }
      else
      {    
        if(DoFeed(settings.RotateCount, Feed::Status::MANUAL, MOTOR_SHOW_PROGRESS))
        {
          settings.SetLastStatus(Feed::StatusInfo(Feed::Status::MANUAL, rtc.now(), GetCombinedDht(/*force:*/true)));
        }

        SaveSettings();
        ShowLastAction();
      }
    }
    else
    if(btnRemoteFeed.isReleased())
    {    
      //INFO("Remoute at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
      BacklightOn();

      if(DoFeed(settings.RotateCount, Feed::Status::REMOUTE, MOTOR_SHOW_PROGRESS))
      {
        settings.SetLastStatus(Feed::StatusInfo(Feed::Status::REMOUTE, rtc.now(), GetCombinedDht(/*force:*/true)));
      }

      SaveSettings();
      ShowLastAction();    
    }
    else
    if(btnPawFeed.isReleased())
    {
      //INFO("Paw at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
      if(pawBtnAvaliabilityTicks == 0)
      {      
        BacklightOn();

        if(DoFeed(settings.RotateCount, Feed::Status::PAW, MOTOR_SHOW_PROGRESS))
        {
          settings.SetLastStatus(Feed::StatusInfo(Feed::Status::PAW, rtc.now(), GetCombinedDht(/*force:*/true)));
        }

        pawBtnAvaliabilityTicks = current;

        digitalWrite(PAW_LED_PIN, LOW);
        
        SaveSettings();        
      }
      else
      {
        //ClearNextTime(); 
        ClearRow(1);      
        
        lcd.print("Wait ");       
        PrintTime(lcd, (PAW_BTN_AVALIABLE_AFTER  - (current - pawBtnAvaliabilityTicks)) / 1000);
        lcd.print("...");

        delay(2000);
      }

      ShowLastAction();
    }
    else
    if(settings.FeedScheduler.IsTimeToAlarm(rtc))
    {
      //INFO("Schedule at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
      BacklightOn();

      if(DoFeed(settings.RotateCount, Feed::Status::SCHEDULE, MOTOR_SHOW_PROGRESS))
      {
        settings.SetLastStatus(Feed::StatusInfo(Feed::Status::SCHEDULE, rtc.now(), GetCombinedDht(/*force:*/true)));
      }    

      SaveSettings();
      ShowLastAction();    
    }  
  }
 
  CheckPawButtonAvaliable(current);
  CheckBacklightDelayAndReturnToMainMenu(millis()); 
  HandleDebugSerialCommands();
}

const bool DoFeed(const uint8_t &feedCount, const Feed::Status &source, const bool &showProgress)
{
  INFO("DoFead count: ", feedCount);
  ClearRow(0);
  lcd.print(" Feeding... -"); lcd.print(Feed::GetFeedStatusString(source, /*shortView:*/false));
  return servo.DoFeed(settings.CurrentPosition, settings.StartAngle, feedCount, showProgress, btnRt);
}

//Menu
void ShowLastAction()
{  
  const Feed::StatusInfo &lastStatus = settings.GetLastStatus();  
  TRACE("LAST: ", lastStatus.Status != Feed::Status::Unknown ? lastStatus.ToString() : NOT_FED_YET_MSG);

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

const bool ShowDht()
{
  ClearRow(0);
  #ifdef USE_DHT
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if(!isnan(t) && !isnan(h))
  {    
    lcd.print(t); lcd.print('C'); lcd.print(' '); lcd.print((uint8_t)h); lcd.print('%'); 
    lcd.print(' '); lcd.print((uint8_t)rtc.temperature()); lcd.print('C');
  }
  else
  //return true;
  #endif  
  {
    lcd.print(' '); lcd.print(rtc.temperature()); lcd.print('C');
  }
  return true;
}

const bool ShowDhtForHistory(const uint16_t &dht)
{
  if(dht > 0)
  {
    uint8_t t = 0, h = 0;
    Helpers::StoreHelper::ExtractFromUint16(dht, t, h);
    ClearRow(1, 8, LCD_COLS, 9);
    lcd.print(t); lcd.print('C'); lcd.print(" "); lcd.print(h); lcd.print('%');
    return true;
  }
  return false;
}

const uint16_t GetCombinedDht(const bool &force)
{
  #ifdef USE_DHT
  float h = dht.readHumidity(force);
  float t = dht.readTemperature();
  
  if(!isnan(t) && !isnan(h))
    return Helpers::StoreHelper::CombineToUint16((uint8_t)t, (uint8_t)h);
  #endif
  return Helpers::StoreHelper::CombineToUint16((uint8_t)rtc.temperature(), 0);
}

int8_t &ShowHistory(int8_t &pos, const int8_t &minPositions, const int8_t &maxPositions, const int8_t &step)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;

  const uint8_t idx = maxPositions - pos - 1;
  const Feed::StatusInfo &status = settings.GetStatusByIndex(pos);  

  TRACE("Hist: ", idx + 1, ": ", status.ToString(), "Count per day: ", settings.GetFeedCountPerDay(status.DT.monthDay()));

  ClearRow(0);
  if(status.Status != Feed::Status::Unknown)
  { 
    const uint8_t countPerDay = settings.GetFeedCountPerDay(status.DT.monthDay());
    lcd.print('#'); lcd.print(idx + 1); lcd.print(": "); lcd.print(status.ToString());
    ClearNextTime();
    lcd.print(status.GetDateString()); lcd.print(' '); lcd.print(countPerDay);
    ShowDhtForHistory(status.DHT);
  } 
  else
  {
    lcd.print('#'); lcd.print(idx + 1); lcd.print(": "); lcd.print(NO_VALUES_MSG);
    ClearNextTime();
  } 

  return pos;
}

int8_t &ShowSchedule(int8_t &pos, const int8_t &minPositions, const int8_t &maxPositions, const int8_t &step)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;  

  ClearRow(0);
  lcd.print("Sched: "); lcd.print(Feed::GetSchedulerSetString(pos, /*shortView:*/false));

  settings.FeedScheduler.Set = pos;

  TRACE("Sched: ", pos, ": ", settings.FeedScheduler.SetToString());

  settings.FeedScheduler.SetNextAlarm(rtc);

  ShowNextFeedTime();

  return pos;
}

int8_t &ShowStartAngle(int8_t &pos, const int8_t &minPositions = 0, const int8_t &maxPositions, const int8_t &step)
{
  pos = pos < minPositions ? maxPositions : pos > maxPositions ? minPositions : pos;

  ClearRow(0);
  lcd.print("Start: "); lcd.print(pos); lcd.print('\''); //lcd.print("°");

  settings.StartAngle = pos;  

  TRACE("Start: ", pos, ": ", settings.StartAngle);  

  return pos;
}

int8_t &ShowRotateCount(int8_t &pos, const int8_t &minPositions, const int8_t &maxPositions, const int8_t &step)
{
  pos = pos < minPositions ? maxPositions - 1 : pos >= maxPositions ? minPositions : pos;  

  ClearRow(0);
  lcd.print("Rotate Count: "); lcd.print(pos + 1);

  settings.RotateCount = pos + 1;

  TRACE("Rotate Count: ", pos + 1);

  return pos;
}

uint8_t debugButtonFromSerial = 0;
void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1) // SHOW DateTime
  {
    PrintToSerialDateTime();   
  }

  if(debugButtonFromSerial == 2) //RESET Settings
  {
    currentMenu = Menu::Main;
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
    INFO("Reset in 8s...");
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();

    INFO("Input: ", readFromSerial);

    if(SetCurrentDateTime(readFromSerial, rtc))
    {      
      PrintToSerialDateTime();      
      settings.FeedScheduler.SetNextAlarm(rtc);      
      SaveSettings();
      ShowNextFeedTime();
      PrintToSerialStatus(); 
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
  if(value.length() >= 8)
  {
    uint16_t yyyy = value.substring(0, 4).toInt();
    uint8_t MM = value.substring(4, 6).toInt();      
    uint8_t dd = value.substring(6, 8).toInt();      

    uint8_t HH = 0;
    uint8_t mm = 0;
    uint8_t ss = 0;

    if(value.length() >= 12)
    {
      HH = value.substring(8, 10).toInt();      
      mm = value.substring(10, 12).toInt();
    }
    else
    {
      String systemTime = (__TIME__);
      if(systemTime.length() == 8)
      {
        HH = systemTime.substring(0, 2).toInt();
        mm = systemTime.substring(3, 5).toInt();
        ss = systemTime.substring(6, 8).toInt();
        TRACE("Sytem time used");
      }
    }

    if(yyyy < 2023 || yyyy > 2100)  {INFO("Wrong: ", yyyy);  return false; }
    if(MM < 1 || MM > 12)           {INFO("Wrong: ", MM);    return false; }
    if(dd < 1 || dd > 31)           {INFO("Wrong: ", dd);    return false; }
    if(HH < 0 || HH > 23)           {INFO("Wrong: ", HH);    return false; }
    if(mm < 0 || mm > 59)           {INFO("Wrong: ", mm);    return false; }

    if(value.length() >= 14)
    {
      ss = value.substring(12, 14).toInt();
      if(ss < 0 || ss > 59)         {INFO("Wrong: ", ss); ss = 0;}
    }

    auto dt = DateTime(yyyy, MM, dd, HH, mm, ss);      
    realTimeClock.now(dt);

    Serial.println("OK ");

    return true;
  }
  return false;
}

void PrintToSerialDateTime()
{
  SS_TRACE(__DATE__, " ", __TIME__);
  SS_TRACE(rtc.now().timestamp());  
}

void PrintToSerialStatus()
{
  //INFO("CurrentPos: ", settings.CurrentPosition);
  TRACE("Sched: ", settings.FeedScheduler.SetToString());
  //TRACE("Next: ", settings.FeedScheduler.GetNextAlarm().timestamp(), " ", settings.FeedScheduler.GetNextAlarm().GetTotalValueWithoutSeconds());  
  TRACE("RTC Alarm: ", rtc.alarm(DS323x::AlarmSel::A2).timestamp(), " rate: ", (uint8_t)rtc.rateA2());
  //TRACE("Curr: ", rtc.now().timestamp(), " ", Feed::FeedDateTime::GetTotalValueWithoutSeconds(rtc.now()));    
  TRACE("Curr Time: ", rtc.now().timestamp());    
  //INFO("Rotate Count: ", settings.RotateCount);  
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
    btnOK.resetTicks();
    btnRt.resetTicks();
    btnManualFeed.resetTicks();
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
    if(currentMenu == Menu::Dht)
    {
      ShowDht();
    }

    if(currentMenu == Menu::History && settings.GetStatusByIndex(historyMenuPos).DHT > 0)
    {
      return;
    }
     
    lcd.setCursor(8, 1);
    lcd.print(dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    prevTicks = currentTicks;
  }
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  INFO("Watchdog enabled.");
}

void ClearRow(const uint8_t &row) { ClearRow(row, 0, LCD_COLS, -1); }
void ClearRow(const uint8_t &row, const uint8_t &columnStart, const uint8_t &columnEnd, const uint8_t &gotoX)
{
  lcd.setCursor(columnStart, row);
  for(uint8_t ch = columnStart; ch < columnEnd; ch++) lcd.print(' ');
  lcd.setCursor(gotoX == -1 ? columnStart : gotoX, row);
}

void SaveSettings()
{
  INFO("Save...");

  ClearRow(1);
  lcd.print("Save...");

  TRACE("Max:", EEPROM.length(), " Total: ", sizeof(settings), " ", "Hist: ", sizeof(settings.FeedHistory));

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings); 

  delay(500);  
}

void LoadSettings()
{
  INFO("Load...");

  ClearRow(1);
  lcd.print("Load...");
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  

  if(settings.CurrentPosition == 65535)
  {
    settings.Reset();    
  }
}