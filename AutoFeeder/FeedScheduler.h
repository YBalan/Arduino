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

    const char *const SetToString(const bool &shortView = false) const { return GetSchedulerSetString(Set, shortView); }

    const bool IsTimeToAlarm(const DateTime &current)
    {
      if(Set == ScheduleSet::NotSet) return false;
      if(_hasAlarmed) return false;

      if(NextAlarm < current) 
      {
        _hasAlarmed = true;
        return false;
      }

      if(NextAlarm.year() == current.year() && NextAlarm.month() == current.month() && NextAlarm.day() == current.day())
      {
        if(NextAlarm.hour() == current.hour() && NextAlarm.minute() == current.minute())
        {
          _hasAlarmed = true;          
        }
      }
      return !_hasAlarmed;
    }

    const bool HasAlarmed() const { return _hasAlarmed; }
    //void HasAlarmed(const bool &hasAlarmed) {_hasAlarmed = hasAlarmed;}

    void SetNextAlarm(const DateTime &current)
    {
      NextAlarm = GetNextTime(current);
    }

    const DateTime &GetNextAlarm() const
    {
      return NextAlarm;
    }

    private:
    const DateTime GetNextTime(const DateTime &current) const
    {
      switch(Set)
      {
        case ScheduleSet::NotSet:
          return current;
        case ScheduleSet::DAILY_2:
        {
          return current + GetTimeSpan(current, 7, 23, 2);
        }        
        default:
          return current;
      }
    }      

    private:
    const TimeSpan GetTimeSpan(const DateTime &current, const short &startHour, const short &endHour, const short &count) const
    {
      short step = endHour - startHour / count;
      for(short nextHour = startHour; startHour <= endHour; nextHour += step )
      {

      }
      return TimeSpan(0, 0, step, 0);
    }
  };
}

#endif //FEED_SCHEDULER_H