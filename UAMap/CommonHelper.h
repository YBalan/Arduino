#pragma once
#ifndef COMMON_HELPER_H
#define COMMON_HELPER_H
#include <vector>

namespace CommonHelper
{
  std::vector<String> split(const String &s, const char &delimiter) {
      std::vector<String> tokens;
      int startIndex = 0; // Index where the current token starts

      // Loop through each character in the string
      for (int i = 0; i < s.length(); i++) {
          // If the current character is the delimiter or it's the last character in the string
          if (s.charAt(i) == delimiter || i == s.length() - 1) {
              // Extract the substring from startIndex to the current position
              String token = s.substring(startIndex, i);
              token.trim();
              tokens.push_back(token);
              startIndex = i + 1; // Update startIndex for the next token
          }
      }
      return std::move(tokens);
  }

  std::vector<int> splitToInt(const String &s, const char &delimiter) {
      std::vector<int> tokens;
      int startIndex = 0; // Index where the current token starts

      // Loop through each character in the string
      for (int i = 0; i < s.length(); i++) {
          // If the current character is the delimiter or it's the last character in the string
          bool endOfString = i == s.length() - 1;
          bool isDelimeter = s.charAt(i) == delimiter;
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
};

#endif //COMMON_HELPER_H