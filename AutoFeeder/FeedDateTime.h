#pragma once
#ifndef FEED_DATETIME_H
#define FEED_DATETIME_H

#include "DateTime.h"
#include "Helpers.h"

//#define ENABLE_TRACE_FEEDDATETIME

#ifdef ENABLE_TRACE_FEEDDATETIME
#define FEEDDATETIME_TRACE(...) SS_TRACE(__VA_ARGS__)
#else
#define FEEDDATETIME_TRACE(...) {}
#endif

namespace Feed
{
  class FeedDateTime : public DateTime
  { 
    public:
    FeedDateTime() : DateTime(0, 0, 0, 0, 0, 0)
    { }

    FeedDateTime(const DateTime &dt) : DateTime(dt)
    { 
      FEEDDATETIME_TRACE("Copy from ", "DateTime");
    }
        
    FeedDateTime& operator= (const FeedDateTime &other)
    {      
      if (this != &other) {  // Check for self-assignment
        FEEDDATETIME_TRACE("operator = ", "Feed", "DateTime");
          yOff = other.yOff;
          m = other.m;
          d = other.d;
          hh = other.hh;
          mm = other.mm;
          ss = other.ss;          
      }
      return *this;  // Return a reference to this object
    }        

    const String GetTimeWithoutSeconds() { return timestamp(DateTime::TIMESTAMP_TIME).substring(0, 5); }   

    const uint8_t DayInMonth(const uint8_t &month, const uint16_t &year) const
    {
      if(month < 1 || month > 12 ) return 31;
      bool leap = month == 2 && year % 4 == 0;
      return daysInMonth[month - 1] + (leap ? 1 : 0);
    }
  };  

  class FeedDateTimeStore
  {
    private:    
    uint16_t _monthDay;    
    uint16_t _hourMinute;
    public:
    FeedDateTimeStore() : _monthDay(0), _hourMinute(0)
    { }

    FeedDateTimeStore(const DateTime &dt) 
      : _monthDay(Helpers::StoreHelper::CombineToUint16(dt.month(), dt.day()))
      , _hourMinute(Helpers::StoreHelper::CombineToUint16(dt.hour(), dt.minute()))
    { }

    FeedDateTimeStore& operator= (const FeedDateTimeStore &other)
    {
      if (this != &other) {  // Check for self-assignment          
          _monthDay = other._monthDay;
          _hourMinute = other._hourMinute;
      }
      return *this;  // Return a reference to this object
    }

    const uint16_t &monthDay() const
    {
      return _monthDay;
    }

    const uint8_t month() const
    {
      uint8_t m = 0, d = 0; 
      Helpers::StoreHelper::ExtractFromUint16(_monthDay, m, d);
      return m;
    }

    const uint8_t day() const
    {
      uint8_t m = 0, d = 0; 
      Helpers::StoreHelper::ExtractFromUint16(_monthDay, m, d);
      return d;
    }

    const uint8_t hour() const
    {
      uint8_t h = 0, m = 0; 
      Helpers::StoreHelper::ExtractFromUint16(_hourMinute, h, m);
      return h;
    }

    const uint8_t minute() const
    {
      uint8_t h = 0, m = 0; 
      Helpers::StoreHelper::ExtractFromUint16(_hourMinute, h, m);
      return m;
    }    
  };  
}

#endif //FEED_DATETIME_H