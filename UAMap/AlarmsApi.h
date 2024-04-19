#include <memory>
#pragma once
#ifndef ALARMS_API_H
#define ALARMS_API_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <algorithm>
#include "StreamUtils.h"

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#include "DEBUGHelper.h"

#ifdef ENABLE_INFO_ALARMS
#define ALARMS_INFO(...) SS_TRACE("[ALARMS API INFO] ", __VA_ARGS__)
#else
#define ALARMS_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_ALARMS
#define ALARMS_TRACE(...) SS_TRACE("[ALARMS API TRACE] ", __VA_ARGS__)
#else
#define ALARMS_TRACE(...) {}
#endif

#define ALARMS_API_BASE_URI "https://api.ukrainealarm.com/api/v3/"
#define ALARMS_API_ALERTS "alerts"
#define ALARMS_API_REGIONS "regions"

struct RegionInfo
{
  String Name;
  uint16_t Id;
};

enum AlarmsApiStatus
{   
  API_OK = 200,
  WRONG_API_KEY = 401,  
  NO_CONNECTION = 1000,
  READ_TIMEOUT,
  NO_WIFI,  
  JSON_ERROR,
  UNKNOWN,
};

enum class UARegion : uint8_t
{
  Khmelnitska = 3,
  Vinnytska = 4,
  Rivnenska = 5,
  Volynska = 8,
  Dnipropetrovska = 9,
  Zhytomyrska = 10,
  Zakarpatska = 11,
  Zaporizka = 12,
  Ivano_Frankivska = 13,
  Kyivska = 14,
  Kirovohradska = 15,
  Luhanska = 16,
  Mykolaivska = 17,
  Odeska = 18,
  Poltavska = 18,
  Sumska = 19,
  Ternopilska =20,
  Kharkivska = 22,
  Khersonska = 23,
  Cherkaska = 24,
  Chernihivska = 25,
  Chernivetska = 26,
  Lvivska = 27,
  Donetska = 28,
  Krym = 127
};


#define MAX_LEDS_FOR_REGION 2
typedef std::array<uint8_t, MAX_LEDS_FOR_REGION> LedRange;
typedef std::map<UARegion, LedRange> AlarmsLedIndexesMap;
AlarmsLedIndexesMap alarmsLedIndexesMap =
{
  { UARegion::Krym,                 {0} },
  { UARegion::Khersonska,           {1} },
  { UARegion::Zaporizka,            {2} },
  { UARegion::Donetska,             {3} },
  { UARegion::Luhanska,             {4} },
  { UARegion::Kharkivska,           {5} },
  { UARegion::Dnipropetrovska,      {6} },
  { UARegion::Mykolaivska,          {7} },
  { UARegion::Odeska,               {8, 9} },
  { UARegion::Vinnytska,            {10} },
  { UARegion::Kirovohradska,        {11} },
  { UARegion::Poltavska,            {12} },
  { UARegion::Sumska,               {13} },
  { UARegion::Chernihivska,         {14} },
  { UARegion::Cherkaska,            {15} },
  { UARegion::Kyivska,              {16} },
  { UARegion::Zhytomyrska,          {17} },
  { UARegion::Khmelnitska,          {18} },
  { UARegion::Chernivetska,         {19} },
  { UARegion::Ternopilska,          {20} },
  { UARegion::Rivnenska,            {21} },
  { UARegion::Volynska,             {22} },
  { UARegion::Lvivska,              {23} },
  { UARegion::Ivano_Frankivska,     {24} },
  { UARegion::Zakarpatska,          {25} },
};

class AlarmsApi
{  
  private:
    static const constexpr uint8_t ledsMapCount = 25;  

    static const constexpr uint8_t alarmsLedsMap[] = 
    {
      127,  //Idx: 0 - "Crimea" //9999
      23,   //Idx: 1 - "Kherson"
      12,   //Idx: 2 - "Zap"
      28,   //Idx: 3 - "Don"
      16,   //Idx: 4 - "Luh"
      22,   //Idx: 5 - "Khar"
       9,   //Idx: 6 - "Dnipro"
      17,   //Idx: 7 - "Mykol"
      18,   //Idx: 8 - "Odesa"
       4,   //Idx: 9 - "Vinn"
      15,   //Idx: 10 - "Kirovograd"
      19,   //Idx: 11 - "Poltava"
      20,   //Idx: 12 - "Summ"
      25,   //Idx: 13 - "Chernihiv"
      24,   //Idx: 14 - "Cherkas"
      14,   //Idx: 15 - "Kyivska"
      10,   //Idx: 16 - "Gitom"
       3,   //Idx: 17 - "Khmel"
      26,   //Idx: 18 - "Chernivetska"
      21,   //Idx: 19 - "Ternopil"
       5,   //Idx: 20 - "Rivne"
       8,   //Idx: 21 - "Volin"
      27,   //Idx: 22 - "Lviv" 
      13,   //Idx: 23 - "Ivano-Frank"
      11,   //Idx: 24 - "Zakarpat"      
      //....
      31,   //Idx: 25 - "Kyiv"
    };

