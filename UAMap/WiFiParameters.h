#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include <vector>

class WiFiParameter
{
  public:
  String Name;
  String Label;
  String JsonPropertyName;
  String Value;
  uint8_t Place;

  protected:
  WiFiParameter(){}

  public:
  //WiFiParameter(WiFiParameter &){}
  WiFiParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue) 
  : WiFiParameter(name, label, json, defaultValue, 0) 
  {}
  
  WiFiParameter(const char *const name, const char *const label, const char *const json, const char *const defaultValue, const uint8_t &place)
  : Name(name)
  , Label(label)
  , JsonPropertyName(json)
  , Value(defaultValue)
  , Place(place)
  {}

  void SetValue(const char *const value)
  {    
    Value = value == 0 ? "" : value;
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
    _parameters.push_back(std::move(param));
    return *this;
  }

  WiFiParameter &GetParameterById(const String &value)
  {
    for(int i = 0; i < Count(); i++)
    {
      auto &p = _parameters[i];
      if(p.Name == value)
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


#endif //SETTINGS_H