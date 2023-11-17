#pragma once
#ifndef FEED_SETTINGS_H
#define FEED_SETTINGS_H

#define FEEDS_STATUS_HISTORY_COUNT 5

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
      FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1] = status;
    }

    const Feed::StatusInfo &GetLastStatus()
    {
      return FeedHistory[FEEDS_STATUS_HISTORY_COUNT - 1];
    }
  };
}

#endif //FEED_SETTINGS_H