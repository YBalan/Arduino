#ifndef TimeHelper_h
#define TimeHelper_h

// #define WeekMillis 604800000lu
// #define DayMillis 86400000lu
// #define HoursMillis 3600000lu
// #define MinuteMillis 60000lu

#define YearSecs    31556952lu
#define MonthSecs   2629800lu
#define WeekSecs    604800lu
#define DaySecs     86400lu
#define HourSecs    3600lu
#define MinuteSecs  60lu

namespace Helpers
{
  struct Time
  {
  public:
    enum TimeUnitType  { Year, Month, Week, Day, Hour, Min, Sec };  
  private:
    struct TimeUnit
    {
      TimeUnitType Type;
      #ifdef DEBUG
      char Name[7];
      #endif
      char Format[4];
      unsigned long Secs;
      char Delimeter[2];
    };

    static const unsigned short ShortTimeStartIndex = 4;
    static const unsigned short TimeUnitsCount = 7;

    #ifdef DEBUG
    inline static const TimeUnit timeUnits[TimeUnitsCount] = 
    {
      {Year,  "Years",  "%sY", YearSecs,    "/"},
      {Month, "Months", "%sM", MonthSecs,   "/"},
      {Week,  "Weeks",  "%sw", WeekSecs,    "/"},
      {Day,   "Days",   "%sd", DaySecs,     "/"},
      {Hour,  "Hours",  "%sh", HourSecs,    ":"},
      {Min,   "Mins",   "%sm", MinuteSecs,  ":"},
      {Sec,   "Secs",   "%ss", 1,           ""},
    };
    #else
    inline static const TimeUnit timeUnits[TimeUnitsCount] = 
    {
      {Year,  "%sY", YearSecs,    "/"},
      {Month, "%sM", MonthSecs,   "/"},
      {Week,  "%sw", WeekSecs,    "/"},
      {Day,   "%sd", DaySecs,     "/"},
      {Hour,  "%sh", HourSecs,    ":"},
      {Min,   "%sm", MinuteSecs,  ":"},
      {Sec,   "%ss", 1,           ""},
    };
    #endif   
  
  public:
    static const char * const HumanizeTime(const unsigned long &timeSec, char *buff, const bool &shortTime)
    {
      char dbuff1[3];
      char dbuff2[3]; 
     
      for(unsigned short idx = shortTime ? ShortTimeStartIndex : 0; idx < TimeUnitsCount; idx++)
      {
        auto unitTime = GetUnitTime(timeSec, idx, buff, dbuff1, dbuff2, !shortTime);
        if(unitTime != 0)
        {
          return unitTime;
        }
      }

      return 0;
    }

  private:
    static const char * const GetUnitTime(const unsigned long &timeSec, const unsigned short &unitIdx, char *buff, char *dbuff1, char *dbuff2, const bool &showZeros){return GetUnitTime(timeSec, unitIdx, buff, dbuff1, dbuff2, showZeros, true);}
    static const char * const GetUnitTime(const unsigned long &timeSec, const unsigned short &unitIdx, char *buff, char *dbuff1, char *dbuff2, const bool &showZeros, const bool &showNextUnit)
    {
      if(unitIdx == TimeUnitsCount - 1)
      {
        #ifdef DEBUG         
        Serial.print(timeUnits[unitIdx].Name); Serial.println(timeSec);
        #endif
        sprintf(buff, timeUnits[unitIdx].Format, ToString(timeSec, true, dbuff1));
        return buff;
      }
      const unsigned long &unitSecs = timeUnits[unitIdx].Secs;      
      if(timeSec >= unitSecs)
      {
        auto nextUnitIdx = unitIdx + 1;
        const unsigned long &nextUnitSecs = timeUnits[nextUnitIdx].Secs;
        unsigned short unitCount = (unsigned short)(timeSec / unitSecs);
        if(unitCount > 0)
        { 
          #ifdef DEBUG         
          Serial.print(timeUnits[unitIdx].Name); Serial.println(unitCount);
          #endif
          if(!showNextUnit)
          {
            sprintf(buff, timeUnits[unitIdx].Format, ToString(unitCount, showZeros, dbuff1));
            return buff;
          }

          char format[10];
          strcpy(format, timeUnits[unitIdx].Format);
          strcat(format, timeUnits[unitIdx].Delimeter);

          unsigned long unitTotalSecs = (unsigned long)unitCount * unitSecs;
          unsigned long timeLeft = timeSec - unitTotalSecs;
          unsigned short nextUnitCount = (unsigned short)(timeLeft / nextUnitSecs);
          if(nextUnitCount > 0)
          { 
            strcat(format, timeUnits[nextUnitIdx].Format);

            sprintf(buff, format, ToString(unitCount, showZeros, dbuff1), ToString(nextUnitCount, showZeros, dbuff2));
            return buff;
          }
          char innerBuff[5];
          for(nextUnitIdx + 1; nextUnitIdx < TimeUnitsCount; nextUnitIdx++)
          {
            auto unitTime = GetUnitTime(timeLeft, nextUnitIdx, innerBuff, dbuff2, dbuff2, showZeros, false);
            if(unitTime != 0)
            {              
              strcat(format, "%s");

              sprintf(buff, format, ToString(unitCount, showZeros, dbuff1), unitTime);
              return buff;
            }
          }          
        }
      }      
      return 0;
    }
    static const char * const ToString(const unsigned short &source, const bool &showZeros, char *buff)
    {
      sprintf(buff, showZeros ? "%02d" : "%d", source);
      return buff;
    }
  };
};
#endif