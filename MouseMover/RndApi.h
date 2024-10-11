#pragma once
#ifndef RANDOM_ORG_API_H
#define RANDOM_ORG_API_H

#ifdef USE_API

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

#ifdef ENABLE_INFO_API
#define API_INFO(...) SS_TRACE(F("[API INFO] "), __VA_ARGS__)
#else
#define API_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_API
#define API_TRACE(...) SS_TRACE(F("[API TRACE] "), __VA_ARGS__)
#else
#define API_TRACE(...) {}
#endif

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

std::unique_ptr<HTTPClient> https2(new HTTPClient);
const String sendRequest(const String& resource, const String &apiKey, int &status, String &statusMsg, const bool &closeHttp = true);
void HandleErrors(const int& httpCode, int& status, String &statusMsg);

#define BASE_URI F("https://www.random.org/integers/")
#define PARAMS_TEMPLATE F("?num=1&min={min}&max={max}&col=1&base=10&format=plain&rnd=new")

const int GetRandomNumber(const int &min, const int &max, int &status, String &statusMsg)
{
  String params = String(PARAMS_TEMPLATE);
  params.replace(F("{min}"), String(min));
  params.replace(F("{max}"), String(max));
  auto httpRes = sendRequest(params, F(""), status, statusMsg, /*closeHttp:*/false);
  API_TRACE(F("Response: "), httpRes);
  if(status == ApiStatusCode::API_OK)
  {
    const auto &rnd = httpRes.toInt();    
    return rnd;
  }
  return -1;  
}

const String sendRequest(const String& resource, const String &apiKey, int &status, String &statusMsg, const bool &closeHttp)
{  
  status = ApiStatusCode::UNKNOWN; 

  #ifdef HTTP_TIMEOUT  
  https2->setTimeout(HTTP_TIMEOUT);
  #endif
  //https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  const auto uri = String(BASE_URI) + resource;

  API_TRACE(F("[HTTPS] begin: "), uri);   
  API_TRACE(F("[HTTPS] apiKey: "), apiKey); 

  #ifdef ESP8266     
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  #ifdef HTTP_TIMEOUT
  client.setTimeout(HTTP_TIMEOUT);      
  #endif
  if (https2->begin(client, uri)) 
  #else
  if(https2->begin(uri))
  #endif
  {  // HTTPS
    #ifdef HTTP_TIMEOUT
    https2->setTimeout(HTTP_TIMEOUT);
    #endif
    String payload;
    if(apiKey.length() > 0)
    {
      https2->addHeader("Authorization", "Bearer " + apiKey);        
      https2->addHeader("Accept", "application/json");      
    }

    API_TRACE("[HTTPS] REQ GET: ", resource);
    // start connection and send HTTP header
    int httpCode = https2->GET();
    // httpCode will be negative on error
    if (httpCode > 0) 
    {
      // HTTP header has been send and Server response header has been handled
      API_TRACE(F("[HTTPS] RES GET: "), uri, "... code: ", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_NOT_MODIFIED) 
      {
        API_TRACE(F(" HEAP: "), ESP.getFreeHeap());
        API_TRACE(F("STACK: "), ESPgetFreeContStack);
        status = ApiStatusCode::API_OK;
        payload = https2->getString();  

        API_TRACE(F("[HTTPS] content-length: "), https2->getSize());          
        //API_TRACE(F("[HTTPS] content: "), payload);
        
        //return "Ok";
      }          
    }

    HandleErrors(httpCode, status, statusMsg);

    if(closeHttp)
      https2->end();

    API_TRACE(F("[HTTPS2] GET: "), resource, F("... status message: "), statusMsg);

    return std::move(payload);

  } else 
  {
    status = ApiStatusCode::UNKNOWN;
    API_TRACE(F("[HTTPS] Unable to connect"));
    statusMsg = "Unknown";
    return std::move(F("[HTTPS] Unable to connect"));
  }      
}   

void HandleErrors(const int& httpCode, int& status, String &statusMsg)
{
  status = httpCode;
  switch(httpCode)
  {
    case ApiStatusCode::API_OK:
      //API_TRACE("[HTTPS2] GET: OK");
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
    case HTTP_CODE_BAD_GATEWAY: //502
      statusMsg = String(F("Bad gateway")) + F("(") + httpCode + F(")");
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

#endif //USE_API
#endif //RANDOM_ORG_API_H