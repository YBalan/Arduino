/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-button-servo-motor
 */

#include <Servo.h>
#include <ezButton.h>
#include <EEPROM.h>

// constants won't change

const int GENERATOR_OFF_ANGLE = 0;
const int INITIAL_ANGLE = 90;
const int START_ANGLE = 0;
const int END_ANGLE = 180;
const int ROTATE_ANGLE = 10;
const int EEPROM_ANGLE_ADDR = 10;

const int GENERATOR_OFF_BTN_PIN = 7;
const int INITIAL_POS_BTN_PIN = 6;
const int ROTATE_RIGHT_BTN_PIN = 5;
const int ROTATE_LEFT_BTN_PIN = 4;
const int SERVO_PIN  = 9; // Arduino pin connected to servo motor's pin

Servo servo; // create servo object to control a servo

// variables will change:
int angle = INITIAL_ANGLE;          // the current angle of servo motor

ezButton generatorOffBtn(GENERATOR_OFF_BTN_PIN); 
ezButton initialPosBtn(INITIAL_POS_BTN_PIN);
ezButton rotateRightBtn(ROTATE_RIGHT_BTN_PIN);
ezButton rotateLeftBtn(ROTATE_LEFT_BTN_PIN);

void setup() {  
  Serial.begin(9600);                // initialize serial
  Serial.println("!!!! START !!!!");
  //pinMode(BUTTON_PIN, INPUT); // set arduino pin to input pull-up mode
  servo.attach(SERVO_PIN);           // attaches the servo on pin 9 to the servo object
 
  generatorOffBtn.setDebounceTime(50); 
  initialPosBtn.setDebounceTime(50); 
  rotateRightBtn.setDebounceTime(50); 
  rotateLeftBtn.setDebounceTime(50); 

  EEPROM.get(EEPROM_ANGLE_ADDR, angle);
  servo.write(angle);  
  Serial.print("Servo start: "); Serial.println(angle);
}

void loop() {
  //Serial.println("Button loop started");
  generatorOffBtn.loop(); // MUST call the loop() function first
  initialPosBtn.loop(); // MUST call the loop() function first
  rotateRightBtn.loop(); // MUST call the loop() function first
  rotateLeftBtn.loop(); // MUST call the loop() function first
  //Serial.println("Button loop ended");
  
  //Serial.println("Button is pressed check");
  if(generatorOffBtn.isPressed()) {
    Serial.println("The ""generatorOffBtn"" is pressed");
    GeneratorOff();    
  }

  if(initialPosBtn.isPressed()) {
    Serial.println("The ""initialPosfBtn"" is pressed");
    InitialPosition();
  }

  if(rotateRightBtn.isPressed()) {
    Serial.println("The ""rotateRightBtn"" is pressed");
    RotateRight();
  }


  if(rotateLeftBtn.isPressed()) {
    Serial.println("The ""rotateLeftBtn"" is pressed");
    RotateLeft();
  }

  // if(button.isReleased())
  //   Serial.println("The button is released");    
}

void GeneratorOff()
{
    //Serial.print("Servo startted "); Serial.println(angle);
    servo.write(angle = GENERATOR_OFF_ANGLE);
    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);
    //delay(200);
}

void InitialPosition()
{
    //Serial.print("Servo startted "); Serial.println(angle);
    servo.write(angle = INITIAL_ANGLE);
    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);
    //delay(200);
}

void RotateLeft()
{
    //Serial.print("Servo startted "); Serial.println(angle);
    int rotate = angle + ROTATE_ANGLE;
    angle = rotate >= END_ANGLE ? END_ANGLE : rotate;
    servo.write(angle);    
    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);
    //delay(200);
}

void RotateRight()
{
    //Serial.print("Servo startted "); Serial.println(angle);
    int rotate = angle - ROTATE_ANGLE;
    angle = rotate <= START_ANGLE ? START_ANGLE : rotate;
    servo.write(angle);    
    EEPROM.put(EEPROM_ANGLE_ADDR, angle);
    Serial.print("Servo ended: "); Serial.println(angle);
    //delay(200);
}



