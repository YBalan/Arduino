#pragma once
#ifndef WIFI_PARAMETERS_H
#define WIFI_PARAMETERS_H

#include <vector>

#include "DEBUGHelper.h"
#ifdef ENABLE_INFO_WIFI
#define WIFIP_INFO(...) SS_TRACE(F("[WiFi PARAMS INFO] "), __VA_ARGS__)
#else
#define WIFIP_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_WIFI
#define WIFIP_TRACE(...) SS_TRACE(F("[WiFi PARAMS TRACE] "), __VA_ARGS__)
#else
#define WIFIP_TRACE(...) {}
#endif


class WiFiParameter : public WiFiManagerParameter
{  
  //WiFiManagerParameter Parameter;
  private:
  String _id;
  String _label;
  String _value;
  String _json;  

  public:
  WiFiParameter(){}

  public:
  //WiFiParameter(WiFiParameter &){}
  WiFiParameter(const char *const id, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &length) 
  : WiFiParameter(id, label, json, defaultValue, length, 0)   
  {
    WIFIP_TRACE(F("WiFiParameter ctor"));    
  }
  
  WiFiParameter(const char *const id, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &length, const uint8_t &place)  
  : _id(id)
  , _label(label)
  , _value(defaultValue)
  , _json(json)
  {
    WIFIP_TRACE(F("WiFiParameter ctor with place"));    
    Init(_id.c_str(), _label.c_str(), _value.c_str(), length, "", WFM_LABEL_DEFAULT);
  }

  WiFiParameter(const WiFiParameter &other)   
   : _id(other._id)
    , _label(other._label)
    , _value(other._value)
    , _json(other._json)
  {    
    WIFIP_TRACE(F("WiFiParameter copy ctor"));   

    Init(_id.c_str(), _label.c_str(), _value.c_str(), other.getValueLength(), other.getCustomHTML(), other.getLabelPlacement());
  }

 WiFiParameter(WiFiParameter&& other) noexcept  
    : _id(std::move(other._id))
    , _label(std::move(other._label))
    , _value(std::move(other._value))
    , _json(std::move(other._json))
  {
    WIFIP_TRACE(F("WiFiParameter move ctor"));

    Init(_id.c_str(), _label.c_str(), _value.c_str(), other.getValueLength(), other.getCustomHTML(), other.getLabelPlacement());
  }

  void Init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement)
  {
    WIFIP_TRACE(F("\tId: "), id);
    WIFIP_TRACE(F("\tLabel: "), label);
    WIFIP_TRACE(F("\tValue: "), defaultValue, " Length: ", length);
    WIFIP_TRACE(F("\tCustom: "), custom);
    WIFIP_TRACE(F("\tLabelPlacement: "), labelPlacement);
    init(id, label, defaultValue, length, custom, labelPlacement);
  }

  void SetValue(const char *const value)
  { 
    _value = value;
    setValue(_value.c_str(), getValueLength());
  }

  const String &ReadValue() 
  { 
    _value = getValue();
    return _value;
  }

  const String &GetValue() const
  {     
    return _value;
  }

  const String &GetId() const
  {    
    return _id;
  }

  const String &GetJson() const
  {
    return _json;
  }

  WiFiManagerParameter* GetParameter()
  {
    //return &Parameter;
    return this;
  }

  virtual bool IsNull() {return false;}
};

class WiFiNullParameter : public WiFiParameter
{
  public:
  WiFiNullParameter() : WiFiParameter(){}
  virtual bool IsNull() {return true;}
};

template <int PARAMS_COUNT>
class WiFiParameters
{
  std::vector<WiFiParameter> _parameters;  
public:
  inline static WiFiNullParameter NullParam;
public:

  WiFiParameters() 
  //: _parameters(PARAMS_COUNT)
  {}

  void AddParameter(const WiFiParameter &param)
  {
    WIFIP_TRACE("Start AddParameter...");
    _parameters.push_back(param);    
    WIFIP_TRACE("End AddParameter...");    
  }

  const WiFiParameter &GetParameterById(const String &value) const
  {
    for(uint8_t i = 0; i < Count(); i++)
    {
      auto &p = _parameters[i];
      if(p.GetId() == value)
        return p;
    }
    return NullParam;
  }

  const WiFiParameter &GetParameterByJsonName(const String &value) const
  {
    for(auto &p : _parameters)
    {
      if(p.GetJson() == value)
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