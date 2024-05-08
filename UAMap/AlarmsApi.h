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

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#ifndef FB_NO_OTA
#include <ESP8266httpUpdate.h>
#endif
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#else   // ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#ifndef FB_NO_OTA
#include <HTTPUpdate.h>
#endif
#include <WiFiClientSecure.h>
#endif

#include "DEBUGHelper.h"
#include "Config.h"

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

#define ALARMS_API_OFFICIAL_BASE_URI "https://api.ukrainealarm.com/api/v3/"
#define ALARMS_API_OFFICIAL_ALERTS "alerts"
#define ALARMS_API_OFFICIAL_REGIONS "regions"

#define ALARMS_API_IOT_BASE_URI "https://api.alerts.in.ua/"
#define ALARMS_API_IOT_ALERTS "v1/iot/active_air_raid_alerts_by_oblast.json"

enum ApiAlarmStatus : uint8_t
{
  Alarmed,
  NotAlarmed,
  PartialAlarmed,
};

struct RegionInfo
{
  String Name;  
  UARegion Id;
  ApiAlarmStatus AlarmStatus;
};

enum ApiStatusCode
{   
  API_OK = 200,
  WRONG_API_KEY = 401,  
  NO_CONNECTION = 1000,
  READ_TIMEOUT,
  NO_WIFI,  
  JSON_ERROR,
  UNKNOWN,
};

#define MAX_REGIONS_COUNT 27
typedef RegionInfo IotApiRegions[MAX_REGIONS_COUNT];
//typedef std::array<RegionInfo, MAX_REGIONS_COUNT> IotApiRegions;

