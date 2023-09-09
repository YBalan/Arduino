#ifndef Light_h
#define Light_h

// #define WeekMillis 604800000lu
// #define DayMillis 86400000lu
// #define HoursMillis 3600000lu
// #define MinuteMillis 60000lu

#define WeekSecs 604800lu
#define DaySecs 86400lu
#define HoursSecs 3600lu
#define MinuteSecs 60lu

#define OFF HIGH
#define ON LOW

#define IGNORE_LESS_THEN_SEC 30

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

  } settings;

  struct Statistic
  {
    unsigned long Max = 0;
    unsigned long Avg = 0;

    void reset() {Max = 0; Avg = 0;}    
  } statistic;

private:
  unsigned long _startTicks = 0;
  char _lastTimeStatus[10] = "NaN";
  char _totalTimeStatus[10] = "NaN";

public:

  const char *GetLastTime(unsigned long &current)
  {
      return _startTicks > 0 && current > 0 ? HumanizeShortTime((current - _startTicks) / 1000, _lastTimeStatus) : _lastTimeStatus;
  }

  const char *GetTotalTime() const
  {
      return _totalTimeStatus;
  }

  const unsigned long &GetStartTicks() const
  {
    return _startTicks;
  }

  const char *GetStatus() const
  {
    return settings.State == ON ? "ON" : "OFF";
  }  

  void resetState() { settings.resetState(); _startTicks = 0; }
  void resetTime() { settings.resetTime(); }
  void resetStatistic() { statistic.reset(); }

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
      unsigned long lastTime = _startTicks > 0 ? (millis() - _startTicks) / 1000 : settings.LastTime;
      
      if(lastTime > IGNORE_LESS_THEN_SEC)
      {
        statistic.Max = lastTime > settings.LastTime ? lastTime : settings.LastTime;
        statistic.Avg = statistic.Avg > 0 ? ceil((statistic.Avg + lastTime) / 2) : lastTime;
        settings.LastTime = lastTime;
      }

      if(!fromStart)
      {
        settings.TotalTime += lastTime;
      }
      _startTicks = 0;     
    }
    HumanizeTotalTime(settings.LastTime, _lastTimeStatus);
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
  static const char * HumanizeShortTime(const unsigned long &timeSec, char *buff){return HumanizeTime(timeSec, buff, true);}
  static const char * HumanizeTotalTime(const unsigned long &timeSec, char *buff){return HumanizeTime(timeSec, buff, false);}
  static const char * HumanizeTime(const unsigned long &timeSec, char *buff, const bool &shortTime)
  {  
    char dbuff1[3];
    char dbuff2[3]; 
    if(!shortTime)
    {      
      {
        short weeks = (short)(timeSec / WeekSecs);
        if(weeks > 0)
        {          
          //Serial.print("weeks: "); Serial.println(weeks);
          unsigned long tmp = (unsigned long)weeks * WeekSecs;
          short days = (short)((timeSec - tmp) / DaySecs);
          sprintf(buff, "%sw:%sd", ToString(weeks, dbuff1), ToString(days, dbuff2));
          return buff;
        }
      } 
      {
        short days = (short)(timeSec / DaySecs);
        if(days > 0)
        {
          //Serial.print("days: "); Serial.println(days);
          unsigned long tmp = (unsigned long)days * DaySecs;
          short hours = (short)((timeSec - tmp) / HoursSecs);          
          sprintf(buff, "%sd:%sh", ToString(days, dbuff1), ToString(hours, dbuff2));
          return buff;
        }
      }
    }
    {
      short hours = (short)(timeSec / HoursSecs);
      if(hours > 0)
      {
        //Serial.print("hours: "); Serial.println(hours);
        unsigned long tmp = (unsigned long)hours * HoursSecs;
        short mins = (short)((timeSec - tmp) / MinuteSecs);        
        sprintf(buff, "%sh:%sm", ToString(hours, dbuff1), ToString(mins, dbuff2));
        return buff;
      }
    }
    {
      short mins = (timeSec / MinuteSecs);
      if(mins > 0)
      {
        //Serial.print("mins: "); Serial.print(mins); Serial.print(" time:"); Serial.println(time);
        unsigned long tmp = (unsigned long)mins * MinuteSecs;
        short secs = (short)(timeSec - tmp);
        sprintf(buff, "%dm:%ds", mins, secs);
        return buff;
      }
    }

    sprintf(buff, "%lus", timeSec);  
    return buff;  
  }
private:  
  static const char *ToString(const short &source, char *buff)
  {
    sprintf(buff, "%s%d", source < 10 ? "0" : "", source);
    return buff;
  }
};
#endif