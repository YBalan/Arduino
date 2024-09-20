#pragma once
#ifndef NSTAT_H
#define NSTAT_H

#include <map>
#include "ESPHelper.h"

#ifdef NETWORK_STATISTIC
struct NetworkStatInfo{ int code; int count; String description; };
std::map<int, NetworkStatInfo> networkStat;
#endif

void FillNetworkStat(const int& code, const String &desc)
{
  #ifdef NETWORK_STATISTIC
  networkStat[code].count++;
  networkStat[code].code = code;
  if(networkStat[code].description == "")
    networkStat[code].description = desc;
  #endif
}

#ifdef NETWORK_STATISTIC  
void PrintNetworkStatInfoToSerial(const NetworkStatInfo &info)
{  
  Serial.print(F("[\"")); Serial.print(info.description); Serial.print(F("\": ")); Serial.print(info.count); Serial.print(F("]; "));   
}
#endif

void PrintNetworkStatToSerial()
{
  #ifdef NETWORK_STATISTIC
  Serial.print(F("Network Statistic: "));
  if(networkStat.size() > 0)
  {
    if(networkStat.count(200) > 0)
      PrintNetworkStatInfoToSerial(networkStat[200]);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != 200)
        PrintNetworkStatInfoToSerial(info);
    }
  }
  const auto &millisec = millis();
  String lifetime = (networkStat.size() > 0 ? String(F(" ")) : String(F("")))
      + (millisec >= 60000 ? String(millisec / 60000) + String(F("min")) : String(millisec / 1000) + String(F("sec")));
  Serial.println(lifetime);

  int resetReason = 0;
  String rst = String(F(" Rst:[")) + GetResetReason(resetReason) + F(" ") + F("(") + String(resetReason) + F(")") + F("]");
  Serial.println(rst);
  #endif
}

void PrintNetworkStatInfo(const NetworkStatInfo &info, String &str)
{  
  str += String(F("[\"")) + info.description + F("\": ") + String(info.count) + F("]; ");
}

void PrintNetworkStatistic(String &str, const int& codeFilter)
{
  str = F("NSTAT: ");
  #ifdef NETWORK_STATISTIC  
  if(networkStat.size() > 0)
  {
    if(networkStat.count(200) > 0 && (codeFilter == 0 || codeFilter == 200))
      PrintNetworkStatInfo(networkStat[200], str);
    for(const auto &de : networkStat)
    {
      const auto &info = de.second;
      if(info.code != 200 && (codeFilter == 0 || codeFilter == info.code))
        PrintNetworkStatInfo(info, str);
    }
  }
  const auto &millisec = millis();
  str += (networkStat.size() > 0 ? String(F(" ")) : String(F("")))
      + (millisec >= 60000 ? String(millisec / 60000) + String(F("min")) : String(millisec / 1000) + String(F("sec")));
      
  int resetReason = 0;
  str += String(F(" Rst:[")) + GetResetReason(resetReason) + F(" ") + F("(") + String(resetReason) + F(")") + F("]");

  #else
  str += F("Off");
  #endif
}

#endif //NSTAT_H