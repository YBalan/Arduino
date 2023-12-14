#ifndef Button_h
#define Button_h

#include <ezButton.h>



class Button : public ezButton
{
private:
  //ezButton _btn;
  uint32_t _startTicks = 0;
  uint16_t  _longPressValue;

public:
  Button(const uint8_t &pin, const uint16_t &longPressValue = 2000) : ezButton(pin), _longPressValue(longPressValue)
  {

  }
  void loop(){ ezButton::loop();  }

  const bool isPressed() 
  {
     if(ezButton::isPressed())  
     {
       _startTicks = millis();
       return true;
     }
     return false;
  }

  const bool isReleased()
  {
     if(ezButton::isReleased())
     {      
      return true;
     }
     return false;
  }

  bool WaitUntilReleased()
  {
    while(true)
    {
      ezButton::loop();
      if(isReleased())
      {
        break;
      }
    }
  }
  
  const uint16_t &GetLongPressValue() const { return _longPressValue; }
  const uint16_t &SetLongPressValue(const uint16_t &longPressValue) { _longPressValue = longPressValue; return _longPressValue; }  

  const unsigned long getTicks() { return millis() - _startTicks; }

  const bool isLongPress() const { auto res = _startTicks != 0 && getTicks() >= _longPressValue; return res; }
  
  const bool isLongPress(const short &longPressValue) const{ auto res = _startTicks != 0 && (millis() - longPressValue) >= _longPressValue; return res; }

  void resetTicks(){ _startTicks = 0; }

  //const int getCount(){ return _btn.getCount(); }

  //void setDebounceTime(const unsigned long & time ){ _btn.setDebounceTime(time); }

  //void resetCount(){ _btn.resetCount(); }

  //const short &getStateRaw(){ return _btn.getStateRaw(); }

  //const short getState(){ return _btn.getState(); }
};

#endif