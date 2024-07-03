#include <avr/wdt.h>
#pragma once
#ifndef AUTO_FEEDER_MENU_HELPER_H
#define AUTO_FEEDER_MENU_HELPER_H

#include "Print.h"
#include "FeedDateTime.h"

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

enum TimeMenu : uint8_t
{
  Year,
  Month,
  Day,
  Hour,
  Minute,
  Second,
};

const bool ChangeTimeMenu(const DS323x &rtc, const LiquidCrystal_I2C &lcd, const ezButton &nextBtn, const ezButton &upBtn, const ezButton &downBtn, const ezButton &returnBtn)
{
  lcd.clear();
  returnBtn.loop();

  uint32_t currentTicks = millis();

  const Feed::FeedDateTime dt = rtc.now();
  auto yyyy = dt.year();
  auto MM =  dt.month();
  auto dd = dt.day();
  int8_t hh = dt.hour();
  int8_t mm = dt.minute();
  //auto ss = dt.second();

  TimeMenu pos = TimeMenu::Year;

  uint8_t column = 0;
  uint8_t row = 0;
  uint8_t symbolsCount = 2;

  char buffer[17];

  lcd.setCursor(0, 0);
  sprintf(buffer, "%04d-%02d-%02d", yyyy, MM, dd);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  sprintf(buffer, "%02d:%02d:%02d", hh, mm, rtc.second());
  lcd.print(buffer);

  while(!returnBtn.isPressed())
  {
    returnBtn.loop();
    nextBtn.loop();
    upBtn.loop();
    downBtn.loop();    

    if(nextBtn.isPressed())
    {
      pos = pos + 1;
      if(pos >= TimeMenu::Second)
        pos = TimeMenu::Year;
    }

    symbolsCount = 2;
    switch(pos)
    {
      case TimeMenu::Year:
        column = 0; row = 0; symbolsCount = 4;
        if(upBtn.isReleased()) yyyy++;
        if(downBtn.isReleased()) yyyy--;
        if(yyyy < 2000) yyyy = 2022;
        if(yyyy > 2050) yyyy = 2022;
        break;
      case TimeMenu::Month:
        column = 5; row = 0;
        if(upBtn.isReleased()) MM++;
        if(downBtn.isReleased()) MM--;
        if(MM < 1) MM = 12;
        if(MM > 12) MM = 1;
        break;
      case TimeMenu::Day:
        column = 8; row = 0;
        if(upBtn.isReleased()) dd++;
        if(downBtn.isReleased()) dd--;
        if(dd < 1) dd = dt.DayInMonth(MM, yyyy);
        if(dd > dt.DayInMonth(MM, yyyy)) dd = 1;
        break;

      case TimeMenu::Hour:
        column = 0; row = 1;
        if(upBtn.isReleased()) hh++;
        if(downBtn.isReleased()) hh--;
        if(hh < 0) hh = 23;
        if(hh > 23) hh = 0;
        break;
      case TimeMenu::Minute:
        column = 3; row = 1;
        if(upBtn.isReleased()) mm++;
        if(downBtn.isReleased()) mm--;
        if(mm < 0) mm = 59;
        if(mm > 59) mm = 0;
        break;
    }   

    if(millis() - currentTicks >= 700)
    {
      lcd.setCursor(column, row);
      for(uint8_t i = 0; i < symbolsCount; i++)      
          lcd.print(' ');  
    }

    if(millis() - currentTicks >= 1000)
    {
      lcd.setCursor(0, 0);
      sprintf(buffer, "%04d-%02d-%02d", yyyy, MM, dd);
      lcd.print(buffer);
      lcd.setCursor(0, 1);
      sprintf(buffer, "%02d:%02d:%02d", hh, mm, rtc.second());
      lcd.print(buffer);

      currentTicks = millis();  
    }   

    wdt_reset();
  }

  rtc.now(DateTime(yyyy, MM, dd, hh, mm, rtc.second()));
}

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