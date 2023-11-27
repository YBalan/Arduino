#pragma once
#ifndef FEED_SETTINGS_H
#define FEED_SETTINGS_H

#ifdef RELEASE
#define FEEDS_STATUS_HISTORY_COUNT 20
#else
#define FEEDS_STATUS_HISTORY_COUNT 5
#endif

#define MIN_FEED_COUNT 1
#define MAX_FEED_COUNT 5

#include "FeedScheduler.h"

namespace Feed
{
  struct Settings
  {    
    unsigned short Delay;
    unsigned short RotateCount = 1;
    unsigned short CurrentPosition = 0;
    Scheduler FeedScheduler;
    //private:
    Feed::StatusInfo FeedHistory[FEEDS_STATUS_HISTORY_COUNT];
    public:
    void SetLastStatus(const Feed::StatusInfo& status)
    {
      Push(status);
    }

    const Feed::StatusInfo &GetLastStatus() const
    {
      return FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1];
    }

    const Feed::StatusInfo &GetStatusByIndex(const short &idx) const
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
      CurrentPosition = 0;
      RotateCount = MIN_FEED_COUNT;
      FeedScheduler.Reset();
      for(short idx = 0; idx < FEEDS_STATUS_HISTORY_COUNT; idx++)
      {
        FeedHistory[idx] = Feed::StatusInfo();
      }
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