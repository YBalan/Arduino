#pragma once
#ifndef ALARMS_API_H
#define ALARMS_API_H

#include <Arduino.h>
#include <vector>
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

enum class AlarmsApiStatus
{
  OK,
  WRONG_API,
  NO_CONNECTION,
  NoWiFi,
  UNKNOWN,
};

class AlarmsApi
{
    std::unique_ptr<BearSSL::WiFiClientSecure> client;    
    
    //create an HTTPClient instance
    HTTPClient https;
    String lastActionIndex;
    String _apiKey;

  public:
    AlarmsApi() : client(new BearSSL::WiFiClientSecure){}
    AlarmsApi(const char* apiKey) : client(new BearSSL::WiFiClientSecure), _apiKey(apiKey) {}
    void setApiKey(const char *apiKey) { _apiKey = apiKey; }
    
    const bool IsStatusChanged(AlarmsApiStatus &status)
    {      
      auto res = sendRequest("alerts/status", _apiKey, status);
      if(status == AlarmsApiStatus::OK && res != "")
      {
        auto newActionIndex = ParseJsonLasActionIndex(res);
        bool isStatusChanged = lastActionIndex < newActionIndex;
        ALARMS_TRACE("LAST: ", lastActionIndex);
        ALARMS_TRACE(" NEW: ", newActionIndex);
        lastActionIndex = newActionIndex;
        return isStatusChanged;
      }

      return true;
    }

    std::vector<uint16_t> getAlarmedRegions(AlarmsApiStatus &status)
    {
      std::vector<uint16_t> res;
      auto httpRes = sendRequest("alerts", _apiKey, status);
      if(status == AlarmsApiStatus::OK && httpRes != "")
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
      return std::move(res);
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
          return last;
          //return atoi(json["lastActionIndex"]);
        }
        return "";
    }

    const String sendRequest(const String& resource, const String &apiKey, AlarmsApiStatus &status)
    {
      status = AlarmsApiStatus::UNKNOWN;
      // Ignore SSL certificate validation
      client->setInsecure();
      https.setTimeout(15000);

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
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE("[HTTPS] RES GET: ", resource, "... code: ", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
          {
            status = AlarmsApiStatus::OK;
            String payload = https.getString();            
            ALARMS_TRACE(payload);
            return payload;
          }
          else
          {
            status = AlarmsApiStatus::WRONG_API;
            auto error = https.errorToString(httpCode);
            ALARMS_TRACE("[HTTPS] GET: ", resource, "... failed, error: ", error);
            return error;
          }
        }
        else 
        {
          status = AlarmsApiStatus::WRONG_API;
          auto error = https.errorToString(httpCode);
          ALARMS_TRACE("[HTTPS] GET: ", resource, "... failed, error: ", error);
          return error;
        }

        https.end();
      } else 
      {
        status = AlarmsApiStatus::NO_CONNECTION;
        ALARMS_TRACE("[HTTPS] Unable to connect");
      }
      return "";
    }    
};

#endif //ALARMS_API_H