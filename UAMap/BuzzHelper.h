#pragma once
#ifndef BUZZ_HELPER_H
#define BUZZ_HELPER_H

#include "DEBUGHelper.h"
#ifdef ENABLE_INFO_BUZZ
#define BUZZ_INFO(...) SS_TRACE("[BUZZ INFO] ", __VA_ARGS__)
#else
#define BUZZ_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_BUZZ
#define BUZZ_TRACE(...) SS_TRACE("[BUZZ TRACE] ", __VA_ARGS__)
#else
#define BUZZ_TRACE(...) {}
#endif

#define DEFAULT_SIREN_PERIOD_DURATION_MS 325
#define DEFAULT_SIREN_UP_FRQ 800
#define DEFAULT_SIREN_DOWN_FRQ 500

#define NOTE_EMPTY 0
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

namespace Buzz
{
  void Siren(const uint8_t &pin
    , const uint16_t &totalTime = 3000
    , const uint16_t &up = DEFAULT_SIREN_UP_FRQ
    , const uint16_t &down = DEFAULT_SIREN_DOWN_FRQ
    , const uint16_t &pause = 0
    , const uint16_t &defultPeriod = DEFAULT_SIREN_PERIOD_DURATION_MS)
  {
    uint16_t repeat = (float)totalTime / (float)((defultPeriod * 2) + (pause * 2));
    for(uint16_t count = 0; count <= repeat; count++)
    {
      tone(pin, up, defultPeriod); 
      
      delay(defultPeriod + pause);
      
      tone(pin, down, defultPeriod);   
      
      delay(defultPeriod + pause);
    }
    
    noTone(pin);
  }

  void AlarmStart(const uint8_t &pin, const int16_t &totalTime = 3000)
  {
    BUZZ_INFO(F("AlarmStart"));
    if(totalTime > 0)
    {      
      BUZZ_INFO(F("Time: "), totalTime);
      Siren(pin, totalTime, /*up:*/500, /*down:*/500, /*pause:*/500, /*period:*/1000);
    }
  }

  void AlarmEnd(const uint8_t &pin, const int16_t &totalTime = 3000)
  {    
    BUZZ_INFO(F("AlarmEnd"));
    if(totalTime > 0)
    {
      BUZZ_INFO(F("Time: "), totalTime);
      Siren(pin, totalTime, /*up:*/800, /*down:*/800, /*pause:*/500, /*period:*/1000);
    }
  }
  
  /*
  //https://www.arduino.cc/en/Tutorial/Tone
  //https://projecthub.arduino.cc/tmekinyan/playing-popular-songs-with-arduino-and-a-buzzer-546f4a
  //https://www.build-electronic-circuits.com/arduino-buzzer/
  struct NoteToPlay
  {
    int note = 0;
    int duration = 4;
  };

  NoteToPlay someMelody[] = { {NOTE_C4, 4}, {NOTE_G3, 8}, {NOTE_G3, 8}, {NOTE_A3, 4}, {NOTE_G3, 4}, {NOTE_EMPTY, 4}, {NOTE_B3, 4}, {NOTE_C4, 4} };

  //Star Wars Main Theme
  NoteToPlay starWarMelody[] = {
  {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 650}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 350}, {NOTE_F5, 150}, {NOTE_C5, 500}, {NOTE_GS4, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 650}, {NOTE_EMPTY, 500},
  };

  void PlayMelody(const uint8_t &pin, const NoteToPlay* const melody)
  {
    const int melodySize = sizeof(melody) / sizeof(NoteToPlay);
    // iterate over the notes of the melody:

    for (int thisNote = 0; thisNote < melodySize; thisNote++) 
    {
      // to calculate the note duration, take one second divided by the note type.

      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.

      int noteDuration = 1000 / melody[thisNote].duration;

      tone(pin, melody[thisNote].note, noteDuration);

      // to distinguish the notes, set a minimum time between them.

      // the note's duration + 30% seems to work well:

      int pauseBetweenNotes = noteDuration * 1.30;

      delay(pauseBetweenNotes);

      // stop the tone playing:

      noTone(pin);
    }
  }  */
};

#endif //BUZZ_HELPER_H
