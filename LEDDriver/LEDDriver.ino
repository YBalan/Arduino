
#include <EEPROM.h>

#include <LiquidCrystal_I2C.h>
#include "../../Shares/Button.h"

#define LED_PIN 3
#define BTN_PIN 4
#define MODE_PIN 5

bool MODE_POWERON_MAX = true;
#define MAX_BR_VALUE 255
#define MIN_BR_VALUE 5
#define MAX_REPEAT_ON_EDGE 10
#define BR_STEP 10
#define BR_DELAY 200

Button MainBtn(BTN_PIN);

unsigned short repeatOnEdge = 0;

struct Settings
{
  short LedBrightnes = MAX_BR_VALUE;
  bool IsOn = true;
  bool BrDirectionDown = true;
} settings;

//DebounceTime
#define DebounceTime 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

void setup() 
{
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println("Start LED Driver");

  pinMode(LED_PIN, OUTPUT);
  pinMode(MODE_PIN, INPUT_PULLUP);

  MainBtn.setDebounceTime(DebounceTime);

  LoadSettings();

  PrintStatus();

  MODE_POWERON_MAX = digitalRead(MODE_PIN) == LOW;
  Serial.print("MODE: POWER ON - MAX Brightness: "); Serial.println(MODE_POWERON_MAX ? "ON" : "OFF");
  
  settings.LedBrightnes = MODE_POWERON_MAX ? MAX_BR_VALUE : settings.LedBrightnes;
  settings.IsOn = MODE_POWERON_MAX ? true : settings.IsOn;
  settings.BrDirectionDown = MODE_POWERON_MAX ? true : settings.BrDirectionDown;

  ChangeBrightness(0, settings.LedBrightnes); 
   
  PrintStatus();
}

void loop() 
{
  MainBtn.loop();  
   
  CheckAndChangeBrightness();

  if(MainBtn.isPressed())
  {
	  
  }
	
  if(MainBtn.isReleased())
  {
    if(!MainBtn.isLongPress())
    {
      Serial.println("Btn pressed NO long press");
      SwitchReverse();      
    }
    
    settings.BrDirectionDown = !settings.BrDirectionDown;
    
    MainBtn.resetTicks();

    PrintStatus();
    SaveSettings();
  }
}

void PrintStatus()
{
  Serial.print("Br: "); Serial.println(settings.LedBrightnes);  
  Serial.print("IsOn: "); Serial.println(settings.IsOn ? "True" : "False");
  Serial.print("BrDirectonDown: "); Serial.println(settings.BrDirectionDown ? "True" : "False");
}

void ChangeBrightness(const unsigned short &start, const unsigned short &end)
{
  auto endValue = end > MAX_BR_VALUE ? MAX_BR_VALUE : end;
  auto startValue = start > MAX_BR_VALUE ? 0 : start;  
  auto step = endValue > startValue ? 1 : -1;  

  while(startValue != endValue)
  {
    analogWrite(LED_PIN, startValue);
    startValue += step;    
    MainBtn.loop();
    if(MainBtn.isPressed())
    {
      //settings.LedBrightnes = startValue;
      //break;
      return;
    }
    delay(step < 0 ? 10 : 30);
  }  

  analogWrite(LED_PIN, endValue);  
}

void CheckAndChangeBrightness()
{
  if(MainBtn.getTicks() > 0 && MainBtn.isLongPress())
  {  
    Serial.println("Btn pressed LONG press");

    auto step = settings.BrDirectionDown ? (BR_STEP * -1) : BR_STEP;
    auto br = settings.LedBrightnes + step;

    if(br >= MIN_BR_VALUE && br <= MAX_BR_VALUE)
    {
      settings.LedBrightnes = br;

      settings.IsOn = settings.LedBrightnes != 0;
      analogWrite(LED_PIN, settings.LedBrightnes);
    }    

    Serial.println(settings.LedBrightnes);

    delay(BR_DELAY);
  }
}

void CheckAndChangeBrightnessCircle()
{
  if(MainBtn.getTicks() > 0 && MainBtn.isLongPress())
  {  
    Serial.println("Btn pressed LONG press (Circle)");

    auto step = settings.BrDirectionDown ? (BR_STEP * -1) : BR_STEP;
    step = (settings.LedBrightnes + step < MIN_BR_VALUE || settings.LedBrightnes + step > MAX_BR_VALUE) && repeatOnEdge++ <= MAX_REPEAT_ON_EDGE ? 0 : step;
    repeatOnEdge = step != 0 ? 0 : repeatOnEdge;
    settings.LedBrightnes += step;

    if(settings.LedBrightnes < MIN_BR_VALUE)
    {
      settings.LedBrightnes = MIN_BR_VALUE;
      settings.BrDirectionDown = false;
    }

    if(settings.LedBrightnes > MAX_BR_VALUE)
    {
      settings.LedBrightnes = MAX_BR_VALUE;
      settings.BrDirectionDown = true;
    }

    settings.IsOn = settings.LedBrightnes != 0;
    analogWrite(LED_PIN, settings.LedBrightnes);
    Serial.println(settings.LedBrightnes);

    delay(BR_DELAY);
  }  
}

const bool SwitchReverse()
{
  return settings.IsOn ? SwitchOff() : SwitchOn();
}

const bool SwitchOff()
{
  Serial.println("Switch off");
  settings.IsOn = false;
  //analogWrite(LED_PIN, 0);
  ChangeBrightness(settings.LedBrightnes, 0);
  // settings.brightnessDirectionDown = false;
  PrintStatus();
  return true;
}

const bool SwitchOn()
{
  Serial.println("Switch on");
  settings.IsOn = true;
  settings.LedBrightnes = settings.LedBrightnes == 0 ? MAX_BR_VALUE : settings.LedBrightnes;
  //analogWrite(LED_PIN, settings.LedBrightnes);
  ChangeBrightness(0, settings.LedBrightnes);
  // settings.brightnessDirectionDown = false;
  PrintStatus();
  return true;
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
  if(settings.LedBrightnes < 0 || settings.LedBrightnes > MAX_BR_VALUE)
  {
    settings.LedBrightnes = MAX_BR_VALUE;
    settings.BrDirectionDown = true;
  }
}