class AlarmsApi
{  
public:  
  #ifdef LANGUAGE_UA
IotApiRegions iotApiRegions = 
{
  {"Автономна Республіка Крим",  UARegion::Crimea,            ApiAlarmStatus::NotAlarmed},          //0
  {"Волинська область",          UARegion::Volynska,          ApiAlarmStatus::NotAlarmed},          //1
  {"Вінницька область",          UARegion::Vinnytska,         ApiAlarmStatus::NotAlarmed},          //2
  {"Дніпропетровська область",   UARegion::Dnipropetrovska,   ApiAlarmStatus::NotAlarmed},          //3
  {"Донецька область",           UARegion::Donetska,          ApiAlarmStatus::NotAlarmed},          //4
  {"Житомирська область",        UARegion::Zhytomyrska,       ApiAlarmStatus::NotAlarmed},          //5
  {"Закарпатська область",       UARegion::Zakarpatska,       ApiAlarmStatus::NotAlarmed},          //6
  {"Запорізька область",         UARegion::Zaporizka,         ApiAlarmStatus::NotAlarmed},          //7
  {"Івано-Франківська область",  UARegion::Ivano_Frankivska,  ApiAlarmStatus::NotAlarmed},          //8
  {"м. Київ",                    UARegion::Kyiv,              ApiAlarmStatus::NotAlarmed},          //9
  {"Київська область",           UARegion::Kyivska,           ApiAlarmStatus::NotAlarmed},          //10
  {"Кіровоградська область",     UARegion::Kirovohradska,     ApiAlarmStatus::NotAlarmed},          //11
  {"Луганська область",          UARegion::Luhanska,          ApiAlarmStatus::NotAlarmed},          //12
  {"Львівська область",          UARegion::Lvivska,           ApiAlarmStatus::NotAlarmed},          //13
  {"Миколаївська область",       UARegion::Mykolaivska,       ApiAlarmStatus::NotAlarmed},          //14
  {"Одеська область",            UARegion::Odeska,            ApiAlarmStatus::NotAlarmed},          //15
  {"Полтавська область",         UARegion::Poltavska,         ApiAlarmStatus::NotAlarmed},          //16
  {"Рівненська область",         UARegion::Rivnenska,         ApiAlarmStatus::NotAlarmed},          //17
  {"м. Севастополь",             UARegion::Sevastopol,        ApiAlarmStatus::NotAlarmed},          //18
  {"Сумська область",            UARegion::Sumska,            ApiAlarmStatus::NotAlarmed},          //19
  {"Тернопільська область",      UARegion::Ternopilska,       ApiAlarmStatus::NotAlarmed},          //20
  {"Харківська область",         UARegion::Kharkivska,        ApiAlarmStatus::NotAlarmed},          //21
  {"Херсонська область",         UARegion::Khersonska,        ApiAlarmStatus::NotAlarmed},          //22
  {"Хмельницька область",        UARegion::Khmelnitska,       ApiAlarmStatus::NotAlarmed},          //23
  {"Черкаська область",          UARegion::Cherkaska,         ApiAlarmStatus::NotAlarmed},          //24
  {"Чернівецька область",        UARegion::Chernivetska,      ApiAlarmStatus::NotAlarmed},          //25
  {"Чернігівська область",       UARegion::Chernihivska,      ApiAlarmStatus::NotAlarmed},          //26
};
#else
IotApiRegions iotApiRegions = 
{
  {"Crimea",                      UARegion::Crimea,            ApiAlarmStatus::NotAlarmed},          //0
  {"Volynska",                    UARegion::Volynska,          ApiAlarmStatus::NotAlarmed},          //1
  {"Vinnytska",                   UARegion::Vinnytska,         ApiAlarmStatus::NotAlarmed},          //2
  {"Dnipropetrovska",             UARegion::Dnipropetrovska,   ApiAlarmStatus::NotAlarmed},          //3
  {"Donetska",                    UARegion::Donetska,          ApiAlarmStatus::NotAlarmed},          //4
  {"Zhytomyrska",                 UARegion::Zhytomyrska,       ApiAlarmStatus::NotAlarmed},          //5
  {"Zakarpatska",                 UARegion::Zakarpatska,       ApiAlarmStatus::NotAlarmed},          //6
  {"Zaporizka",                   UARegion::Zaporizka,         ApiAlarmStatus::NotAlarmed},          //7
  {"Ivano-Frankivska",            UARegion::Ivano_Frankivska,  ApiAlarmStatus::NotAlarmed},          //8
  {"Kyiv",                        UARegion::Kyiv,              ApiAlarmStatus::NotAlarmed},          //9
  {"Kyivska",                     UARegion::Kyivska,           ApiAlarmStatus::NotAlarmed},          //10
  {"Kirovohradska",               UARegion::Kirovohradska,     ApiAlarmStatus::NotAlarmed},          //11
  {"Luhanska",                    UARegion::Luhanska,          ApiAlarmStatus::NotAlarmed},          //12
  {"Lvivska",                     UARegion::Lvivska,           ApiAlarmStatus::NotAlarmed},          //13
  {"Mykolaivska",                 UARegion::Mykolaivska,       ApiAlarmStatus::NotAlarmed},          //14
  {"Odeska",                      UARegion::Odeska,            ApiAlarmStatus::NotAlarmed},          //15
  {"Poltavska",                   UARegion::Poltavska,         ApiAlarmStatus::NotAlarmed},          //16
  {"Rivnenska",                   UARegion::Rivnenska,         ApiAlarmStatus::NotAlarmed},          //17
  {"Sevastopol",                  UARegion::Sevastopol,        ApiAlarmStatus::NotAlarmed},          //18
  {"Sumska",                      UARegion::Sumska,            ApiAlarmStatus::NotAlarmed},          //19
  {"Ternopilska",                 UARegion::Ternopilska,       ApiAlarmStatus::NotAlarmed},          //20
  {"Kharkivska",                  UARegion::Kharkivska,        ApiAlarmStatus::NotAlarmed},          //21
  {"Khersonska",                  UARegion::Khersonska,        ApiAlarmStatus::NotAlarmed},          //22
  {"Khmelnitska",                 UARegion::Khmelnitska,       ApiAlarmStatus::NotAlarmed},          //23
  {"Cherkaska",                   UARegion::Cherkaska,         ApiAlarmStatus::NotAlarmed},          //24
  {"Chernivetska",                UARegion::Chernivetska,      ApiAlarmStatus::NotAlarmed},          //25
  {"Chernihivska",                UARegion::Chernihivska,      ApiAlarmStatus::NotAlarmed},          //26
};
#endif 
  public: 
  // // Iterator to the beginning of the custom vector
  //   const auto &begin() const {
  //       return std::begin(iotApiRegions);
  //   }

  //   // Iterator to the end of the custom vector
  //   const auto &end() const {
  //       return std::end(iotApiRegions);
  //   }

