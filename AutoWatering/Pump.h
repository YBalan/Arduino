#ifndef Pump_h
#define Pump_h

#include "PumpState.h"

const bool DoNotUseEnoughLevel = true;

const short DefaultWatchDogForPumpInSecs = 10;
const int DefaultWatchDogForPump = 10;//1000 * 2 * DefaultWatchDogForPumpInSecs;

const short DefaultWateringRequiredLevel = 800;
const short DefaultWateringEnoughLevel = DoNotUseEnoughLevel ? 300 : 300;

const unsigned long AdditionalTimeout = DoNotUseEnoughLevel ? 0 : 2000; //TODO: define watchdog additional time

const unsigned long ULONG_MAX = 0UL - 1UL;
const short SensorMaxValue = 1023;

const short DefaultSensorNotChangedCount = 3;
const short SensorChangesLevel = 10;

struct Pump
{  
public:
  struct PumpSettings
  {
    unsigned long WatchDog = 0;
    int Count = 0;
    short WateringRequired = -1;
    short WateringEnough = -1;

    short PumpState = OFF;
    short SensorNotChangedCount = 0;

    void resetSettings()
    {
      WatchDog = DefaultWatchDogForPump;
      Count = 0;
      WateringRequired = DefaultWateringRequiredLevel;
      WateringEnough = DefaultWateringEnoughLevel;      
    }
    void resetState()
    {
      PumpState = OFF;
      SensorNotChangedCount = 0;
    }

    void reset()
    {
      resetSettings();
      resetState();
    }

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

  const PumpState getState() const { return Settings.PumpState; }
  const bool isOff() const { return Settings.PumpState == OFF || Settings.PumpState == MANUAL_OFF || Settings.PumpState == TIMEOUT_OFF || Settings.PumpState == CALIBRATING; }
  const bool isOn() const { return Settings.PumpState == ON || Settings.PumpState == MANUAL_ON; }
  const bool IsCalibratingRequired(){ return Settings.WatchDog == DefaultWatchDogForPump; }  

  const bool Start(){ return Start(PumpState::ON); }

  const bool Start(const PumpState &state)
  {
    if(Settings.PumpState == SENSOR_OFF && state == ON) { return false; }
    if(state == MANUAL_ON) { Settings.SensorNotChangedCount = 0; }

    _startTiks = millis();
    Settings.PumpState = state;
    Settings.Count += state == ON ? 1 : 0;
    _sensorValueStart = GetSensorValue();
    digitalWrite(_pumpPin, ON);
    return true;
  }

  const bool End(){ return End(PumpState::OFF); }

  const bool End(const PumpState &state)
  {    
    Settings.PumpState = state;

    _sensorValueEnd = GetSensorValue();

    if(state == CALIBRATING)
    {
      auto time = millis() - _startTiks; 
      Settings.WatchDog = time + AdditionalTimeout; //TODO: define watchdog additional time      
      Settings.WateringRequired = _sensorValueStart;
      Settings.WateringEnough = DoNotUseEnoughLevel ? DefaultWateringEnoughLevel : _sensorValueEnd;
    }

    Settings.PumpState = HandleSensorState(state);

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
    if(Settings.PumpState == MANUAL_ON || _startTiks == 0){ return false; }
    return (millis() - _startTiks) >= Settings.WatchDog;
  }

  const bool IsWateringRequired()
  {    
    _sensorValue = GetSensorValue();

    if(IsCalibratingRequired()) { return false; }
    if(Settings.PumpState == TIMEOUT_OFF && !DoNotUseEnoughLevel){ return false; }
    if(Settings.PumpState == MANUAL_ON) { return false; }

    if(isSensorUsed() && isOff())
    {
      return DoNotUseEnoughLevel 
        ? _sensorValue > Settings.WateringRequired && _sensorValue < SensorMaxValue
        : _sensorValue > Settings.WateringRequired && _sensorValue < SensorMaxValue;
    }
    return false;
  }

  const bool IsWateringEnough()
  {
    _sensorValue = GetSensorValue();

    if(IsCalibratingRequired()) { return false; }
    if(Settings.PumpState == MANUAL_ON) { return false; }

    if(isSensorUsed() && isOn())
    {
      return 
      _sensorValue == SensorMaxValue ||
      _sensorValue <= Settings.WateringEnough && _sensorValue > 0;
    }

    return false;
  }

  const bool isSensorUsed() const
  {
    return DoNotUseEnoughLevel 
        ? Settings.WateringRequired > 0 && Settings.WateringRequired <= SensorMaxValue
        : Settings.WateringEnough < Settings.WateringRequired;
  }

  void Reset()
  {
    Settings.reset();    
  }

  void ResetState()
  {
    Settings.resetState();
  }
  
public:  
  void PrintStatus(const bool & showDebugInfo) const
  {   
    auto ticks = millis();
    if(showDebugInfo)
    {      
      char buff[200];
      sprintf(buff, "Pump:[%d] Sensor:[%d (r:>%d to e:<=%d {used:%d}) Alarm:%d] State:[%s] {Count:%d} (start:%lu, ticks:%lu sub:%lu) timeout:{%lu}", _place, _sensorValue, Settings.WateringRequired, Settings.WateringEnough, isSensorUsed(), Settings.SensorNotChangedCount, GetStatus(Settings.PumpState), Settings.Count, _startTiks, ticks, (ticks - _startTiks), Settings.WatchDog);
      Serial.print(buff);
    }
    else
    {
      char buff[100];
      sprintf(buff, "Pump:[%d] Sensor:[%d (r:>%d to e:<=%d {used:%d}) Alarm:%d] State:[%s] {Count:%d} {timeout:%lu}", _place, _sensorValue, Settings.WateringRequired, Settings.WateringEnough, isSensorUsed(), Settings.SensorNotChangedCount, GetStatus(Settings.PumpState), Settings.Count, Settings.WatchDog);    
      Serial.print(buff);
    }
    Serial.println();
  }

private:
  const PumpState HandleSensorState(const PumpState &state)
  {
    if(state == TIMEOUT_OFF || state == OFF)
    {
      if(abs(_sensorValueStart - _sensorValueEnd) <= SensorChangesLevel)
      {
        Settings.SensorNotChangedCount++;
      }
      else
      {
        Settings.SensorNotChangedCount = 0;
      }

      if(Settings.SensorNotChangedCount >= DefaultSensorNotChangedCount)
      {        
        return SENSOR_OFF;
      }
    }
    
    return state;
  }
};

#endif