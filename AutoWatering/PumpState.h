#ifndef PumpState_h
#define PumpState_h

enum PumpState : uint8_t
{
  ON = 0,
  OFF = 1,  
  CALIBRATING = 2,
  MANUAL_ON = 3,
  MANUAL_OFF = 4,
  TIMEOUT_OFF = 5,
  SENSOR_OFF = 6,
  AERATION_ON = 7,
  AERATION_OFF = 8,
  UNKNOWN,
};

static const __FlashStringHelper* const GetState(const PumpState & state)
{
    switch(state)
    {
      case ON:
        return F("WATERING ON");
      case OFF:
        return F("WATERING OFF");
      case CALIBRATING:
        return F("CALIBRATING");
      case MANUAL_ON:
        return F("MANUAL ON");
      case MANUAL_OFF:
        return F("MANUAL OFF");
      case TIMEOUT_OFF:
        return F("TIMEOUT OFF");
      case SENSOR_OFF:
        return F("SENSOR OFF");
      case AERATION_ON:
        return F("AERATION ON");
      case AERATION_OFF:
        return F("AERATION OFF");
      default:
        return F("UNKNOWN");
    }
}  

#endif
