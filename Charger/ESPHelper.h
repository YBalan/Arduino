#pragma once
#ifndef ESP_HELPER_H
#define ESP_HELPER_H

const String GetResetReason(int &resetReason)
{
  #ifdef ESP8266
  resetReason = ESP.getResetInfoPtr()->reason;
  return ESP.getResetReason();  
  #else //ESP32
  // Get the reset reason
  /*esp_reset_reason_t*/ resetReason = esp_reset_reason();

  // Print the reset reason to the Serial Monitor
  
  switch (resetReason) {
    case ESP_RST_UNKNOWN:
      return F("Unknown");
      break;
    case ESP_RST_POWERON:
      return F("Power on");
      break;
    case ESP_RST_EXT:
      return F("External reset");
      break;
    case ESP_RST_SW:
      return F("Software reset");
      break;
    case ESP_RST_PANIC:
      return F("Exception/panic");
      break;
    case ESP_RST_INT_WDT:
      return F("Watchdog reset (Interrupt)");
      break;
    case ESP_RST_TASK_WDT:
      return F("Watchdog reset (Task)");
      break;
    case ESP_RST_WDT:
      return F("Watchdog reset");
      break;
    case ESP_RST_DEEPSLEEP:
      return F("Deep sleep reset");
      break;
    case ESP_RST_BROWNOUT:
      return F("Brownout reset");
      break;
    case ESP_RST_SDIO:
      return F("SDIO reset");
      break;
    default:
      return F("Unknown reason");
      break;
  }
  #endif
}

void PrintFSInfo(String &fsInfo)
{
  #ifdef ESP8266
  FSInfo fs_info;
  SPIFFS.info(fs_info);   
  const auto &total = fs_info.totalBytes;
  const auto &used = fs_info.usedBytes;
  #else //ESP32
  const auto &total = SPIFFS.totalBytes();
  const auto &used = SPIFFS.usedBytes();
  #endif
  fsInfo = String(F("SPIFFS: ")) + F("Total: ") + String(total) + F(" ") + F("Used: ") + String(used);  
}

#endif //ESP_HELPER_H