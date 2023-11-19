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
  #define S_PRNT(value) Serial.print((value))
  #define S_PRINT(value) Serial.println((value))
  #define S_PRINT2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_PRINT3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))  
#else  
  #define S_PRNT(value) while(0)
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
    BacklightOn();

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::MANUAL, dtNow));

    ShowLastAction();
  }
  else
  if(btnRemoteFeed.isReleased())
  {    
    S_PRINT2("Remoute at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::REMOUTE, dtNow));

    ShowLastAction();
  }
  else
  if(btnPawFeed.isReleased())
  {
    S_PRINT2("Paw at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::PAW, dtNow));
    
    ShowLastAction();
  }
  else
  if(settings.FeedScheduler.IsTimeToAlarm(rtc.now()))
  {
    S_PRINT2("Schedule at: ", dtNow.timestamp(DateTime::TIMESTAMP_TIME));
    BacklightOn();

    DoFeed();
    settings.SetLastStatus(Feed::StatusInfo(Feed::Status::SCHEDULE, dtNow));
    settings.FeedScheduler.SetNextAlarm(dtNow);

    ShowLastAction();
  }  
 
  CheckBacklightDelay(millis()); 
  HandleDebugSerialCommands();
}

#define MOTOR_RIGHT_POS 0
#define MOTOR_LEFT_POS  180
#define MOTOR_ROTATE_DELAY 100
#define MOTOR_ROTATE_VALUE 5
#define MOTOR_STEP_BACK_MODE true
#define MOTOR_STEP_BACK_VALUE 10

#define MOTOR_SHOW_PROGRESS true

void DoFeed()
{
  AttachServo();
  S_PRINT3("Do Feed (", settings.CurrentPosition, ")");
    
  short pos = settings.CurrentPosition;
  const short endPos = pos == MOTOR_LEFT_POS ? MOTOR_RIGHT_POS : MOTOR_LEFT_POS;
  const short rotate = pos == MOTOR_LEFT_POS ? -MOTOR_ROTATE_VALUE : MOTOR_ROTATE_VALUE;
  const short rotateBack = pos == MOTOR_LEFT_POS ? -MOTOR_STEP_BACK_VALUE : MOTOR_STEP_BACK_VALUE;

  S_PRINT2("CurrentPos: ", pos);
  S_PRINT2("    EndPos: ", endPos);
  S_PRINT2("    Rotate: ", rotate);

  if(pos >= MOTOR_RIGHT_POS && pos <= MOTOR_LEFT_POS)
  {
    while (rotate > 0 ? pos <= endPos : pos >= endPos)
    {
      btnRt.loop();
      if(btnRt.isReleased()) break;
      
      if(MOTOR_SHOW_PROGRESS)
      {
        //map(value, fromLow, fromHigh, toLow, toHigh)
        //short lcdPos = map(long, long, long, long, long);
      }

      servo.write(pos);
      if(MOTOR_STEP_BACK_MODE)
      {
        short stepBackPos = pos - rotateBack;
        if(stepBackPos >= MOTOR_RIGHT_POS + MOTOR_STEP_BACK_VALUE && stepBackPos <= MOTOR_LEFT_POS - MOTOR_STEP_BACK_VALUE)
        {
          delay(MOTOR_ROTATE_DELAY);
          servo.write(stepBackPos);
        }
      }

      S_PRINT2("CurrentPos: ", pos);

      pos += rotate;
      delay(MOTOR_ROTATE_DELAY);

      wdt_reset();
    }
  }

  settings.CurrentPosition = endPos;

  S_PRINT3("Do Feed (", settings.CurrentPosition, ")");  
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