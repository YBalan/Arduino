#include <EEPROM.h>
#include <avr/wdt.h>
#include <DS323x.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG
#include "DEBUGHelper.h"

#include "../../Shares/Button.h"
#include "FeedStatusInfo.h"
#include "FeedSettings.h"
#include "FeedScheduler.h"
#include "FeedMotor.h"
#include "LcdProgressBar.h"

//DebounceTime
#define DEBOUNCE_TIME 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

short debugButtonFromSerial = 0;

#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);

DS323x rtc;

Helpers::LcdProgressBar progress(lcd, 1, 0, 16, Helpers::LcdProgressSettings::NUMBERS_CENTER);

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
#define PAW_LED_PIN A3

Button btnOK(OK_PIN);
ezButton btnUp(UP_PIN);
ezButton btnDw(DW_PIN);
ezButton btnRt(RT_PIN);

Button btnManualFeed(MANUAL_FEED_PIN);
ezButton btnRemoteFeed(REMOTE_FEED_PIN);
ezButton btnPawFeed(PAW_FEED_PIN);

void setup() 
{
  Serial.begin(9600);  
  while (!Serial);

  Serial.println();
  Serial.println();
  S_PRINT("!!!! Start Auto Feeder !!!!");
  S_PRINT3(__DATE__, " ", __TIME__);  

  lcd.init();
  lcd.init();

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");    

  Wire.begin();
  delay(2000);
  rtc.attach(Wire);
  
  ShowLcdTime(1000, rtc.now());

  btnOK.setDebounceTime(DEBOUNCE_TIME);
  btnUp.setDebounceTime(DEBOUNCE_TIME);
  btnDw.setDebounceTime(DEBOUNCE_TIME);
  btnRt.setDebounceTime(DEBOUNCE_TIME);

  btnManualFeed.setDebounceTime(DEBOUNCE_TIME);
  btnRemoteFeed.setDebounceTime(DEBOUNCE_TIME);
  btnPawFeed.setDebounceTime(DEBOUNCE_TIME);

  servo.Init();

  LoadSettings();
  PrintStatus();

  EnableWatchDog();
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
    S_PRINT("Ok btn is pressed...");
    BacklightOn();
  }

  if(btnOK.isReleased())
  {
    if(btnOK.isLongPress())
    {
      btnOK.resetTicks();
    }
  }

  if(btnManualFeed.isReleased() || btnUp.isReleased())
  {
    S_PRINT2("Manual at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));    
    BacklightOn();

    if(DoFeed(MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::MANUAL, dtNow));
    }

    ShowLastAction();
  }
  else
  if(btnRemoteFeed.isReleased())
  {    
    S_PRINT2("Remoute at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    if(DoFeed(MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::REMOUTE, dtNow));
    }

    ShowLastAction();
  }
  else
  if(btnPawFeed.isReleased())
  {
    S_PRINT2("Paw at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    if(DoFeed(MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::PAW, dtNow));
    }
    
    ShowLastAction();
  }
  else
  if(settings.FeedScheduler.IsTimeToAlarm(rtc.now()))
  {
    S_PRINT2("Schedule at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    if(DoFeed(MOTOR_SHOW_PROGRESS))
    {
      settings.SetLastStatus(Feed::StatusInfo(Feed::Status::SCHEDULE, dtNow));
    }
    settings.FeedScheduler.SetNextAlarm(dtNow);

    ShowLastAction();
  }  
 
  CheckBacklightDelay(millis()); 
  HandleDebugSerialCommands();
}

const bool DoFeed(const bool &showProgress)
{
  return servo.DoFeed(settings.CurrentPosition, showProgress, btnRt);
}

void ShowLastAction()
{
  S_PRINT2("LAST: ", settings.GetLastStatus().ToString());
  
  ClearRow(0);

  const Feed::StatusInfo &lastStatus = settings.GetLastStatus();  
  if(lastStatus.Status != Feed::Status::Unknown)  
    lcd.print("LAST: "); lcd.print(lastStatus.ToString());
}

void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 1)
  {
    PrintToSerialDateTime();   
  }
  
  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == 11)
  {
    S_PRINT("Going to reset after 8secs...");
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    auto readFromSerial = Serial.readString();

    if(SetCurrentDateTime(readFromSerial, rtc))
    {      
      PrintToSerialDateTime();
    }
    else
    {    
      debugButtonFromSerial = readFromSerial.toInt();
      Serial.println(debugButtonFromSerial);
    }
  }
  
  wdt_reset();
}

//Format: yyyyMMddhhmmss (20231116163401)
const bool SetCurrentDateTime(const String &value, DS323x &realTimeClock)
{
  if(value.length() >= 12)
  {
    S_PRINT2("Entered debug value: ", value);

    short yyyy = value.substring(0, 4).toInt();
    short MM = value.substring(4, 6).toInt();      
    short dd = value.substring(6, 8).toInt();      

    short HH = value.substring(8, 10).toInt();      
    short mm = value.substring(10, 12).toInt();

    short ss = 0;

    if(yyyy < 2023 || yyyy > 2100)  {S_PRINT2("Wrong value: ", yyyy); return false; }
    if(MM < 1 || MM > 12)           {S_PRINT2("Wrong value: ", MM); return false; }
    if(dd < 1 || dd > 31)           {S_PRINT2("Wrong value: ", dd); return false; }
    if(HH < 0 || HH > 23)           {S_PRINT2("Wrong value: ", HH); return false; }
    if(mm < 0 || mm > 59)           {S_PRINT2("Wrong value: ", mm); return false; }

    if(value.length() >= 14)
    {
      ss = value.substring(12, 14).toInt();
      if(ss < 0 || ss > 59)         {S_PRINT2("Wrong value: ", ss); ss = 0;}
    }

    auto dt = DateTime(yyyy, MM, dd, HH, mm, ss);      
    realTimeClock.now(dt);

    return true;
  }
  return false;
}

void PrintToSerialDateTime()
{
  S_PRINT2("Current DateTime: ", rtc.now().timestamp());
  S_PRNT  ("System  DateTime: "); S_PRINT3(__DATE__, " ", __TIME__);
}

void BacklightOn()
{
  lcd.backlight();
  backlightStartTicks = millis();
}

unsigned long prevTicks = 0;
void ShowLcdTime(const unsigned long &currentTicks, const DateTime &dtNow)
{ 
  if(currentTicks - prevTicks >= 1000)
  {
    //S_PRINT(currentTicks);
    ClearRow(1, 8);    
    lcd.print(dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    prevTicks = currentTicks;
  }
}

const bool CheckBacklightDelay(const unsigned long &currentTicks)
{
  if(backlightStartTicks > 0 && currentTicks - backlightStartTicks >= BACKLIGHT_DELAY)
  {      
    lcd.noBacklight();
    backlightStartTicks = 0;
    return true;
  }
  return false;
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  S_PRINT("Watchdog enabled.");
}

void ClearRow(const short &row) { ClearRow(row, 0); }
void ClearRow(const short &row, const short &gotoY)
{
  lcd.setCursor(0, row);
  lcd.print("                  ");
  lcd.setCursor(gotoY, row);
}

void PrintStatus()
{  
  S_PRINT("Status: ");
}

void SaveSettings()
{
  S_PRINT("Save Settings...");

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings); 
}

void LoadSettings()
{
  S_PRINT("Load Settings...");
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  

  if(settings.CurrentPosition == 65535)
  {
    settings.CurrentPosition = 0;
  }
}