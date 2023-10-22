#ifndef PumpState_h
#define PumpState_h

enum PumpState
{
  ON = 0,
  OFF = 1,  
  CALIBRATING = 2,
  MANUAL_ON = 3,
  MANUAL_OFF = 4,
  TIMEOUT_OFF = 5,
  SENSOR_OFF = 6,
  UNKNOWN,
};

static const char * const GetState(const PumpState & state)
{
    switch(state)
    {
      case ON:
        return "WATERING ON";
      case OFF:
        return "WATERING OFF";
      case CALIBRATING:
        return "CALIBRATING";
      case MANUAL_ON:
        return "MANUAL ON";
      case MANUAL_OFF:
        return "MANUAL OFF";
      case TIMEOUT_OFF:
        return "TIMEOUT OFF";
      case SENSOR_OFF:
        return "SENSOR OFF";
      default:
        return "UNKNOWN";
    }
}  

#endif