
#include <ezButton.h>
#include <EEPROM.h>


#define OFF HIGH
#define ON LOW

const int WC_IN_PIN = 13;
const int WC_SW_PIN = 3; //White

const int BATH_IN_PIN = 12;
const int BATH_SW_PIN = 2; //Brown

const int OFF_BTN_PIN = 11;

//EEPROM
const int EEPROM_SETTINGS_ADDR = 10;

//DebounceTime
const int DebounceTime = 50;

struct Settings
{
    int WCState = OFF;
    int BathState = OFF;
    int WCCounter = 0;
    int BathCounter = 0;

    void reset()
    {
      WCState = OFF;
      BathState = OFF;
      WCCounter = 0;
      BathCounter = 0;
    }

} settings;

ezButton WCBtn(WC_IN_PIN); 
ezButton BathBtn(BATH_IN_PIN);
ezButton OffBtn(OFF_BTN_PIN);

int debugButtonFromSerial = 0;

void setup()
{
  Serial.begin(9600);                // initialize serial
     
  Serial.println();
  Serial.println();
  Serial.println("!!!! START WC Toilet & Bathroom Auto light 3!!!!");

  WCBtn.setDebounceTime(DebounceTime);
  BathBtn.setDebounceTime(DebounceTime);
  OffBtn.setDebounceTime(DebounceTime);

  pinMode(WC_SW_PIN, OUTPUT);
  pinMode(BATH_SW_PIN, OUTPUT);

  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);
  
  Serial.print("EEPROM: ");
  PrintStatus();

  Switch();
}

void loop()
{
  WCBtn.loop();
  BathBtn.loop();
  OffBtn.loop();

  if(OffBtn.isPressed())
  {
    debugButtonFromSerial = 0;
    Serial.println("The ""OffBtn"" is pressed: ");
    settings.reset();
    Switch();
    PrintStatus();
  }

  if(WCBtn.isPressed() || debugButtonFromSerial == 1)
  {
    debugButtonFromSerial = 0;
    Serial.print("The ""WCBtn"" is pressed: "); Serial.println(WCBtn.getCount());
    
    if(settings.WCCounter == 0)
    {
      settings.WCState = settings.WCState == OFF ? ON : OFF;
      
      settings.WCCounter = 0;

      SaveSettings();
      Switch();      
    } 
    else
    {
      settings.WCCounter++;
    }   

    WCBtn.resetCount();
    PrintStatus();    
    
  }

  if(WCBtn.isReleased())
  {
    Serial.print("The ""WCBtn"" is released: "); Serial.println(WCBtn.getCount());

    settings.WCCounter++;

    if(settings.WCCounter >= 3 || settings.WCCounter == 0)
    {
      settings.WCState = settings.WCState == OFF ? ON : OFF;
      
      settings.WCCounter = 0;

      SaveSettings();
      Switch();      
    } 

    PrintStatus();
  }

  if(BathBtn.isPressed() || debugButtonFromSerial == 2)
  {
    debugButtonFromSerial = 0;
    Serial.print("The ""BathBtn"" is pressed: "); Serial.println(BathBtn.getCount());
        
    if(settings.BathCounter++ >= 1)
    {
      settings.BathState = settings.BathState == OFF ? ON : OFF;
      settings.BathCounter = 0;

      SaveSettings();
      Switch();
    }   
    
    BathBtn.resetCount();
    PrintStatus();
  }

  // if(BathBtn.isReleased())
  // {
  //   Serial.print("The ""BathBtn"" is released: "); Serial.println(BathBtn.getCount());    
  //   settings.BathCounter++;
  //   PrintStatus();
  // }

  if(Serial.available() > 0)
  {
    debugButtonFromSerial = Serial.readString().toInt();
  }

  //SaveSettings();
}

void Switch()
{
  digitalWrite(WC_SW_PIN, settings.WCState);
  digitalWrite(BATH_SW_PIN, settings.BathState);
}

void PrintStatus()
{  
  char buffer[256];
  sprintf(buffer, "Status: WC:[%s][%d] BATH:[%s][%d]", GetStatus(settings.WCState), settings.WCCounter, GetStatus(settings.BathState), settings.BathCounter);
  Serial.println(buffer);
}

char * GetStatus(int state)
{
  return state == OFF ? "OFF" : "ON";
}

void SaveSettings()
{
  Serial.println("Save Settings...");
  EEPROM.put(EEPROM_SETTINGS_ADDR, settings);
}