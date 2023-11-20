#pragma once
#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#ifdef DEBUG
  #define S_PRNT(value) Serial.print((value))
  #define S_PRINT(value) Serial.println((value))
  #define S_PRINT2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_PRINT3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))  
#else  
  #define S_PRNT(value) while(0)
  #define S_PRINT(value) while(0)
  #define S_PRINT2(value1, value2) while(0)
  #define S_PRINT3(value1, value2, value3) while(0)
#endif


#endif //DEBUG_HELPER_H