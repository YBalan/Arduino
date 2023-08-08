/*
 * Created by Yurii Balan
 *
 * This example code is in the private domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-button-servo-motor
 */

#include "Constants.h"
#include <Servo.h>
#include <math.h>
#include <ezButton.h>
#include <EEPROM.h>

// variables will change:
Servo servo; // create servo object to control a servo
int angle = INITIAL_ANGLE;          // the current angle of servo motor

ezButton generatorOffBtn(GENERATOR_OFF_BTN_PIN); 
ezButton initialPosBtn(INITIAL_POS_BTN_PIN);
ezButton rotateRightBtn(ROTATE_RIGHT_BTN_PIN);
ezButton rotateLeftBtn(ROTATE_LEFT_BTN_PIN);

void setup() 
{  
    Serial.begin(9600);                // initialize serial
    Serial.println("!!!! START !!!!");

    generatorOffBtn.setDebounceTime(50); 
    initialPosBtn.setDebounceTime(50); 
    rotateRightBtn.setDebounceTime(50); 
    rotateLeftBtn.setDebounceTime(50); 

    EEPROM.get(EEPROM_ANGLE_ADDR, angle);

    servo.attach(SERVO_PIN);
    servo.write(angle);  
    servo.detach();
    
    Serial.print("Servo start: "); Serial.println(angle);
}

void loop() 
{
    //Serial.println("Button loop started");
    generatorOffBtn.loop(); // MUST call the loop() function first
    initialPosBtn.loop(); // MUST call the loop() function first
    rotateRightBtn.loop(); // MUST call the loop() function first
    rotateLeftBtn.loop(); // MUST call the loop() function first
    //Serial.println("Button loop ended");
    
    //Serial.println("Button is pressed check");
    if(generatorOffBtn.isPressed()) 
    {
      Serial.println("The ""generatorOffBtn"" is pressed");
      GeneratorOff(ROTATE_DELAY);    
    }

    if(initialPosBtn.isPressed()) 
    {
      Serial.println("The ""initialPosfBtn"" is pressed");
      InitialPosition();
    }

    if(rotateRightBtn.isPressed()) 
    {
      Serial.println("The ""rotateRightBtn"" is pressed");
      RotateRight();
    }

    if(rotateLeftBtn.isPressed()) 
    {
      Serial.println("The ""rotateLeftBtn"" is pressed");
      RotateLeft();
    }
}

void GeneratorOff(int rotateDelay)
{
    AttachServo();
    //Serial.print("Servo started "); Serial.println(angle);
    if(delay > 0)
    {
      char buffer[256];
      int rotateAngle = angle <= GENERATOR_OFF_ANGLE ? ROTATE_WITH_DELAY_ANGLE : -ROTATE_WITH_DELAY_ANGLE;
      int rotateCount = abs((angle - GENERATOR_OFF_ANGLE) / ROTATE_WITH_DELAY_ANGLE);

      sprintf(buffer, "Servo started at: %d; Destination: %d; Moved by: %d; %d Times. (with delay: %d)", angle, GENERATOR_OFF_ANGLE, rotateAngle, rotateCount, rotateDelay);
      Serial.println(buffer);

      for(int i = 0; i < rotateCount; i++)
      {        
        servo.write(angle += rotateAngle);
        Serial.print("Servo moved: "); Serial.println(angle);     
        delay(rotateDelay);
      }      
    }

    servo.write(GENERATOR_OFF_ANGLE);

    DetachServo();

    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);    
}

void InitialPosition()
{
    AttachServo();
    //Serial.print("Servo started "); Serial.println(angle);
    servo.write(angle = INITIAL_ANGLE);

    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);    
}

void RotateLeft()
{
    AttachServo();
    //Serial.print("Servo started "); Serial.println(angle);
    int rotate = angle + ROTATE_ANGLE;
    angle = rotate >= END_ANGLE ? END_ANGLE : rotate;
    servo.write(angle);    

    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);    
}

void RotateRight()
{    
    AttachServo();
    //Serial.print("Servo started "); Serial.println(angle);
    int rotate = angle - ROTATE_ANGLE;
    angle = rotate <= START_ANGLE ? START_ANGLE : rotate;
    servo.write(angle);  

    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);    
}

void AttachServo()
{
  if(!servo.attached())
  {
    Serial.println("Servo Attached");
    servo.attach(SERVO_PIN);
  }
}

void DetachServo()
{  
    Serial.println("Servo Detached");
    servo.detach();  
}





