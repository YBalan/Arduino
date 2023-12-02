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

    const String GetTimeWithoutSeconds() { return timestamp(DateTime::TIMESTAMP_TIME).substring(0, 5); }    

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
    const uint32_t GetTotalValueWithoutSeconds() const
    {
      return (32140800L * (year() - 2000)) + (2678400L * month()) + (86400L * day()) + (3600L * hour()) + (60 * minute());  //time2long(day(), hour(), minute(), 0);
    }

    static const uint32_t GetTotalValueWithoutSeconds(const DateTime &dt)
    {
      return (32140800L * (dt.year() - 2000)) + (2678400L * dt.month()) + (86400L * dt.day()) + (3600L * dt.hour()) + (60 * dt.minute());  //time2long(day(), hour(), minute(), 0);
    }    
  };

  class StoreHelper
  {
    public:    
    static const uint16_t CombineToUint16(const uint8_t &first, const uint8_t &second)
    {
      return (first << 8) | second; // Store first digit in higher bits and second digit in lower bits
    }

    static void ExtractFromUint16(const uint16_t &combined, uint8_t &first, uint8_t &second)
    {
      // Extract the first digit (higher bits)
      first = (combined >> 8) & 0xFF; // Shift right to get the first digit and mask other bits

      // Extract the second digit (lower bits)
      second = combined & 0xFF; // Mask higher bits to get the second digit
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
      : _monthDay(Feed::StoreHelper::CombineToUint16(dt.month(), dt.day()))
      , _hourMinute(Feed::StoreHelper::CombineToUint16(dt.hour(), dt.minute()))
    { }

    FeedDateTimeStore& operator= (const FeedDateTimeStore &other)
    {
      if (this != &other) {  // Check for self-assignment          
          _monthDay = other._monthDay;
          _hourMinute = other._hourMinute;
      }
      return *this;  // Return a reference to this object
    }

    const uint8_t month() const
    {
      uint8_t m = 0, d = 0; 
      StoreHelper::ExtractFromUint16(_monthDay, m, d);
      return m;
    }

    const uint8_t day() const
    {
      uint8_t m = 0, d = 0; 
      StoreHelper::ExtractFromUint16(_monthDay, m, d);
      return d;
    }

    const uint8_t hour() const
    {
      uint8_t h = 0, m = 0; 
      StoreHelper::ExtractFromUint16(_hourMinute, h, m);
      return h;
    }

    const uint8_t minute() const
    {
      uint8_t h = 0, m = 0; 
      StoreHelper::ExtractFromUint16(_hourMinute, h, m);
      return m;
    }    
  };  
}

#endif //FEED_DATETIME_H