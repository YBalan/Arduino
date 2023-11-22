#pragma once
#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#ifdef DEBUG
  #define S_DBG(value) Serial.print((value))
  #define S_DEBUG(value) Serial.println((value))
  #define S_DEBUG2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_DEBUG3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))  
#else  
  #define S_DBG(value) while(0)
  #define S_DEBUG(value) while(0)
  #define S_DEBUG2(value1, value2) while(0)
  #define S_DEBUG3(value1, value2, value3) while(0)
#endif


#ifdef TRACE
  #define S_TRCE(value) Serial.print((value))
  #define S_TRACE(value) Serial.println((value))
  #define S_TRACE2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_TRACE3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))  
  #define S_TRACE4(value1, value2, value3, value4) Serial.print((value1)); Serial.print((value2)); Serial.print((value3)); Serial.println((value4))
  #define S_TRACE5(value1, value2, value3, value4, value5) Serial.print((value1)); Serial.print((value2)); Serial.print((value3)); Serial.print((value4)); Serial.println((value5))
#else  
  #define S_TRCE(value) while(0)
  #define S_TRACE(value) while(0)
  #define S_TRACE2(value1, value2) while(0)
  #define S_TRACE3(value1, value2, value3) while(0)
  #define S_TRACE4(value1, value2, value3, value4) while(0)
  #define S_TRACE5(value1, value2, value3, value4, value5) while(0)
#endif

#ifdef INFO
  #define S_INF(value) Serial.print((value))
  #define S_INFO(value) Serial.println((value))
  #define S_INFO2(value1, value2) Serial.print((value1)); Serial.println((value2))
  #define S_INFO3(value1, value2, value3) Serial.print((value1)); Serial.print((value2)); Serial.println((value3))
  #define S_INFO4(value1, value2, value3, value4) Serial.print((value1)); Serial.print((value2)); Serial.print((value3)); Serial.println((value4))
  #define S_INFO5(value1, value2, value3, value4, value5) Serial.print((value1)); Serial.print((value2)); Serial.print((value3)); Serial.print((value4)); Serial.println((value5))
#else  
  #define S_INF(value) while(0)
  #define S_INFO(value) while(0)
  #define S_INFO2(value1, value2) while(0)
  #define S_INFO3(value1, value2, value3) while(0)
  #define S_INFO4(value1, value2, value3, value4) while(0)
  #define S_INFO5(value1, value2, value3, value4, value5) while(0)
#endif


#endif //DEBUG_HELPER_H