#include <EEPROM.h>
#include <avr/wdt.h>
#include <LiquidCrystal_I2C.h>

#include "../../Shares/Button.h"
#include "Pump.h"

#define RELEASE

#define UseWaterSensor true
bool HasWater = !UseWaterSensor;

#define ChannelsCount 4

//EEPROM
#define EEPROM_SETTINGS_ADDR 10

//DebounceTime
#define DebounceTime 50

#define PRINT_SENSOR_STATUS_DELAY 500
unsigned long startShowSensorStatusTicks = 0;
bool ShowSensorStatus = true;
short waterSensorShorClickCount = 0;

#define WATER_ALARM_PIN 3
#define WATER_SENSOR_PIN 2
Button waterSensor(WATER_SENSOR_PIN);

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
  {/*Place:*/1, /*PumpPin*/7, /*SensorPin:*/A0, DefaultWatchDogForPump, DEFAULT_WATERING_REQUIRED_LEVEL, DefaultWateringEnoughLevel},
  {/*Place:*/2, /*PumpPin*/6, /*SensorPin:*/A1, DefaultWatchDogForPump, DEFAULT_WATERING_REQUIRED_LEVEL, DefaultWateringEnoughLevel},
  {/*Place:*/3, /*PumpPin*/5, /*SensorPin:*/A2, DefaultWatchDogForPump, DEFAULT_WATERING_REQUIRED_LEVEL, DefaultWateringEnoughLevel},
  {/*Place:*/4, /*PumpPin*/4, /*SensorPin:*/A3, DefaultWatchDogForPump, DEFAULT_WATERING_REQUIRED_LEVEL, DefaultWateringEnoughLevel},
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

  LoadSettings();

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

