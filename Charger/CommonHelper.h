#pragma once
#ifndef COMMON_HELPER_H
#define COMMON_HELPER_H
#include <vector>

namespace CommonHelper
{

  template <typename V>
  const bool saveMap(File &file, const std::map<String, V> &map) {
      if (file) { 
      // Save the size of the map
      int mapSize = map.size();      
      file.write((const uint8_t*)(&mapSize), sizeof(mapSize));

      // Save each key-value pair
      for (const auto& [key, value] : map) {
          // Save the key
          int keyLength = key.length();  // Get the length of the key
          file.write((const uint8_t*)(&keyLength), sizeof(keyLength));
          file.write((const uint8_t*)(key.c_str()), keyLength);

          // Save the struct (value)
          file.write((const uint8_t*)(&value), sizeof(value));   
      }
      return true;
    }
    return false;
  }

  template <typename V>
  const bool loadMap(File &file, std::map<String, V> &map, const int &maxElements = 100, const int &maxKeySize = 500) {
      if (file) {        
      int mapSize = 0;
      file.read((uint8_t*)(&mapSize), sizeof(mapSize));    
      map.clear();  // Clear the map before loading new data

      if(maxElements == 0 || mapSize <= maxElements) {
        for (int i = 0; i < mapSize; ++i) {
          if(file.available()){
            // Load the key
            int keyLength = 0;
            file.read((uint8_t*)(&keyLength), sizeof(keyLength));

            if(keyLength > maxKeySize) continue;

            char* keyBuffer = new char[keyLength + 1];  // +1 for null terminator
            file.read((uint8_t*)(keyBuffer), keyLength);
            keyBuffer[keyLength] = '\0';  // Null-terminate the key string

            String key(keyBuffer);  // Convert char* to String
            delete[] keyBuffer;

            // Load the struct (value)
            V value;
            file.read((uint8_t*)(&value), sizeof(value));       

            // Insert into map
            map[key] = value;
          }
        }
        return true;
      }
    } 
    return false;   
  }

  const String toString(const int &value, const uint8_t &digits, const char &symbol = '0')
  {
    char buffer[digits + 2];
    String format = String(F("%")) + symbol + String(digits) + F("d");    
    snprintf(buffer, sizeof(buffer), format.c_str(), value);
    return std::move(String(buffer));
  }

  const String join(const std::set<String> &v, const char &delimiter)
  {
    String res;
    for(const auto &value : v) res += value + delimiter;
    return std::move(res);
  }

  const String join(const std::map<String, int32_t> &v, const char &delimiter)
  {
    String res;
    for(const auto &value : v) res += value.first + delimiter;
    return std::move(res);
  }

  std::vector<String> split(const String &s, const char &delimiter, const char &delimiter2) {
      std::vector<String> tokens;
      int startIndex = 0; // Index where the current token starts

      // Loop through each character in the string
      for (int i = 0; i < s.length(); i++) {
          // If the current character is the delimiter or it's the last character in the string
          bool endOfString = i == s.length() - 1;
          bool isDelimeter = (s.charAt(i) == delimiter) || (s.charAt(i) == delimiter2);
          if (isDelimeter || endOfString) {
              // Extract the substring from startIndex to the current position
              String token = s.substring(startIndex, endOfString ? (!isDelimeter ? s.length() : i) : i);
              token.trim();
              tokens.push_back(token);
              startIndex = i + 1; // Update startIndex for the next token
          }
      }
      return std::move(tokens);
  }  

  std::vector<String> split(const String &s, const char &delimiter) { return split(s, delimiter, delimiter); }

  std::vector<int> splitToInt(const String &s, const char &delimiter, const char &delimiter2) {
      std::vector<int> tokens;
      int startIndex = 0; // Index where the current token starts

      // Loop through each character in the string
      for (int i = 0; i < s.length(); i++) {
          // If the current character is the delimiter or it's the last character in the string
          bool endOfString = i == s.length() - 1;
          bool isDelimeter = (s.charAt(i) == delimiter) || (s.charAt(i) == delimiter2);
          if (isDelimeter || endOfString) {
              // Extract the substring from startIndex to the current position
              String token = s.substring(startIndex, endOfString ? (!isDelimeter ? s.length() : i) : i);
              token.trim();
              tokens.push_back(token.toInt());
              startIndex = i + 1; // Update startIndex for the next token
          }
      }
      return std::move(tokens);
  }

  std::vector<int> splitToInt(const String &s, const char &delimiter) {
    return splitToInt(s, delimiter, delimiter);
  }
};

#endif //COMMON_HELPER_H