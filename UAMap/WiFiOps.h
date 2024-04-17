#pragma once
#ifndef WIFI_OPS_H
#define WIFI_OPS_H

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#ifdef ESP32
  #include <SPIFFS.h>
#endif

#include "DEBUGHelper.h"
#include "WiFiParameters.h"

#ifdef ENABLE_INFO_WIFI
#define WIFI_INFO(...) SS_TRACE("[WiFi OPS INFO] ", __VA_ARGS__)
#else
#define WIFI_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_WIFI
#define WIFI_TRACE(...) SS_TRACE("[WiFi OPS TRACE] ", __VA_ARGS__)
#else
#define WIFI_TRACE(...) {}
#endif

//#define API_TOKEN_LENGTH 42
//define your default values here, if there are different values in config.json, they are overwritten.
//char api_token[API_TOKEN_LENGTH] = "YOUR_API_TOKEN";

namespace WiFiOps
{

  // callbacks
//  called after AP mode and config portal has started
//   setAPCallback( std::function<void(WiFiManager*)> func );
//  called after webserver has started
//   setWebServerCallback( std::function<void()> func );
//  called when settings reset have been triggered
//   setConfigResetCallback( std::function<void()> func );
//  called when wifi settings have been changed and connection was successful ( or setBreakAfterConfig(true) )
//   setSaveConfigCallback( std::function<void()> func );
//  called when saving either params-in-wifi or params page
//   setSaveParamsCallback( std::function<void()> func );
//  called when saving params-in-wifi or params before anything else happens (eg wifi)
//   setPreSaveConfigCallback( std::function<void()> func );
//  called just before doing OTA update
//   setPreOtaUpdateCallback( std::function<void()> func );
namespace SaveCallback
{
  //flag for saving data
  static bool _shouldSaveConfig = false;

  //callback notifying us of the need to save config
  void saveConfigCallback () {
    WIFI_INFO("Should save config");
    _shouldSaveConfig = true;
  }
}

template <int PARAMS_COUNT = 0>
class WiFiOps
{
  private:
    String _title;
    String _APName;
    String _APPass;
    bool _addMacToAPName;
    WiFiParameters<PARAMS_COUNT> _parameters;
    uint8_t _lastPlace;
  public:
    
