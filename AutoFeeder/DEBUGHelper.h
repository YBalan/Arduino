#pragma once
#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#define BUTTON_IS_RELEASED_MSG "Btn rel..."
#define BUTTON_IS_PRESSED_MSG "Btn press..."
#define BUTTON_IS_LONGPRESSED_MSG "Btn LONG press..."

#ifdef ENABLE_TRACE
  #define SS_TRACE(...) { PrintArguments(__VA_ARGS__); }

  template<typename T>
  void PrintArguments(T arg) {
    Serial.println(arg);
  }

  template<typename T, typename... Args>
  void PrintArguments(T arg, Args... args) {
    Serial.print(arg);
    //Serial.print(" ");
    PrintArguments(args...);
  }

#else
  #define SS_TRACE(...) {}
#endif


#endif //DEBUG_HELPER_H