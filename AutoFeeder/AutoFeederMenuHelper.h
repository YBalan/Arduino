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

typedef const bool (*MenuItemFunc) (int8_t pos);
typedef const bool (*Menu2PosItemFunc) (int8_t posLeft, int8_t posRight);
class MenuItemBase
{
  protected:  
  MenuItemBase * _nextMenu;
  MenuItemFunc const _showFunc;
  protected:
  MenuItemBase() : _nextMenu(0), _showFunc(0) {}
  MenuItemBase(MenuItemFunc showFunc) : _nextMenu(0), _showFunc(showFunc) {}
  //MenuItemBase(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : _nextMenu(&nextMenu), _showFunc(showFunc) {}  
  public:
  virtual const bool operator==(const Menu& right) const { return right == this->GetMenu(); }
  virtual const bool CanBeShown() const { return true; }
  virtual MenuItemBase* const GetNextMenu() { return _nextMenu == 0 ? this : this->CanBeShown() ? _nextMenu : _nextMenu->GetNextMenu(); }
  public:
  virtual const bool UpButtonPressed() { return _showFunc == 0 ? false : _showFunc(0); }
  virtual const bool DownButtonPressed(){ return _showFunc == 0 ? false : _showFunc(0); }
  virtual const bool Show() const { return _showFunc == 0 ? false : _showFunc(0); }
  virtual const Menu GetMenu() const = 0;
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
  virtual const bool UpButtonPressed(){ return _showFunc == 0 ? false : _showFunc(++_position); }
  virtual const bool DownButtonPressed(){ return _showFunc == 0 ? false : _showFunc(--_position); }
  virtual const bool Show() const { return _showFunc == 0 ? false : _showFunc(_position); }
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
  MenuItemFunc const _canBeShownFunc;
  private:
  virtual const Menu GetMenu() const { return Menu::Dht; }
  virtual const bool CanBeShown() const { return _canBeShownFunc == 0 ? true : _canBeShownFunc(0); }
  public:  
  MenuDhtItem(MenuItemFunc showFunc, MenuItemFunc canBeShownFunc) : MenuItemBase(showFunc), _canBeShownFunc(canBeShownFunc) {}
  //MenuDhtItem(MenuItemFunc showFunc, const MenuItemBase &nextMenu) : MenuItemBase(showFunc, nextMenu) {}  
};

class MenuHistoryItem : public MenuItemPositionBase
{  
  virtual const Menu GetMenu() const { return Menu::History; }  
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
  virtual const bool Show() const { return _showFunc == 0 ? false : _showFunc(_position, _positionRight); }
  public:  
  MenuScheduleRangeItem(Menu2PosItemFunc showFunc) : MenuItemPositionBase(), _showFunc(showFunc), _positionRight(0) {}
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