  private:
    std::unique_ptr<BearSSL::WiFiClientSecure> client;    
    std::unique_ptr<HTTPClient> https2;
    //create an HTTPClient instance
    //HTTPClient https;
    String lastActionIndex;
    String _apiKey;

  public:
    AlarmsApi() : client(new BearSSL::WiFiClientSecure), https2(new HTTPClient){}
    AlarmsApi(const char* apiKey) : client(new BearSSL::WiFiClientSecure), https2(new HTTPClient), _apiKey(apiKey) {}

    void setApiKey(const char *const apiKey) { _apiKey = apiKey; }
    void setApiKey(const String &apiKey) { _apiKey = apiKey; }
    
    const bool IsStatusChanged(int &status, String &statusMsg)
    {      
      auto httpRes = sendRequest("alerts/status", _apiKey, status, /*closeHttp:*/false);
      if(status == AlarmsApiStatus::API_OK)
      {
        auto newActionIndex = ParseJsonLasActionIndex(httpRes);
        bool isStatusChanged = lastActionIndex < newActionIndex;
        ALARMS_TRACE("LAST: ", lastActionIndex);
        ALARMS_TRACE(" NEW: ", newActionIndex);
        lastActionIndex = newActionIndex;
        return isStatusChanged;
      }
     
      statusMsg = status != AlarmsApiStatus::API_OK ? httpRes : "";
      https2->end();
      return true;
    }

    const std::vector<uint16_t> getAlarmedRegions(int &status, String &statusMsg)
    {
      std::vector<uint16_t> res;
      auto httpRes = sendRequest("alerts", _apiKey, status, /*closeHttp:*/false);
      if(status == AlarmsApiStatus::API_OK)
      {
        DynamicJsonDocument doc(httpRes.length());
        auto deserializeError = deserializeJson(doc, httpRes);
        //serializeJson(doc, Serial);
        if (!deserializeError) 
        {
          JsonArray arr = doc.as<JsonArray>();

          for (JsonVariant value : arr) {
            
            if(String(value["regionType"]) == "State")
            {
              ALARMS_TRACE(String(value));
              res.push_back(String(value["regionId"]).toInt());
            }
          }
        }
        else
        {          
          ALARMS_TRACE("[JSON] Deserialization error: ", deserializeError.c_str());
        }
      }
      statusMsg = status != AlarmsApiStatus::API_OK ? httpRes : "";
      https2->end();
      return std::move(res);
    }    

    public:
    const String ParseJsonLasActionIndex(const String &httpRes)
    {      
        DynamicJsonDocument json(38);
        auto deserializeError = deserializeJson(json, httpRes);
        //serializeJson(json, Serial);
        if (!deserializeError) 
        {
          String last(json["lastActionIndex"]);
          //ALARMS_TRACE("LAST: ", last);
          return std::move(last);
          //return atoi(json["lastActionIndex"]);
        }        
        ALARMS_TRACE("[JSON] Deserialization error: ", deserializeError.c_str());
        return "";
    }

