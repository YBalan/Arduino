#include <EEPROM.h>
#include <avr/wdt.h>
#include <DS323x.h>
#include <LiquidCrystal_I2C.h>
#include "../../Shares/Button.h"

#define DEBUG
//DebounceTime
#define DEBOUNCE_TIME 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

short debugButtonFromSerial = 0;

#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);

DS323x rtc;

struct Settings
{
} settings;


void setup() 
{
  lcd.init();
  lcd.init();

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");  

  Serial.begin(9600);  

  Wire.begin();
  delay(2000);
  rtc.attach(Wire);

  Serial.println();
  Serial.println();
  Serial.println("Start Auto Feeder");

  LoadSettings();
  PrintStatus();

  EnableWatchDog();
}

void loop() 
{
  const auto current = millis();

  CheckBacklightDelay(current); 
  HandleDebugSerialCommands();
}

void HandleDebugSerialCommands()
{  
  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == 11)
  {
    Serial.println("Going to reset after 8secs...");
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    //Serial.println(Serial.readString().toInt());
    debugButtonFromSerial = Serial.readString().toInt();
    Serial.println(debugButtonFromSerial);
  }  

  //delay(50);
  wdt_reset();
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
  Serial.println("Watchdog enabled.");
}

void PrintStatus()
{  
  Serial.print("");
}

void SaveSettings()
{
  Serial.println("Save Settings...");

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings); 
}

void LoadSettings()
{
  Serial.println("Load Settings...");
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  
}