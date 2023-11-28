#pragma once
#ifndef FEED_STATUS_INFO_H
#define FEED_STATUS_INFO_H

#include "DateTime.h"
#include "FeedDateTime.h"

namespace Feed
{
  enum class Status : short
  {  
    Unknown,
    MANUAL,
    REMOUTE,
    PAW,
    SCHEDULE,
    TEST,
  };  

  static const char *const GetFeedStatusString(const Status &status, const bool &shortView)
  {    
    switch(status)
    {
      case Status::MANUAL:    return "MAN";
      case Status::REMOUTE:   return "REM";
      case Status::PAW:       return "PAW";
      case Status::SCHEDULE:  return "SCH";
      case Status::TEST:      return "TST";
      default:                return "NaN";
    }
  }

  struct StatusInfo
  {
    Feed::Status Status;
    FeedDateTime DT;

    StatusInfo() : Status(Status::Unknown), DT()
    { }

    StatusInfo(const Feed::Status &status, const DateTime &dt) : Status(status), DT(dt) 
    { }

    StatusInfo& operator=(const StatusInfo &info)
    {
      if (this != &info) {  // Check for self-assignment
          Status = info.Status;
          DT = info.DT;
      }
      return *this;  // Return a reference to this object
    }

    const String ToString(const bool shortView = false) const
    {
      char buff[17];
      
      sprintf(buff, "%02d:%02d -%s", DT.hour(), DT.minute(), GetFeedStatusString(Status, shortView));
      return buff;
    } 

    const String GetDateString()
    {
      char buff[6];
      sprintf(buff, "%02d/%02d", DT.month(), DT.day());
      return buff;
    }  
  };
}

#endif //FEED_STATUS_INFO_H