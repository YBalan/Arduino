/***************************************************
DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>
 
 ***************************************************
 This example shows the basic function of library for DFPlayer.
 
 Created 2023-12-26

 ****************************************************/

#define VER 1.1
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN
#define ENABLE_TRACE_MAIN
#define ENABLE_DFP_TRACE

#include "Arduino.h"
#include <EEPROM.h>
#include <ezButton.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

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

#ifdef ENABLE_DFP_TRACE
#define DFP_TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define DFP_TRACE(...) {}
#endif

#include "DEBUGHelper.h"
//DebounceTime
#define DEBOUNCE_TIME 50

//EEPROM
#define EEPROM_SETTINGS_ADDR 0

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

#define BTN_OPEN_PIN 2
ezButton openBtn(BTN_OPEN_PIN);
#define BTN_PLAY_PIN 4
ezButton playBtn(BTN_PLAY_PIN);
#define BTN_STOP_PIN 3
ezButton stopBtn(BTN_STOP_PIN);

struct Settings
{
  uint8_t CurrentVolume = 16;
  uint16_t CurrentTrack = 0;
  void Reset() { CurrentVolume = 16; CurrentTrack = 0; } 
}settings;

bool IsDFOk = true;

void setup()
{
  //pinMode(10, INPUT_PULLUP);
  //pinMode(11, INPUT_PULLUP);

  mySoftwareSerial.begin(9600);
  while (!mySoftwareSerial);
  
  Serial.begin(9600);
  while (!Serial);

  openBtn.setDebounceTime(DEBOUNCE_TIME);
  playBtn.setDebounceTime(DEBOUNCE_TIME);
  stopBtn.setDebounceTime(DEBOUNCE_TIME);
  
  Serial.println();
  Serial.println(F("MP3 Music Box Started"));
  Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);
  DFP_TRACE(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  myDFPlayer.setTimeOut(1000);
  
  if (!myDFPlayer.begin(mySoftwareSerial, true, true)) {  //Use softwareSerial to communicate with mp3.
    DFP_TRACE(F("Unable to begin:"));
    DFP_TRACE(F("1.Please recheck the connection!"));
    DFP_TRACE(F("2.Please insert the SD card!"));

    IsDFOk = false;
    // while(true){
    //   delay(0); // Code to compatible with ESP8266 watch dog.
    // }
  }
  DFP_TRACE("DFPlayer Mini ", IsDFOk ? "online." : "offline."); 
  
  LoadSettings();

  if(IsDFOk)
  {
    SetVolume(settings.CurrentVolume); 
    myDFPlayer.enableLoop();
    myDFPlayer.enableLoopAll();
  }

  PrintStatusToSerial();
}

void loop()
{
  static unsigned long timer = millis();

  openBtn.loop();
  playBtn.loop();
  stopBtn.loop();

  if(openBtn.isPressed())
  {
    INFO("OPEN ", BUTTON_IS_PRESSED_MSG, " V:", IsDFOk ? myDFPlayer.readVolume() : -1);
    myDFPlayer.stop();
    //PrintStatusToSerial();
    if(IsDFOk)
    {
      settings.CurrentVolume = myDFPlayer.readVolume();
      SaveSettings();
    }
  }

  if(openBtn.isReleased())
  {
    INFO("OPEN ", BUTTON_IS_RELEASED_MSG, " V:", IsDFOk ? myDFPlayer.readVolume() : -1);

    if(IsDFOk)    
      SetVolume(settings.CurrentVolume);

    myDFPlayer.start();    
    //PrintStatusToSerial();
    if(IsDFOk)
    {
      settings.CurrentVolume = myDFPlayer.readVolume();
      SaveSettings();
    }
  }

  if(playBtn.isReleased())
  {
    INFO("PLAY ", BUTTON_IS_RELEASED_MSG, " V:", IsDFOk ? myDFPlayer.readVolume() : -1);

    if(IsDFOk)
      SetVolume(settings.CurrentVolume);    
    
    INFO("CurrentTrack: ", settings.CurrentTrack);

    if(settings.CurrentTrack == 0)
    {
      settings.CurrentTrack = IsDFOk ? myDFPlayer.readCurrentFileNumber() : 1;
      myDFPlayer.start();    
    }
    else
    {
      settings.CurrentTrack = 0;
      myDFPlayer.pause();    
    }

    INFO("CurrentTrack: ", settings.CurrentTrack);

    //PrintStatusToSerial();
    if(IsDFOk)
    {
      settings.CurrentVolume = myDFPlayer.readVolume();
      SaveSettings();
    }
  }  

  if(stopBtn.isReleased())
  {
    INFO("STOP ", BUTTON_IS_RELEASED_MSG, " V:", IsDFOk ? myDFPlayer.readVolume() : -1);
    myDFPlayer.stop();
    //PrintStatusToSerial();

    settings.CurrentTrack = 0;

    if(IsDFOk)
    {
      settings.CurrentVolume = myDFPlayer.readVolume();
      SaveSettings();
    }
  }
  
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

  if(Serial.available() > 0)
  {
    const auto debug = Serial.readString().toInt();
    TRACE("Debug input:", debug);
    switch(debug)
    {
      case 0:
        PrintStatusToSerial();
      break;
      case 1:
        TRACE("Pause...");
        myDFPlayer.pause();        
      break;
      case 2:
        TRACE("Start...");
        myDFPlayer.start();
      break;
      case 3:        
        myDFPlayer.volumeDown();
        TRACE("Volume Down...", myDFPlayer.readVolume());
      break;
      case 4:        
        myDFPlayer.volumeUp();
        TRACE("Volume Up...", myDFPlayer.readVolume());
      break;
      case 5:        
        SetVolume(5);
        TRACE("Volume 5...", myDFPlayer.readVolume());
      break;
      case 6:        
        SetEQNext();
        TRACE("EQ Next...", myDFPlayer.readEQ());
      break;
    }
  }
}

