#pragma once
#ifndef WIFI_PARAMETERS_H
#define WIFI_PARAMETERS_H

#include <vector>

#ifdef ENABLE_INFO_WIFI
#define WIFIP_INFO(...) SS_TRACE("[WiFi PARAMS INFO] ", __VA_ARGS__)
#else
#define WIFIP_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_WIFI
#define WIFIP_TRACE(...) SS_TRACE("[WiFi PARAMS TRACE] ", __VA_ARGS__)
#else
#define WIFIP_TRACE(...) {}
#endif


class WiFiParameter
{
  public:
  WiFiManagerParameter Parameter;
  String JsonPropertyName;  

  protected:
  WiFiParameter(){}

  public:
  //WiFiParameter(WiFiParameter &){}
  WiFiParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue) 
  : WiFiParameter(name, label, json, defaultValue, 0) 
  {}
  
  WiFiParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &place)
  : Parameter(name, label, defaultValue, 100)  
  , JsonPropertyName(json)  
  {}

   WiFiParameter(const WiFiParameter &copy)
   : Parameter(copy.Parameter)
   , JsonPropertyName(copy.JsonPropertyName)
   {
    WIFIP_TRACE("WiFIParameter copy ctor");
   }

  void SetValue(const char *const value)
  {    
    Parameter.setValue(value, strlen(value));
  }

  const String GetValue() const
  {
    return Parameter.getValue();
  }

  const String GetName() const
  {
    return Parameter.getID();
  }

  WiFiManagerParameter* GetParameter()
  {
    return &Parameter;
  }

  /*WiFiParameter(WiFiParameter&& other) noexcept  
  : Name(std::move(other.Name))
  , Label(std::move(other.Label))
  , JsonPropertyName(std::move(other.JsonPropertyName))
  , Value(std::move(other.Value))
  , Place(other.Place)
  {
  }*/

  /*WiFiParameter& operator=(WiFiParameter&& other) noexcept
  {
    Name = std::move(other.Name);
    Label = std::move(other.Label);
    JsonPropertyName = std::move(other.JsonPropertyName);
    Value = std::move(other.Value);
    Place = other.Place;

    return *this;
  }*/

  virtual bool IsNull() {return false;}
};

class WiFiNullParameter : public WiFiParameter
{
  public:
  WiFiNullParameter() : WiFiParameter(){}
  virtual bool IsNull() {return true;}
};

class WiFiParameters
{
  std::vector<WiFiParameter> _parameters;
public:
  inline static WiFiNullParameter NullParam;
public:
  WiFiParameters &AddParameter(const WiFiParameter &param)
  {
    _parameters.push_back(param);
    return *this;
  }

  WiFiParameter &GetParameterById(const String &value)
  {
    for(int i = 0; i < Count(); i++)
    {
      auto &p = _parameters[i];
      if(p.GetName() == value)
        return p;
    }
    return NullParam;
  }

  WiFiParameter &GetParameterByJsonName(const String &value)
  {
    for(auto &p : _parameters)
    {
      if(p.JsonPropertyName == value)
        return p;
    }
    return NullParam;
  }

  // Iterator to the beginning of the custom vector
    auto begin() const {
        return _parameters.begin();
    }

    // Iterator to the end of the custom vector
    auto end() const {
        return _parameters.end();
    }

  WiFiParameter &GetAt(const int& index) { return _parameters[index]; }

  const int Count() const { return _parameters.size(); }
};


#endif //WIFI_PARAMETERS_H