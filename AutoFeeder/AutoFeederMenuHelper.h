#pragma once
#ifndef AUTO_FEEDER_MENU_HELPER_H
#define AUTO_FEEDER_MENU_HELPER_H

#include "Print.h"

#define NOT_FED_YET_MSG "  NOT Fed Yet!"
#define NO_VALUES_MSG "No values"

#define RETURN_TO_MAIN_MENU_AFTER 60000

enum Menu : uint8_t
{
  Main,
  Dht,
  History,
  Schedule,
  RotateCount,
  StartAngle,
} currentMenu;

//bool IsBusy = false;
int8_t mainMenuPos = 0;
int8_t historyMenuPos = 0;
int8_t scheduleMenuPos = 0;
int8_t rotateCountMenuPos = 0;
int8_t startAngleMenuPos = 0;

static void PrintTime(const Print &print, const short &timeInSeconds, const char &sep = ' ', const bool &showUnitName = true, const bool &showMinsIfZero = false, const bool &showSecsIfZero = true)
{  
  short t = timeInSeconds;
  const short ss = t % 60;
  t /= 60;
  const short mm = t % 60;         
  
  if(showMinsIfZero || mm > 0) 
  {
    char buffmin[3];
    sprintf(buffmin, "%02d", mm);
    print.write(buffmin); 
     if(showUnitName)     
      print.write("min"); 
    print.write(sep); 
  }
  if(showSecsIfZero || ss > 0)
  {
    char buffsec[3];
    sprintf(buffsec, "%02d", ss);
    print.write(buffsec); 
    if(showUnitName)
      print.write("sec");
  }
}

#endif //AUTO_FEEDER_MENU_HELPER_H