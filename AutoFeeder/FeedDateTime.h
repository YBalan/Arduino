#pragma once
#ifndef FEED_DATETIME_H
#define FEED_DATETIME_H

#include "DateTime.h"

namespace Feed
{
  class FeedDateTime : public DateTime
  {
    public:
    FeedDateTime() : DateTime()
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
  };
}

#endif //FEED_DATETIME_H