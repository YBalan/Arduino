/*
 * Created by Yurii Balan
 *
 * This example code is in the private domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-button-servo-motor https://arduinogetstarted.com/tutorials/arduino-servo-motor
 */

#include <Servo.h>
#include <math.h>
#include <ezButton.h>
#include <EEPROM.h>

#include "Constants.h"

//EEPROM
const int EEPROM_SETTINGS_ADDR = 10;

//DebounceTime
const int DebounceTime = 50;

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
     
    Serial.println();
    Serial.println();
    Serial.println("!!!! START fuck 111 !!!!");

    generatorOffBtn.setDebounceTime(DebounceTime);
    generatorOffRemoteBtn.setDebounceTime(DebounceTime);
    initialPosBtn.setDebounceTime(DebounceTime);
    rotateRightBtn.setDebounceTime(DebounceTime);
    rotateLeftBtn.setDebounceTime(DebounceTime);

    EEPROM.get(EEPROM_SETTINGS_ADDR, settings);

    Serial.print("EEPROM: ");
    PrintServosStatus();

    if(CorrectAngles())
    {
      Serial.print("Correct: ");
      PrintServosStatus();
    }

    AttachServos();

    servo.write(settings.angle);
    servo2.write(settings.angle2);

    PrintServosStatus();
    DetachServos();
    SaveSettings();
}

void loop() 
{
    //Serial.println("Button loop started");
    generatorOffBtn.loop();         // MUST call the loop() function first
    generatorOffRemoteBtn.loop();   // MUST call the loop() function first
    initialPosBtn.loop();           // MUST call the loop() function first
    rotateRightBtn.loop();          // MUST call the loop() function first
    rotateLeftBtn.loop();           // MUST call the loop() function first
    //Serial.println("Button loop ended");
    
    //Serial.println("Button is pressed check");
    if(generatorOffBtn.isReleased() || debugButtonFromSerial == 4) 
    {
      Serial.println("The ""generatorOffBtn"" is pressed");
      GeneratorOff(ROTATE_DELAY, /*returnToPrevState: */false);
      generatorOffBtn.resetCount();
      debugButtonFromSerial = 0;
    }

    if(generatorOffRemoteBtn.isReleased() || debugButtonFromSerial == 5) 
    {
      Serial.println("The ""generatorOffRemoteBtn"" is pressed");
      GeneratorOff(ROTATE_DELAY, /*returnToPrevState: */true);
      generatorOffRemoteBtn.resetCount();
      debugButtonFromSerial = 0;
    }

    if(initialPosBtn.isPressed() || debugButtonFromSerial == 3) 
    {
      Serial.println("The ""initialPosfBtn"" is pressed");
      InitialPosition();
      initialPosBtn.resetCount();
      debugButtonFromSerial = 0;
    }

    if(rotateLeftBtn.isPressed() || debugButtonFromSerial == 2) 
    {
      Serial.println("The ""rotateLeftBtn"" is pressed");
      RotateLeft();
      rotateLeftBtn.resetCount();
      debugButtonFromSerial = 0;
    }

    if(rotateRightBtn.isPressed() || debugButtonFromSerial == 1) 
    {
      Serial.println("The ""rotateRightBtn"" is pressed");
      RotateRight();
      rotateRightBtn.resetCount();
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

void GeneratorOff(const short &rotateDelay, const bool &returnToPrevState)
{
    AttachServos();

    Settings prevState = settings;

    //Serial.print("Servo started "); Serial.println(angle);
    if(rotateDelay > 0)
    {      
      short rotateAngle = settings.angle <= GENERATOR_OFF_ANGLE ? ROTATE_WITH_DELAY_ANGLE : -ROTATE_WITH_DELAY_ANGLE;
      short rotateCount = abs((settings.angle - GENERATOR_OFF_ANGLE) / ROTATE_WITH_DELAY_ANGLE);

      #ifdef DEBUG
      char buffer[256];
      sprintf(buffer, "Servos started at: %d & %d; Destination: %d & %d; Moved by: %d; %d Times. (with delay: %d)", settings.angle, settings.angle2, GENERATOR_OFF_ANGLE, GENERATOR_OFF_2_ANGLE, rotateAngle, rotateCount, rotateDelay);
      Serial.println(buffer);
      #endif

      for(int i = 0; i < rotateCount; i++)
      {        
        servo.write(settings.angle += rotateAngle);
        servo2.write(settings.angle2 -= rotateAngle);
        PrintServosStatus();
        delay(rotateDelay);
      }      
    }

    servo.write(GENERATOR_OFF_ANGLE);
    servo2.write(GENERATOR_OFF_2_ANGLE);

    PrintServosStatus();
    SaveSettings();

    if(returnToPrevState)
    {      
      delay(10000);
      Serial.println("Return to previous state...");
      servo.write(prevState.angle);
      servo2.write(prevState.angle2);

      settings = prevState;
    }    

    PrintServosStatus();
    DetachServos();
    SaveSettings();
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
    Serial.println("Servo1 Attached");
    servo.attach(SERVO_PIN);
  }

  if(!servo2.attached())
  {
    Serial.println("Servo2 Attached");
    servo2.attach(SERVO2_PIN);
  }  
}

void DetachServos()
{  
    Serial.println("Servos Detached");
    servo.detach();
    servo2.detach();
}

void PrintServosStatus()
{  
  char buffer[100];
  sprintf(buffer, "Status: angle1:[%d] angle2:[%d]", settings.angle, settings.angle2);
  Serial.println(buffer);
}

void SaveSettings()
{
  Serial.println("Save Settings...");
  EEPROM.put(EEPROM_SETTINGS_ADDR, settings);
}




