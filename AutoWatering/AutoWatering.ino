#include <EEPROM.h>
#include <avr/wdt.h>
#include <LiquidCrystal_I2C.h>

#define VER F("1.4")

//#define RELEASE
//#define DEBUG

#ifdef DEBUG
#define VER_POSTFIX F("D")
#else
#define VER_POSTFIX F("R")
#endif

#define ENABLE_TRACE

#ifdef DEBUG
#define ENABLE_INFO_MAIN
#define ENABLE_TRACE_MAIN
#endif

#include "../../Shares/Button.h"
#include "../../Shares/DEBUGHelper.h"

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

#include "Pump.h"

#define USE_WATER_SENSOR true
bool HasWater = !USE_WATER_SENSOR;

#define PUMPS_COUNT 4

//EEPROM
#define EEPROM_SETTINGS_ADDR 10

//DebounceTime
#define DebounceTime 50

#define PRINT_SENSOR_STATUS_DELAY 1000u
unsigned long startShowSensorStatusTicks = 0;
short waterSensorShortClickCount = 0;

short currentPumpToShowFullStatus = -1;

#define WATER_ALARM_PIN 3
#define WATER_SENSOR_PIN 2
Button waterSensor(WATER_SENSOR_PIN);

#define SHEDULE_WATERING_BTN_PIN 8
ezButton scheduleWatering(SHEDULE_WATERING_BTN_PIN);

#define BACKLIGHT_BTN_PIN 9
ezButton backLightBtn(BACKLIGHT_BTN_PIN);

#define LCD_COLS 16
#define LCD_ROWS 2
#define BACKLIGHT_DELAY 60000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

#define HANDLE_PUMPS_IN_PARALLEL true

Button buttons[] = 
{
  {10}, //Button pin for Pump 1
  {11}, //Button pin for Pump 2
  {12}, //Button pin for Pump 3
  {13}, //Button pin for Pump 4
};

Pump pumps[] =
{
  {/*Place:*/1, /*PumpPin*/7, /*SensorPin:*/A0, DEFAULT_PUMP_TIMEOUT, DEFAULT_WATERING_REQUIRED_LEVEL, DEFAULT_WATERING_ENOUGH_LEVEL},
  {/*Place:*/2, /*PumpPin*/6, /*SensorPin:*/A1, DEFAULT_PUMP_TIMEOUT, DEFAULT_WATERING_REQUIRED_LEVEL, DEFAULT_WATERING_ENOUGH_LEVEL},
  {/*Place:*/3, /*PumpPin*/5, /*SensorPin:*/A2, DEFAULT_PUMP_TIMEOUT, DEFAULT_WATERING_REQUIRED_LEVEL, DEFAULT_WATERING_ENOUGH_LEVEL},
  {/*Place:*/4, /*PumpPin*/4, /*SensorPin:*/A3, DEFAULT_PUMP_TIMEOUT, DEFAULT_WATERING_REQUIRED_LEVEL, DEFAULT_WATERING_ENOUGH_LEVEL},
};

short debugButtonFromSerial = 0;

