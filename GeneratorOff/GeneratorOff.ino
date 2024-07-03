/*
 * Created by Yurii Balan
 *
 * This example code is in the private domain
 * YouTube: https://youtube.com/shorts/Uko7_9OD-78
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-button-servo-motor https://arduinogetstarted.com/tutorials/arduino-servo-motor
 */

#include <Servo.h>
#include <math.h>
#include <ezButton.h>
#include <EEPROM.h>

#include "Constants.h"

#define VER 1.1
#define ENABLE_TRACE
#define ENABLE_INFO_MAIN
#define ENABLE_TRACE_MAIN

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
#include "DEBUGHelper.h"

//EEPROM
#define EEPROM_SETTINGS_ADDR 10

//DebounceTime
#define DEBOUNCE_TIME 50

// variables will change:
Servo servo;                        // create servo object to control a servo
Servo servo2;

struct Settings
{
    int angle = INITIAL_ANGLE;
    int angle2 = INITIAL_2_ANGLE;
} settings;

ezButton generatorOffBtn(GENERATOR_OFF_BTN_PIN); 
ezButton generatorOffRemoteBtn(GENERATOR_OFF_REMOTE_BTN_PIN); 
ezButton initialPosBtn(INITIAL_POS_BTN_PIN);
ezButton rotateRightBtn(ROTATE_RIGHT_BTN_PIN);
ezButton rotateLeftBtn(ROTATE_LEFT_BTN_PIN);

int debugButtonFromSerial = 0;

void setup() 
{  
    Serial.begin(9600);                // initialize serial
    while (!Serial);
     
    Serial.println();
    Serial.println();
    Serial.println("!!!! START Generator Off !!!!");
    Serial.print("Flash Date: "); Serial.print(__DATE__); Serial.print(' '); Serial.print(__TIME__); Serial.print(' '); Serial.print("V:"); Serial.println(VER);

    generatorOffBtn.setDebounceTime(DEBOUNCE_TIME);
    generatorOffRemoteBtn.setDebounceTime(DEBOUNCE_TIME);
    initialPosBtn.setDebounceTime(DEBOUNCE_TIME);
    rotateRightBtn.setDebounceTime(DEBOUNCE_TIME);
    rotateLeftBtn.setDebounceTime(DEBOUNCE_TIME);

    generatorOffRemoteBtn.setCountMode(COUNT_RISING);

    //EEPROM.get(EEPROM_SETTINGS_ADDR, settings);

    // INFO("EEPROM: ");
    // PrintServosStatus();

    // if(CorrectAngles())
    // {
    //   INFO("Corrected: ");
    //   PrintServosStatus();
    // }

    // AttachServos();

    // servo.write(settings.angle);
    // servo2.write(settings.angle2);

    // PrintServosStatus();
    // DetachServos();

    // SaveSettings();
}

void loop() 
{    
    generatorOffBtn.loop();         // MUST call the loop() function first
    generatorOffRemoteBtn.loop();   // MUST call the loop() function first
    initialPosBtn.loop();           // MUST call the loop() function first
    rotateRightBtn.loop();          // MUST call the loop() function first
    rotateLeftBtn.loop();           // MUST call the loop() function first
    
    if(generatorOffBtn.isReleased() || debugButtonFromSerial == 4) 
    {
      INFO("OFF ", BUTTON_IS_PRESSED_MSG);
      GeneratorOff(ROTATE_DELAY, /*returnToPrevState: */false);      
      debugButtonFromSerial = 0;
    }

    if(generatorOffRemoteBtn.isReleased() || debugButtonFromSerial == 5) 
    {
      const auto count = generatorOffRemoteBtn.getCount();
      INFO("REMOTE ", "OFF ", BUTTON_IS_PRESSED_MSG, " Count: ", count);

      if(count == 1)
      {
        GeneratorOff(ROTATE_DELAY, /*returnToPrevState: */false);      
      }
      if(count == 2)
      {
        InitialPosition();
        generatorOffRemoteBtn.resetCount();
      }

      debugButtonFromSerial = 0;
    }

    if(initialPosBtn.isPressed() || debugButtonFromSerial == 3) 
    {
      INFO("Initial Position ", BUTTON_IS_PRESSED_MSG);
      InitialPosition();      
      debugButtonFromSerial = 0;
    }

    if(rotateLeftBtn.isPressed() || debugButtonFromSerial == 2) 
    {
      INFO("ROTATE LEFT ", BUTTON_IS_PRESSED_MSG);
      RotateLeft();      
      debugButtonFromSerial = 0;
    }

    if(rotateRightBtn.isPressed() || debugButtonFromSerial == 1) 
    {
      INFO("ROTATE RIGHT ", BUTTON_IS_PRESSED_MSG);
      RotateRight();      
      debugButtonFromSerial = 0;
    }   

    if(Serial.available() > 0)
    {
      debugButtonFromSerial = Serial.readString().toInt();
    }
}

