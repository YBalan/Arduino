#pragma once
#ifndef AUTO_FEEDER_MENU_HELPER_H
#define AUTO_FEEDER_MENU_HELPER_H

#define NOT_FED_YET_MSG "  NOT Fed Yet!"
#define NO_VALUES_MSG "No values"

#define RETURN_TO_MAIN_MENU_AFTER 60000

enum Menu : short
{
  Main,
  History,
  Schedule,
  FeedCount,
} currentMenu;

//bool IsBusy = false;
short mainMenuPos = 0;
short historyMenuPos = 0;
short scheduleMenuPos = 0;
short feedCountMenuPos = 0;

#endif //AUTO_FEEDER_MENU_HELPER_H