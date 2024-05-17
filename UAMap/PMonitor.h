#pragma once
#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#ifdef USE_POWER_MONITOR

  #define PM_DEFAULT_ADJUST_FACTOR 0.945

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
  #include <INA226_WE.h>
  #include <math.h>
  #include "Settings.h"
  #define I2C_ADDRESS 0x40  

#endif

namespace PMonitor
{
  #ifdef USE_POWER_MONITOR
    //#include <Wire.h>
    //#include  <GyverINA.h>
    //INA226 ina;

    //https://wolles-elektronikkiste.de/en/ina226-current-and-power-sensor    
    INA226_WE ina226 = INA226_WE(I2C_ADDRESS);
    static bool IsPMAvaliable = false;
    struct PMSettings
    {
      float adjFactor = PM_DEFAULT_ADJUST_FACTOR;
      float relay1Alarm = 0.0;
      float relay2Alarm = 0.0;
      void reset(){ adjFactor = PM_DEFAULT_ADJUST_FACTOR; relay1Alarm = 0.0; relay2Alarm = 0.0; }
    } _pmSettings;    
    void (*PMVoltageCallBack)(const float value) = nullptr;
    #ifdef DEBUG
    float fakeVoltage = 14.6;
    #endif
  #endif

  const bool testConnection(void) 
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return false;
    #else
        Wire.beginTransmission(I2C_ADDRESS);
        return (bool)!Wire.endTransmission();
    #endif
  }

  const bool Init(const uint8_t &sda = 4, const uint8_t &scl = 5)
  {    
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return false;
    #else
      IsPMAvaliable = false;
      //if(ina.begin(sda, scl))
      ina226.init();
      #if defined(ESP32) || defined(ESP8266)
        if (sda || scl) Wire.begin(sda, scl);  // Инициализация для ESP
        else Wire.begin();
      #else
        Wire.begin();
      #endif
      if(testConnection())
      {
        PM_INFO(PM_AVALIABLE_MSG);       

        ina226.setMeasureMode(TRIGGERED); // choose mode and uncomment for change of default

        ina226.setAverage(AVERAGE_16); 

        ina226.setResistorRange(0.1, 1.3); // choose resistor 0.1 Ohm and gain range up to 1.3A
 
          /* If the current values delivered by the INA226 differ by a constant factor
            from values obtained with calibrated equipment you can define a correction factor.
            Correction factor = current delivered from calibrated equipment / current delivered by INA226*/
        
          //ina226.setCorrectionFactor(0.70);
        
          //Serial.println("INA226 Current Sensor Example Sketch - Continuous");
        
        //ina226.waitUntilConversionCompleted(); //if you comment this line the first data might be zero

        //PM_INFO(F("\tINA Calibration value: "), ina.getCalibration());

        //ina.setSampleTime(INA226_VBUS, INA226_CONV_2116US);   // Повысим время выборки напряжения вдвое
        //ina.setSampleTime(INA226_VSHUNT, INA226_CONV_8244US); // Повысим время выборки тока в 8 раз
        
        //ina.setAveraging(INA226_AVG_X4); // Включим встроенное 4х кратное усреднение, по умолчанию усреднения нет 

        // ina226.setCorrectionFactor(0.95);
        // ina226.waitUntilConversionCompleted(); //makes no sense - in triggered mode we wait anyway for completed conversion

        IsPMAvaliable = true;
        return IsPMAvaliable;
      }
      PM_INFO(PM_NOT_AVALIABLE_MSG, F(" SDA: "), sda, F(" "), F("SCL: "), scl);
      return IsPMAvaliable;
    #endif
  }

  const float GetValues()
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return 0.0;
    #else    
      float busVoltage_V = 0.0;      
      
      ina226.startSingleMeasurement();
      ina226.readAndClearFlags();
      busVoltage_V = ina226.getBusVoltage_V();
      PM_TRACE(F("Bus Voltage [V]: "), busVoltage_V);

      #ifdef DEBUG
      float shuntVoltage_mV = 0.0;
      float loadVoltage_V = 0.0;
      
      float current_mA = 0.0;
      float power_mW = 0.0; 

      shuntVoltage_mV = ina226.getShuntVoltage_mV();      
      current_mA = ina226.getCurrent_mA();
      power_mW = ina226.getBusPower();
      loadVoltage_V  = busVoltage_V + (shuntVoltage_mV/1000);        

      PM_TRACE(F("Shunt Voltage [mV]: "), shuntVoltage_mV);      
      PM_TRACE(F("Load Voltage [V]: "), loadVoltage_V);
      PM_TRACE(F("Current[mA]: "), current_mA);
      PM_TRACE(F("Bus Power [mW]: "), power_mW);
      if(!ina226.overflow){
        PM_TRACE(F("Values OK - no overflow"));
      }
      else{
        PM_TRACE(F("Overflow! Choose higher current range"));
      }
      #endif

      return busVoltage_V;
    #endif
  }

  const float GetVoltage(const bool &sleep = true)
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return 0.0;
    #else        
      if(IsPMAvaliable)
      {
        //ina.sleep(false);  
        //PM_INFO(F("\tINA Calibration value: "), ina.getCalibration());    
        //auto result = ina.getVoltage();
        //if(sleep) ina.sleep(true);
        //return result;
        #ifdef DEBUG
        GetValues();
        #endif        
        //ina226.powerUp();
        ina226.startSingleMeasurement();
        ina226.readAndClearFlags();
        auto result = ina226.getBusVoltage_V()  * _pmSettings.adjFactor;
        //if(sleep) ina226.powerDown();
        return result;
      }      
      PM_INFO(PM_NOT_AVALIABLE_MSG);
      #ifdef DEBUG
      Init();
      return fakeVoltage -= PM_MENU_ALARM_DECREMENT;
      #endif
      return NAN;    
    #endif
  }

  const float AdjCalibration(const float &adj)
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return 0.0;
    #else  
      if(IsPMAvaliable)
      {    
        // ina.adjCalibration(adj);    
        // const auto &value = ina.getCalibration();
        // PM_INFO(F("\tINA Calibration value: "), value);            
        //return value;
        //ina226.setCorrectionFactor(adj);
        _pmSettings.adjFactor = adj;
        return adj;
      }      
      PM_INFO(PM_NOT_AVALIABLE_MSG);
      #ifdef DEBUG
      Init();      
      #endif
      return _pmSettings.adjFactor;    
    #endif
  }

  const float GetCalibration(void)
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return 0.0;
    #else                    
      return _pmSettings.adjFactor;    
    #endif
  }

  const bool SaveSettings()
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return false;
    #else  
      String fileName = F("/pmconfig.bin");
      PM_INFO(F("Save Settings to: "), fileName);
      if(SaveFile(fileName.c_str(), (byte *)&_pmSettings, sizeof(_pmSettings)))
      {
        PM_INFO(F("Write to: "), fileName);
        return true;
      }
      PM_INFO(F("Failed to open: "), fileName);
      return false;
    #endif
  }

  const bool LoadSettings()
  {
    #ifndef USE_POWER_MONITOR
      //PM_INFO(PM_NOT_USED_MSG);
      return false;
    #else  
      String fileName = F("/pmconfig.bin");
      SETTINGS_INFO(F("Load Settings from: "), fileName);

      const bool &res = LoadFile(fileName.c_str(), (byte *)&_pmSettings, sizeof(_pmSettings));

      if(_pmSettings.adjFactor <= 0 || _pmSettings.adjFactor > 1)
      {
        _pmSettings.reset();
      }

      PM_INFO(F("adjFactor: "), _pmSettings.adjFactor);
      
      return res;
    #endif
  }

};

#endif //POWER_MONITOR_H