    const String GetRegionNameById(const UARegion &id)
    {
      for(const auto &r : iotApiRegions)
      {
        if(r.Id == id)
          return std::move(r.Name);
      }
      return F("");
    }
    RegionInfo* const GetRegionById(const UARegion &id)
    {
      for(auto &r : iotApiRegions)
      {
        if(r.Id == id)
          return &r;
      }
      return nullptr;
    }
  private:    
    std::unique_ptr<HTTPClient> https2;    
    String lastActionIndex;
    String _apiKey;
    String _uriBase;
    bool _isOfficialApi = false;

  public:
    AlarmsApi() : /*client(new BearSSL::WiFiClientSecure),*/ https2(new HTTPClient){}
    AlarmsApi(const char* apiKey) : /*client(new BearSSL::WiFiClientSecure),*/ https2(new HTTPClient), _apiKey(apiKey) {}

    void setApiKey(const char *const apiKey) { _apiKey = apiKey; }
    void setApiKey(const String &apiKey) { _apiKey = apiKey; }

    void setBaseUri(const char *const uriBase) { _uriBase = uriBase; CheckApi(); }
    void setBaseUri(const String &uriBase) { _uriBase = uriBase; CheckApi(); }

    const String& GetApiKey() const { return _apiKey; }

    void CheckApi()
    {
      if(!_uriBase.endsWith("/")) _uriBase += '/';
      _uriBase.toLowerCase();
      _isOfficialApi = _uriBase == ALARMS_API_OFFICIAL_BASE_URI;
    }
    
    const bool IsStatusChanged(int &status, String &statusMsg)
    {     
      if(_isOfficialApi)
      { 
        auto httpRes = sendRequest("alerts/status", _apiKey, status, statusMsg, /*closeHttp:*/false);
        if(status == ApiStatusCode::API_OK)
        {
          String newActionIndex;
          DynamicJsonDocument json(38);
          auto deserializeError = deserializeJson(json, httpRes);
          //serializeJson(json, Serial);
          if (!deserializeError) 
          {
            newActionIndex = json["lastActionIndex"].as<const char *>();          

            bool isStatusChanged = lastActionIndex < newActionIndex;
            ALARMS_TRACE("LAST: ", lastActionIndex);
            ALARMS_TRACE(" NEW: ", newActionIndex);
            lastActionIndex = newActionIndex;
            return isStatusChanged;
          } 
          ALARMS_TRACE(F("[JSON] Deserialization error: "), deserializeError.c_str());        
        }     
        
        https2->end();
      }
      status = ApiStatusCode::API_OK;
      return true;
    }    

    public:    
    const std::vector<RegionInfo *> getAlarmedRegions2(int &status, String &statusMsg, const String &resource = ALARMS_API_IOT_ALERTS)
    {
      std::vector<RegionInfo *> res;
      status = ApiStatusCode::UNKNOWN;    

      https2->setTimeout(HTTP_TIMEOUT);            
      // https2->useHTTP10(true);
      https2->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
      
      const auto uri = _uriBase + resource;      

      ALARMS_TRACE(F("[HTTPS2] begin: "), uri);   
      ALARMS_TRACE(F("[HTTPS2] apiKey: "), _apiKey); 

      #ifdef ESP8266      
      BearSSL::WiFiClientSecure client;
      client.setInsecure();
      client.setTimeout(HTTP_TIMEOUT);      
      if (https2->begin(client, uri)) 
      #else
      if(https2->begin(uri))
      #endif
      { // HTTPS    

        https2->setTimeout(HTTP_TIMEOUT);

        https2->addHeader("Authorization", (_isOfficialApi ? "" : "Bearer ") + _apiKey);        
        https2->addHeader("Accept", "application/json");      

        ALARMS_TRACE(F("[HTTPS2] REQ GET: "), resource);
        // start connection and send HTTP header
        int httpCode = https2->GET();
        // httpCode will be negative on error
        if (httpCode > 0) 
        {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE(F("[HTTPS2] RES GET: "), resource, F("... code: "), httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NOT_MODIFIED) 
          { 
            status = ApiStatusCode::API_OK;          
            ALARMS_TRACE(F(" HEAP: "), ESP.getFreeHeap());
            ALARMS_TRACE(F("STACK: "), ESPgetFreeContStack);

            if(resource.endsWith(".json"))
            {
              ALARMS_TRACE(F("[HTTPS2] IoT Dev API"));
              
              ALARMS_TRACE(F("[HTTPS2] Start Parse String content")); 
              ALARMS_TRACE(F("[HTTPS2] content-length: "), https2->getSize());

              const String &payload = https2->getString();
              
              ALARMS_TRACE(F("[HTTPS2] Content: "), payload);                

              for(uint8_t idx = 0; idx < MAX_REGIONS_COUNT; idx++)
              {
                if(idx + 1 < payload.length())
                {
                  const auto &ch = payload[idx + 1];
                  RegionInfo *r = &iotApiRegions[idx];
                  if(ch == 'N')
                  {
                    r->AlarmStatus = ApiAlarmStatus::NotAlarmed;
                  }
                  else                 
                  {                    
                    r->AlarmStatus = ch == 'P' ? ApiAlarmStatus::PartialAlarmed : ApiAlarmStatus::Alarmed;
                    res.push_back(r);
                  }
                }
              }  

              https2->end();
            }
            else
            {
              ParseHugeJson();
            }                 
          }          
        }

        ALARMS_TRACE(F(" HEAP: "), ESP.getFreeHeap());
        ALARMS_TRACE(F("STACK: "), ESPgetFreeContStack);     

        HandleErrors(httpCode, status, statusMsg);

        ALARMS_TRACE(F("[HTTPS2] GET: "), resource, F("... status message: "), statusMsg);
        
        https2->end();
           
        return std::move(res);
        
      }
      else 
      {
        status = ApiStatusCode::UNKNOWN;
        ALARMS_TRACE(F("[HTTPS] Unable to connect"));
        statusMsg = F("Unknown");
        return std::move(res);
      }      
    }

