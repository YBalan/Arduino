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
  #define SERIAL_PRINTLN(str) Serial.println((str))
  #define SERIAL_PRINT(str) Serial.print((str))
#else
  #define SERIAL_PRINTLN(str) do{}while(0)
  #define SERIAL_PRINT(str) do{}while(0)
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

#define SERVO_PIN 3
Servo servo;

Feed::Settings settings;

#define OK_PIN 10
#define UP_PIN 9
#define DW_PIN 8
#define MANUAL_FEED_PIN 11
#define REMOTE_FEED_PIN 12
#define PAW_FEED_PIN 13

Button btnOK(OK_PIN);
ezButton btnUp(UP_PIN);
ezButton btnDw(DW_PIN);
Button btnManualFeed(MANUAL_FEED_PIN);
ezButton btnRemoteFeed(REMOTE_FEED_PIN);
ezButton btnPawFeed(PAW_FEED_PIN);

void setup() 
{
  lcd.init();
  lcd.init();

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");  

  Serial.begin(9600);  

  Serial.println();
  Serial.println();
  SERIAL_PRINTLN("!!!!!!!!!!!!!!!!!!!!! Start Auto Feeder !!!!!!!!!!!!!!!!!!!!!!!!");

  Wire.begin();
  delay(2000);
  rtc.attach(Wire);  

  Serial.println(rtc.now().timestamp());

  btnOK.setDebounceTime(DEBOUNCE_TIME);
  btnUp.setDebounceTime(DEBOUNCE_TIME);
  btnDw.setDebounceTime(DEBOUNCE_TIME);
  btnManualFeed.setDebounceTime(DEBOUNCE_TIME);
  btnRemoteFeed.setDebounceTime(DEBOUNCE_TIME);
  btnPawFeed.setDebounceTime(DEBOUNCE_TIME);

  AttachServo();

  LoadSettings();
  PrintStatus();

  EnableWatchDog();
}

void loop() 
{
  const auto current = millis();

  btnOK.loop();
  btnUp.loop();
  btnDw.loop();
  btnManualFeed.loop();
  btnRemoteFeed.loop();
  btnPawFeed.loop();  

  CheckBacklightDelay(current); 
  HandleDebugSerialCommands();
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
    SERIAL_PRINTLN("Going to reset after 8secs...");
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
  SERIAL_PRINTLN("Current DateTime: ");
  Serial.println(rtc.now().timestamp());
}

void BacklightOn()
{
  lcd.backlight();
  backlightStartTicks = millis();
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
  SERIAL_PRINTLN("Watchdog enabled.");
}

void PrintStatus()
{  
  Serial.print("");
}

void AttachServo()
{
  if(!servo.attached())
  {
    SERIAL_PRINTLN("Servo Attached");
    servo.attach(SERVO_PIN);
  }  
}

void DetachServo()
{  
    SERIAL_PRINTLN("Servo Detached");
    servo.detach();    
}

void SaveSettings()
{
  SERIAL_PRINTLN("Save Settings...");

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings); 
}

void LoadSettings()
{
  SERIAL_PRINTLN("Load Settings...");
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  
}