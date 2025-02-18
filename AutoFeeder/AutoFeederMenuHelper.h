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
  ScheduleRange,
  RotateCount,
  StartAngle,
}; //currentMenu;

//bool IsBusy = false;
// int8_t mainMenuPos = 0;
// int8_t historyMenuPos = 0;
// int8_t scheduleMenuPos = 0;
// int8_t rotateCountMenuPos = 0;
// int8_t startAngleMenuPos = 0;

typedef const bool (*MenuItemFunc) (int8_t &pos);
typedef const bool (*Menu2PosItemFunc) (int8_t &posLeft, int8_t &posRight);
class MenuItemBase
{
  protected:  
  MenuItemBase * _nextMenu;
  MenuItemFunc _showFunc;
  protected:
  MenuItemBase() : _nextMenu(0), _showFunc(0) {}
  MenuItemBase(MenuItemFunc showFunc) : _nextMenu(0), _showFunc(showFunc) {}
  //MenuItemBase(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : _nextMenu(&nextMenu), _showFunc(showFunc) {}  
  public:
  virtual const bool operator==(const Menu& right) const { return right == this->GetMenu(); }
  virtual const bool CanBeShown() const { return true; }
  virtual MenuItemBase* const GetNextMenu() { return _nextMenu == 0 ? this : _nextMenu; }
  public:
  virtual const bool UpButtonPressed() { int8_t p = 0; return _showFunc == 0 ? false : _showFunc(p); }
  virtual const bool DownButtonPressed(){ int8_t p = 0; return _showFunc == 0 ? false : _showFunc(p); }
  virtual const bool Show(const int8_t &pos) { int8_t p = pos; return _showFunc == 0 ? false : _showFunc(p); }
  virtual const bool IsSaveSettingsRequired() const { return false; }
  virtual const Menu GetMenu() const = 0;
  virtual const int8_t &GetPosition() const { return 0; }
  MenuItemBase &SetNextMenuItem(MenuItemBase &nextMenu) { _nextMenu = &nextMenu; return nextMenu; }
};

class MenuItemPositionBase : public MenuItemBase
{
  protected:
  int8_t _position;
  protected:
  MenuItemPositionBase() : MenuItemBase(), _position(0) {}
  MenuItemPositionBase(MenuItemFunc showFunc) : MenuItemBase(showFunc), _position(0) {}
 // MenuItemPositionBase(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemBase(showFunc, nextMenu), _position(0) {}
  protected:
  virtual const bool IsSaveSettingsRequired() const { return true; }
  virtual const bool UpButtonPressed(){ return _showFunc == 0 ? false : _showFunc(++_position); }
  virtual const bool DownButtonPressed(){ return _showFunc == 0 ? false : _showFunc(--_position); }
  virtual const bool Show(const int8_t &pos) { _position = pos; return _showFunc == 0 ? false : _showFunc(_position); }
  public:
  virtual const int8_t &GetPosition() const { return _position; }
};

class MenuMainItem : public MenuItemBase
{  
  virtual const Menu GetMenu() const { return Menu::Main; }  
  public: 
  MenuMainItem() : MenuItemBase() {} 
  MenuMainItem(MenuItemFunc showFunc) : MenuItemBase(showFunc) {}
  //MenuMainItem(const MenuItemBase &nextMenu) : MenuItemBase(0, nextMenu) {}
  //MenuMainItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemBase(showFunc, nextMenu) {}
};

class MenuDhtItem : public MenuItemBase
{  
  private:
  MenuItemFunc _canBeShownFunc;
  private:
  virtual const Menu GetMenu() const { return Menu::Dht; }
  virtual const bool CanBeShown() const { int8_t p = 0; return _canBeShownFunc == 0 ? true : _canBeShownFunc(p); }
  public:  
  MenuDhtItem(MenuItemFunc showFunc, MenuItemFunc canBeShownFunc) : MenuItemBase(showFunc), _canBeShownFunc(canBeShownFunc) {}
  //MenuDhtItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemBase(showFunc, nextMenu) {}  
};

class MenuHistoryItem : public MenuItemPositionBase
{  
  virtual const Menu GetMenu() const { return Menu::History; } 
  virtual const bool IsSaveSettingsRequired() const { return false; }
  public:  
  MenuHistoryItem(MenuItemFunc showFunc) : MenuItemPositionBase(showFunc) {}
  //MenuHistoryItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemPositionBase(showFunc, nextMenu) {}
};

class MenuScheduleItem : public MenuItemPositionBase
{  
  virtual const Menu GetMenu() const { return Menu::Schedule; }  
  public:  
  MenuScheduleItem(MenuItemFunc showFunc) : MenuItemPositionBase(showFunc) {}
  //MenuHistoryItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemPositionBase(showFunc, nextMenu) {}
};

class MenuScheduleRangeItem : public MenuItemPositionBase
{  
  private:
  int8_t _positionRight;
  Menu2PosItemFunc const _showFunc;
  private:
  virtual const bool CanBeShown() const { return false; }
  virtual const Menu GetMenu() const { return Menu::ScheduleRange; }  
  virtual const bool UpButtonPressed(){ return _showFunc == 0 ? false : _showFunc(_position, ++_positionRight); }
  virtual const bool DownButtonPressed(){ return _showFunc == 0 ? false : _showFunc(++_position, _positionRight); }
  virtual const bool Show(int8_t &pos) { _position = pos; return _showFunc == 0 ? false : _showFunc(_position, _positionRight); }
  public:  
  MenuScheduleRangeItem(Menu2PosItemFunc showFunc) : MenuItemPositionBase(), _showFunc(showFunc), _positionRight(-10) {}
  //MenuHistoryItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemPositionBase(showFunc, nextMenu) {}
};

class MenuStartAngleItem : public MenuItemPositionBase
{  
  virtual const Menu GetMenu() const { return Menu::StartAngle; }  
  public:  
  MenuStartAngleItem(MenuItemFunc showFunc) : MenuItemPositionBase(showFunc) {}
  //MenuHistoryItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemPositionBase(showFunc, nextMenu) {}
};

class MenuRotateCountItem : public MenuItemPositionBase
{  
  virtual const Menu GetMenu() const { return Menu::RotateCount; }  
  public:  
  MenuRotateCountItem(MenuItemFunc showFunc) : MenuItemPositionBase(showFunc) {}
  //MenuHistoryItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemPositionBase(showFunc, nextMenu) {}
};

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