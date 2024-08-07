#pragma once
#ifndef LED_STATE_H
#define LED_STATE_H

#include "pixeltypes.h"
#include "Settings.h"

struct LedState
{
  int8_t Idx = -1;
  int16_t BlinkPeriod;
  int32_t BlinkTotalTime;
  bool FixedBrightnessIfAlarmed;
  bool IsAlarmed;
  bool IsPartialAlarmed;
  CRGB Color;
  uint32_t PeriodTicks = 0;
  uint32_t TotalTimeTicks = 0;  

  LedState() { }
  LedState(const CRGB &color) : Color(color) { }  

  void StopBlink()
  {
    BlinkPeriod = 0;
    BlinkTotalTime = 0;
    PeriodTicks = 0;
    TotalTimeTicks = 0;
  }

  void StartBlink(const int16_t &period, const int32_t &totalTime = -1)
  {
    BlinkPeriod = period;
    BlinkTotalTime = totalTime;
    PeriodTicks = 0;
    TotalTimeTicks = 0;
  }

  const uint8_t setBrightness(uint8_t brDown)
  {    
    CHSV _targetHSV = rgb2hsv_approximate(Color); // to use the current color of leds[0]      
    
    _targetHSV.v = brDown;  // set the brightness 0 -> 255 , the limit is 255

    Color = _targetHSV;
    return _targetHSV.v;
  }

  void SetColors(const UAMap::Settings &settings)
  {
    Color = IsAlarmed ? (IsPartialAlarmed ? settings.PartialAlarmedColor : settings.AlarmedColor) : settings.NotAlarmedColor;
  }
};

#endif //LED_STATE_H