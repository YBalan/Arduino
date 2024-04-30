#pragma once
#ifndef BUZZ_HELPER_H
#define BUZZ_HELPER_H

#include "DEBUGHelper.h"
#ifdef ENABLE_INFO_BUZZ
#define BUZZ_INFO(...) SS_TRACE("[BUZZ INFO] ", __VA_ARGS__)
#else
#define BUZZ_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_BUZZ
#define BUZZ_TRACE(...) SS_TRACE("[BUZZ TRACE] ", __VA_ARGS__)
#else
#define BUZZ_TRACE(...) {}
#endif

#define DEFAULT_SIREN_PERIOD_DURATION_MS 325
#define DEFAULT_SIREN_UP_FRQ 800
#define DEFAULT_SIREN_DOWN_FRQ 500

namespace Buzz
{
  void Siren(const uint8_t &pin
    , const uint16_t &totalTime = 3000
    , const uint16_t &up = DEFAULT_SIREN_UP_FRQ
    , const uint16_t &down = DEFAULT_SIREN_DOWN_FRQ
    , const uint16_t &pause = 0
    , const uint16_t &defultPeriod = DEFAULT_SIREN_PERIOD_DURATION_MS)
  {
    uint16_t repeat = (float)totalTime / (float)((defultPeriod * 2) + (pause * 2));
    for(uint16_t count = 0; count <= repeat; count++)
    {
      tone(pin, up, defultPeriod); 
      
      delay(defultPeriod + pause);
      
      tone(pin, down, defultPeriod);   
      
      delay(defultPeriod + pause);
    }
    
    noTone(pin);
  }

  void AlarmStart(const uint8_t &pin, const int16_t &totalTime = 3000)
  {
    BUZZ_INFO(F("AlarmStart"));
    if(totalTime > 0)
    {      
      BUZZ_INFO(F("Time: "), totalTime);
      Siren(pin, totalTime, /*up:*/500, /*down:*/500, /*pause:*/500, /*period:*/1000);
    }
  }

  void AlarmEnd(const uint8_t &pin, const int16_t &totalTime = 3000)
  {    
    BUZZ_INFO(F("AlarmEnd"));
    if(totalTime > 0)
    {
      BUZZ_INFO(F("Time: "), totalTime);
      Siren(pin, totalTime, /*up:*/800, /*down:*/800, /*pause:*/500, /*period:*/1000);
    }
  }
};

#endif //BUZZ_HELPER_H