void setup()
{
  lcd.init();
  lcd.init();

  String ver = String(VER) + VER_POSTFIX;

  BacklightOn();
  lcd.setCursor(0, 0);
  //lcd.print(F("Hello!   "));  
  lcd.print(F("V: ")); lcd.print(ver);
  lcd.setCursor(0, 1);
  lcd.print(__DATE__);  

  delay(3000);

  Serial.begin(9600);
  while(!Serial);

  Serial.println();
  Serial.println(F("!!!! Start Auto Watering !!!!"));
  Serial.print(F("Flash Date: ")); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print(F("V:")); Serial.println(ver);

  InitializePumps();
  InitializeButtons();

  pinMode(WATER_ALARM_PIN, INPUT_PULLUP);
  pinMode(WATER_ALARM_PIN, OUTPUT);
  digitalWrite(WATER_ALARM_PIN, LOW);  

  waterSensor.setDebounceTime(DebounceTime);
  scheduleWatering.setDebounceTime(DebounceTime);
  backLightBtn.setDebounceTime(DebounceTime);

  LoadSettings();
  //ResetSettings();  
  DebugSerialPrintPumpsStatus(millis());  

  startShowSensorStatusTicks = millis();

  EnableWatchDog();
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

const bool CheckPrintSensorStatusDelay(const unsigned long &currentTicks)
{
  if(startShowSensorStatusTicks > 0 && (currentTicks - startShowSensorStatusTicks) >= PRINT_SENSOR_STATUS_DELAY)
  { 
    LcdPrintSensorStatus(HasWater, currentPumpToShowFullStatus);
    startShowSensorStatusTicks = currentTicks;
    return true;
  }

  return false;
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  INFO(F("Watchdog enabled"));
}

void loop()
{
  LoopButtons();
    
  waterSensor.loop();
  scheduleWatering.loop();
  backLightBtn.loop();

  const auto current = millis();

  CheckBacklightDelay(current); 

  if(USE_WATER_SENSOR && (waterSensor.isPressed() || waterSensor.getState() == ON))
  { 
    if(HasWater)
    {
      TRACE(F("NO Water"));
      SetAllPumps(PumpState::OFF);
      currentPumpToShowFullStatus = -1;
    }

    digitalWrite(WATER_ALARM_PIN, HIGH);
    HasWater = false;  
  } 

  if(USE_WATER_SENSOR && waterSensor.getState() == OFF)
  {
    if(!HasWater)
    {
      TRACE(F("Has Water"));
      ResetPumpStates(PumpState::TIMEOUT_OFF);
      
      DebugSerialPrintPumpsStatus(current);
    }
    
    digitalWrite(WATER_ALARM_PIN, LOW);
    HasWater = true;
  }

  if(waterSensor.isReleased())
  {    
    if(waterSensor.getTicks() <= 1000)
    {
      waterSensorShortClickCount++;      
      TRACE(3 - waterSensorShortClickCount, F(" "), F("Hard reset"));
      waterSensor.resetTicks();
    }
    else
    {
      waterSensorShortClickCount = 0;
    }

    if(waterSensorShortClickCount >= 3)
    {
      waterSensorShortClickCount = 0;
      TRACE(F("Hard reset"));
      ResetSettings();
      
      DebugSerialPrintPumpsStatus(current);      
    }
  }

  if(scheduleWatering.isReleased() || debugButtonFromSerial == 9)
  {
    INFO(F("Schedule "), BUTTON_IS_PRESSED_MSG);
    ResetPumpStates();
  }

  if(backLightBtn.isReleased() || debugButtonFromSerial == 10)
  {
    auto count = backLightBtn.getCount();    
    INFO(F("Backlight "), BUTTON_IS_PRESSED_MSG, F(" "), count);
    BacklightOn();
    if(count > 1)
    {
      currentPumpToShowFullStatus++;
    }
    if(currentPumpToShowFullStatus == PUMPS_COUNT)
    {
      currentPumpToShowFullStatus = -1;
      backLightBtn.resetCount();
    }
  }
  
  for (short pumpIdx = 0; pumpIdx < PUMPS_COUNT; pumpIdx++) 
  {
    if(pumps[pumpIdx].CheckAerationStatus(current) && HasWater)
    {
      if(pumps[pumpIdx].getState() == AERATION_ON)
      {
        INFO(pumpIdx + 1, F(" "), F("AERATION OFF")); 
        pumps[pumpIdx].End(AERATION_OFF);
      }else
      if(pumps[pumpIdx].getState() == AERATION_OFF)
      {
        INFO(pumpIdx + 1, F(" "), F("AERATION ON")); 
        pumps[pumpIdx].Start(AERATION_ON);
        BacklightOn();
        SaveSettings();
      }
    }

    if(pumps[pumpIdx].IsWatchDogTriggered(current))
    {
      //Serial.print(pumpIdx + 1); Serial.print(F("  TIMEOUT: "));
      INFO(pumpIdx + 1, F("  TIMEOUT: "));
      pumps[pumpIdx].End(TIMEOUT_OFF);

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);

      SaveSettings();
    }

    if(pumps[pumpIdx].IsWateringRequired() && HasWater)
    {
      BacklightOn();

      //Serial.print(pumpIdx + 1); Serial.print(F(" REQUIRED: "));
      INFO(pumpIdx + 1, F(" REQUIRED: ")); 
      pumps[pumpIdx].Start();      

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);
      
      SaveSettings();
    }

    if(pumps[pumpIdx].IsWateringEnough() && HasWater)
    {
      //Serial.print(pumpIdx + 1); Serial.print(F("   ENOUGH: "));
      INFO(pumpIdx + 1, F("   ENOUGH: ")); 
      pumps[pumpIdx].End(DO_NOT_USE_ENOUGH_LOW_LEVEL ? SENSOR_OFF : OFF);
      
      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);

      SaveSettings();
    }

    if((buttons[pumpIdx].isPressed()) 
        || debugButtonFromSerial == pumpIdx + 1)
    {
      BacklightOn();
      
      TRACE(pumpIdx + 1, F(" Pressed: ")); 

      currentPumpToShowFullStatus = pumpIdx;
      
      const auto &state = pumps[pumpIdx].getState();
      (state == MANUAL_ON || state == AERATION_ON || state == AERATION_OFF) ? pumps[pumpIdx].End(MANUAL_OFF) : pumps[pumpIdx].Start(MANUAL_ON);

      if(pumps[pumpIdx].isOff())
      {
        currentPumpToShowFullStatus = -1;
      }

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/true);
    }    
    
    if(buttons[pumpIdx].isReleased()
        || debugButtonFromSerial == -(pumpIdx + 1))
    { 
      TRACE(pumpIdx + 1, F(" Released: "), buttons[pumpIdx].getTicks(), F("ms. => "));

      if(buttons[pumpIdx].isLongPress())
      {
        currentPumpToShowFullStatus = -1;
        buttons[pumpIdx].resetTicks();
        pumps[pumpIdx].End(CALIBRATING);
        SaveSettings();        
      }

      if(currentPumpToShowFullStatus == -1)
      {
        currentPumpToShowFullStatus = GetFirstPumpOn();
      }
      
      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/true);
    }
  }  

  CheckPrintSensorStatusDelay(current);
  HandleDebugSerialCommands();  
}

