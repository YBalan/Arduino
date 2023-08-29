#ifndef Pump_h
#define Pump_h

#include "Shares.h"

struct Pump
{
public:
  struct PumpSettings
  {
    unsigned long WatchDog = 0;
    short WateringRequired = -1;
    short WateringEnough = -1; 
  } Settings;

private:
  unsigned long _startTiks = 0;  
  short _place = -1;
  short _pumpPin = -1;
  short _sensorPin = -1;  

private:
  short _sensorValue = -1;
  short _sensorValueStart = -1;
  short _sensorValueEnd = -1;
  short _pumpState = OFF;

public:
  Pump(const short &place, const short &pumpPin, const short &sensorPin, const unsigned int &watchDog, const short &wateringRequired, const short &wateringEnough)
  {
    _place = place;
    _pumpPin = pumpPin;
    _sensorPin = sensorPin;
    Settings.WatchDog = watchDog;
    Settings.WateringRequired = wateringRequired;
    Settings.WateringEnough = wateringEnough;
  }
public:
  const bool Initialize()
  {
    pinMode(_pumpPin, INPUT_PULLUP);
    pinMode(_pumpPin, OUTPUT);
    return _pumpPin > 1;
  }  

  const PumpState getState() { return _pumpState; }
  void setState(PumpState state) { _pumpState = state; }

  const bool Start(){ Start(/*calibrate:*/false); }

  const bool Start(const bool & calibrate)
  {
    _startTiks = GetTicks();
    _pumpState = calibrate ? CALIBRATING : ON;
    _sensorValueStart = GetSensorValue();
    digitalWrite(_pumpPin, ON);
    return true;
  }

  const bool End(){ End(/*calibrate:*/false); }

  const bool End(const bool & calibrate)
  {
    _pumpState = calibrate ? CALIBRATING : OFF;

    if(calibrate)
    {
      auto time = GetTicks() - _startTiks; 
      Settings.WatchDog = time + 2000u; //TODO: define watch dog additional time

      _sensorValueEnd = GetSensorValue();
      Settings.WateringRequired = _sensorValueStart;
      Settings.WateringEnough = _sensorValueEnd;
    }

    _startTiks = 0;
    digitalWrite(_pumpPin, OFF);
    return calibrate;
  }

  const short GetSensorValue()
  {
    return _sensorValue = analogRead(_sensorPin);
  }

  const bool IsWatchDogTriggered()
  {
    if(_pumpState == CALIBRATING || _startTiks == 0){ return false; }
    return (GetTicks() - _startTiks) >= Settings.WatchDog;
  }

  const bool IsWateringRequired()
  {
    _sensorValue = GetSensorValue();
    
    return _sensorValue < Settings.WateringRequired && _sensorValue > Settings.WateringEnough;
  }

  const bool IsWateringEnough()
  {
    _sensorValue = GetSensorValue();
    
    return _sensorValue >= Settings.WateringEnough && _sensorValue < 1023;
  }
  
  void PrintStatus(const bool & showDebugInfo)
  {   
    auto ticks = GetTicks();
    if(showDebugInfo)
    {      
      char buff[200];
      sprintf(buff, "Pump:[%d]{pin:%d} State:[%s] Sensor:[%d (%d to %d)] (start:%lu, ticks:%lu [%lu]) wd:{%lu}", _place, _pumpPin, GetStatus(_pumpState), GetSensorValue(), Settings.WateringRequired, Settings.WateringEnough, _startTiks, ticks, (ticks - _startTiks), Settings.WatchDog);
      Serial.print(buff);
    }
    else
    {
      char buff[100];
      sprintf(buff, "Pump:[%d] State:[%s] Sensor:[%d (%d to %d)]", _place, GetStatus(_pumpState), GetSensorValue(), Settings.WateringRequired, Settings.WateringEnough);    
      Serial.print(buff);
    }
    Serial.println();
  }

public:
  static const unsigned long GetTicks(){ return millis(); } 
  static const char * const GetStatus(const short & state)
  {
     switch(state)
     {
        case ON:
          return "ON";
        case OFF:
          return "OFF";
        case CALIBRATING:
          return "CALIBRATING";
        default:
          return "UNKNOWN";
     }
  }  
};

#endif