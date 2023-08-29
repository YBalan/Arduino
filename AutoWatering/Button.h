#ifndef Button_h
#define Button_h

#include "Shares.h"
#include <ezButton.h>

struct Button
{
  const int LongPressValue = 2000;

  Button(const short & pin) : _btn(pin)
  {

  }

private:

  ezButton _btn = -1;
  unsigned long _startTicks = 0;

public:
  void loop(){ _btn.loop();  }

  const bool isPressed() 
  {
     if(_btn.isPressed())  
     {
       _startTicks = millis();
       return true;
     }
     return false;
  }

  const bool isReleased()
  {
     if(_btn.isReleased())
     {       
       return true;
     }
     return false;
  }

  const unsigned long getTicks() { return millis() - _startTicks; }

  const bool isLongPress(){ return getTicks() >= LongPressValue; }

  const int getCount(){ return _btn.getCount(); }

  void setDebounceTime(const unsigned long & time ){ _btn.setDebounceTime(time); }

  void resetCount(){ _btn.resetCount(); }

  short getStateRaw(){ return _btn.getStateRaw(); }
};

#endif