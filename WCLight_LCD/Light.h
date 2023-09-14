#ifndef Light_h
#define Light_h

#include "../../Shares/TimeHelper.h"
#include "Statistic.h"

#define OFF HIGH
#define ON LOW

#define IGNORE_LESS_THEN_SEC 20

class Light
{
public:
  struct Settings
  {
      int State = OFF;    
      int Counter = 0;    
      int Count = 0;
    
      unsigned long LastTime = 0;    
      unsigned long TotalTime = 0;    

    public:
      void resetState()
      {
        State = OFF;      
        Counter = 0;      
      }

      void resetTime()
      {
        Count = 0;      
        LastTime = 0;      
        TotalTime = 0;      
      }

      void setToOff()
      {
        Counter = 3;
      }

  } settings;

  Statistic statistic;

private:
  unsigned long _startTicks = 0;
  char _lastTimeStatus[10] = "NaN";
  char _totalTimeStatus[10] = "NaN";

public:

  const char * const GetLastTime(unsigned long &current)
  {
      return _startTicks > 0 && current > 0 ? HumanizeShortTime((current - _startTicks) / 1000, _lastTimeStatus) : _lastTimeStatus;
  }

  const char * const GetTotalTime() const
  {
      return _totalTimeStatus;
  }

  const unsigned long &GetStartTicks() const
  {
    return _startTicks;
  }

  const char * const GetStatus() const
  {
    return settings.State == ON ? "ON" : "OFF";
  }  

  void resetState() { settings.resetState(); _startTicks = 0; }
  void resetTime() { settings.resetTime(); }
  void resetStatistic() { statistic.reset(); }
  void setToOff() { settings.setToOff(); }

  void CalculateTimeAndStatistics(){CalculateTimeAndStatistics(false);}
  void CalculateTimeAndStatistics(const bool &fromStart)
  {
    if(settings.State == ON)
    {
      _startTicks = millis();
      settings.Count += 1;
    }
    if(settings.State == OFF)
    {  
      if(!fromStart)
      {  
        unsigned long lastTime = _startTicks > 0 ? (millis() - _startTicks) / 1000 : settings.LastTime;
        
        if(lastTime > IGNORE_LESS_THEN_SEC)
        { 
          statistic.Max = lastTime > statistic.Max ? lastTime : statistic.Max;
          statistic.Min = lastTime < statistic.Min || statistic.Min == 0 ? lastTime : statistic.Min;
          statistic.Calc(lastTime);        
          settings.LastTime = lastTime;
        }
      
        settings.TotalTime += lastTime;
      }
      _startTicks = 0;     
    }
    HumanizeShortTime(settings.LastTime, _lastTimeStatus);
    HumanizeTotalTime(settings.TotalTime, _totalTimeStatus);
  }

  const bool Pressed()
  {
    bool switched = false;
    if(settings.Counter == 0)
    {
      settings.State = settings.State == OFF ? ON : OFF;
      
      settings.Counter = 0;

      CalculateTimeAndStatistics();

      switched = true;
    } 
    else
    {
      settings.Counter++;
    }
    return switched;
  }

  const bool Released()
  {
    bool switched = false;

    settings.Counter++;

    if(settings.Counter >= 3 || settings.Counter == 0)
    {
      settings.State = settings.State == OFF ? ON : OFF;
      
      settings.Counter = 0;

      CalculateTimeAndStatistics();

      switched = true;
    } 
    return switched;  
  }

public:
  static const char * const HumanizeShortTime(const unsigned long &timeSec, char *buff){return Helpers::Time::HumanizeTime(timeSec, buff, true);}
  static const char * const HumanizeTotalTime(const unsigned long &timeSec, char *buff){return Helpers::Time::HumanizeTime(timeSec, buff, false);} 
  
};
#endif