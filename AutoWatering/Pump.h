#ifndef Pump_h
#define Pump_h

#include "PumpState.h"

#define NO_WATER_CODE               "NW"
#define TIMEOUT_CODE                "T"
#define CALIBRATING_REQUIRED_CODE   "CL"

#define SENSOR_LOST_CODE            "L"
#define SENSOR_NOT_USED_CODE        "NS"
#define SENSOR_MAX_VALUE_CODE       "D"
#define SENSOR_MIN_VALUE_CODE       "W"

#define NO_WATER_LCODE               "NoW"
#define TIMEOUT_LCODE                "ToT"
#define CALIBRATING_REQUIRED_LCODE   "Cal"

#define SENSOR_LOST_LCODE            "LOST"
#define SENSOR_NOT_USED_LCODE        "NoS"
#define SENSOR_MAX_VALUE_LCODE       "Dry"
#define SENSOR_MIN_VALUE_LCODE       "Wet"


#define DO_NOT_USE_ENOUGH_LOW_LEVEL true

#define DEFAULT_PUMP_TIMEOUT_SEC 10
#define DEFAULT_PUMP_TIMEOUT 10//1000 * 2 * DEFAULT_PUMP_TIMEOUT_SEC;

#define DEFAULT_WATERING_REQUIRED_LEVEL 800
#define DEFAULT_WATERING_ENOUGH_LEVEL (DO_NOT_USE_ENOUGH_LOW_LEVEL ? 300 : 300)

#define ADDITIONAL_WATCHDOG_TIME (DO_NOT_USE_ENOUGH_LOW_LEVEL ? 0 : 2000) //TODO: define watchdog additional time

#define ULONG_MAX (0UL - 1UL)
#define SENSOR_VALUE_MAX 1023

#define SENSOR_DOES_NOT_CHANGED_ATTEMPTS 3
#define SENSOR_CHANGES_LEVEL 10

struct Pump
{  
public:
  struct PumpSettings
  {
    unsigned long WatchDog = 0;
    int Count = 0;
    short WateringRequired = -1;
    short WateringEnough = -1;

    unsigned short PumpState = OFF;
    unsigned short SensorNotChangedCount = 0;

