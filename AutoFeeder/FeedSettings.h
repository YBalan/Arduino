#pragma once
#ifndef FEED_SETTINGS_H
#define FEED_SETTINGS_H

#ifdef RELEASE
#define FEEDS_STATUS_HISTORY_COUNT 25
#else
#define FEEDS_STATUS_HISTORY_COUNT 5
#endif

#define MIN_FEED_COUNT 1
#define MAX_FEED_COUNT 5
#define MOTOR_START_POS 30 //Impact on portion size
#define MOTOR_START_POS_INCREMENT 10

#include "FeedScheduler.h"

namespace Feed
{
  struct Settings
  {    
    unsigned short Delay;
    uint8_t RotateCount = 1;
    uint8_t CurrentPosition = MOTOR_START_POS;
    uint8_t StartAngle = MOTOR_START_POS;
    Scheduler FeedScheduler;
    uint8_t Reserved1 = 0;
    uint8_t Reserved2 = 0;
    uint8_t Reserved3 = 3;
    
    //private:
    Feed::StatusInfo FeedHistory[FEEDS_STATUS_HISTORY_COUNT];
    public:
    void SetLastStatus(const Feed::StatusInfo& status)
    {
      PushFirst(status);
    }

    const Feed::StatusInfo &GetLastStatus() const
    {
      return FeedHistory[0];
    }

    const Feed::StatusInfo &GetStatusByIndex(const uint8_t &idx) const
    {
      if(idx >= 0 && idx < FEEDS_STATUS_HISTORY_COUNT)
      {
        return FeedHistory[idx];
      }
      return Feed::StatusInfo();
    }

    void Reset()
    {
      Delay = 0;
      CurrentPosition = MOTOR_START_POS;
      RotateCount = MIN_FEED_COUNT;
      StartAngle = MOTOR_START_POS;
      FeedScheduler.Reset();
      for(int8_t idx = 0; idx < FEEDS_STATUS_HISTORY_COUNT; idx++)
      {
        FeedHistory[idx] = Feed::StatusInfo();
      }
    }

    private:
    void PushLast(const Feed::StatusInfo& status)
    {
      if(FEEDS_STATUS_HISTORY_COUNT > 1)
      {
        for(int8_t idx = 1; idx < FEEDS_STATUS_HISTORY_COUNT; idx++)
        {
          FeedHistory[idx - 1] = FeedHistory[idx];
        }
      }
      FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1] = status;      
    }

    void PushFirst(const Feed::StatusInfo& status)
    {
      if(FEEDS_STATUS_HISTORY_COUNT > 1)
      {
        for(int8_t idx = FEEDS_STATUS_HISTORY_COUNT - 2; idx >= 0 ; idx--)
        {
          FeedHistory[idx + 1] = FeedHistory[idx];
        }
      }
      FeedHistory[0] = status;      
    }
  };
}

#endif //FEED_SETTINGS_H