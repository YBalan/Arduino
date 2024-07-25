#ifndef Pump_h
#define Pump_h

#include "PumpState.h"
#include <String.h>

#define NO_WATER_CODE               F("NW")
#define TIMEOUT_CODE                F("T")
#define CALIBRATING_REQUIRED_CODE   F("CL")
#define AERATING_CODE               F("A")

#define SENSOR_LOST_CODE            F("L")
#define SENSOR_NOT_USED_CODE        F("NS")
#define SENSOR_MAX_VALUE_CODE       F("D")
#define SENSOR_MIN_VALUE_CODE       F("W")

#define NO_WATER_LCODE               F("NoW")
#define TIMEOUT_LCODE                F("ToT")
#define CALIBRATING_REQUIRED_LCODE   F("Cal")
#define AERATING_LCODE               F("Aer")

#define SENSOR_LOST_LCODE            F("LOST")
#define SENSOR_NOT_USED_LCODE        F("NoS")
#define SENSOR_MAX_VALUE_LCODE       F("Dry")
#define SENSOR_MIN_VALUE_LCODE       F("Wet")


#define DO_NOT_USE_ENOUGH_LOW_LEVEL true

#define DEFAULT_PUMP_TIMEOUT_SEC 10
#define DEFAULT_PUMP_TIMEOUT 10//1000 * 2 * DEFAULT_PUMP_TIMEOUT_SEC;

#define DEFAULT_WATERING_REQUIRED_LEVEL 800
#define DEFAULT_WATERING_ENOUGH_LEVEL (DO_NOT_USE_ENOUGH_LOW_LEVEL ? 300 : 300)

#define ADDITIONAL_WATCHDOG_TIME (DO_NOT_USE_ENOUGH_LOW_LEVEL ? 0 : 2000) //TODO: define watchdog additional time

#define ULONG_MAX (0UL - 1UL)
#define SENSOR_VALUE_MAX 1023
#define SENSOR_VALUE_MIN 10

#define SENSOR_DOES_NOT_CHANGED_ATTEMPTS 3
#define SENSOR_CHANGES_LEVEL 10

