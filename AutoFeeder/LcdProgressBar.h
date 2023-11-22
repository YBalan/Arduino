#pragma once
#ifndef LCD_PROGRESS_BAR_H
#define LCD_PROGRESS_BAR_H

#include <LiquidCrystal_I2C.h>

#define LCD_PROGRESS_BAR_TRACE_NAME "PBar: "

namespace Helpers
{
  enum LcdProgressCommands : short
  {
    Init = 4,
    Clear = 5,
  };

  enum class LcdProgressSettings : short
  {
    NUMBERS_OFF,
    NUMBERS_RIGHT,
    NUMBERS_LEFT,
    NUMBERS_CENTER,
  };

  class LcdProgressBar : public Print
  {
    LiquidCrystal_I2C * const _lcd = 0;
    
    short _row;
    short _startPos;
    short _maxSymbolsCount;

    LcdProgressSettings _settings;

    uint8_t bar0[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; 
    uint8_t bar1[8] = { 0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10 };
    uint8_t bar2[8] = { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 };
    uint8_t bar3[8] = { 0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C };
    uint8_t bar4[8] = { 0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E };
    uint8_t bar5[8] = { 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F };    

  public:
    LcdProgressBar(const LiquidCrystal_I2C &lcd, const short &row = 1, const short &startPos = 0, const short &maxSymbolsCount = 16, const LcdProgressSettings &settings = LcdProgressSettings::NUMBERS_CENTER) 
      : _lcd(&lcd)
      , _row(row)
      , _startPos(startPos)
      , _maxSymbolsCount(maxSymbolsCount)
      , _settings(settings)      
    { }

    void ShowProgress(const short &value)
    {
      ShowProgress(_lcd, value, _row, _startPos, _maxSymbolsCount, _settings);
    }
    
    void ShowProgress(const short &value, const short &row, const short &startPos, const short &maxSymbolsCount, const LcdProgressSettings &settings)
    {
      ShowProgress(_lcd, value, row, startPos, maxSymbolsCount, settings);
    }  

  private: //virtuals
    virtual size_t write(uint8_t value)
    {
      ShowProgress(value);
      return 1;
    }

    virtual size_t write(const uint8_t *buffer, size_t size)
    { 
      if(_lcd != 0)
      {
        if(size == LcdProgressCommands::Init)
        {
          S_INFO2(LCD_PROGRESS_BAR_TRACE_NAME, "Init");

          _lcd->createChar(0, bar0);
          _lcd->createChar(1, bar1);
          _lcd->createChar(2, bar2);
          _lcd->createChar(3, bar3);
          _lcd->createChar(4, bar4);
          _lcd->createChar(5, bar5);
        }else
        if(size == LcdProgressCommands::Clear)
        {
          S_INFO2(LCD_PROGRESS_BAR_TRACE_NAME, "Clear");

          _lcd->setCursor(_startPos, _row);
          for(short ch = _startPos; ch < _maxSymbolsCount; ch++) 
          {
            _lcd->setCursor(ch, _row);
            _lcd->write(0);
          }  
        }
      }
      return 1;
    }

  private:
    static void ShowProgress(LiquidCrystal_I2C * const lcd, const short &value, const short &row, const short &startPos, const short &maxSymbolsCount, const LcdProgressSettings &settings)
    {
      S_INFO3(LCD_PROGRESS_BAR_TRACE_NAME, value, "%");

      if(lcd != 0)
      {
        short segment = map(value, 0, 100, 0, (6 * maxSymbolsCount) - 1);
        short symbol = segment / 6;
        lcd->setCursor(symbol, row);
        lcd->write(segment % 6);
        
        short prevSymbolToFill = symbol - 1;
        if(prevSymbolToFill >= 0 && prevSymbolToFill < maxSymbolsCount)
        {
          lcd->setCursor(prevSymbolToFill, row);
          lcd->write(5);
        }
        
        if(settings != LcdProgressSettings::NUMBERS_OFF)
        {
          lcd->setCursor(startPos, row);
          switch(settings)
          {
            case LcdProgressSettings::NUMBERS_LEFT:
              //lcd->setCursor(startCursorPos, row);
              break;
            case LcdProgressSettings::NUMBERS_RIGHT:
              if(maxSymbolsCount >= 4)
                lcd->setCursor(maxSymbolsCount - 4 + (value == 100 ? 0 : 1), row);              
              break;
            case LcdProgressSettings::NUMBERS_CENTER:
            default:
              if(maxSymbolsCount >= 4)
                lcd->setCursor((maxSymbolsCount / 2) - (value == 100 ? 2 : 1), row);
              break;
          }
          
          char pctNumbers[5];
          sprintf(pctNumbers, "%2d%%", value);
          lcd->print(pctNumbers);
        }

        return;
      }      
    }
  };
}

#endif //LCD_PROGRESS_BAR_H