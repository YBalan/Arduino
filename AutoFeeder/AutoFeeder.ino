#include <EEPROM.h>
#include <avr/wdt.h>
#include <DS323x.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#include "../../Shares/Button.h"
#include "FeedStatusInfo.h"
#include "FeedSettings.h"
#include "FeedScheduler.h"

#define DEBUG

#ifdef DEBUG
  #define S_PRINT(value) Serial.println((value))
  #define S_PRINT2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_PRINT3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))  
#else  
  #define S_PRINT(value) while(0)
  #define S_PRINT2(value1, value2) while(0)
  #define S_PRINT3(value1, value2, value3) while(0)
#endif

//DebounceTime
#define DEBOUNCE_TIME 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

short debugButtonFromSerial = 0;

#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);

DS323x rtc;

#define SERVO_PIN 9
Servo servo;

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

  Serial.println();
  Serial.println();
  S_PRINT("!!!!!!!!!!!!!!!!!!!!! Start Auto Feeder !!!!!!!!!!!!!!!!!!!!!!!!");

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

  //AttachServo();

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

  if(btnManualFeed.isReleased() || btnRt.isReleased())
  {
    S_PRINT2("Manual at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));    

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::MANUAL, dtNow));

    ShowLastAction();
  }
  else
  if(btnRemoteFeed.isReleased())
  {
    S_PRINT2("Remoute at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::REMOUTE, dtNow));
  }
  else
  if(btnPawFeed.isReleased())
  {
    S_PRINT2("Paw at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::PAW, dtNow));
  }
  else
  if(settings.FeedScheduler.IsTimeToAlarm(rtc.now()))
  {
    S_PRINT2("Schedule at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::SCHEDULE, dtNow));
    settings.FeedScheduler.SetNextAlarm(dtNow);
  }  
 
  CheckBacklightDelay(millis()); 
  HandleDebugSerialCommands();
}

void DoFeed()
{
  AttachServo();
  S_PRINT3("Do Feed (", settings.CurrentPosition, ")");
  if(settings.CurrentPosition == 0)
  {
    servo.write(360);
    settings.CurrentPosition = 270;
  }
  else
  {
    servo.write(0);
    settings.CurrentPosition = 0;
  }
  delay(3000);
  DetachServo();
}

void ShowLastAction()
{
  S_PRINT2("LAST: ", settings.GetLastStatus().ToString());
  ClearRow(0);
  lcd.print("LAST: "); lcd.print(settings.GetLastStatus().ToString());
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

  //delay(50);
  wdt_reset();
}

//Format: yyyyMMddhhmmss (20231116163401)
const bool SetCurrentDateTime(const String &value, DS323x &realTimeClock)
{
  if(value.length() >= 12)
  {
    Serial.println(value);

    short yyyy = value.substring(0, 4).toInt();
    short MM = value.substring(4, 6).toInt();      
    short dd = value.substring(6, 8).toInt();      

    short HH = value.substring(8, 10).toInt();      
    short mm = value.substring(10, 12).toInt();

    if(yyyy < 2023 || yyyy > 2100)  {Serial.print("Wrong value - "); Serial.println(yyyy); return false; }
    if(MM < 1 || MM > 12)           {Serial.print("Wrong value - "); Serial.println(MM); return false; }
    if(dd < 1 || dd > 31)           {Serial.print("Wrong value - "); Serial.println(dd); return false; }
    if(HH < 0 || HH > 23)           {Serial.print("Wrong value - "); Serial.println(HH); return false; }
    if(mm < 0 || mm > 59)           {Serial.print("Wrong value - "); Serial.println(mm); return false; }        

    auto dt = DateTime(yyyy, MM, dd, HH, mm, 0);      
    realTimeClock.now(dt);

    return true;
  }
  return false;
}

void PrintToSerialDateTime()
{
  S_PRINT2("Current DateTime: ", rtc.now().timestamp());  
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
    ClearRow(1);    
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

void ClearRow(const short &row)
{
  lcd.setCursor(0, row);
  lcd.print("                  ");
  lcd.setCursor(0, row);
}

void PrintStatus()
{  
  S_PRINT("Status: ");
}

void AttachServo()
{
  if(!servo.attached())
  {
    S_PRINT("Servo Attached");
    servo.attach(SERVO_PIN);
  }  
}

void DetachServo()
{  
    S_PRINT("Servo Detached");
    servo.detach();    
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
}