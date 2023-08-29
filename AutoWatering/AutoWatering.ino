#include <EEPROM.h>
#include <ezButton.h>

#include "Button.h"
#include "Pump.h"


bool HasWater = false;
bool UseWaterChecker = false;

const short ChannelsCount = 4;

const short DefaultWatchDogForPumpInSecs = 10;
const int DefaultWatchDogForPump = 1000 * 2 * DefaultWatchDogForPumpInSecs;

const short DefaultWateringRequiredLevel = 500;
const short DefaultWateringEnoughLevel = 100;

const unsigned long ULONG_MAX = 0UL - 1UL;

const int WaterSwitchPin = 2;
ezButton waterSwitch(WaterSwitchPin);

Button buttons[] = 
{
  {10}, 
  {11}, 
  {12}, 
  {13},
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

bool firstLoop = true;

void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println();
  Serial.println("!!!!!!!!!!! Auto Watering started 2!!!!!!!!!!!");
  Serial.println();

  InitializePumps();
  InitializeButtons();

  waterSwitch.setDebounceTime(DebounceTime);

  LoadSettings();
}

void loop()
{
  LoopButtons();

  waterSwitch.loop();

  if(UseWaterChecker && (waterSwitch.isPressed() || waterSwitch.getStateRaw() == ON))
  {    
    Serial.println("Has Water");
    HasWater = true;
  }

  if(UseWaterChecker && (waterSwitch.isReleased() || waterSwitch.getStateRaw() == OFF))
  {
    Serial.println("!!!! NO Water !!!!");
    HasWater = false;
    delay(10000);
    return;
  }  

  for (short i = 0; i < ChannelsCount; i++) 
  {
    if(buttons[i].getStateRaw() == ON && buttons[i].isLongPress())
    {
      pumps[i].setState(CALIBRATING);
    }

    if(pumps[i].IsWatchDogTriggered())
    {
      Serial.print("WatchDog is triggered for: ");
      pumps[i].PrintStatus(/*showDebugInfo:*/true);
      pumps[i].End();      
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }

    if(pumps[i].IsWateringRequired() && pumps[i].getState() == OFF)
    {
      Serial.print(i + 1); Serial.print(" Watering required: ");      
      pumps[i].Start();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }

    if(pumps[i].IsWateringEnough() && pumps[i].getState() == ON)
    {
      Serial.print(i + 1); Serial.print(" Watering enough: ");      
      pumps[i].End();
      pumps[i].PrintStatus(/*showDebugInfo:*/false);
    }    

    if((buttons[i].isPressed()) 
        || debugButtonFromSerial == i + 1)
    {
      auto count = buttons[i].getCount();
      Serial.print(i + 1); Serial.print(" Pressed: "); Serial.print(count); Serial.print(" => ");
      if(pumps[i].getState() == ON)
      {
        pumps[i].End();
      }
      else
      {
        pumps[i].Start();
      }
      pumps[i].PrintStatus(/*showDebugInfo:*/true);
    }    
    
    if(buttons[i].isReleased()
        || debugButtonFromSerial == -(i + 1))
    {
      auto count = buttons[i].getCount();

      //char buff[30]; sprintf(buff, "%lu", buttons[i].getTicks());
      Serial.print(i + 1); Serial.print(" Released: "); Serial.print(buttons[i].getTicks()); Serial.print(" => ");

      if(buttons[i].isLongPress())
      {
        pumps[i].End(/*calibrating:*/true);
        SaveSettings();        
      }

      pumps[i].PrintStatus(/*showDebugInfo:*/true);
    }
  }

  //ResetButtonsCounts();

  if(debugButtonFromSerial == 5)
  {
    ResetSettings();
  }

  debugButtonFromSerial = 0;
  if(Serial.available() > 0)
  {
    debugButtonFromSerial = Serial.readString().toInt();
  }

  //delay(20);
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

    if(pumps[i].Settings.WatchDog == ULONG_MAX || pumps[i].Settings.WatchDog == 0)
    {
      Serial.print(i + 1); Serial.println(" Storage is empty or corrupted. Return to default values...");
      pumps[i].Settings.WatchDog = DefaultWatchDogForPump;
      pumps[i].Settings.WateringRequired = DefaultWateringRequiredLevel;
      pumps[i].Settings.WateringEnough = DefaultWateringEnoughLevel;
    }

    //pumps[i].PrintStatus(/*showDebugInfo:*/true);
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
    Pump::PumpSettings settings;
    EEPROM.put(addr, settings);
    addr += sizeof(settings);
  }
}