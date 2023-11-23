#pragma once
#ifndef FEED_SCHEDULER_H
#define FEED_SCHEDULER_H

#define FEEDS_SCHEDULER_SETTINGS_COUNT 7

namespace Feed
{
  enum ScheduleSet : short
  {
    NotSet = 0,

    DAILY_2,
    DAILY_4,
    DAILY_6,

    DAYNIGHT_8,
    DAYNIGHT_10,
    DAYNIGHT_12,
  };

  static const char *const GetSchedulerSetString(const ScheduleSet &set, const bool &shortView)
  {    
    switch(set)
    {
      case ScheduleSet::DAILY_2:      return shortView ? "D2"   : "Day 2";
      case ScheduleSet::DAILY_4:      return shortView ? "D4"   : "Day 4";
      case ScheduleSet::DAILY_6:      return shortView ? "D6"   : "Day 6";

      case ScheduleSet::DAYNIGHT_8:   return shortView ? "DN8"  : "DayNght 8";
      case ScheduleSet::DAYNIGHT_10:  return shortView ? "DN10" : "DayNght 10";
      case ScheduleSet::DAYNIGHT_12:  return shortView ? "DN12" : "DayNght 12";
      case ScheduleSet::NotSet:
      default:                        return shortView ? "NA"   : "NotSet";
    }
  }

  struct Scheduler
  {
    ScheduleSet Set;
    FeedDateTime NextAlarm;

    private:
    bool _hasAlarmed;

    public: 

    Scheduler() : Set(NotSet), NextAlarm(), _hasAlarmed(false) { }

    const char *const SetToString(const bool &shortView = false) const { return GetSchedulerSetString(Set, shortView); }

    const bool IsTimeToAlarm(const DateTime &current)
    {
      if(Set == ScheduleSet::NotSet) return false;      

      //S_TRACE4("Next Alarm: ", NextAlarm.timestamp(), " Cur: ", current.timestamp());
           
      if(NextAlarm < current)
      {
        //_hasAlarmed = true;
        SetNextAlarm(current);
        return false;
      }

      if(NextAlarm == current)
      {       
        S_TRACE4("Alarm: ", NextAlarm.timestamp(), " Cur: ", current.timestamp());
        //_hasAlarmed = true;
        SetNextAlarm(current);
        return true;        
      }
      return false;
    }

    const bool HasAlarmed() const { return _hasAlarmed; }
    //void HasAlarmed(const bool &hasAlarmed) {_hasAlarmed = hasAlarmed;}

    void SetNextAlarm(const DateTime &current)
    {
      NextAlarm = GetNextTime(current);
      S_TRACE2("Set Next Alarm: ", NextAlarm.timestamp());
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
      switch(Set)
      {
        case ScheduleSet::NotSet:
          return current;
        case ScheduleSet::DAILY_2:        
          return GetNextDateTime(current, 7, 23, 2);
        case ScheduleSet::DAILY_4:
          return GetNextDateTime(current, 7, 23, 4);
        case ScheduleSet::DAILY_6:
          return GetNextDateTime(current, 7, 23, 6);

        case ScheduleSet::DAYNIGHT_8:
          return GetNextDateTime(current, 0, 24, 8);
        case ScheduleSet::DAYNIGHT_10:
          return GetNextDateTime(current, 0, 24, 10);
        case ScheduleSet::DAYNIGHT_12:
          return GetNextDateTime(current, 0, 24, 12);
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

      S_TRACE4("Step: ", step.hours(), ":", step.minutes());
      
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

      return nextTime;
    }
  };
}

#endif //FEED_SCHEDULER_H