#ifdef DEBUG
#define AERATION_TIMEOUT 2000
#else
#define AERATION_TIMEOUT 60000
#endif

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

    void resetState(const ::PumpState &state)
    {
      PumpState = state;
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
  const bool isOff() const { return Settings.PumpState == OFF || Settings.PumpState == MANUAL_OFF || Settings.PumpState == TIMEOUT_OFF || Settings.PumpState == CALIBRATING || Settings.PumpState == SENSOR_OFF || Settings.PumpState == AERATION_OFF; }
  const bool isOn() const { return Settings.PumpState == ON || Settings.PumpState == MANUAL_ON || Settings.PumpState == AERATION_ON; }
  const bool IsCalibratingRequired(){ return Settings.WatchDog == DEFAULT_PUMP_TIMEOUT; }  
  const unsigned long getTicks() const {return _startTiks;}

  const bool Start(){ return Start(PumpState::ON); }

  const bool Start(const PumpState &state)
  {
    if(Settings.PumpState == SENSOR_OFF && state == ON) { return false; }
    auto newState = state;
    if(state == MANUAL_ON) 
    {
      if(Settings.WatchDog >= AERATION_TIMEOUT) 
      {
        newState = AERATION_ON; 
      }
      Settings.SensorNotChangedCount = 0; 
    }

    _startTiks = millis();
    Settings.PumpState = newState;
    Settings.Count += (newState == ON || newState == AERATION_ON) ? 1 : 0;
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

    if(state == AERATION_OFF)
    {
       _startTiks = millis();
      Settings.PumpState = AERATION_OFF;
    }    
    else
    {
      _startTiks = 0; 
      Settings.PumpState = HandleSensorState(state);           
    }
    digitalWrite(_pumpPin, OFF);
    return state == CALIBRATING;
  }  

  const short GetSensorValue() const
  {
    return analogRead(_sensorPin);
  }

  const bool IsWatchDogTriggered(const unsigned long &currentTicks) const
  {
    if(Settings.PumpState == AERATION_ON || Settings.PumpState == AERATION_OFF) { return false; }
    if(Settings.PumpState == MANUAL_ON || _startTiks == 0){ return false; }
    return (currentTicks - _startTiks) >= Settings.WatchDog;
  }

  const bool CheckAerationStatus(const unsigned long &currentTicks)
  { 
    if(Settings.PumpState != AERATION_ON && Settings.PumpState != AERATION_OFF) { return false; }
    
    auto res = _startTiks > 0 && (currentTicks - _startTiks) >= Settings.WatchDog;
    //TRACE(F("CheckAerationStatus: "), _startTiks, F(" "), res ? F("True") : F("False"));
    return res;
  }

  const bool IsWateringRequired()
  {    
    _sensorValue = GetSensorValue();

    if(IsCalibratingRequired()) { return false; }
    if(Settings.PumpState == TIMEOUT_OFF) { return false; }
    if(Settings.PumpState == MANUAL_ON) { return false; } 
    if(Settings.PumpState == SENSOR_OFF) { return false; } 
    if(Settings.PumpState == AERATION_ON || Settings.PumpState == AERATION_OFF) { return false; }

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
    if(Settings.PumpState == AERATION_ON || Settings.PumpState == AERATION_OFF) { return false; }

    if(DO_NOT_USE_ENOUGH_LOW_LEVEL && isOn() && _sensorValue <= SENSOR_VALUE_MIN) { return true; }

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

  void ResetState(const PumpState &state)
  {
    Settings.resetState(state);
  }
  
public:  
  const String GetFullStatus(const bool &hasWater) const  { return GetStatus(hasWater, false); }
  const String GetShortStatus(const bool &hasWater) const  { return GetStatus(hasWater, true); }
  const String GetStatus(const bool &hasWater, const bool &shortStatus) const
  {
    String sensorValueBuff;
    String statusBuff; 
    sensorValueBuff = (shortStatus ? SENSOR_LOST_CODE : SENSOR_LOST_LCODE);
    statusBuff = (shortStatus ? TIMEOUT_CODE : TIMEOUT_LCODE);
    bool showCount = true;
    
    if(_sensorValue < SENSOR_VALUE_MAX && _sensorValue >= 0)
    {
      if(_sensorValue >= (SENSOR_VALUE_MAX - SENSOR_CHANGES_LEVEL))
      {
        sensorValueBuff = (shortStatus ? SENSOR_MAX_VALUE_CODE : SENSOR_MAX_VALUE_LCODE);
      }
      else if(_sensorValue <= (0 + SENSOR_CHANGES_LEVEL))
      {
        sensorValueBuff = (shortStatus ? SENSOR_MIN_VALUE_CODE : SENSOR_MIN_VALUE_LCODE);
      }
      else
      {
        sensorValueBuff = String(ToPct(_sensorValue, shortStatus));
      }
    }    

    if(Settings.PumpState != SENSOR_OFF && Settings.PumpState != TIMEOUT_OFF)  
    {
      if(Settings.WatchDog == DEFAULT_PUMP_TIMEOUT_SEC)
      {
        statusBuff = (shortStatus ? CALIBRATING_REQUIRED_CODE : CALIBRATING_REQUIRED_LCODE);
        showCount = false;
      }else
      if(Settings.WatchDog >= AERATION_TIMEOUT)
      {
        statusBuff = String(shortStatus ? AERATING_CODE : AERATING_LCODE) + String(isOn() ? F("On") : F("Off"));        
        showCount = false;
      }
      else
      {
        showCount = isSensorUsed();
        if(!hasWater)
        {
          statusBuff = (shortStatus ? NO_WATER_CODE : NO_WATER_LCODE);
        }
        else
        {            
          showCount 
            ? statusBuff = String(ToPct(Settings.WateringRequired, shortStatus)) 
            //: sprintf(statusBuff, shortStatus ? SENSOR_NOT_USED_CODE : SENSOR_NOT_USED_LCODE);
            : (statusBuff = shortStatus ? (isOn() ? F("On") : F("Off")) : SENSOR_NOT_USED_LCODE);          
        }
      }
    }
    
    showCount = showCount || !shortStatus;
    //sprintf(buff, showCount ? "%s/%s:%02d" : "%s/%s", sensorValueBuff, statusBuff, Settings.Count);      
    
    return sensorValueBuff + F("/") + statusBuff + (showCount ? (String(F(":")) + String(Settings.Count)) : String(F("")));
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