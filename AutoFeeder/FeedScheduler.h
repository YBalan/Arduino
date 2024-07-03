#pragma once
#ifndef FEED_SCHEDULER_H
#define FEED_SCHEDULER_H

#include <DS323x.h>

//#define ENABLE_TRACE_FEEDSCHEDULER

#ifdef ENABLE_TRACE_FEEDSCHEDULER
#define FEEDSCHEDULER_TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define FEEDSCHEDULER_TRACE(...) {}
#endif

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

    const bool IsTimeToAlarm(const DS323x &rtc)
    {
      if(Set != ScheduleSet::NotSet && rtc.hasAlarmed(DS323x::AlarmSel::A2))
      {
        SetNextAlarm(rtc);
        return true;
      }
      return false;
    }
    
    void SetNextAlarm(const DS323x &rtc)
    {
      if(Set == ScheduleSet::NotSet)
      {
        rtc.clearAlarm(DS323x::AlarmSel::A2);
        rtc.enableAlarm2(false);
        return;
      }

      NextAlarm = GetNextTime(rtc.now());
      
      rtc.clearAlarm(DS323x::AlarmSel::A2);      
      rtc.format(DS323x::AlarmSel::A2, DS323x::Format::HOUR_24);
      rtc.dydt(DS323x::AlarmSel::A2, DS323x::DYDT::DYDT_DATE);
      rtc.ampm(DS323x::AlarmSel::A2, DS323x::AMPM::AMPM_PM);
      rtc.day(DS323x::AlarmSel::A2, NextAlarm.day());
      rtc.hour(DS323x::AlarmSel::A2, NextAlarm.hour());
      rtc.minute(DS323x::AlarmSel::A2, NextAlarm.minute());
      rtc.second(DS323x::AlarmSel::A2, 0);
      rtc.rate(DS323x::A2Rate::MATCH_MINUTE_HOUR);

      rtc.enableAlarm2(true);

      FEEDSCHEDULER_TRACE("RTC Alarm: ", rtc.alarm(DS323x::AlarmSel::A2).timestamp(), " rate: ", (uint8_t)rtc.rateA2());
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
      FeedDateTime nextTime = DateTime(current.year(), current.month(), currentDay, 0, 0, 0);      
      float stepFloat = (float)(endHour - startHour) / count;
      const auto timeSpan = TimeSpan( (int32_t)( stepFloat * 3600L ));
      const auto step = TimeSpan(0, timeSpan.hours(), timeSpan.minutes() ,0);

      FEEDSCHEDULER_TRACE("Step: ", stepFloat, " ", step.hours(), ":", step.minutes());
      //FEEDSCHEDULER_TRACE(step.totalseconds());
      
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

      FEEDSCHEDULER_TRACE("Get Next Alarm: ", nextTime.timestamp());
      return nextTime;
    }
  };
}

#endif //FEED_SCHEDULER_H