#ifndef Button_h
#define Button_h

#include <ezButton.h>



class Button
{
private:
  ezButton _btn;
  unsigned long _startTicks = 0;
  short  _longPressValue;

public:
  Button(const short & pin, const short &longPressValue = 2000) : _btn(pin), _longPressValue(longPressValue)
  {

  }
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
  
  const short &GetLongPressValue() const { return _longPressValue; }
  const short &SetLongPressValue(const short &longPressValue) { _longPressValue = longPressValue; return _longPressValue; }  

  const unsigned long getTicks() { return millis() - _startTicks; }

  const bool isLongPress() const { auto res = _startTicks != 0 && getTicks() >= _longPressValue; return res; }
  
  const bool isLongPress(const short &longPressValue) const{ auto res = _startTicks != 0 && (millis() - longPressValue) >= _longPressValue; return res; }

  void resetTicks(){ _startTicks = 0; }

  const int getCount(){ return _btn.getCount(); }

  void setDebounceTime(const unsigned long & time ){ _btn.setDebounceTime(time); }

  void resetCount(){ _btn.resetCount(); }

  //const short &getStateRaw(){ return _btn.getStateRaw(); }

  const short getState(){ return _btn.getState(); }
};

#endif