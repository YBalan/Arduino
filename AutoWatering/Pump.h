#ifndef Pump_h
#define Pump_h

#include "Shares.h"

struct Pump
{
  const short SensorMaxValue = 1023;
  const unsigned long AdditionalTimeout = 2000; //TODO: define watchdog additional time
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
  const bool Initialize() const
  {
    pinMode(_pumpPin, INPUT_PULLUP);
    pinMode(_pumpPin, OUTPUT);
    return _pumpPin > 1;
  }  

  const PumpState getState() const { return _pumpState; }
  void setState(const PumpState &state) { _pumpState = state; }

  const bool Start(){ return Start(PumpState::ON); }

  const bool Start(const PumpState &state)
  {
    _startTiks = millis();
    _pumpState = state;
    _sensorValueStart = analogRead(_sensorPin);
    digitalWrite(_pumpPin, ON);
    return true;
  }

  const bool End(){ return End(PumpState::OFF); }

  const bool End(const PumpState &state)
  {    
    _pumpState = state;

    if(state == CALIBRATING)
    {
      auto time = millis() - _startTiks; 
      Settings.WatchDog = time + AdditionalTimeout; //TODO: define watchdog additional time

      _sensorValueEnd = analogRead(_sensorPin);
      Settings.WateringRequired = _sensorValueStart;
      Settings.WateringEnough = _sensorValueEnd;
    }

    _startTiks = 0;
    digitalWrite(_pumpPin, OFF);
    return state == CALIBRATING;
  }

  const short GetSensorValue() const
  {
    return analogRead(_sensorPin);
  }

  const bool IsWatchDogTriggered() const
  {
    if(_pumpState == CALIBRATING || _pumpState == MANUAL_ON || _startTiks == 0){ return false; }
    return (millis() - _startTiks) >= Settings.WatchDog;
  }

  const bool IsWateringRequired()
  {
    //if(_pumpState == CALIBRATING || _pumpState == MANUAL_ON || !isSensorUsed()){ return false; }
    
    if(isSensorUsed() && (_pumpState == OFF || _pumpState == MANUAL_OFF))
    {
      _sensorValue = analogRead(_sensorPin);    
    
      return _sensorValue > Settings.WateringRequired && _sensorValue < SensorMaxValue;
    }
    return false;
  }

  const bool IsWateringEnough()
  {
    //if(_pumpState == CALIBRATING || _pumpState == MANUAL_ON || !isSensorUsed()){ return false; }

    if(isSensorUsed() && (_pumpState == ON))
    {
      _sensorValue = analogRead(_sensorPin);
    
      return _sensorValue <= Settings.WateringEnough && _sensorValue > 0;
    }
  }

  const bool isSensorUsed() const
  {
    return Settings.WateringEnough < Settings.WateringRequired;
  }
  
  void PrintStatus(const bool & showDebugInfo) const
  {   
    auto ticks = millis();
    if(showDebugInfo)
    {      
      char buff[200];
      sprintf(buff, "Pump:[%d] State:[%s] Sensor:[%d (r:>%d to e:<=%d)] (start:%lu, ticks:%lu sub:%lu) timeout:{%lu}", _place, GetStatus(_pumpState), _sensorValue, Settings.WateringRequired, Settings.WateringEnough, _startTiks, ticks, (ticks - _startTiks), Settings.WatchDog);
      Serial.print(buff);
    }
    else
    {
      char buff[100];
      sprintf(buff, "Pump:[%d] State:[%s] Sensor:[%d (r:>%d to e:<=%d)]", _place, GetStatus(_pumpState), _sensorValue, Settings.WateringRequired, Settings.WateringEnough);    
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
        case MANUAL_ON:
          return "MANUAL ON";
        case MANUAL_OFF:
          return "MANUAL OFF";
        default:
          return "UNKNOWN";
     }
  }  
};

#endif