    const std::map<uint16_t, RegionInfo> getAlarmedRegions2(int &status, String &statusMsg, const String &resource = ALARMS_API_ALERTS)
    {
      std::map<uint16_t, RegionInfo> res;
      status = AlarmsApiStatus::UNKNOWN;
      //HTTPClient https2;
      //BearSSL::WiFiClientSecure client2;      

      https2->setTimeout(3000);
      client->setTimeout(3000);
      //client2.setInsecure();
      client->setInsecure();
      https2->useHTTP10(true);
      https2->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      
      ALARMS_TRACE("[HTTPS2] begin: ", resource);   
      ALARMS_TRACE("[HTTPS2] apiKey: ", _apiKey); 
      if (https2->begin(*client, ALARMS_API_BASE_URI + resource)) 
      { // HTTPS        
        https2->addHeader("Authorization", _apiKey);        
        https2->addHeader("Accept", "application/json");      

        ALARMS_TRACE("[HTTPS2] REQ GET: ", resource);
        // start connection and send HTTP header
        int httpCode = https2->GET();
        // httpCode will be negative on error
        if (httpCode > 0) 
        {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE("[HTTPS2] RES GET: ", resource, "... code: ", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
          {
           // const String &p = https2->getString();
            ALARMS_TRACE(" HEAP: ", ESP.getFreeHeap());
            ALARMS_TRACE("STACK: ", ESP.getFreeContStack());
            ALARMS_TRACE("[HTTPS2] content-length: ", https2->getSize());
            status = AlarmsApiStatus::API_OK;
            DynamicJsonDocument doc(2048);
            
            ALARMS_TRACE("[HTTPS2] Full Json content: "); //https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
            ReadLoggingStream loggingStream(https2->getStream(), Serial);        
            
            DeserializationError deserializeError;

            if(resource == ALARMS_API_ALERTS ? loggingStream.find("[") : loggingStream.find("\"states\":["))
            {
              Serial.println();
              StaticJsonDocument<32> filter;
              filter["regionType"] = true;
              filter["regionId"] = true;
              filter["regionName"] = true;
              filter["regionEngName"] = true;

              ALARMS_TRACE("[HTTPS2] Start Parse Json content");             
              do
              {
                deserializeError = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
                if(!deserializeError)
                {
                  if(String(doc["regionType"]) == "State")
                  {
                    ALARMS_TRACE(doc.as<const char*>());    

                    const char *engNamePtr = doc["regionEngName"].as<const char*>();                

                    RegionInfo rInfo;
                    rInfo.Id = atoi(doc["regionId"].as<const char*>());                  
                    rInfo.Name = engNamePtr != 0 && strlen(engNamePtr) > 0 ? engNamePtr : doc["regionName"].as<const char*>();

                    res[rInfo.Id] = rInfo;
                    ALARMS_TRACE("\tRegion: ID: ", rInfo.Id, " Name: ", rInfo.Name);
                  }
                }
                else
                {
                  ALARMS_TRACE("[JSON] Deserialization error: ", deserializeError.c_str());
                }
              }while(loggingStream.findUntil(",","]"));
            }
            ALARMS_TRACE("[HTTPS2] End Parse Json content: ", deserializeError.c_str());

            ALARMS_TRACE(" HEAP: ", ESP.getFreeHeap());
            ALARMS_TRACE("STACK: ", ESP.getFreeContStack());
            /*auto deserializeError = deserializeJson(doc, *client);
            //serializeJson(doc, Serial);
            if (!deserializeError) 
            {
              JsonArray arr = resource == ALARMS_API_ALERTS ? doc.as<JsonArray>() : doc["states"].as<JsonArray>();  

              ALARMS_TRACE("\tJson response size: ", arr.size());            
              
              for (const JsonVariant &value : arr) 
              {                
                if(String(value["regionType"]) == "State")
                {
                  //ALARMS_TRACE(value.as<const char*>());

                  RegionInfo rInfo;
                  rInfo.Id = atoi(value["regionId"].as<const char*>());                  
                  rInfo.Name = value["regionName"].as<const char*>();

                  res[rInfo.Id] = rInfo;
                  ALARMS_TRACE("\tRegion: ID: ", rInfo.Id, " Name: ", rInfo.Name);
                }
              }                            
            }
            else
            {          
              ALARMS_TRACE("[JSON] Deserialization error: ", deserializeError.c_str());
              httpCode = AlarmsApiStatus::JSON_ERROR;
              statusMsg = String("Json: ") + deserializeError.c_str();
            }*/
          }          
        }

        status = httpCode;
        switch(httpCode)
        {
          case AlarmsApiStatus::API_OK:
            ALARMS_TRACE("[HTTPS2] GET: OK");
            statusMsg = "OK (200)";
            break;
          case HTTP_CODE_UNAUTHORIZED:
            statusMsg = String(https2->errorToString(httpCode)) + "(" + httpCode + ")";
            status = AlarmsApiStatus::WRONG_API_KEY;
            break;
          case HTTPC_ERROR_READ_TIMEOUT:
            statusMsg = String(https2->errorToString(httpCode)) + "(" + httpCode + ")";
            status = AlarmsApiStatus::READ_TIMEOUT;
            break;       
          case HTTPC_ERROR_CONNECTION_FAILED:
            statusMsg = String(https2->errorToString(httpCode)) + "(" + httpCode + ")";
            status = AlarmsApiStatus::NO_CONNECTION;
            break;
          case AlarmsApiStatus::JSON_ERROR:
            status = AlarmsApiStatus::JSON_ERROR;
            //statusMsg = "Json error";
            break;
          default:
            
            break;
        }

        ALARMS_TRACE("[HTTPS2] GET: ", resource, "... status message: ", statusMsg);
        
        https2->end();
           
        return std::move(res);
        
      } else 
      {
        status = AlarmsApiStatus::UNKNOWN;
        ALARMS_TRACE("[HTTPS] Unable to connect");
        statusMsg = "Unknown";
        return std::move(res);
      }      
    }

    const String sendRequest(const String& resource, const String &apiKey, int &status, const bool &closeHttp)
    {
      status = AlarmsApiStatus::UNKNOWN;
      // Ignore SSL certificate validation
      client->setInsecure();
      
      //HTTPClient https;
      https2->setTimeout(3000);      
      //https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

      ALARMS_TRACE("[HTTPS] begin: ", resource);   
      ALARMS_TRACE("[HTTPS] apiKey: ", apiKey); 
      if (https2->begin(*client, ALARMS_API_BASE_URI + resource)) 
      {  // HTTPS
        String payload;
        https2->addHeader("Authorization", apiKey);        
        https2->addHeader("Accept", "application/json");      

        ALARMS_TRACE("[HTTPS] REQ GET: ", resource);
        // start connection and send HTTP header
        int httpCode = https2->GET();
        // httpCode will be negative on error
        if (httpCode > 0) 
        {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE("[HTTPS] RES GET: ", resource, "... code: ", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
          {
            ALARMS_TRACE(" HEAP: ", ESP.getFreeHeap());
            ALARMS_TRACE("STACK: ", ESP.getFreeContStack());
            status = AlarmsApiStatus::API_OK;
            payload = https2->getString();  

            ALARMS_TRACE("[HTTPS] content-length: ", https2->getSize());          
            //ALARMS_TRACE("[HTTPS] content: ", payload);
           
            //return "Ok";
          }          
        }

        status = httpCode;
        switch(httpCode)
        {
          case HTTP_CODE_UNAUTHORIZED:
            status = AlarmsApiStatus::WRONG_API_KEY;
            break;
          case HTTPC_ERROR_READ_TIMEOUT:
            status = AlarmsApiStatus::READ_TIMEOUT;
            break;       
          case HTTPC_ERROR_CONNECTION_FAILED:
            status = AlarmsApiStatus::NO_CONNECTION;
            break;
        }

        if(closeHttp)
          https2->end();

        String httpResultMsg = "OK (200)";
        if(status == AlarmsApiStatus::API_OK)
        {
          ALARMS_TRACE("[HTTPS] GET: OK");
          return std::move(payload);
        }
        else
        {
          httpResultMsg = String(https2->errorToString(httpCode)) + "(" + httpCode + ")";
          ALARMS_TRACE("[HTTPS] GET: ", resource, "... failed, error: ", httpResultMsg);
          return std::move(httpResultMsg);
        }
      } else 
      {
        status = AlarmsApiStatus::UNKNOWN;
        ALARMS_TRACE("[HTTPS] Unable to connect");
        return std::move("[HTTPS] Unable to connect");
      }      
    }   

    public:
    const int8_t getLedIndexByRegionId(const uint16_t &regionId) const
    {
      int8_t res = -1;
      const int alarmsLedsMapLength = sizeof(alarmsLedsMap) / sizeof(alarmsLedsMap[0]);
      ALARMS_INFO("LED Map Count: ", alarmsLedsMapLength)
      for(uint8_t idx = 0; idx < alarmsLedsMapLength; idx++)
      {
        auto mapValue = alarmsLedsMap[idx];
        if(mapValue == (regionId == 9999 ? 127 : regionId))
        {
          res = idx;
          break;
        }
      }
      ALARMS_INFO("regionId: ", regionId, " mapped to idx: ", res);
      return res;
    } 

    static const std::vector<uint8_t> getLedIndexesByRegionId(const uint16_t &regionId)
    {       
      auto region = (UARegion)(regionId == 9999 ? 127 : regionId);

      return getLedIndexesByRegion(region);
    }   

    static const std::vector<uint8_t> getLedIndexesByRegion(const UARegion &region) 
    {
      std::vector<uint8_t> res;
      ALARMS_INFO("LED Map Count: ", alarmsLedIndexesMap.size());

      if(alarmsLedIndexesMap.count(region) > 0)
      {
        const auto &ledRange = alarmsLedIndexesMap[region];        
        for(uint8_t i = 0; i < ledRange.size(); i++)
        {
          if(i == 0 || ledRange[i] > 0)
          {
            res.push_back(ledRange[i]);
            ALARMS_INFO("regionId: ", regionId, " mapped to idx: ", ledRange[i]);      
          }
        }        
      }
      return std::move(res);
    }
};

#endif //ALARMS_API_H