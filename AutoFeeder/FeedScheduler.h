#pragma once
#ifndef FEED_SCHEDULER_H
#define FEED_SCHEDULER_H

#define FEEDS_SCHEDULER_SETTINGS_COUNT 7

namespace Feed
{
  enum ScheduleSet : short
  {
    NotSet = 0,

    DAILY_4,
    DAILY_6,
    DAILY_8,

    DAYNIGHT_10,
    DAYNIGHT_12,
    DAYNIGHT_14,
  };

  static const char *const GetSchedulerSetString(const ScheduleSet &set, const bool &shortView)
  {    
    switch(set)
    {
      case ScheduleSet::DAILY_4:      return "Day 4";
      case ScheduleSet::DAILY_6:      return "Day 6";
      case ScheduleSet::DAILY_8:      return "Day 8";

      case ScheduleSet::DAYNIGHT_10:  return "DayNght 10";
      case ScheduleSet::DAYNIGHT_12:  return "DayNght 12";
      case ScheduleSet::DAYNIGHT_14:  return "DayNght 14";
      case ScheduleSet::NotSet:
      default:                        return "NotSet";
    }
  }

  struct Scheduler
  {
    ScheduleSet Set;
    FeedDateTime NextAlarm;

    public: 

    Scheduler() : Set(NotSet), NextAlarm() { }

    const char *const SetToString(const bool &shortView = false) const { return GetSchedulerSetString(Set, shortView); }

    const bool IsTimeToAlarm(const DateTime &current)
    {
      if(Set == ScheduleSet::NotSet) return false;   

      volatile auto nextAlarmDtValue = FeedDateTime::GetTotalValueWithoutSeconds(NextAlarm);
      volatile auto currentDtValue   = FeedDateTime::GetTotalValueWithoutSeconds(current);

      //S_TRACE("Next Alarm < Current: ", nextAlarmDtValue, " Cur: ", currentDtValue);      
      //S_TRACE("Alarm: ", NextAlarm.timestamp(), " Cur: ", current.timestamp());
           
      if(nextAlarmDtValue < currentDtValue)
      {
        //S_TRACE("Next Alarm < Current: ", nextAlarmDtValue, " Cur: ", currentDtValue);        
        SetNextAlarm(current);
        return false;
      }

      if(nextAlarmDtValue == currentDtValue)
      {       
        S_TRACE4("Alarm: ", NextAlarm.timestamp(), " Cur: ", current.timestamp());        
        SetNextAlarm(current);
        return true;        
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
    const DateTime GetNextTime(const DateTime &current) const
    {
      //return current + TimeSpan(0, 0, 4, 0);
      switch(Set)
      {
        case ScheduleSet::NotSet:
          return current;
        case ScheduleSet::DAILY_4:        
          return GetNextDateTime(current, 7, 23, 4);
        case ScheduleSet::DAILY_6:
          return GetNextDateTime(current, 7, 23, 6);
        case ScheduleSet::DAILY_8:
          return GetNextDateTime(current, 7, 23, 8);

        case ScheduleSet::DAYNIGHT_10:
          return GetNextDateTime(current, 0, 24, 10);
        case ScheduleSet::DAYNIGHT_12:
          return GetNextDateTime(current, 0, 24, 12);
        case ScheduleSet::DAYNIGHT_14:
          return GetNextDateTime(current, 0, 24, 14);
        default:
          return current;
      }
    }      

    private:
    const DateTime GetNextDateTime(const DateTime &current, const short &startHour, const short &endHour, const short &count) const
    {
      const auto currentDay = current.day();
      FeedDateTime nextTime = DateTime(current.year(), current.month(), current.day());      
      const auto step = TimeSpan( (int)( ((endHour - startHour) / count) * 3600 ));

      //S_TRACE("Step: ", step.hours(), ":", step.minutes());
      
      for(nextTime += TimeSpan(startHour * 3600); nextTime.hour() <= endHour; nextTime += step)
      {
        if(nextTime.hour() > current.hour())
        {
          break;
        }
      }

      if(nextTime.day() > currentDay || nextTime.hour() > endHour)
      {
        const auto nextDayStart = DateTime(current.year(), current.month(), currentDay + 1);
        return GetNextDateTime(nextDayStart, startHour, endHour, count);
      }

      S_TRACE2("Get Next Alarm: ", nextTime.timestamp());
      return nextTime;
    }
  };
}

#endif //FEED_SCHEDULER_H