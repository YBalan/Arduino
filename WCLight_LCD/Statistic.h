#ifndef Statistic_h
#define Statistic_h

#define STATISTIC_DATA_LENGHT 40
#define USHORT_MAX 65535u

struct Statistic
{    
  unsigned long Max = 0;
  unsigned long Min = 0;
  unsigned long Avg = 0;
  unsigned long Med = 0;

  unsigned short Data[STATISTIC_DATA_LENGHT];    
public:
  void reset() {Max = 0; Min = 0; Avg = 0; Med = 0; for(short i=0; i<STATISTIC_DATA_LENGHT; i++){Data[i] = 0;}}
  void Calc(unsigned long &newValue)
  {
    CalcAvg(newValue);
    CalcMed();
  }
  void CalcAvg(unsigned long &newValue)
  {
    //Serial.print("Calc Avg: newValue: "); Serial.println(newValue);

    if(newValue == 0) { return Avg; }
    unsigned long sum = 0;
    short filledCount = 0;
    for(short i = 1; i < STATISTIC_DATA_LENGHT; i++)
    {        
      if(Data[i] > 0)
      {
        Data[i - 1] = Data[i];
        sum += Data[i];
        filledCount++;
      }
    }

    Data[STATISTIC_DATA_LENGHT - 1] = newValue;
    sum += newValue;
    filledCount++;
    Avg = ceil(sum / filledCount);

    //Serial.print("Calc Avg: Avg: "); Serial.print(Avg); Serial.print(" Sum: "); Serial.print(sum); Serial.print(" Filled: "); Serial.println(filledCount);
  }
  
  void CalcMed()
  {
    qsort(Data, STATISTIC_DATA_LENGHT, sizeof(Data[0]), sort_asc);

    auto startIdx = GetStartIndex();
    startIdx = startIdx == -1 ? 0 : startIdx;
    auto idx = startIdx + ((STATISTIC_DATA_LENGHT - startIdx) / 2);

    if(startIdx == STATISTIC_DATA_LENGHT - 1){ Med = Data[startIdx]; return; }

    if(STATISTIC_DATA_LENGHT % 2 == 0)
    {        
      Med = (Data[idx] + Data[idx - 1]) / 2;
    }
    else
    {
      Med = Data[idx];
    }
  }

  const unsigned short GetByIndex(const short &idx)
  {
    if(idx >= 0 && idx < STATISTIC_DATA_LENGHT)
    {
      return Data[idx];
    }
    return USHORT_MAX;
  }

  const short GetStartIndex() const
  {
    short startIdx = -1;
    for(short i=0;i<STATISTIC_DATA_LENGHT;i++){ if(Data[i] > 0) { startIdx=i; break; }}
    return startIdx;
  }

  const short GetFilledCount() const
  {
    auto startIdx = GetStartIndex();
    return startIdx == -1 ? 0 : STATISTIC_DATA_LENGHT - startIdx;
  }

  void PrintData()
  {
    for(short i=0; i<STATISTIC_DATA_LENGHT; i++){Serial.print(Data[i]); Serial.print(";");}
  }

  private:
    // qsort requires you to create a sort function
  static const short sort_asc(const void *cmp1, const void *cmp2)
  {
    // Need to cast the void * to unsigned long *
    unsigned short b = *((unsigned short *)cmp1);
    unsigned short a = *((unsigned short *)cmp2);
    // The comparison
    return a > b ? -1 : (a < b ? 1 : 0);
    // A simpler, probably faster way:
    //return b - a;
  }  
};

#endif