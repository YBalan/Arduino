#pragma once
#ifndef FEED_MOTOR_H
#define FEED_MOTOR_H

#include <Servo.h>
#include "ezButton.h"
#include "DEBUGHelper.h"

#define MOTOR_RIGHT_POS 0
#define MOTOR_LEFT_POS  180
#define MOTOR_ROTATE_DELAY 100
#define MOTOR_ROTATE_VALUE 5
#define MOTOR_STEP_BACK_MODE true
#define MOTOR_STEP_BACK_VALUE 10

namespace Feed
{
  class FeedMotor
  {
    short _pin;
    Servo _servo;
    Print * const _printProgress = 0;
  public:
    FeedMotor(const short &pin) : _pin(pin)
    { }
    FeedMotor(const short &pin, const Print &print) : _pin(pin), _printProgress(&print)
    { }

    const short &Init() const { return _printProgress != 0 ? _printProgress->write("", /*Helpers::LcdProgressCommands::Init*/4) : 0; }

    const bool DoFeed(unsigned short &currentPosition, const bool &showProgress, const ezButton &cancelButton)
    {
      if(!Attach()) return false;

      S_PRINT3("Do Feed (", currentPosition, ")");
        
      short pos = currentPosition;
      const short endPos = pos == MOTOR_LEFT_POS ? MOTOR_RIGHT_POS : MOTOR_LEFT_POS;
      const short rotate = pos == MOTOR_LEFT_POS ? -MOTOR_ROTATE_VALUE : MOTOR_ROTATE_VALUE;
      const short rotateBack = pos == MOTOR_LEFT_POS ? -MOTOR_STEP_BACK_VALUE : MOTOR_STEP_BACK_VALUE;

      S_PRINT2("CurrentPos: ", pos);
      S_PRINT2("    EndPos: ", endPos);
      S_PRINT2("    Rotate: ", rotate);

      if(showProgress && _printProgress != 0) _printProgress->write("", /*Helpers::LcdProgressCommands::Clear*/5);

      if(pos >= MOTOR_RIGHT_POS && pos <= MOTOR_LEFT_POS)
      {
        while (rotate > 0 ? pos <= endPos : pos >= endPos)
        {
          cancelButton.loop();
          if(cancelButton.isPressed())
          { S_PRINT("Cancel!"); break; }
          
          if(showProgress && _printProgress != 0)
          {
            short pct = map(rotate < 0 ? MOTOR_LEFT_POS - pos : pos, MOTOR_RIGHT_POS, MOTOR_LEFT_POS, 0, 100);         
            _printProgress->write(pct);
          }

          _servo.write(pos);
          if(MOTOR_STEP_BACK_MODE)
          {
            short stepBackPos = pos - rotateBack;
            if(stepBackPos >= MOTOR_RIGHT_POS + MOTOR_STEP_BACK_VALUE && stepBackPos <= MOTOR_LEFT_POS - MOTOR_STEP_BACK_VALUE)
            {
              delay(MOTOR_ROTATE_DELAY);
              _servo.write(stepBackPos);
            }
          }

          S_PRINT2("CurrentPos: ", pos);

          pos += rotate;
          delay(MOTOR_ROTATE_DELAY);

          wdt_reset();
        }

        currentPosition = endPos;

        S_PRINT3("Do Feed (", currentPosition, ")");

        Detach();
        return true;
      }

      Detach();
      return false;
    }

    const bool Attach()
    {
      if(!_servo.attached())
      {
        if(_servo.attach(_pin) != INVALID_SERVO)
        {
          S_PRINT2("Servo Attached to pin: ", _pin);
          return true;
        }
        S_PRINT2("INVALID SERVO on pin: ", _pin);
        return false;
      }
      S_PRINT2("Servo already Attached to pin: ", _pin);
      return true;
    }

    const bool Detach()
    {
      if(_servo.attached())
      {
        _servo.detach();
        S_PRINT2("Servo Detached from pin: ", _pin);
        return true;
      }
      return false;
    }
  };
}

#endif //FEED_MOTOR_H