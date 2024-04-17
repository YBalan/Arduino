#pragma once
#ifndef ALARMS_API_H
#define ALARMS_API_H

#include <Arduino.h>
#include <vector>
#include <algorithm>

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

enum AlarmsApiStatus
{   
  API_OK = 200,
  WRONG_API_KEY = 401,  
  NO_CONNECTION = 1000,
  READ_TIMEOUT,
  NO_WIFI,  
  UNKNOWN,
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
       9,   //Idx: 6 - "Dnopro"
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
    
    //create an HTTPClient instance
    HTTPClient https;
    String lastActionIndex;
    String _apiKey;

  public:
    AlarmsApi() : client(new BearSSL::WiFiClientSecure){}
    AlarmsApi(const char* apiKey) : client(new BearSSL::WiFiClientSecure), _apiKey(apiKey) {}
    void setApiKey(const char *const apiKey) { _apiKey = apiKey; }
    void setApiKey(const String &apiKey) { _apiKey = apiKey; }
    
    const bool IsStatusChanged(int &status, String &statusMsg)
    {      
      auto httpRes = sendRequest("alerts/status", _apiKey, status);
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
      return true;
    }

    const std::vector<uint16_t> getAlarmedRegions(int &status, String &statusMsg)
    {
      std::vector<uint16_t> res;
      auto httpRes = sendRequest("alerts", _apiKey, status);
      if(status == AlarmsApiStatus::API_OK)
      {
        DynamicJsonDocument doc(2048);
        auto deserializeError = deserializeJson(doc, httpRes);
        //serializeJson(doc, Serial);
        if ( ! deserializeError ) 
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
      }
      statusMsg = status != AlarmsApiStatus::API_OK ? httpRes : "";
      return std::move(res);
    }

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

    private:
    static const String ParseJsonLasActionIndex(const String &buf)
    {      
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf);
        //serializeJson(json, Serial);
        if ( ! deserializeError ) {
          String last(json["lastActionIndex"]);
          //ALARMS_TRACE("LAST: ", last);
          return std::move(last);
          //return atoi(json["lastActionIndex"]);
        }
        return "";
    }

    const String sendRequest(const String& resource, const String &apiKey, int &status)
    {
      status = AlarmsApiStatus::UNKNOWN;
      // Ignore SSL certificate validation
      client->setInsecure();
      
      https.setTimeout(2000);

      ALARMS_TRACE("[HTTPS] begin: ", resource);   
      ALARMS_TRACE("[HTTPS] apiKey: ", apiKey); 
      if (https.begin(*client, ALARMS_API_BASE_URI + resource)) 
      {  // HTTPS
        
        https.addHeader("Authorization", apiKey);
        //https.addHeader("Authorization", "81ac3496:23fb7b9afd1eca3fd65cfb38ecb36954");
        https.addHeader("Accept", "application/json");      

        //https.setAuthorization("81ac3496:23fb7b9afd1eca3fd65cfb38ecb36954");
        ALARMS_TRACE("[HTTPS] REQ GET: ", resource);
        // start connection and send HTTP header
        int httpCode = https.GET();
        // httpCode will be negative on error
        if (httpCode > 0) 
        {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE("[HTTPS] RES GET: ", resource, "... code: ", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
          {
            status = AlarmsApiStatus::API_OK;
            String payload = https.getString();            
            ALARMS_TRACE(payload);
            https.end();
            return std::move(payload);
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
        auto error = String(https.errorToString(httpCode)) + "(" + httpCode + ")";
        ALARMS_TRACE("[HTTPS] GET: ", resource, "... failed, error: ", error);
        https.end();

        return std::move(error);                
      } else 
      {
        status = AlarmsApiStatus::UNKNOWN;
        ALARMS_TRACE("[HTTPS] Unable to connect");
        return std::move("[HTTPS] Unable to connect");
      }      
    }    
};

#endif //ALARMS_API_H