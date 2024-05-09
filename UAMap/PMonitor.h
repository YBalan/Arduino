#pragma once
#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#ifdef USE_POWER_MONITOR

  #define PIN_PMONITOR_SCL 12
  #define PIN_PMONITOR_SDA 14

  #define PM_NOT_AVALIABLE_MSG F("PM Not avaliable")
  #define PM_NOT_USED_MSG F("PM Not Used")
  #define PM_AVALIABLE_MSG F("PM Avaliable")

  #include "DEBUGHelper.h"

  #ifdef ENABLE_INFO_PMONITOR
  #define PM_INFO(...) SS_TRACE("[PM INFO] ", __VA_ARGS__)
  #else
  #define PM_INFO(...) {}
  #endif

  #ifdef ENABLE_TRACE_PMONITOR
  #define PM_TRACE(...) SS_TRACE("[PM TRACE] ", __VA_ARGS__)
  #else
  #define PM_TRACE(...) {}
  #endif

#include <Wire.h>

#endif

namespace PMonitor
{
  #ifdef USE_POWER_MONITOR
    #include  <GyverINA.h>
    INA226 ina;
    static bool IsPMAvaliable = false;
    void (*PMVoltageCallBack)(const float value) = nullptr;
    #ifdef DEBUG
    float fakeVoltage = 14.6;
    #endif
  #endif

  bool Init()
  {    
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return false;
    #else
      IsPMAvaliable = false;
      if(ina.begin(PIN_PMONITOR_SDA, PIN_PMONITOR_SCL))
      {
        PM_INFO(PM_AVALIABLE_MSG);

        PM_INFO(F("\tINA Calibration value: "), ina.getCalibration());

        ina.setSampleTime(INA226_VBUS, INA226_CONV_2116US);   // Повысим время выборки напряжения вдвое
        ina.setSampleTime(INA226_VSHUNT, INA226_CONV_8244US); // Повысим время выборки тока в 8 раз
        
        ina.setAveraging(INA226_AVG_X4); // Включим встроенное 4х кратное усреднение, по умолчанию усреднения нет 

        IsPMAvaliable = true;
        return IsPMAvaliable;
      }
      PM_INFO(PM_NOT_AVALIABLE_MSG);
      return IsPMAvaliable;
    #endif
  }

  const float GetVoltage(const bool &sleep = false)
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return 0.0;
    #else  
      if(IsPMAvaliable)
      {
        ina.sleep(false);      
        auto result = ina.getVoltage();
        if(sleep) ina.sleep(true);
        return result;
      }      
      PM_INFO(PM_NOT_AVALIABLE_MSG);
      #ifdef DEBUG
      return fakeVoltage -= PM_MENU_ALARM_DECREMENT;
      #endif
      return 0.0;    
    #endif
  }

};

#endif //POWER_MONITOR_H