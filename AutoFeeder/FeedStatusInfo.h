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
  };  

  static const char *const GetStatusString(const Status &status, const bool &shortStatus)
  {    
    switch(status)
    {
      case Status::MANUAL:    return shortStatus ? "M" : "MAN";
      case Status::REMOUTE:   return shortStatus ? "R" : "REM";
      case Status::PAW:       return shortStatus ? "P" : "PAW";
      case Status::SCHEDULE:  return shortStatus ? "S" : "SCH";
      default:                return shortStatus ? "NA": "NaN";
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

    const String ToString(const bool shortStatus = false) const
    {
      return String(GetStatusString(shortStatus)) + " " + DT.timestamp(DateTime::TIMESTAMP_TIME);
    }    

    const char *const GetStatusString(const bool &shortStatus) const
    {    
      return Feed::GetStatusString(Status, shortStatus);
    }
  };
}

#endif //FEED_STATUS_INFO_H