void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 5)
  {
    //Serial.println(F("Reset settings..."));
    ResetSettings();    
    SerialPrintPumpsStatus(millis());
  }

  if(debugButtonFromSerial == 6)
  {
    //Serial.println(F("Print status..."));    
    //SerialPrintPumpsStatus(millis());
    DebugSerialPrintPumpsStatus(millis());
    SerialPrintSensorsStatus();
  }

  if(debugButtonFromSerial == 7)
  {
    SetAllPumps(ON);    
    SerialPrintPumpsStatus(millis());
  }

  if(debugButtonFromSerial == 8)
  {
    SetAllPumps(OFF);    
    SerialPrintPumpsStatus(millis());
  }

  // if(debugButtonFromSerial == 9)
  // {
  //   ResetCounts();    
  //   SerialPrintPumpsStatus(millis());
  // }

  // if(debugButtonFromSerial == 10)
  // {
  //   ShowSensorStatus = !ShowSensorStatus;
  // }

  if(debugButtonFromSerial == 10)
  {
     pumps[3].Settings.setWatchDog(AERATION_TIMEOUT);
     pumps[3].Settings.PumpState = AERATION_ON;
  }

  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == 11)
  {
    TRACE(F("Reset in 8s..."));
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    //Serial.println(Serial.readString().toInt());
    debugButtonFromSerial = Serial.readString().toInt();
    TRACE(debugButtonFromSerial);
  }  

  //delay(50);
  wdt_reset();
}

const short GetFirstPumpOn()
{
  for(unsigned short pump = 0; pump < PUMPS_COUNT; pump++)
  {
    if(pumps[pump].getState() == MANUAL_ON || pumps[pump].getState() == AERATION_ON)
    {
      return pump;      
    }
  }
  return -1;
}

void InitializeButtons()
{
  // Initialize the buttons
  for (short i = 0; i < PUMPS_COUNT; i++) 
  {
    buttons[i].setDebounceTime(DebounceTime); // Set debounce time in milliseconds    
  }
}

void InitializePumps()
{
  for (short i = 0; i < PUMPS_COUNT; i++) 
  {
    if(!pumps[i].Initialize())
    {
      //Serial.print(i + 1); Serial.print(F(" Wrong pump initialization: ")); Serial.println(i);
      //TRACE()
    }
  }
}

void SetAllPumps(const PumpState &state)
{
  //Serial.print(F("Set All Pumps: ")); Serial.println(GetState(state));
  TRACE(F("Set All Pumps: "), GetState(state));
  for (short i = 0; i < PUMPS_COUNT; i++) 
  {
    state == ON || state == MANUAL_ON ? pumps[i].Start(state) : pumps[i].End(state);    
  }
}

void LoopButtons()
{
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    buttons[i].loop(); 
  }
}

void ResetButtonsCounts()
{
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    buttons[i].resetCount();
  }
}

void SerialPrintPumpsStatus(const unsigned long &currentTicks)
{   
  for (short i = 0; i < PUMPS_COUNT; i++)
  {    
    INFO(i + 1, pumps[i].GetStatus(HasWater, /*shortStatus:*/false));
  }  
}

void DebugSerialPrintPumpsStatus(const unsigned long &currentTicks)
{   
  #ifdef ENABLE_TRACE_MAIN
  for (short i = 0; i < PUMPS_COUNT; i++)
  {    
    DebugSerialPrintPumpStatus(i, currentTicks, /*showDebugInfo:*/true);
  }  
  #endif
}

void DebugSerialPrintPumpStatus(const unsigned short &pumpIdx, const unsigned long &currentTicks, const bool &showDebugInfo)
{
  TRACE(pumpIdx + 1, F(" "), GetState(pumps[pumpIdx].getState()), F(" wd:"), pumps[pumpIdx].Settings.WatchDog, F(" wd s:"), pumps[pumpIdx].Settings.WatchDogSec); 
}

