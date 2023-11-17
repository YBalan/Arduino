#pragma once
#ifndef FEED_SCHEDULER_H
#define FEED_SCHEDULER_H

namespace Feed
{
  enum class ScheduleSet : short
  {
    NotSet = 0,

    DAILY_2 = 2,
    DAILY_4 = 4,
    DAILY_6 = 6,
  };

  struct Scheduler
  {
    ScheduleSet Set;
    FeedDateTime NextAlarm;

    private:
    bool _hasAlarmed;

    public:   

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
        if(NextAlarm.hour() == current.hour())
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