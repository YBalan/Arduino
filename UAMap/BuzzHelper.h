#pragma once
#ifndef BUZZ_HELPER_H
#define BUZZ_HELPER_H

#include "DEBUGHelper.h"
#include "CommonHelper.h"

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
      Siren(pin, totalTime, /*up:*/800, /*down:*/800, /*pause:*/500, /*period:*/1000);
    }
  }

  void AlarmEnd(const uint8_t &pin, const int16_t &totalTime = 3000)
  {    
    BUZZ_INFO(F("AlarmEnd"));
    if(totalTime > 0)
    {
      BUZZ_INFO(F("Time: "), totalTime);
      Siren(pin, totalTime, /*up:*/500, /*down:*/500, /*pause:*/500, /*period:*/1000);
    }
  }
  
  
  //https://www.arduino.cc/en/Tutorial/Tone
  //https://projecthub.arduino.cc/tmekinyan/playing-popular-songs-with-arduino-and-a-buzzer-546f4a
  //https://www.build-electronic-circuits.com/arduino-buzzer/

  #define NOTES_MINIMUM_DISTINGUISH_SET 1.20
  #define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))

  struct NoteToPlay
  {
    int note = 0;
    int duration = 4;
  };

  typedef std::vector<NoteToPlay> Melody;

  #ifdef USE_BUZZER_MELODIES

  #define pitchesMelodyStr F("262,4,196,8,196,8,220,4,196,4,0,4,247,4,262,4,")
  /*static const Melody pitchesMelody = { {NOTE_C4, 4}, {NOTE_G3, 8}, {NOTE_G3, 8}, {NOTE_A3, 4}, {NOTE_G3, 4}, {NOTE_EMPTY, 4}, {NOTE_B3, 4}, {NOTE_C4, 4} };*/

  //Star Wars Main Theme
  #define starWarMelodyStr F("440,500,0,500,440,500,0,500,440,500,0,500,349,350,523,150,440,500,0,500,349,350,523,150,440,650,0,500,659,500,0,500,659,500,0,500,659,500,0,350,698,150,523,500,415,500,349,350,523,150,440,650,0,500,")
  /*static const Melody starWarMelody = {
  {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 500}, {NOTE_EMPTY, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 650}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 500}, {NOTE_E5, 500}, {NOTE_EMPTY, 350}, {NOTE_F5, 150}, {NOTE_C5, 500}, {NOTE_GS4, 500}, {NOTE_F4, 350}, {NOTE_C5, 150}, {NOTE_A4, 650}, {NOTE_EMPTY, 500},
  };*/

  #define nokiaMelodyStr F("659,8,587,8,370,4,415,4,554,8,494,8,294,4,330,4,494,8,440,8,277,4,330,4,440,2,")
  /*static const Melody nokiaMelody = {
    {NOTE_E5, 8}, {NOTE_D5, 8}, {NOTE_FS4, 4}, {NOTE_GS4, 4}, 
    {NOTE_CS5, 8}, {NOTE_B4, 8}, {NOTE_D4, 4}, {NOTE_E4, 4}, 
    {NOTE_B4, 8}, {NOTE_A4, 8}, {NOTE_CS4, 4}, {NOTE_E4, 4},
    {NOTE_A4, 2}
  };*/

  #define happyBirthdayMelodyStr F("262,4,262,8,294,4,262,4,349,4,330,2,262,4,262,8,294,4,262,4,392,4,349,2,262,4,262,8,523,4,440,4,349,4,330,4,294,4,466,4,466,8,440,4,349,4,392,4,349,2,")
  /*static const Melody happyBirthdayMelody = {
    {NOTE_C4, 4}, {NOTE_C4, 8}, 
    {NOTE_D4, 4}, {NOTE_C4, 4}, {NOTE_F4, 4},
    {NOTE_E4, 2}, {NOTE_C4, 4}, {NOTE_C4, 8}, 
    {NOTE_D4, 4}, {NOTE_C4, 4}, {NOTE_G4, 4},
    {NOTE_F4, 2}, {NOTE_C4, 4}, {NOTE_C4, 8},
    
    {NOTE_C5, 4}, {NOTE_A4, 4}, {NOTE_F4, 4}, 
    {NOTE_E4, 4}, {NOTE_D4, 4}, {NOTE_AS4, 4}, {NOTE_AS4, 8},
    {NOTE_A4, 4}, {NOTE_F4, 4}, {NOTE_G4, 4},
    {NOTE_F4, 2},
  };  */

   /*static const Melody pacmanMelody = {
    {NOTE_B4, 16}, {NOTE_B5, 16}, {NOTE_FS5, 16}, {NOTE_DS5, 16},
    {NOTE_B5, 32}, {NOTE_FS5, 16}, {NOTE_DS5, 8}, {NOTE_C5, 16},
    {NOTE_C6, 16}, {NOTE_G6, 16}, {NOTE_E6, 16}, {NOTE_C6, 32}, {NOTE_G6, 16}, {NOTE_E6, 8},
    
    {NOTE_B4, 16}, {NOTE_B5, 16}, {NOTE_FS5, 16}, {NOTE_DS5, 16}, {NOTE_B5, 32},
    {NOTE_FS5, 16}, {NOTE_DS5, 8}, {NOTE_DS5, 32}, {NOTE_E5, 32}, {NOTE_F5, 32},
    {NOTE_F5, 32}, {NOTE_FS5, 32}, {NOTE_G5, 32}, {NOTE_G5, 32}, {NOTE_GS5, 32}, {NOTE_A5, 16}, {NOTE_B5, 8},
  };*/

  #define xmasMelodyStr F("659,8,659,8,659,4,659,8,659,8,659,4,659,8,784,8,523,8,587,8,659,2,698,8,698,8,698,8,698,8,698,8,659,8,659,8,659,16,659,16,659,8,587,8,587,8,659,8,587,4,784,4,")
  /*static const Melody xmasMelody = {
    {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 4},
    {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 4},
    {NOTE_E5, 8}, {NOTE_G5, 8}, {NOTE_C5, 8}, {NOTE_D5, 8},
    {NOTE_E5, 2},
    {NOTE_F5, 8}, {NOTE_F5, 8}, {NOTE_F5, 8}, {NOTE_F5, 8},
    {NOTE_F5, 8}, {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_E5, 16}, {NOTE_E5, 16},
    {NOTE_E5, 8}, {NOTE_D5, 8}, {NOTE_D5, 8}, {NOTE_E5, 8},
    {NOTE_D5, 4}, {NOTE_G5, 4},
  };*/
  #endif

  

  int MelodyLengthMs(const Melody &melody, const bool &showTrace = true)
  {
    int res = 0;   
    // iterate over the notes of the melody:
    
    for (int thisNote = 0; thisNote < melody.size(); thisNote++) 
    {
      int noteDuration = melody[thisNote].duration < 50 ? (1000 / melody[thisNote].duration) : melody[thisNote].duration;
      int pauseBetweenNotes = noteDuration * NOTES_MINIMUM_DISTINGUISH_SET;

      res += pauseBetweenNotes;
    }    

    if(showTrace)
      BUZZ_INFO(F("MelodyLengthMs: "), res, F("ms..."));
    return res;
  }

  const int PlayMelody(const uint8_t &pin, const Melody &melody, const float &distinguishFactor = NOTES_MINIMUM_DISTINGUISH_SET)
  { 
    BUZZ_INFO(F("Play Melody"));    
      // iterate over the notes of the melody:

    auto melodySizeMs = MelodyLengthMs(melody, /*showTrace:*/false);
    BUZZ_INFO(F("\t"), F("Size: "), melody.size(), F(" "), F("Length: "), melodySizeMs, F("ms..."));

    for (int thisNote = 0; thisNote < melody.size(); thisNote++) 
    {
      // to calculate the note duration, take one second divided by the note type.

      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.

      int noteDuration = melody[thisNote].duration < 50 ? (1000 / melody[thisNote].duration) : melody[thisNote].duration;

      BUZZ_TRACE(F("\tDuration"), noteDuration);

      tone(pin, melody[thisNote].note, noteDuration);

      // to distinguish the notes, set a minimum time between them.

      // the note's duration + NOTES_MINIMUM_DISTINGUISH_SET% seems to work well:

      int pauseBetweenNotes = noteDuration * distinguishFactor;

      BUZZ_TRACE(F("\tPause"), pauseBetweenNotes);

      delay(pauseBetweenNotes);

      // stop the tone playing:

      noTone(pin);

      yield(); // watchdog
    }    

    return melodySizeMs;
  }  

  const std::vector<NoteToPlay> GetMelody(const String &s)
  {
    const auto & tokens = CommonHelper::splitToInt(s, ',', '_');
    bool isOdd = tokens.size() % 2 != 0;
    int melodySize = (tokens.size() / 2) + (isOdd ? 1 : 0);

    std::vector<NoteToPlay> result(melodySize);

    int resultIdx = 0;
    for(int tokenIdx = 0; tokenIdx < tokens.size(); tokenIdx += 2)
    {
      result[resultIdx].note = tokens[tokenIdx];
      if(tokenIdx + 1 < tokens.size())
        result[resultIdx].duration = tokens[tokenIdx + 1];
      else
        result[resultIdx].duration = 50; //default duration

      resultIdx++;
    }

    return std::move(result);
  }

  const String GetMelodyString(const Melody &melody)
  {
    String result;
    for(const auto &n : melody)
    {
      result += String(n.note) + F(",") + String(n.duration) + F(",");
    }
    return std::move(result);
  }

  const int PlayMelody(const uint8_t &pin, const String &s, const float &distinguishFactor = NOTES_MINIMUM_DISTINGUISH_SET)
  {
    const auto &melody = GetMelody(s);
    // BUZZ_TRACE(F("Melody vector:"));    
    // for(const auto &n : melody)
    // {
    //   BUZZ_TRACE(F("\t"), n.note, F(":"), n.duration);
    // }
    return PlayMelody(pin, melody, distinguishFactor);
  }
  
};

#endif //BUZZ_HELPER_H