  public:    
    WiFiOps(const String &title = "WiFIManager", const String &apName = "AutoConnectAP", const String &apPass = "00000", const bool &addMacToAPName = true) 
    : _title(std::move(title))
    , _APName(std::move(apName))
    , _APPass(std::move(apPass))
    , _addMacToAPName(addMacToAPName)
    , _lastPlace(0)
    {
      
    }
    WiFiOps &AddParameter(const WiFiParameter& param)
    {
      WIFI_TRACE("AddParameter...");
      _parameters.AddParameter(param);
      _lastPlace++;
      return *this;
    }
    WiFiOps &AddParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &length) 
    {
      WIFI_TRACE("AddParameter...");
      WiFiParameter p(name, label, json, defaultValue, length, _lastPlace);
      _parameters.AddParameter(p);
      _lastPlace++;
      return *this;      
    }
    WiFiOps &AddParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &length, const uint8_t &place)
    {
      WIFI_TRACE("AddParameter...");
      WiFiParameter p(name, label, json, defaultValue, length, place);
      _parameters.AddParameter(p);
      _lastPlace = place + 1;
      return *this;
    }

    const String &GetParameterValueById(const String &id) const
    {
      return _parameters.GetParameterById(id).GetValue();
    }

  public:
    void ClearFSSettings()
    {
      //clean FS, for testing
      SPIFFS.format();
    }

    const int ParametersCount() const
    {
      return _parameters.Count();
    }

    const WiFiParameters<PARAMS_COUNT> &TryToConnectOrOpenConfigPortal(const bool &resetSettings = false)
    {
      WIFI_TRACE("TryToConnectOrOpenConfigPortal...");

      LoadFSSettings(_parameters);

      // The extra parameters to be configured (can be either global or just in the setup)
      // After connecting, parameter.getValue() will get you the configured value
      // id/name placeholder/prompt default length
      //WiFiManagerParameter custom_api_token("apikey", "Alarms API token", api_token, API_TOKEN_LENGTH);

      //WiFiManager
      //Local intialization. Once its business is done, there is no need to keep it around
      WiFiManager wifiManager;

      //Set titlle
      wifiManager.setTitle(_title);

      //set config save notify callback
      wifiManager.setSaveConfigCallback(SaveCallback::saveConfigCallback);

      //set static ip
      //wifiManager.setSTAStaticIPConfig(IPAddress(172, 16, 1, 99), IPAddress(172, 16, 1, 1), IPAddress(255, 255, 255, 0));

      //wifiManager.setHttpPort(8080);

      WIFI_TRACE("Adding parameters to WiFiManager...");
      //add all your parameters here
      //wifiManager.addParameter(&custom_api_token);
      
      for(uint8_t pIdx = 0; pIdx < _parameters.Count(); pIdx++)
      {
        auto &parameter = _parameters.GetAt(pIdx);
        WIFI_TRACE("\tAdding parameter: ", parameter.GetId(), " value: ", parameter.GetValue());
        wifiManager.addParameter(parameter.GetParameter()); 
      }     

      WIFI_TRACE("WiFiManager parameters count: ", wifiManager.getParametersCount());

      //reset settings - for testing
      if(resetSettings)
      {
        WIFI_TRACE("Reset Settings");
        wifiManager.resetSettings();
      }

      //set minimu quality of signal so it ignores AP's under that quality
      //defaults to 8%
      wifiManager.setMinimumSignalQuality();

      //sets timeout until configuration portal gets turned off
      //useful to make it all retry or go to sleep
      //in seconds
      //wifiManager.setTimeout(120);

      auto mac = WiFi.macAddress();
      mac = mac.substring(mac.length() - 5, mac.length());
      //fetches ssid and pass and tries to connect
      //if it does not connect it starts an access point with the specified name
      //here  "AutoConnectAP"
      //and goes into a blocking loop awaiting configuration
      if (!wifiManager.autoConnect((_APName + (_addMacToAPName ? "_" + mac : "")).c_str(), _APPass.c_str())) {
        WIFI_INFO("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
      }

      //if you get here you have connected to the WiFi
      WIFI_INFO("connected...yeey :)");

      //read updated parameters
      //strcpy(api_token, custom_api_token.getValue());
      //WIFI_TRACE("The values in the file are: ");
      //WIFI_TRACE("\tapi_token : ", String(api_token));      

      SaveFSSettings(_parameters);

      WIFI_TRACE("The values in the file are: ");
      for(const auto &p : _parameters)
      {
        WIFI_TRACE("\tParameter: ", p.GetId(), " json property: ", p.GetJson(), " value: ", p.GetValue());
      }

      WIFI_INFO("local ip");
      WIFI_INFO(WiFi.localIP());
      WIFI_INFO("MAC:");
      WIFI_INFO(WiFi.macAddress());

      return _parameters;
    }

    //save the custom parameters to FS
    void SaveFSSettings(WiFiParameters<PARAMS_COUNT> &parametersToSave)
    {  
      WIFI_TRACE("Save Settings...");
      if (SaveCallback::_shouldSaveConfig) {
        WIFI_TRACE("saving config");
    #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
    #else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
    #endif
        //json["api_token"] = apiToken;

        for(int pIdx = 0; pIdx < parametersToSave.Count(); pIdx++)
        {                
          auto &p = parametersToSave.GetAt(pIdx);
          auto &readValue = p.ReadValue();
          WIFI_TRACE("Save parameter: ", p.GetId(), " json property: ", p.GetJson(), " json value: ", readValue);
          json[p.GetJson()] = readValue;
        }

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
          WIFI_INFO("failed to open config file for writing");
        }

    #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        serializeJson(json, Serial); Serial.println();
        serializeJson(json, configFile);
    #else
        json.printTo(Serial); Serial.println();
        json.printTo(configFile);
    #endif
        configFile.close();
        //end save
      }
    }

    //read configuration from FS json
    void LoadFSSettings(WiFiParameters<PARAMS_COUNT> &parametersToLoad)
    { 
      WIFI_TRACE("Load Settings...");
      WIFI_TRACE("mounting FS...");

      if (SPIFFS.begin()) {
        WIFI_TRACE("mounted file system");
        if (SPIFFS.exists("/config.json")) {
          //file exists, reading and loading
          WIFI_TRACE("reading config file");
          File configFile = SPIFFS.open("/config.json", "r");
          if (configFile) {
            WIFI_TRACE("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);

    #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
            DynamicJsonDocument json(1024);
            auto deserializeError = deserializeJson(json, buf.get());
            serializeJson(json, Serial); Serial.println();
            
            if ( ! deserializeError ) {
    #else
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
            json.printTo(Serial); Serial.println();
            if (json.success()) {
    #endif
              WIFI_TRACE("parsed json");         
              //strcpy(apiToken, json["api_token"]);

              for(int pIdx = 0; pIdx < parametersToLoad.Count(); pIdx++)
              {                
                auto &p = parametersToLoad.GetAt(pIdx);
                
                bool isJsonValueExist = json.containsKey(p.GetJson());
                const char * jsonValue = json[p.GetJson()];
                WIFI_TRACE("Load parameter: ", p.GetId(), " json property: ", p.GetJson(), " json value: ", jsonValue, " json exist: ", (isJsonValueExist ? "true" : "false"));
                if(isJsonValueExist)
                {
                  p.SetValue(jsonValue);
                }
              }

            } else {
              WIFI_TRACE("failed to load json config");
            }
            configFile.close();
          }
        }
      } else {
        WIFI_TRACE("failed to mount FS");
      }
      //end read
    }
};

}
#endif //WIFI_OPS_H