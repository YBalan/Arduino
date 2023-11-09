#pragma once
#ifndef FEED_SETTINGS_H
#define FEED_SETTINGS_H

#define FEEDS_STATUS_HISTORY_COUNT 5

namespace Feed
{
  struct Settings
  {
    unsigned short Delay;
    private:
    Feed::StatusInfo FeedHistory[FEEDS_STATUS_HISTORY_COUNT];
    public:
    const 
  };
}

#endif //FEED_SETTINGS_H