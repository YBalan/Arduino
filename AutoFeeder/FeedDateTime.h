#pragma once
#ifndef FEED_DATETIME_H
#define FEED_DATETIME_H

#include "DateTime.h"

namespace Feed
{
  class FeedDateTime : public DateTime
  {
    public:
    FeedDateTime() : DateTime(1, 1, 1, 0, 0, 0)
    { }

    FeedDateTime(const DateTime &dt) : DateTime(dt)
    { }

    FeedDateTime& operator= (const FeedDateTime &other)
    {
      if (this != &other) {  // Check for self-assignment
          yOff = other.yOff;
          m = other.m;
          d = other.d;
          hh = other.hh;
          mm = other.mm;
          ss = other.ss;
      }
      return *this;  // Return a reference to this object
    }

    //const FeedDateTime &hour(const short &hour) { hh = hour; return *this; }

    const String GetTimeWithoutSeconds() { return timestamp(DateTime::TIMESTAMP_TIME).substring(0, 5); }   

    FeedDateTime &operator+=(const TimeSpan& span)
    {
        d += span.days();
        hh += span.hours();
        mm += span.minutes();
        ss += span.seconds();

        return *this;
    } 

    bool operator<(const DateTime& right) const
    {
        return GetTotalValueWithoutSeconds() < GetTotalValueWithoutSeconds(right);
    }
    bool operator>(const DateTime& right) const
    {
        return right < *this;
    }
    bool operator<=(const DateTime& right) const
    {
        return !(*this > right);
    }
    bool operator>=(const DateTime& right) const
    {
        return !(*this < right);
    }
    bool operator==(const DateTime& right) const
    {
        return GetTotalValueWithoutSeconds() == GetTotalValueWithoutSeconds(right);
    }
    bool operator!=(const DateTime& right) const
    {
        return !(*this == right);
    }

    public:
    const long GetTotalValueWithoutSeconds() const
    {
      return time2long(day(), hour(), minute(), 0);
    }

    static const long GetTotalValueWithoutSeconds(const DateTime &dt)
    {
      return time2long(dt.day(), dt.hour(), dt.minute(), 0);
    }    
  };
}

#endif //FEED_DATETIME_H