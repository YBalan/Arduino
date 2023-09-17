#include <EEPROM.h>
#include <avr/wdt.h>
#include <LiquidCrystal_I2C.h>

#include "../../Shares/Button.h"
#include "Pump.h"

#define RELEASE

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

#define BACKLIGHT_DELAY 50000
unsigned long backlightStartTicks = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

  BacklightOn();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");  

  Serial.begin(9600);

  Serial.println();
  Serial.println();
  Serial.println("!!!!!!!!!!! Auto Watering started 2!!!!!!!!!!!");
  Serial.println();

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

const bool CheckBacklightDelay(const unsigned long currentTicks)
{
  if(backlightStartTicks > 0 && currentTicks - backlightStartTicks >= BACKLIGHT_DELAY)
  {      
    lcd.noBacklight();
    backlightStartTicks = 0;
    return true;
  }

  return false;
}

const bool CheckPrintSensorStatusDelay(const unsigned long currentTicks)
{
  if(startShowSensorStatusTicks > 0 && (currentTicks - startShowSensorStatusTicks) >= PRINT_SENSOR_STATUS_DELAY)
  { 
    //SerialPrintSensorsStatus();
    //Serial.print(currentTicks); Serial.print(":"); Serial.println(startShowSensorStatusTicks);

    LcdPrintSensorStatus(HasWater, currentPumpToShowFullStatus);
    startShowSensorStatusTicks = currentTicks;
    return true;
  }

  return false;
}

void EnableWatchDog()
{
  wdt_enable(WDTO_8S); 
  Serial.println("Watchdog enabled.");
}

void loop()
{
  LoopButtons();
    
  waterSensor.loop();
  scheduleWatering.loop();
  backLightBtn.loop();

  const auto &current = millis();

  CheckBacklightDelay(current);

  CheckPrintSensorStatusDelay(current);

  if(USE_WATER_SENSOR && (waterSensor.isPressed() || waterSensor.getState() == ON))
  { 
    if(HasWater)
    {
      Serial.println("!!!! NO Water !!!!");
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
      Serial.println("Has Water");
      //ResetPumpStates();
      
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
      Serial.print(3 - waterSensorShortClickCount); Serial.println(" Left to Hard reset settings.");
      waterSensor.resetTicks();
    }
    else
    {
      waterSensorShortClickCount = 0;
    }

    if(waterSensorShortClickCount >= 3)
    {
      waterSensorShortClickCount = 0;
      Serial.println("Hard reset settings...");
      ResetSettings();
      
      DebugSerialPrintPumpsStatus(current);      
    }
  }

  if(scheduleWatering.isReleased() || debugButtonFromSerial == 9)
  {
    Serial.println("Schedule Watering pressed...");
    ResetPumpStates();
  }

  if(backLightBtn.isReleased() || debugButtonFromSerial == 10)
  {
    auto count = backLightBtn.getCount();
    Serial.print("backLightBtn pressed... "); Serial.println(count);
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
    if(pumps[pumpIdx].IsWatchDogTriggered(current))
    {
      Serial.print(pumpIdx + 1); Serial.print("  TIMEOUT: ");
      pumps[pumpIdx].End(TIMEOUT_OFF);

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);

      SaveSettings();
    }

    if(pumps[pumpIdx].IsWateringRequired() && HasWater)
    {
      BacklightOn();

      Serial.print(pumpIdx + 1); Serial.print(" REQUIRED: ");      
      pumps[pumpIdx].Start();      

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);
      
      SaveSettings();
    }

    if(pumps[pumpIdx].IsWateringEnough() && HasWater)
    {
      Serial.print(pumpIdx + 1); Serial.print("   ENOUGH: ");      
      pumps[pumpIdx].End();
      
      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/false);

      SaveSettings();
    }

    if((buttons[pumpIdx].isPressed()) 
        || debugButtonFromSerial == pumpIdx + 1)
    {
      BacklightOn();

      Serial.print(pumpIdx + 1); Serial.print(" Pressed: ");

      currentPumpToShowFullStatus = pumpIdx;
      
      pumps[pumpIdx].getState() == MANUAL_ON ? pumps[pumpIdx].End(MANUAL_OFF) : pumps[pumpIdx].Start(MANUAL_ON);

      if(pumps[pumpIdx].isOff())
      {
        currentPumpToShowFullStatus = -1;
      }

      DebugSerialPrintPumpStatus(pumpIdx, current, /*showDebugInfo:*/true);
    }    
    
    if(buttons[pumpIdx].isReleased()
        || debugButtonFromSerial == -(pumpIdx + 1))
    {      
      Serial.print(pumpIdx + 1); Serial.print(" Released: "); Serial.print(buttons[pumpIdx].getTicks()); Serial.print("ms. => ");      

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

  HandleDebugSerialCommands();  
}

void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 5)
  {
    //Serial.println("Reset settings...");
    ResetSettings();    
    SerialPrintPumpsStatus(millis());
  }

  if(debugButtonFromSerial == 6)
  {
    Serial.println("Print status...");    
    SerialPrintPumpsStatus(millis());
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

void DebugSerialPrintPumpStatus(const unsigned short &pumpIdx, const unsigned long &currentTicks, const bool &showDebugInfo)
{
  #ifdef DEBUG
  char serialBuff[200];
  Serial.println(pumps[pumpIdx].PrintStatus(/*showDebugInfo:*/showDebugInfo, currentTicks, serialBuff));
  #else
  Serial.print(GetState(pumps[pumpIdx].getState())); Serial.print(" (wd:"); Serial.print(pumps[pumpIdx].Settings.WatchDog / 1000); Serial.println("s.)");
  #endif
}

const short GetFirstPumpOn()
{
  for(unsigned short pump = 0; pump < PUMPS_COUNT; pump++)
  {
    if(pumps[pump].getState() == MANUAL_ON)
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
      Serial.print(i + 1); Serial.print(" Wrong pump initialization: "); Serial.println(i);
    }
  }
}