void SerialPrintSensorsStatus()
{ 
  // Serial.print(F("                              "));
  // for (short i = 0; i < PUMPS_COUNT; i++)
  // {
  //   char buff[40];    
  //   sprintf(buff, F("[%d: %d/%d] "), i + 1, pumps[i].GetSensorValue(), pumps[i].Settings.WateringRequired);
  //   Serial.print(buff);    
  // }
  // Serial.println();  
}

const String &LcdPrintSensorStatus(const bool hasWater, const short currentPump)
{  
  lcd.clear();

  //Serial.println(F("lcd"));

  if(!hasWater)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("NO Water"));
  }   

  static String pumpShortStatus;
  //pumpShortStatus.clear();
  
  if(currentPump < 0 || currentPump >= PUMPS_COUNT) //ShortStatus
  {     
    pumpShortStatus = pumps[0].GetShortStatus(hasWater) + F("|") + pumps[1].GetShortStatus(hasWater);
    
    TRACE(F("1|2 => "), pumpShortStatus);    
    
    lcd.setCursor(0, 0);
    lcd.print(pumpShortStatus);    
    
    pumpShortStatus = pumps[2].GetShortStatus(hasWater) + F("|") + pumps[3].GetShortStatus(hasWater);
    
    TRACE(F("3|4 => "), pumpShortStatus);     

    lcd.setCursor(0, 1);
    lcd.print(pumpShortStatus);    
  }
  else
  { 
    String currentPumpStr = String(currentPump + 1);
    pumpShortStatus = currentPumpStr + F(":") +  pumps[currentPump].GetFullStatus(hasWater);
    TRACE(F("Pump: "), pumpShortStatus);
    
    lcd.setCursor(0, 0);
    lcd.print(pumpShortStatus);
    
    const auto &wdSecs = pumps[currentPump].Settings.WatchDogSec;
    if(pumps[currentPump].isOn())
    {
      unsigned short secs = (millis() - pumps[currentPump].getTicks()) * 0.001;      
      pumpShortStatus = currentPumpStr + F(":") + F("On") + F(":") + String(secs) + F("|") + F("WD") + F(":") + String(wdSecs) + F("s");
    }
    else
    {      
      pumpShortStatus = currentPumpStr + F(":") + F("Off") + F("|") + F("WD") + F(":") + String(wdSecs) + F("s");
    }    
    //Serial.println(pumpBuff1);
    TRACE(F("Pump: "), pumpShortStatus);

    lcd.setCursor(0, 1);
    lcd.print(pumpShortStatus);
  }

  return pumpShortStatus;
}

void SaveSettings()
{
  TRACE(F("Save Settings"));
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    const auto &settings = pumps[i].Settings;
    EEPROM.put(addr, settings);
    addr += sizeof(settings);
  }
}

void LoadSettings()
{
  TRACE(F("Load Settings"));
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    auto &settings = pumps[i].Settings;
    pumps[i].Settings = EEPROM.get(addr, settings);

    if(pumps[i].Settings.WatchDog == ULONG_MAX || pumps[i].Settings.PumpState >= UNKNOWN)
    {
      TRACE(i + 1, F(" Storage is empty or corrupted. Return to default values..."));
      pumps[i].Reset();
    }

    addr += sizeof(settings);
  }  
}

void ResetSettings()
{
  TRACE(F("Reset Settings"));
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {    
    pumps[i].Reset();
    pumps[i].ResetState(TIMEOUT_OFF);
    const auto &settings = pumps[i].Settings;
    EEPROM.put(addr, settings);
    addr += sizeof(settings);
  }
}

void ResetCounts()
{
  TRACE(F("Reset Counts"));
  for (short i = 0; i < PUMPS_COUNT; i++)
  {    
    pumps[i].Settings.Count = 0;
  }
  SaveSettings();
}

void ResetPumpStates()
{
  for (short i = 0; i < PUMPS_COUNT; i++) 
  {
    pumps[i].ResetState();
  }
}

void ResetPumpStates(const PumpState &state)
{
  for (short i = 0; i < PUMPS_COUNT; i++) 
  {
    pumps[i].ResetState(state);
  }
}

void WaitUntilWaterSensorReleased()
{
  while(true)
  {
    waterSensor.loop();
    if(waterSensor.isReleased())
    { 
      //Serial.println(F("Has Water"));
      TRACE(F("Has Water"));
      ResetPumpStates();
      break;
    }

    LoopButtons();
    for (short i = 0; i < PUMPS_COUNT; i++) 
    {
      if(buttons[i].isPressed())
      {
        //Serial.print(i + 1); Serial.println(F(" !!!! NO Water !!!!"));
        TRACE(i + 1, F(" !!!! NO Water !!!!"));
      }
    }

    HandleDebugSerialCommands();

    delay(20);
  }
}