//0~5 (0 - Normal, 1 - Pop, 2 - Rock, 3 - Jazz, 4 - Classic, 5 - Bass)
void SetEQNext()
{
  auto currentEQ = myDFPlayer.readEQ();
  currentEQ++;
  currentEQ = currentEQ > 5 ? 0 : currentEQ;
  myDFPlayer.EQ(currentEQ);
}

void SetVolume(const uint8_t &volume)
{
  DFP_TRACE("SetVolume");
  auto currentVolume = myDFPlayer.readVolume();
  DFP_TRACE("from:", currentVolume, " to:", volume);

  if(currentVolume == volume) return;

  bool up = currentVolume < volume;

  while(currentVolume != volume)
  {
    if(up) myDFPlayer.volumeUp();
    else myDFPlayer.volumeDown();
    currentVolume += up ? 1 : -1;

    delay(50);
  }  

  DFP_TRACE("SetVolume", ":", myDFPlayer.readVolume());
}

void SaveSettings()
{
  INFO("Save...");  

  TRACE("Max:", EEPROM.length(), " Total: ", sizeof(settings));

  EEPROM.put(EEPROM_SETTINGS_ADDR, settings);
}

void LoadSettings()
{
  INFO("Load..."); 
  
  EEPROM.get(EEPROM_SETTINGS_ADDR, settings);  

  if(settings.CurrentVolume == 255)
  {
    settings.Reset();    
  }
}

void PrintStatusToSerial()
{
  if(IsDFOk)
  {
    Serial.print("State: "); Serial.println(myDFPlayer.readState()); //read mp3 state
    Serial.print("Volume: "); Serial.println(myDFPlayer.readVolume()); //read current volume
    Serial.print("EQ: "); Serial.println(myDFPlayer.readEQ()); //read EQ setting
    Serial.print("Files Count: "); Serial.println(myDFPlayer.readFileCounts()); //read all file counts in SD card
    Serial.print("DFP CurrentTrack: ");Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
    Serial.print("EEPROM CurrentTrack: ");Serial.println(settings.CurrentTrack);
    //Serial.println(myDFPlayer.readFileCountsInFolder(3)); //read file counts in folder SD:/03
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      DFP_TRACE(F("Time Out!"));
      break;
    case WrongStack:
      DFP_TRACE(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      DFP_TRACE(F("Card Inserted!"));
      if(IsDFOk)
        SetVolume(settings.CurrentVolume);
      PrintStatusToSerial();      
      break;
    case DFPlayerCardRemoved:
      DFP_TRACE(F("Card Removed!"));
      //PrintStatusToSerial();
      break;
    case DFPlayerCardOnline:
      DFP_TRACE(F("Card Online!"));
      if(IsDFOk)
        SetVolume(settings.CurrentVolume);
      PrintStatusToSerial();
      break;
    case DFPlayerUSBInserted:
      DFP_TRACE("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      DFP_TRACE("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      DFP_TRACE(F("Number:"), value, " ", "Play Finished!");      
      break;
    case DFPlayerFeedBack:
      DFP_TRACE(F("Number:"), value, " ", "Play Feedback!");
      break;
    case DFPlayerError:
      DFP_TRACE(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          DFP_TRACE(F("Card not found"));
          break;
        case Sleeping:
          DFP_TRACE(F("Sleeping"));
          break;
        case SerialWrongStack:
          DFP_TRACE(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          DFP_TRACE(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          DFP_TRACE(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          DFP_TRACE(F("Cannot Find File"));
          break;
        case Advertise:
          DFP_TRACE(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      DFP_TRACE("Default: ", " type:", type, " value:", value);
      break;
  }  
}
