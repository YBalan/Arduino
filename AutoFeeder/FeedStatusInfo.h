#pragma once
#ifndef FEED_STATUS_INFO_H
#define FEED_STATUS_INFO_H

namespace Feed
{
  enum Status
  {  
    MANUAL,
    REMOTE,
    PAW,
    SCHEDULE,
  };  

  static const char *const GetStatusString(const Status &status, const bool &shortStatus)
  {    
    switch(status)
    {
      case MANUAL:    return shortStatus ? "M" : "MAN";
      case REMOTE:    return shortStatus ? "R" : "REM";
      case PAW:       return shortStatus ? "P" : "PAW";
      case SCHEDULE:  return shortStatus ? "S" : "SCH";
      default:        return shortStatus ? "NA": "NaN";
    }
  }

  struct StatusInfo
  {
    Feed::Status Status;
    DateTime DateTime;
    const String ToString(const bool &shortStatus) const
    {
      return String(GetStatusString(shortStatus)) + " " + DateTime.timestamp();
    }    

    const char *const GetStatusString(const bool &shortStatus) const
    {    
      return Feed::GetStatusString(Status, shortStatus);
    }
  };
}

#endif //FEED_STATUS_INFO_H