void SetAllPumps(const PumpState &state)
{
  Serial.print("Set All Pumps: "); Serial.println(GetState(state));
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
    char buff[200];
    Serial.println(pumps[i].PrintStatus(/*showDebugInfo:*/true, currentTicks, buff));
  }  
}

void DebugSerialPrintPumpsStatus(const unsigned long &currentTicks)
{   
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    Serial.print(i + 1); Serial.print(" "); DebugSerialPrintPumpStatus(i, currentTicks, /*showDebugInfo:*/false);
  }  
}

void SerialPrintSensorsStatus()
{ 
  Serial.print("                              ");
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    char buff[40];    
    sprintf(buff, "[%d: %d/%d] ", i + 1, pumps[i].GetSensorValue(), pumps[i].Settings.WateringRequired);
    Serial.print(buff);    
  }
  Serial.println();  
}

const char * const LcdPrintSensorStatus(const bool &hasWater, const short &currentPump)
{
  char pumpBuff1[20];  
  char pumpBuff12[10];
  char pumpBuff13[10];    

  lcd.clear();

  //Serial.println("lcd");

  if(!hasWater)
  {
    lcd.setCursor(0, 0);
    lcd.print("No Water");
  }  

  const bool shortStatus = currentPump <= -1 || currentPump >= PUMPS_COUNT;
  
  if(shortStatus)
  { 
    sprintf(pumpBuff1, "%s|%s", pumps[0].GetShortStatus(hasWater, pumpBuff12), pumps[1].GetShortStatus(hasWater, pumpBuff13));    
    #ifdef DEBUG
    Serial.print("1|2 => "); Serial.println(pumpBuff1);
    #endif
    
    lcd.setCursor(0, 0);
    lcd.print(pumpBuff1);
    
    sprintf(pumpBuff1, "%s|%s", pumps[2].GetShortStatus(hasWater, pumpBuff12), pumps[3].GetShortStatus(hasWater, pumpBuff13));
    #ifdef DEBUG
    Serial.print("3|4 => "); Serial.println(pumpBuff1);
    #endif

    lcd.setCursor(0, 1);
    lcd.print(pumpBuff1);    
  }
  else
  {    
    sprintf(pumpBuff1, "%d:%s", currentPump + 1, pumps[currentPump].GetFullStatus(hasWater, pumpBuff12));
    #ifdef DEBUG
    Serial.print("Pump: "); Serial.println(pumpBuff1);
    #endif
    
    lcd.setCursor(0, 0);
    lcd.print(pumpBuff1);
    
      const unsigned short wdSecs = pumps[currentPump].Settings.WatchDog / 1000;
    if(pumps[currentPump].isOn())
    {
      const unsigned short secs = (millis() - pumps[currentPump].getTicks()) / 1000;       
      sprintf(pumpBuff1, "%d:ON:%02d|WD:%02ds", currentPump + 1, secs, wdSecs);
    }
    else
    {
      sprintf(pumpBuff1, "%d:OFF|WD:%02ds", currentPump + 1, wdSecs);
    }
    #ifdef DEBUG
    Serial.print("Pump: "); Serial.println(pumpBuff1);
    #endif

    lcd.setCursor(0, 1);
    lcd.print(pumpBuff1);
  }

  return pumpBuff1;
}

void SaveSettings()
{
  Serial.println("Save Settings...");
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    auto settings = pumps[i].Settings;
    EEPROM.put(addr, settings);
    addr += sizeof(settings);
  }
}

void LoadSettings()
{
  Serial.println("Load Settings...");
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {
    auto settings = pumps[i].Settings;
    pumps[i].Settings = EEPROM.get(addr, settings);

    if(pumps[i].Settings.WatchDog == ULONG_MAX || pumps[i].Settings.WatchDog == 0 || pumps[i].Settings.PumpState >= UNKNOWN)
    {
      Serial.print(i + 1); Serial.println(" Storage is empty or corrupted. Return to default values...");
      pumps[i].Reset();
    }

    addr += sizeof(settings);
  }  
}

void ResetSettings()
{
  Serial.println("Reset Settings...");
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < PUMPS_COUNT; i++)
  {    
    pumps[i].Reset();
    auto settings = pumps[i].Settings;
    EEPROM.put(addr, settings);
    addr += sizeof(settings);
  }
}

void ResetCounts()
{
  Serial.println("Reset Counts...");
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

void WaitUntilWaterSensorReleased()
{
  while(true)
  {
    waterSensor.loop();
    if(waterSensor.isReleased())
    { 
      Serial.println("Has Water");
      ResetPumpStates();
      break;
    }

    LoopButtons();
    for (short i = 0; i < PUMPS_COUNT; i++) 
    {
      if(buttons[i].isPressed())
      {
        Serial.print(i + 1); Serial.println(" !!!! NO Water !!!!");
      }
    }

    HandleDebugSerialCommands();

    delay(20);
  }
}