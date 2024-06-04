#pragma once
#ifndef PRAPORS_H
#define PRAPORS_H

#include "Config.h"

void SetRegionColor(const UARegion &region, const CRGB &color);

void fill_ua_prapor2()
{ 
  SetRegionColor(UARegion::Crimea,            CRGB::Yellow);
  SetRegionColor(UARegion::Khersonska,        CRGB::Yellow);
  SetRegionColor(UARegion::Zaporizka,         CRGB::Yellow);
  SetRegionColor(UARegion::Donetska,          CRGB::Yellow);
  SetRegionColor(UARegion::Dnipropetrovska,   CRGB::Yellow);
  SetRegionColor(UARegion::Mykolaivska,       CRGB::Yellow);
  SetRegionColor(UARegion::Odeska,            CRGB::Yellow);
  SetRegionColor(UARegion::Kirovohradska,     CRGB::Yellow);
  SetRegionColor(UARegion::Vinnytska,         CRGB::Yellow);
  SetRegionColor(UARegion::Khmelnitska,       CRGB::Yellow);
  SetRegionColor(UARegion::Chernivetska,      CRGB::Yellow);
  SetRegionColor(UARegion::Ivano_Frankivska,  CRGB::Yellow);
  SetRegionColor(UARegion::Ternopilska,       CRGB::Yellow);
  SetRegionColor(UARegion::Lvivska,           CRGB::Yellow);
  SetRegionColor(UARegion::Zakarpatska,       CRGB::Yellow);


  SetRegionColor(UARegion::Luhanska,          CRGB::Blue);
  SetRegionColor(UARegion::Kharkivska,        CRGB::Blue);
  SetRegionColor(UARegion::Poltavska,         CRGB::Blue);
  SetRegionColor(UARegion::Sumska,            CRGB::Blue);
  SetRegionColor(UARegion::Chernihivska,      CRGB::Blue);
  SetRegionColor(UARegion::Kyivska,           CRGB::Blue);
  SetRegionColor(UARegion::Cherkaska,         CRGB::Blue);
  SetRegionColor(UARegion::Zhytomyrska,       CRGB::Blue);
  SetRegionColor(UARegion::Rivnenska,         CRGB::Blue);
  SetRegionColor(UARegion::Volynska,          CRGB::Blue);

}

//Three Horizontal Colors
void fill_bg_prapor()
{ 
  //Bottom
  SetRegionColor(UARegion::Zakarpatska,       CRGB::Red);
  SetRegionColor(UARegion::Chernivetska,      CRGB::Red);
  SetRegionColor(UARegion::Odeska,            CRGB::Red);
  SetRegionColor(UARegion::Mykolaivska,       CRGB::Red);
  SetRegionColor(UARegion::Crimea,            CRGB::Red);
  SetRegionColor(UARegion::Khersonska,        CRGB::Red);
  SetRegionColor(UARegion::Zaporizka,         CRGB::Red);
  SetRegionColor(UARegion::Donetska,          CRGB::Red);
  

  //Center
  SetRegionColor(UARegion::Lvivska,           CRGB::Green);
  SetRegionColor(UARegion::Ternopilska,       CRGB::Green);
  SetRegionColor(UARegion::Ivano_Frankivska,  CRGB::Green);
  SetRegionColor(UARegion::Khmelnitska,       CRGB::Green);    
  SetRegionColor(UARegion::Vinnytska,         CRGB::Green);
  SetRegionColor(UARegion::Kirovohradska,     CRGB::Green);
  SetRegionColor(UARegion::Dnipropetrovska,   CRGB::Green);  
  SetRegionColor(UARegion::Luhanska,          CRGB::Green);
  SetRegionColor(UARegion::Kharkivska,        CRGB::Green);
  SetRegionColor(UARegion::Poltavska,         CRGB::Green);

  //Top
  SetRegionColor(UARegion::Sumska,            CRGB::White);
  SetRegionColor(UARegion::Chernihivska,      CRGB::White);
  SetRegionColor(UARegion::Kyivska,           CRGB::White);
  SetRegionColor(UARegion::Cherkaska,         CRGB::White);
  SetRegionColor(UARegion::Zhytomyrska,       CRGB::White);
  SetRegionColor(UARegion::Rivnenska,         CRGB::White);
  SetRegionColor(UARegion::Volynska,          CRGB::White);
}

//Three Vertical Colors
void fill_md_prapor()
{ 
  //Left  
  SetRegionColor(UARegion::Volynska,          CRGB::Blue);
  SetRegionColor(UARegion::Lvivska,           CRGB::Blue);
  SetRegionColor(UARegion::Zakarpatska,       CRGB::Blue);
  SetRegionColor(UARegion::Ivano_Frankivska,  CRGB::Blue);
  SetRegionColor(UARegion::Ternopilska,       CRGB::Blue);
  SetRegionColor(UARegion::Rivnenska,         CRGB::Blue);
  SetRegionColor(UARegion::Khmelnitska,       CRGB::Blue);
  SetRegionColor(UARegion::Chernivetska,      CRGB::Blue);

  //Center
  SetRegionColor(UARegion::Zhytomyrska,       CRGB::Yellow);
  SetRegionColor(UARegion::Vinnytska,         CRGB::Yellow);  
  SetRegionColor(UARegion::Odeska,            CRGB::Yellow);
  SetRegionColor(UARegion::Chernihivska,      CRGB::Yellow);
  SetRegionColor(UARegion::Kyivska,           CRGB::Yellow);
  SetRegionColor(UARegion::Cherkaska,         CRGB::Yellow);
  SetRegionColor(UARegion::Kirovohradska,     CRGB::Yellow);
  SetRegionColor(UARegion::Mykolaivska,       CRGB::Yellow);

  SetRegionColor(UARegion::Khersonska,        CRGB::Yellow);
  SetRegionColor(UARegion::Crimea,            CRGB::Yellow); 

  //Right
  SetRegionColor(UARegion::Zaporizka,         CRGB::Red);
  SetRegionColor(UARegion::Donetska,          CRGB::Red);
  SetRegionColor(UARegion::Dnipropetrovska,   CRGB::Red);
  SetRegionColor(UARegion::Luhanska,          CRGB::Red);
  SetRegionColor(UARegion::Kharkivska,        CRGB::Red);
  SetRegionColor(UARegion::Poltavska,         CRGB::Red);
  SetRegionColor(UARegion::Sumska,            CRGB::Red);
}

#endif //PRAPORS_H