const bool CorrectAngles()
{
  bool res = false;
  if(settings.angle < START_ANGLE || settings.angle > END_ANGLE)
  {
    settings.angle = INITIAL_ANGLE;
    res = true;
  }
  if(settings.angle2 < END_2_ANGLE || settings.angle2 > START_2_ANGLE )
  {
    settings.angle2 = INITIAL_2_ANGLE;
    res = true;
  }

  return res;
}

void GeneratorOff(const uint8_t &rotateDelay, const bool &returnToPrevState)
{
    AttachServos();

    Settings prevState = settings;

    //Serial.print("Servo started "); Serial.println(angle);
    if(rotateDelay > 0)    
    {      
      settings.angle = servo.read();
      settings.angle2 = servo2.read();
      
      short rotateAngle = settings.angle <= GENERATOR_OFF_ANGLE ? ROTATE_WITH_DELAY_ANGLE : -ROTATE_WITH_DELAY_ANGLE;
      short rotateCount = abs((settings.angle - GENERATOR_OFF_ANGLE) / ROTATE_WITH_DELAY_ANGLE);      

      TRACE("Servos started at: ", settings.angle, " & ", settings.angle2, " Destination: ", GENERATOR_OFF_ANGLE, " & ", GENERATOR_OFF_2_ANGLE, " Moved by: ", rotateAngle, " Count: ", rotateCount, " (with delay: ", rotateDelay, ")");

      for(uint8_t i = 0; i < rotateCount; i++)
      {        
        servo.write(settings.angle += rotateAngle);
        servo2.write(settings.angle2 -= rotateAngle);
        //PrintServosStatus();
        delay(rotateDelay);
      }      
    }

    servo.write(GENERATOR_OFF_ANGLE);
    servo2.write(GENERATOR_OFF_2_ANGLE);

    PrintServosStatus();
    SaveSettings();
    DetachServos();

    if(returnToPrevState)
    {      
      delay(10000);
      INFO("Return to previous state...");

      AttachServos();

      servo.write(prevState.angle);
      servo2.write(prevState.angle2);

      settings = prevState;

      PrintServosStatus();
      DetachServos();
      SaveSettings();
    }
}

void InitialPosition()
{
    AttachServos();
    //Serial.print("Servo started "); Serial.println(angle);
    servo.write(settings.angle = INITIAL_ANGLE);
    servo2.write(settings.angle2 = INITIAL_2_ANGLE);

    PrintServosStatus();
    DetachServos();
    SaveSettings();
}

void RotateLeft()
{
    AttachServos();
    //Serial.print("Servo started "); Serial.println(angle);
    {
      int rotate = settings.angle + ROTATE_ANGLE;
      settings.angle = rotate >= END_ANGLE ? END_ANGLE : rotate;
      servo.write(settings.angle);
    }

    {
      int rotate2 = settings.angle2 - ROTATE_ANGLE;      
      settings.angle2 = rotate2 <= END_2_ANGLE ? END_2_ANGLE : rotate2;
      servo2.write(settings.angle2);      
    }

    PrintServosStatus();
    DetachServos();
    SaveSettings();
}

void RotateRight()
{   
    AttachServos();
    //Serial.print("Servo started "); Serial.println(angle);
    {
      int rotate = settings.angle - ROTATE_ANGLE;
      settings.angle = rotate <= START_ANGLE ? START_ANGLE : rotate;
      servo.write(settings.angle);
    }

    {
      int rotate2 = settings.angle2 + ROTATE_ANGLE;      
      settings.angle2 = rotate2 >= START_2_ANGLE ? START_2_ANGLE : rotate2;
      servo2.write(settings.angle2);
    }

    PrintServosStatus();
    DetachServos();
    SaveSettings();
}

void AttachServos()
{
  if(!servo.attached())
  {
    INFO("Servo1 Attached");
    servo.attach(SERVO_PIN);
  }

  if(!servo2.attached())
  {
    INFO("Servo2 Attached");
    servo2.attach(SERVO2_PIN);
  }  
}

void DetachServos()
{  
    INFO("Servos Detached");
    servo.detach();
    servo2.detach();
}

void PrintServosStatus()
{ 
  INFO("Status: angle1:[", settings.angle, "] angle2:[", settings.angle2, "]");
}

void SaveSettings()
{
  INFO("Save Settings...");
  //EEPROM.put(EEPROM_SETTINGS_ADDR, settings);
}