    void resetSettings()
    {
      WatchDog = DEFAULT_PUMP_TIMEOUT;
      Count = 0;
      WateringRequired = DEFAULT_WATERING_REQUIRED_LEVEL;
      WateringEnough = DEFAULT_WATERING_ENOUGH_LEVEL;      
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
  const bool isOff() const { return Settings.PumpState == OFF || Settings.PumpState == MANUAL_OFF || Settings.PumpState == TIMEOUT_OFF || Settings.PumpState == CALIBRATING || Settings.PumpState == SENSOR_OFF; }
  const bool isOn() const { return Settings.PumpState == ON || Settings.PumpState == MANUAL_ON; }
  const bool IsCalibratingRequired(){ return Settings.WatchDog == DEFAULT_PUMP_TIMEOUT; }  
  const unsigned long &getTicks() const {return _startTiks;}

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
      Settings.WatchDog = time + ADDITIONAL_WATCHDOG_TIME; //TODO: define watchdog additional time      
      Settings.WateringRequired = _sensorValueStart;
      Settings.WateringEnough = DO_NOT_USE_ENOUGH_LOW_LEVEL ? DEFAULT_WATERING_ENOUGH_LEVEL : _sensorValueEnd;
      Settings.Count = 0;
      Settings.SensorNotChangedCount = 0;
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

  const bool IsWatchDogTriggered(const unsigned long &currentTicks) const
  {
    //Serial.println(_startTiks);
    if(Settings.PumpState == MANUAL_ON || _startTiks == 0){ return false; }
    return (currentTicks - _startTiks) >= Settings.WatchDog;
  }

  const bool IsWateringRequired()
  {    
    _sensorValue = GetSensorValue();

    if(IsCalibratingRequired()) { return false; }
    if(Settings.PumpState == TIMEOUT_OFF && !DO_NOT_USE_ENOUGH_LOW_LEVEL){ return false; }
    if(Settings.PumpState == MANUAL_ON) { return false; }    

    if(isSensorUsed() && isOff())
    {
      return DO_NOT_USE_ENOUGH_LOW_LEVEL 
        ? _sensorValue > Settings.WateringRequired && _sensorValue < SENSOR_VALUE_MAX
        : _sensorValue > Settings.WateringRequired && _sensorValue < SENSOR_VALUE_MAX;
    }

    if(!isSensorUsed() && isOff() && Settings.PumpState != TIMEOUT_OFF && Settings.PumpState != MANUAL_OFF)
    {
      return true;
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
      _sensorValue == SENSOR_VALUE_MAX ||
      _sensorValue <= Settings.WateringEnough && _sensorValue > 0;
    }

    return false;
  }

  const bool isSensorUsed() const
  {
    return DO_NOT_USE_ENOUGH_LOW_LEVEL 
        ? Settings.WateringRequired > 0 && Settings.WateringRequired < SENSOR_VALUE_MAX && Settings.WateringEnough < Settings.WateringRequired
        : Settings.WateringEnough < Settings.WateringRequired;
  }

  void Reset()
  {
    Settings.reset();
    Settings.resetState();
  }

  void ResetState()
  {
    Settings.resetState();
  }
  
public:  
  const char * const PrintStatus(const bool &showDebugInfo, const unsigned long &currentTicks, char *buff) const
  { 
    const auto &ticks = currentTicks == 0 ? millis() : currentTicks;
    if(showDebugInfo)
    { 
      sprintf(buff, "Pump:[%d] Sensor:[%d (r:>%d to e:<=%d {used:%d}) Alarm:%d] State:[%s] {Count:%d} (start:%lu, ticks:%lu sub:%lu) timeout:{%lu}", _place, _sensorValue, Settings.WateringRequired, Settings.WateringEnough, isSensorUsed(), Settings.SensorNotChangedCount, GetState(Settings.PumpState), Settings.Count, _startTiks, ticks, (ticks - _startTiks), Settings.WatchDog);      
    }
    else
    {      
      sprintf(buff, "Pump:[%d] Sensor:[%d (r:>%d to e:<=%d {used:%d}) Alarm:%d] State:[%s] {Count:%d} {timeout:%lu}", _place, _sensorValue, Settings.WateringRequired, Settings.WateringEnough, isSensorUsed(), Settings.SensorNotChangedCount, GetState(Settings.PumpState), Settings.Count, Settings.WatchDog);      
    }
    return buff;
  }

  const char * const GetFullStatus(const bool &hasWater, char *buff) const  { return GetStatus(hasWater, false, buff); }
  const char * const GetShortStatus(const bool &hasWater, char *buff) const  { return GetStatus(hasWater, true, buff); }
  const char * const GetStatus(const bool &hasWater, const bool &shortStatus, char *buff) const
  {
    char sensorValueBuff[6];
    char statusBuff[6]; 
    strcpy(sensorValueBuff, shortStatus ? SENSOR_LOST_CODE : SENSOR_LOST_LCODE);
    strcpy(statusBuff, shortStatus ? TIMEOUT_CODE : TIMEOUT_LCODE);
    bool showCount = true;
    
    if(_sensorValue < SENSOR_VALUE_MAX && _sensorValue >= 0)
    {
      if(_sensorValue >= (SENSOR_VALUE_MAX - SENSOR_CHANGES_LEVEL))
      {
        sprintf(sensorValueBuff, shortStatus ? SENSOR_MAX_VALUE_CODE : SENSOR_MAX_VALUE_LCODE);
      }
      else if(_sensorValue <= (0 + SENSOR_CHANGES_LEVEL))
      {
        sprintf(sensorValueBuff, shortStatus ? SENSOR_MIN_VALUE_CODE : SENSOR_MIN_VALUE_LCODE);
      }
      else
      {
        sprintf(sensorValueBuff, "%d", ToPct(_sensorValue, shortStatus));
      }
    }    

    if(Settings.PumpState != SENSOR_OFF && Settings.PumpState != TIMEOUT_OFF)  
    {
      if(Settings.WatchDog == DEFAULT_PUMP_TIMEOUT_SEC)
      {
        sprintf(statusBuff, shortStatus ? CALIBRATING_REQUIRED_CODE : CALIBRATING_REQUIRED_LCODE);
        showCount = false;
      }
      else
      {
        showCount = isSensorUsed();
        if(!hasWater)
        {
          sprintf(statusBuff, shortStatus ? NO_WATER_CODE : NO_WATER_LCODE);
        }
        else
        {            
          showCount ? sprintf(statusBuff, "%d", ToPct(Settings.WateringRequired, shortStatus)) : sprintf(statusBuff, shortStatus ? SENSOR_NOT_USED_CODE : SENSOR_NOT_USED_LCODE);          
        }
      }
    }
    
    showCount = showCount || !shortStatus;
    sprintf(buff, showCount ? "%s/%s:%02d" : "%s/%s", sensorValueBuff, statusBuff, Settings.Count);      
    
    return buff;
  }

private:
  const PumpState HandleSensorState(const PumpState &state)
  {
    if(!isSensorUsed() && (state == CALIBRATING || state == MANUAL_OFF))
    {
      return TIMEOUT_OFF;
    }

    if(state == TIMEOUT_OFF || state == OFF)
    {
      if(abs(_sensorValueStart - _sensorValueEnd) <= SENSOR_CHANGES_LEVEL)
      {
        Settings.SensorNotChangedCount++;
      }
      else
      {
        Settings.SensorNotChangedCount = 0;
      }

      if(Settings.SensorNotChangedCount >= SENSOR_DOES_NOT_CHANGED_ATTEMPTS)
      {        
        return SENSOR_OFF;
      }
    }
    
    return state;
  }

public:
  static const short ToPct(const short &value, const bool &shortStatus)
  {
    //map(value, fromLow, fromHigh, toLow, toHigh)
    return map(value, 0, 1023, 0, shortStatus ? 10 : 100);
  }
};

#endif