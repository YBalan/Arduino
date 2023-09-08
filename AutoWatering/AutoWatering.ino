#include <EEPROM.h>
#include <avr/wdt.h>
#include <LiquidCrystal_I2C.h>

#include "../../Shares/Button.h"
#include "Pump.h"

bool UseWaterChecker = true;

const short ChannelsCount = 4;

const int PrintsSensorsStatusDelay = 5000;
unsigned long startShowSensorStatus = 0;
bool ShowSensorStatus = true;

const int WaterAlarmPin = 3;
const int WaterSensorPin = 2;
Button waterSensor(WaterSensorPin);

Button buttons[] = 
{
  {10}, //Button pin for Pump 1
  {11}, //Button pin for Pump 2
  {12}, //Button pin for Pump 3
  {13}, //Button pin for Pump 4
};

Pump pumps[] =
{
  {/*Place:*/1, /*PumpPin*/7, /*SensorPin:*/A1, DefaultWatchDogForPump, DefaultWateringRequiredLevel, DefaultWateringEnoughLevel},
  {/*Place:*/2, /*PumpPin*/6, /*SensorPin:*/A2, DefaultWatchDogForPump, DefaultWateringRequiredLevel, DefaultWateringEnoughLevel},
  {/*Place:*/3, /*PumpPin*/5, /*SensorPin:*/A3, DefaultWatchDogForPump, DefaultWateringRequiredLevel, DefaultWateringEnoughLevel},
  {/*Place:*/4, /*PumpPin*/4, /*SensorPin:*/A4, DefaultWatchDogForPump, DefaultWateringRequiredLevel, DefaultWateringEnoughLevel},
};

//EEPROM
const int EEPROM_SETTINGS_ADDR = 10;

//DebounceTime
const short DebounceTime = 50;

short debugButtonFromSerial = 0;

void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println();
  Serial.println("!!!!!!!!!!! Auto Watering started 2!!!!!!!!!!!");
  Serial.println();

  InitializePumps();
  InitializeButtons();

  pinMode(WaterAlarmPin, INPUT_PULLUP);
  pinMode(WaterAlarmPin, OUTPUT);
  digitalWrite(WaterAlarmPin, LOW);  

  waterSensor.setDebounceTime(DebounceTime);

  LoadSettings();

  startShowSensorStatus = millis();

  wdt_enable(WDTO_8S); 
  Serial.println("Watchdog enabled.");
}

short waterSensorShorClickCount = 0;

void loop()
{
  LoopButtons();
    
  waterSensor.loop();

  if(ShowSensorStatus && (millis() - startShowSensorStatus) >= PrintsSensorsStatusDelay)
  {      
    PrintSensorsStatus();
    startShowSensorStatus = millis();
  }

  if(UseWaterChecker && (waterSensor.isPressed() || waterSensor.getState() == ON))
  { 
    Serial.println("!!!! NO Water !!!!");
    
    digitalWrite(WaterAlarmPin, HIGH);

    SetAllPumps(PumpState::OFF);
    
    WaitUntilWaterSensorReleased();

    digitalWrite(WaterAlarmPin, LOW);  
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

    if(pumps[i].IsWateringRequired())
    {
      Serial.print(i + 1); Serial.print(" REQUIRED: ");      
      pumps[i].Start();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
      SaveSettings();
    }

    if(pumps[i].IsWateringEnough())
    {
      Serial.print(i + 1); Serial.print("   ENOUGH: ");      
      pumps[i].End();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }

    if((buttons[i].isPressed()) 
        || debugButtonFromSerial == i + 1)
    {
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