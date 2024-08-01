#pragma once
#ifndef ESP_SERVO_H
#define ESP_SERVO_H

#ifdef ESP32
#include <ESP32Servo.h>
#else //EDP8266
#include <ESP8266_ISR_Servo.h>
#endif

#include "DEBUGHelper.h"
#ifdef ENABLE_TRACE_SERVO
#define SERVO_TRACE(...) SS_TRACE(F("[SERVO TRACE] "), __VA_ARGS__)
#else
#define SERVO_TRACE(...) {}
#endif

class ESPServo
{
  private:
    Servo _servo;
    uint8_t _pin;

  public:
    ESPServo(const uint8_t pin) : _pin(pin), _servo() { }
    const bool attach()
    {
      if(!_servo.attached())
      {
        //if(_servo.attach(_pin, MOTOR_MIN_US, MOTOR_MAX_US) != INVALID_SERVO)
        const auto &attachRes = _servo.attach(_pin);
        if(attachRes == 0)
        {
          SERVO_TRACE(F("Attached: "), _pin);
          return true;
        }
        SERVO_TRACE(F("INVALID: "), attachRes, F(" On: "), _pin);
        return false;
      }
      SERVO_TRACE(F("Attached: "), _pin, F(" already"));
      return true;
    }
    const bool detach()
    {
      if(_servo.attached())
      {
        _servo.detach();
        SERVO_TRACE(F("Detached: "), _pin);
        return true;
      }
      return false;
    }
    const bool attached()
    {
      return _servo.attached();
    }

    const int read()
    {
      return _servo.read();
    }

    const int move(const int &step, const int &delayValue = 0)
    {      
      const auto &current = read();        
      int newPos = (current > 180 ? abs(step) : current + step);
      SERVO_TRACE(current, current < newPos ? F(" -> ") : F(" <- "), newPos);
      _servo.write(newPos);
      if(delayValue > 0)
        delay(delayValue);
      return delayValue == 0 ? newPos : read();
    } 

    const int pos(const uint8_t &newPos, const int &delayValue = 0)
    {
      const auto &current = read();      
      SERVO_TRACE(current, current < newPos ? F(" -> ") : F(" <- "), newPos);
      _servo.write(newPos);
      if(delayValue > 0)
        delay(delayValue);
      return delayValue == 0 ? newPos : read();
    }

    const int init()
    {
      _servo.write(read());
      return read();
    }
};

#endif //ESP_SERVO_H