    void HandleErrors(const int& httpCode, int& status, String &statusMsg)
    {
      status = httpCode;
      switch(httpCode)
      {
        case ApiStatusCode::API_OK:
          //ALARMS_TRACE("[HTTPS2] GET: OK");
          statusMsg = String(F("OK")) + F("(") + httpCode + F(")");
          break;
        // ------------------- CODES
        case HTTP_CODE_UNAUTHORIZED: //401
          statusMsg = String(F("Unauthorized")) + F("(") + httpCode + F(")");
          status = ApiStatusCode::WRONG_API_KEY;
          break;
        case HTTP_CODE_TOO_MANY_REQUESTS: //429
          statusMsg = String(F("To Many Requests")) + F("(") + httpCode + F(")");
          break;
        case HTTP_CODE_NOT_MODIFIED: //304
          statusMsg = String(F("Not Modified")) + F("(") + httpCode + F(")");
          break;
        case HTTP_CODE_FORBIDDEN: //403
          statusMsg = String(F("Forbidden. Possible token blocked")) + F("(") + httpCode + F(")");
          break;
        case HTTP_CODE_METHOD_NOT_ALLOWED: //405
          statusMsg = String(F("Method not allowed")) + F("(") + httpCode + F(")");
          break;
        // ------------------- ERRORS
        case HTTPC_ERROR_READ_TIMEOUT:
          statusMsg = String(https2->errorToString(httpCode)) + F("(") + httpCode + F(")");
          status = ApiStatusCode::READ_TIMEOUT;
          break;  
        #ifdef ESP8266     
        case HTTPC_ERROR_CONNECTION_FAILED:
          statusMsg = String(https2->errorToString(httpCode)) + F("(") + httpCode + F(")");
          status = ApiStatusCode::NO_CONNECTION;
          break;
        #else
        case HTTPC_ERROR_CONNECTION_REFUSED:
          statusMsg = String(https2->errorToString(httpCode)) + F("(") + httpCode + F(")");
          status = ApiStatusCode::NO_CONNECTION;
          break;        
        #endif
        case 521:
          statusMsg = String(F("Server-side problem")) + F("(") + httpCode + F(")");
          //status = ApiStatusCode::NO_CONNECTION;
        break;
        // ------------------- CUSTOM ERRORS
        case ApiStatusCode::JSON_ERROR:
          status = ApiStatusCode::JSON_ERROR;
          //statusMsg = "Json error";            
          break;        
        default:
          statusMsg = String(https2->errorToString(httpCode)) + F("(") + httpCode + F(")");
          break;
      }
    }