const bool CheckPrintSensorStatusDelay(const unsigned long &cuurentTicks)
{
  if(ShowSensorStatus && (cuurentTicks - startShowSensorStatusTicks) >= PRINT_SENSOR_STATUS_DELAY)
  {    
    #ifdef DEBUG      
    PrintSensorsStatus();
    #endif

    LcdPrintSensorStatus(HasWater);
    startShowSensorStatusTicks = cuurentTicks;
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

  auto current = millis();

  CheckBacklightDelay(current);

  CheckPrintSensorStatusDelay(current);

  if(UseWaterSensor && (waterSensor.isPressed() || waterSensor.getState() == ON))
  { 
    if(HasWater)
    {
      Serial.println("!!!! NO Water !!!!");
      SetAllPumps(PumpState::OFF);
    }

    digitalWrite(WATER_ALARM_PIN, HIGH);
    HasWater = false;  
  } 

  if(UseWaterSensor && waterSensor.getState() == OFF)
  {
    if(!HasWater)
    {
      Serial.println("Has Water");
      for (short i = 0; i < ChannelsCount; i++) 
      {
        pumps[i].ResetState();
      }
      PrintStatus();
    }
    
    digitalWrite(WATER_ALARM_PIN, LOW);
    HasWater = true;
  }

  if(waterSensor.isReleased())
  {    
    if(waterSensor.getTicks() <= 1000)
    {
      waterSensorShorClickCount++;
      Serial.print(3 - waterSensorShorClickCount); Serial.println(" Left to Hard reset settings.");
      waterSensor.resetTicks();
    }
    else
    {
      waterSensorShorClickCount = 0;
    }

    if(waterSensorShorClickCount >= 3)
    {
      waterSensorShorClickCount = 0;
      Serial.println("Hard reset settings...");
      ResetSettings();
      PrintStatus();
    }
  }

  for (short i = 0; i < ChannelsCount; i++) 
  {
    if(pumps[i].IsWatchDogTriggered())
    {
      Serial.print(i + 1); Serial.print("  TIMEOUT: ");
      //pumps[i].PrintStatus(/*showDebugInfo:*/true);
      pumps[i].End(TIMEOUT_OFF);      
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }

    if(HasWater && pumps[i].IsWateringRequired())
    {
      BacklightOn();

      Serial.print(i + 1); Serial.print(" REQUIRED: ");      
      pumps[i].Start();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
      SaveSettings();
    }

    if(HasWater && pumps[i].IsWateringEnough())
    {
      Serial.print(i + 1); Serial.print("   ENOUGH: ");      
      pumps[i].End();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }

    if((buttons[i].isPressed()) 
        || debugButtonFromSerial == i + 1)
    {
      BacklightOn();

      auto count = buttons[i].getCount();
      Serial.print(i + 1); Serial.print(" Pressed: "); Serial.print(count); Serial.print(" => ");
      
      pumps[i].getState() == MANUAL_ON ? pumps[i].End(MANUAL_OFF) : pumps[i].Start(MANUAL_ON);

      pumps[i].PrintStatus(/*showDebugInfo:*/true);
    }    
    
    if(buttons[i].isReleased()
        || debugButtonFromSerial == -(i + 1))
    {      
      Serial.print(i + 1); Serial.print(" Released: "); Serial.print(buttons[i].getTicks()); Serial.print("ms. => ");

      if(buttons[i].isLongPress())
      {
        buttons[i].resetTicks();
        pumps[i].End(CALIBRATING);
        SaveSettings();        
      }

      pumps[i].PrintStatus(/*showDebugInfo:*/true);      
    }
  }  

  HandleDebugSerialCommands();  
}

void HandleDebugSerialCommands()
{
  if(debugButtonFromSerial == 5)
  {
    ResetSettings();
    PrintStatus();
  }

  if(debugButtonFromSerial == 6)
  {
    PrintStatus();
    PrintSensorsStatus();
  }

  if(debugButtonFromSerial == 7)
  {
    SetAllPumps(ON);
  }

  if(debugButtonFromSerial == 8)
  {
    SetAllPumps(OFF);
  }

  if(debugButtonFromSerial == 9)
  {
    ResetCounts();
  }

  if(debugButtonFromSerial == 10)
  {
    ShowSensorStatus = !ShowSensorStatus;
  }

  //Reset after 8 secs see watch dog timer
  if(debugButtonFromSerial == 11)
  {
    delay(10 * 1000);
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    debugButtonFromSerial = Serial.readString().toInt();
  }  

  //delay(50);
  wdt_reset();
}

void InitializeButtons()
{
  // Initialize the buttons
  for (short i = 0; i < ChannelsCount; i++) 
  {
    buttons[i].setDebounceTime(DebounceTime); // Set debounce time in milliseconds    
  }
}

void InitializePumps()
{
  for (short i = 0; i < ChannelsCount; i++) 
  {
    if(!pumps[i].Initialize())
    {
      Serial.print("Wrong pump initialization: "); Serial.println(i);
    }
  }
}

void SetAllPumps(const PumpState &state)
{
  Serial.print("Set All Pumps: "); Serial.println(GetStatus(state));
  for (short i = 0; i < ChannelsCount; i++) 
  {
    state == ON || state == MANUAL_ON ? pumps[i].Start(state) : pumps[i].End(state);
    pumps[i].PrintStatus(/*showDebugInfo:*/false);
  }
}

void LoopButtons()
{
  for (short i = 0; i < ChannelsCount; i++)
  {
    buttons[i].loop(); 
  }
}

void ResetButtonsCounts()
{
  for (short i = 0; i < ChannelsCount; i++)
  {
    buttons[i].resetCount();
  }
}

void PrintStatus()
{ 
  for (short i = 0; i < ChannelsCount; i++)
  {
    pumps[i].PrintStatus(/*showDebugInfo:*/true);
  }
}

void PrintSensorsStatus()
{
  Serial.print("                              ");
  for (short i = 0; i < ChannelsCount; i++)
  {
    char buff[100];
    sprintf(buff, "[%d: %d/%d] ", i + 1, pumps[i].GetSensorValue(), pumps[i].Settings.WateringRequired);
    Serial.print(buff);    
  }
  Serial.println();  
}

const char * LcdPrintSensorStatus(const bool &hasWater)
{  
  lcd.clear();
  
  char pumpBuff1[20];  
  char pumpBuff12[10];
  char pumpBuff13[10];

  // char pumpBuff2[20];  
  // char pumpBuff22[10];
  // char pumpBuff23[10];
  
  sprintf(pumpBuff1, "%s|%s", pumps[0].GetShortStatus(hasWater, pumpBuff12), pumps[1].GetShortStatus(hasWater, pumpBuff13));
  Serial.print("1|2 => "); Serial.println(pumpBuff1);
  
  lcd.setCursor(0, 0);
  lcd.print(pumpBuff1);
  
  sprintf(pumpBuff1, "%s|%s", pumps[2].GetShortStatus(hasWater, pumpBuff12), pumps[3].GetShortStatus(hasWater, pumpBuff13));

  lcd.setCursor(0, 1);
  lcd.print(pumpBuff1);

  Serial.print("3|4 => "); Serial.println(pumpBuff1);

  return pumpBuff1;
}

void SaveSettings()
{
  Serial.println("Save Settings...");
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < ChannelsCount; i++)
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
  for (short i = 0; i < ChannelsCount; i++)
  {
    auto settings = pumps[i].Settings;
    pumps[i].Settings = EEPROM.get(addr, settings);

    if(pumps[i].Settings.WatchDog == ULONG_MAX || pumps[i].Settings.WatchDog == 0 || pumps[i].Settings.PumpState >= UNKNOWN)
    {
      Serial.print(i + 1); Serial.println(" Storage is empty or corrupted. Return to default values...");
      pumps[i].Settings.reset();
    }

    addr += sizeof(settings);
  }
  PrintStatus();
}

void ResetSettings()
{
  Serial.println("Reset Settings...");
  auto addr = EEPROM_SETTINGS_ADDR;
  for (short i = 0; i < ChannelsCount; i++)
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
  for (short i = 0; i < ChannelsCount; i++)
  {    
    pumps[i].Settings.Count = 0;
  }
  SaveSettings();
}

void WaitUntilWaterSensorReleased()
{
  while(true)
  {
    waterSensor.loop();
    if(waterSensor.isReleased())
    { 
      Serial.println("Has Water");
      for (short i = 0; i < ChannelsCount; i++) 
      {
        pumps[i].ResetState();
      }
      break;
    }

    LoopButtons();
    for (short i = 0; i < ChannelsCount; i++) 
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