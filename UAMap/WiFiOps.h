#pragma once
#ifndef WIFI_OPS_H
#define WIFI_OPS_H

#include "DEBUGHelper.h"

#ifdef ENABLE_INFO_WIFI
#define WIFI_INFO(...) SS_TRACE("[WiFI OPS INFO] ", __VA_ARGS__)
#else
#define WIFI_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_WIFI
#define WIFI_TRACE(...) SS_TRACE("[WiFI OPS TRACE] ", __VA_ARGS__)
#else
#define WIFI_TRACE(...) {}
#endif



//define your default values here, if there are different values in config.json, they are overwritten.
char api_token[42] = "YOUR_API_TOKEN";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  WIFI_INFO("Should save config");
  shouldSaveConfig = true;
}

void ClearFSSettings()
{
  //clean FS, for testing
  SPIFFS.format();
}

void SaveFSSettings(const char* const apiToken);

void TryToConnect(const bool &resetSettings = false)
{
  //read configuration from FS json
  WIFI_INFO("mounting FS...");

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
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          WIFI_TRACE("parsed json");         
          strcpy(api_token, json["api_token"]);
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

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_api_token("apikey", "Alarms API token", api_token, 42);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
 //wifiManager.setSTAStaticIPConfig(IPAddress(172, 16, 1, 99), IPAddress(172, 16, 1, 1), IPAddress(255, 255, 255, 0));

  //wifiManager.setHttpPort(8080);

  //add all your parameters here
  wifiManager.addParameter(&custom_api_token);

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

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    WIFI_INFO("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  WIFI_INFO("connected...yeey :)");

  //read updated parameters
  strcpy(api_token, custom_api_token.getValue());
  WIFI_TRACE("The values in the file are: ");
  WIFI_TRACE("\tapi_token : " + String(api_token));

  SaveFSSettings(api_token);

  WIFI_INFO("local ip");
  WIFI_INFO(WiFi.localIP());
}

//save the custom parameters to FS
void SaveFSSettings(const char* const apiToken)
{  
  if (shouldSaveConfig) {
    WIFI_TRACE("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["api_token"] = apiToken;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      WIFI_INFO("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }
}

#endif //WIFI_OPS_H