#pragma once
#ifndef FEED_MOTOR_H
#define FEED_MOTOR_H

#include <Servo.h>
#include "ezButton.h"
#include "DEBUGHelper.h"


//#define MOTOR_DS3218MG_270

//#define MOTOR_MIN_US 500
//#define MOTOR_MAX_US 2500
//#define MOTOR_RIGHT_POS MOTOR_START_POS
#define MOTOR_MAX_POS  180

#define MOTOR_ROTATE_DELAY 100
#define MOTOR_ROTATE_VALUE 5
#define MOTOR_STEP_BACK_MODE true
#define MOTOR_STEP_BACK_VALUE 10

namespace Feed
{
  class FeedMotor
  {
    uint8_t _pin;
    Servo _servo;
    Print * const _printProgress = 0;
  public:
    FeedMotor(const uint8_t &pin) : _pin(pin)
    { }
    FeedMotor(const uint8_t &pin, const Print &print) : _pin(pin), _servo(), _printProgress(&print)
    { }

    const size_t &Init() const { return _printProgress != 0 ? _printProgress->write("", /*Helpers::LcdProgressCommands::Init*/4) : 0; }

    const bool DoFeed(unsigned short &currentPosition, const uint8_t &startPosition, const uint8_t &feedCount, const bool &showProgress, const ezButton &cancelButton)
    {
      if(!Attach()) return false;
      
      S_INFO2("Servo read: ", _servo.readMicroseconds());      

      bool res = false;

      if(showProgress && _printProgress != 0) _printProgress->write("", /*Helpers::LcdProgressCommands::Clear*/5);

      const uint8_t totalCount = (MOTOR_MAX_POS - startPosition) * feedCount;
      uint8_t pctValue = 0;
      bool cancel = false;

      currentPosition = currentPosition == MOTOR_MAX_POS ? MOTOR_MAX_POS : startPosition;

      for(uint8_t feed = 1; feed <= feedCount; feed++)
      {   
        cancelButton.loop();
        if(cancel || cancelButton.isPressed())
        { S_INFO("Cancel!"); break; }

        short pos = currentPosition;
        short endPos = pos >= MOTOR_MAX_POS ? startPosition : MOTOR_MAX_POS;
        const int8_t rotate = pos >= MOTOR_MAX_POS ? -MOTOR_ROTATE_VALUE : MOTOR_ROTATE_VALUE;
        const int8_t rotateBack = pos >= MOTOR_MAX_POS ? -MOTOR_STEP_BACK_VALUE : MOTOR_STEP_BACK_VALUE;

        S_INFO2("   Feed No: ", feed);
        S_INFO2("CurrentPos: ", pos);
        S_INFO2("    EndPos: ", endPos);
        S_INFO2("    Rotate: ", rotate);

        if(feed > 1) pos += rotate;

        if(pos >= startPosition && pos <= MOTOR_MAX_POS)
        {
          while (rotate > 0 ? pos <= endPos : pos >= endPos)
          {
            currentPosition = pos;
            S_INFO2("CurrentPos: ", currentPosition);

            cancelButton.loop();
            if(cancelButton.isPressed())
            { S_INFO("Cancel!"); cancel = true; break; }
            
            if(showProgress && _printProgress != 0)
            {              
              S_INFO2("  PctValue: ", pctValue);
              uint8_t pct = map(pctValue, 0, totalCount, 0, 100);         
              _printProgress->write(pct);
              pctValue += abs(rotate);
            }
           
            _servo.write(pos);

            if(MOTOR_STEP_BACK_MODE)
            {
              int16_t stepBackPos = pos - rotateBack;
              if(stepBackPos >= startPosition + MOTOR_STEP_BACK_VALUE && stepBackPos <= MOTOR_MAX_POS - MOTOR_STEP_BACK_VALUE)
              {
                cancelButton.loop();
                if(cancelButton.isPressed())
                { S_INFO("Cancel!"); cancel = true; break; }

                delay(MOTOR_ROTATE_DELAY);
                _servo.write(stepBackPos);
              }
            }             

            cancelButton.loop();
            if(cancelButton.isPressed())
            { S_INFO("Cancel!"); cancel = true; break; }       

            pos += rotate;
            delay(MOTOR_ROTATE_DELAY);

            wdt_reset();
          }               
        }
        res = true;
      }      

      S_INFO2("Servo read: ", _servo.readMicroseconds());
      Detach();
      return res;
    }

    const bool Attach()
    {
      if(!_servo.attached())
      {
        //if(_servo.attach(_pin, MOTOR_MIN_US, MOTOR_MAX_US) != INVALID_SERVO)
        if(_servo.attach(_pin) != INVALID_SERVO)
        {
          S_INFO2("Attached: ", _pin);
          return true;
        }
        S_INFO2("INVALID SERVO: ", _pin);
        return false;
      }
      S_INFO3("Attached: ", _pin, " already");
      return true;
    }

    const bool Detach()
    {
      if(_servo.attached())
      {
        _servo.detach();
        S_INFO2("Detached: ", _pin);
        return true;
      }
      return false;
    }
  };
}

#endif //FEED_MOTOR_H