    const String sendRequest(const String& resource, const String &apiKey, int &status, String &statusMsg, const bool &closeHttp = true)
    {
      status = ApiStatusCode::UNKNOWN;   
      https2->setTimeout(HTTP_TIMEOUT);
      //https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

      const auto uri = (_uriBase.endsWith("/") ? _uriBase : _uriBase + '/') + resource;

      ALARMS_TRACE(F("[HTTPS] begin: "), uri);   
      ALARMS_TRACE(F("[HTTPS] apiKey: "), apiKey); 

      #ifdef ESP8266     
      BearSSL::WiFiClientSecure client;
      client.setInsecure();
      client.setTimeout(HTTP_TIMEOUT);      
      if (https2->begin(client, uri)) 
      #else
      if(https2->begin(uri))
      #endif
      {  // HTTPS

        https2->setTimeout(HTTP_TIMEOUT);
        String payload;
        https2->addHeader("Authorization", (_isOfficialApi ? "" : "Bearer ") + apiKey);        
        https2->addHeader("Accept", "application/json");      

        ALARMS_TRACE("[HTTPS] REQ GET: ", resource);
        // start connection and send HTTP header
        int httpCode = https2->GET();
        // httpCode will be negative on error
        if (httpCode > 0) 
        {
          // HTTP header has been send and Server response header has been handled
          ALARMS_TRACE(F("[HTTPS] RES GET: "), uri, "... code: ", httpCode);
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NOT_MODIFIED) 
          {
            ALARMS_TRACE(F(" HEAP: "), ESP.getFreeHeap());
            ALARMS_TRACE(F("STACK: "), ESPgetFreeContStack);
            status = ApiStatusCode::API_OK;
            payload = https2->getString();  

            ALARMS_TRACE(F("[HTTPS] content-length: "), https2->getSize());          
            //ALARMS_TRACE(F("[HTTPS] content: "), payload);
           
            //return "Ok";
          }          
        }

        HandleErrors(httpCode, status, statusMsg);

        if(closeHttp)
          https2->end();

        ALARMS_TRACE(F("[HTTPS2] GET: "), resource, F("... status message: "), statusMsg);

        return std::move(payload);

      } else 
      {
        status = ApiStatusCode::UNKNOWN;
        ALARMS_TRACE(F("[HTTPS] Unable to connect"));
        statusMsg = "Unknown";
        return std::move(F("[HTTPS] Unable to connect"));
      }      
    }   

    void ParseHugeJson()
    {
      // ALARMS_TRACE("[HTTPS2] Official API");

              // ALARMS_TRACE("[HTTPS2] Full Json content: "); //https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
              // ReadLoggingStream loggingStream(https2->getStream(), Serial);        
            
              // DeserializationError deserializeError;

              // if(resource == ALARMS_API_OFFICIAL_ALERTS ? loggingStream.find("[") : loggingStream.find("\"states\":["))
              // {
              //   DynamicJsonDocument doc(1024);
              //   Serial.println();
              //   StaticJsonDocument<32> filter;
              //   filter["regionType"] = true;
              //   filter["regionId"] = true;
              //   filter["regionName"] = true;
              //   filter["regionEngName"] = true;

              //   ALARMS_TRACE(F("[HTTPS2] Start Parse Json content"));             
              //   do
              //   {
              //     deserializeError = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
              //     if(!deserializeError)
              //     {
              //       if(String(doc["regionType"]) == "State")
              //       {
              //         ALARMS_TRACE(doc.as<const char*>());    

              //         const char *engNamePtr = doc["regionEngName"].as<const char*>();                

              //         RegionInfo rInfo;
              //         int regionId = atoi(doc["regionId"].as<const char*>());                  
              //         rInfo.Id = regionId == 9999 ? UARegion::Crimea : (UARegion)regionId;
              //         rInfo.Name = engNamePtr != 0 && strlen(engNamePtr) > 0 ? engNamePtr : doc["regionName"].as<const char*>();

              //         //res.push_back(rInfo);
              //         ALARMS_TRACE(F("\tRegion: ID: "), (uint8_t)rInfo.Id, F(" Name: "), rInfo.Name);
              //       }
              //     }
              //     else
              //     {
              //       ALARMS_TRACE(F("[JSON] Deserialization error: "), deserializeError.c_str());
              //     }
              //   }while(loggingStream.findUntil(",","]"));
              // }
              // ALARMS_TRACE(F("[HTTPS2] End Parse Json content: "), deserializeError.c_str());   
              // https2->end();           
    }   
    
};

#endif //ALARMS_API_H