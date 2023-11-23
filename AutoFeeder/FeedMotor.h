#pragma once
#ifndef FEED_MOTOR_H
#define FEED_MOTOR_H

#include <Servo.h>
#include "ezButton.h"
#include "DEBUGHelper.h"


#define MOTOR_DS3218MG_270

#define MOTOR_MIN_US 500
#define MOTOR_MAX_US 2500
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
    FeedMotor(const short &pin, const Print &print) : _pin(pin), _servo(), _printProgress(&print)
    { }

    const short &Init() const { return _printProgress != 0 ? _printProgress->write("", /*Helpers::LcdProgressCommands::Init*/4) : 0; }

    const bool DoFeed(unsigned short &currentPosition, const short &feedCount, const bool &showProgress, const ezButton &cancelButton)
    {
      if(!Attach()) return false;

      float usFactor = (MOTOR_MAX_US - MOTOR_MIN_US) / MOTOR_LEFT_POS;
      S_INFO4("Servo read: ", _servo.readMicroseconds(), " factor: ", usFactor);      

      bool res = false;

      if(showProgress && _printProgress != 0) _printProgress->write("", /*Helpers::LcdProgressCommands::Clear*/5);

      const short totalCount = (MOTOR_LEFT_POS - MOTOR_RIGHT_POS) * feedCount;
      short pctValue = 0;
      bool cancel = false;

      for(short feed = 1; feed <= feedCount; feed++)
      {   
        cancelButton.loop();
        if(cancel || cancelButton.isPressed())
        { S_TRACE("Cancel!"); break; }

        short pos = currentPosition;
        short endPos = pos >= MOTOR_LEFT_POS ? MOTOR_RIGHT_POS : MOTOR_LEFT_POS;
        const short rotate = pos >= MOTOR_LEFT_POS ? -MOTOR_ROTATE_VALUE : MOTOR_ROTATE_VALUE;
        const short rotateBack = pos >= MOTOR_LEFT_POS ? -MOTOR_STEP_BACK_VALUE : MOTOR_STEP_BACK_VALUE;

        S_TRACE2("   Feed No: ", feed);
        S_TRACE2("CurrentPos: ", pos);
        S_TRACE2("    EndPos: ", endPos);
        S_TRACE2("    Rotate: ", rotate);

        if(feed > 1) pos += rotate;

        if(pos >= MOTOR_RIGHT_POS && pos <= MOTOR_LEFT_POS)
        {
          while (rotate > 0 ? pos <= endPos : pos >= endPos)
          {
            currentPosition = pos;
            S_TRACE2("CurrentPos: ", currentPosition);

            cancelButton.loop();
            if(cancelButton.isPressed())
            { S_TRACE("Cancel!"); cancel = true; break; }
            
            if(showProgress && _printProgress != 0)
            {              
              S_TRACE2("  PctValue: ", pctValue);
              short pct = map(pctValue, MOTOR_RIGHT_POS, totalCount, 0, 100);         
              _printProgress->write(pct);
              pctValue += abs(rotate);
            }
           
            _servo.write(pos);

            if(MOTOR_STEP_BACK_MODE)
            {
              short stepBackPos = pos - rotateBack;
              if(stepBackPos >= MOTOR_RIGHT_POS + MOTOR_STEP_BACK_VALUE && stepBackPos <= MOTOR_LEFT_POS - MOTOR_STEP_BACK_VALUE)
              {
                cancelButton.loop();
                if(cancelButton.isPressed())
                { S_TRACE("Cancel!"); cancel = true; break; }

                delay(MOTOR_ROTATE_DELAY);
                _servo.write(stepBackPos);
              }
            }             

            cancelButton.loop();
            if(cancelButton.isPressed())
            { S_TRACE("Cancel!"); cancel = true; break; }       

            pos += rotate;
            delay(MOTOR_ROTATE_DELAY);

            wdt_reset();
          }               
        }
        res = true;
      }

      if(_servo.readMicroseconds() == MOTOR_MAX_US - MOTOR_STEP_BACK_VALUE * usFactor)
        _servo.writeMicroseconds(MOTOR_MAX_US);

      if(_servo.readMicroseconds() == MOTOR_MIN_US + MOTOR_STEP_BACK_VALUE * usFactor)
        _servo.writeMicroseconds(MOTOR_MIN_US);

      S_INFO2("Servo read: ", _servo.readMicroseconds());
      Detach();
      return res;
    }

    const bool Attach()
    {
      if(!_servo.attached())
      {
        if(_servo.attach(_pin, MOTOR_MIN_US, MOTOR_MAX_US) != INVALID_SERVO)
        {
          S_TRACE2("Attached: ", _pin);
          return true;
        }
        S_TRACE2("INVALID SERVO: ", _pin);
        return false;
      }
      S_TRACE3("Attached: ", _pin, " already");
      return true;
    }

    const bool Detach()
    {
      if(_servo.attached())
      {
        _servo.detach();
        S_TRACE2("Detached: ", _pin);
        return true;
      }
      return false;
    }
  };
}

#endif //FEED_MOTOR_H