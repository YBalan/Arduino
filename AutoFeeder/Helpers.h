#pragma once
#ifndef HELPERS_H
#define HELPERS_H

namespace Helpers
{
  class StoreHelper
  {
    public:    
    static const uint16_t CombineToUint16(const uint8_t &first, const uint8_t &second)
    {
      return (first << 8) | second; // Store first digit in higher bits and second digit in lower bits
    }

    static void ExtractFromUint16(const uint16_t &combined, uint8_t &first, uint8_t &second)
    {
      // Extract the first digit (higher bits)
      first = (combined >> 8) & 0xFF; // Shift right to get the first digit and mask other bits

      // Extract the second digit (lower bits)
      second = combined & 0xFF; // Mask higher bits to get the second digit
    }
  };
}

#endif //HELPERS_H