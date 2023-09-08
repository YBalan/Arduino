#ifndef Button_h
#define Button_h

#include <ezButton.h>

struct Button
{
  const int LongPressValue = 2000;

  Button(const short & pin) : _btn(pin)
  {

  }

private:
  ezButton _btn;
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

  bool WaitUntilReleased()
  {
    while(true)
    {
      _btn.loop();
      if(isReleased())
      {
        break;
      }
    }
  }

  const unsigned long getTicks() { return millis() - _startTicks; }

  const bool isLongPress(){ auto res = _startTicks != 0 && getTicks() >= LongPressValue; return res; }

  void resetTicks(){ _startTicks = 0; }

  const int getCount(){ return _btn.getCount(); }

  void setDebounceTime(const unsigned long & time ){ _btn.setDebounceTime(time); }

  void resetCount(){ _btn.resetCount(); }

  //const short &getStateRaw(){ return _btn.getStateRaw(); }

  const short getState(){ return _btn.getState(); }
};

#endif