#pragma once
#ifndef FEED_SETTINGS_H
#define FEED_SETTINGS_H

#define FEEDS_STATUS_HISTORY_COUNT 10

#include "FeedScheduler.h"

namespace Feed
{
  struct Settings
  {    
    unsigned short Delay;
    unsigned short RotateCount;
    unsigned short CurrentPosition;
    Scheduler FeedScheduler;
    private:
    Feed::StatusInfo FeedHistory[FEEDS_STATUS_HISTORY_COUNT];
    public:
    void SetLastStatus(const Feed::StatusInfo& status)
    {
      Push(status);
    }

    const Feed::StatusInfo &GetLastStatus()
    {
      return FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1];
    }

    const Feed::StatusInfo &GetStatusByIndex(const short &idx)
    {
      if(idx >= 0 && idx < FEEDS_STATUS_HISTORY_COUNT)
      {
        return FeedHistory[idx];
      }
      return Feed::StatusInfo();
    }

    private:
    void Push(const Feed::StatusInfo& status)
    {
      if(FEEDS_STATUS_HISTORY_COUNT > 1)
      {
        for(short idx = 1; idx < FEEDS_STATUS_HISTORY_COUNT; idx++)
        {
          FeedHistory[idx - 1] = FeedHistory[idx];
        }
      }
      FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1] = status;      
    }
  };
}

#endif //FEED_SETTINGS_H