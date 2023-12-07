#pragma once
#ifndef FEED_SCHEDULER_H
#define FEED_SCHEDULER_H

namespace Feed
{
  enum ScheduleSet : uint8_t
  {
    NotSet = 0,

    DAILY_2,
    DAILY_3,
    DAILY_4,
    DAILY_6,    

    DAYNIGHT_8,
    DAYNIGHT_10,
    DAYNIGHT_12,
    DAYNIGHT_14,

    MAX,
  };

  static const uint8_t GetSchedulerSetCount(const ScheduleSet &set, bool &isDayNight)
  {    
    isDayNight = false;
    switch(set)
    {
      case DAILY_2:                          return 2;
      case DAILY_3:                          return 3;
      case DAILY_4:                          return 4;
      case DAILY_6:                          return 6;


      case DAYNIGHT_8:    isDayNight = true; return 8;
      case DAYNIGHT_10:   isDayNight = true; return 10;
      case DAYNIGHT_12:   isDayNight = true; return 12;
      case DAYNIGHT_14:   isDayNight = true; return 14;
      case ScheduleSet::NotSet:
      default:                               return 0;
    }
  }

  static const String GetSchedulerSetString(const ScheduleSet &set, const bool &shortView)
  {    
    bool isDayNight = false;
    const short count = GetSchedulerSetCount(set, isDayNight);    
    return set == ScheduleSet::NotSet ? "NotSet" : String(count) + (isDayNight ? "DayNight" : "Day"); 
  }

  struct Scheduler
  {
    ScheduleSet Set;
    FeedDateTime NextAlarm;

    public: 

    Scheduler() : Set(ScheduleSet::NotSet), NextAlarm() { }

    const String SetToString(const bool &shortView = false) const { return GetSchedulerSetString(Set, shortView); }

    const bool IsTimeToAlarm(const DateTime &current)
    {
      if(Set == ScheduleSet::NotSet) return false;   

      uint32_t nextAlarmDtValue = FeedDateTime::GetTotalValueWithoutSeconds(NextAlarm);
      uint32_t currentDtValue   = FeedDateTime::GetTotalValueWithoutSeconds(current);
           
      if(nextAlarmDtValue < currentDtValue)
      {
        S_TRACE4("Next Alarm < Current: ", nextAlarmDtValue, "Curr: ", currentDtValue);
        S_TRACE4("Next Alarm < Current: ", NextAlarm.timestamp(), "Curr: ", current.timestamp());     
        SetNextAlarm(current);
        return false;
      }

      if(nextAlarmDtValue == currentDtValue)
      {       
        S_TRACE4("Alarm: ", NextAlarm.timestamp(), "Curr: ", current.timestamp());        
        SetNextAlarm(current);
        return true;        
      }
      else
      {
        //S_TRACE4("Next: ", nextAlarmDtValue, "Curr: ", currentDtValue);
        //S_TRACE4("Next: ", NextAlarm.timestamp(), "Curr: ", current.timestamp());
      }
      return false;
    }

    void SetNextAlarm(const DateTime &current)
    {
      NextAlarm = GetNextTime(current);      
    }

    const FeedDateTime &GetNextAlarm() const
    {
      return NextAlarm;
    }

    void Reset()
    {
      Set = Feed::ScheduleSet::NotSet;
      NextAlarm = FeedDateTime();
    }

    private:
    const FeedDateTime GetNextTime(const DateTime &current) const
    {
      //return current + TimeSpan(0, 0, 4, 0);

      if(Set == ScheduleSet::NotSet) return current;
      bool isDayNight = false;
      const uint8_t count = GetSchedulerSetCount(Set, isDayNight);
      return GetNextDateTime(current, isDayNight ? 0 : 7, 23, count);      
    }      

    private:
    const FeedDateTime GetNextDateTime(const DateTime &current, const short &startHour, const short &endHour, const short &count) const
    {
      if(count == 0) return current;
      const auto currentDay = current.day();
      FeedDateTime nextTime = DateTime(current.year(), current.month(), currentDay);      
      float stepFloat = (float)(endHour - startHour) / count;
      const auto timeSpan = TimeSpan( (int32_t)( stepFloat * 3600L ));
      const auto step = TimeSpan(0, timeSpan.hours(), timeSpan.minutes() ,0);

      S_TRACE6("Step: ", stepFloat, " ", step.hours(), ":", step.minutes());
      //S_TRACE(step.totalseconds());
      
      for(nextTime = nextTime + TimeSpan(0, startHour, 0, 0); nextTime.hour() < endHour && nextTime.day() == currentDay; nextTime = nextTime + step)
      {
        if(nextTime > current)
        {
          break;
        }
      }

      if(nextTime.day() > currentDay || nextTime.hour() >= endHour)
      {
        //current.Date + one day
        const auto nextDayStart = DateTime(current.year(), current.month(), currentDay) + TimeSpan(86400L);
        return GetNextDateTime(nextDayStart, startHour, endHour, count);
      }

      S_TRACE2("Get Next Alarm: ", nextTime.timestamp());
      return nextTime;
    }
  };
}

#endif //FEED_SCHEDULER_H