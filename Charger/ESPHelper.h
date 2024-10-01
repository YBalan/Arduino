#pragma once
#ifndef ESP_HELPER_H
#define ESP_HELPER_H

const String ResetReasonToString(const int &resetReason, const bool &shortView)
{
  switch (resetReason) {
    case ESP_RST_UNKNOWN:
      return shortView ? F("Unknwn") : F("Unknown");
      break;
    case ESP_RST_POWERON:
      return shortView ? F("PowrOn") : F("Power on");
      break;
    case ESP_RST_EXT:
      return shortView ? F("ExtRst") : F("External reset");
      break;
    case ESP_RST_SW:
      return shortView ? F("SftRst") : F("Software reset");
      break;
    case ESP_RST_PANIC:
      return shortView ? F("Excptn") : F("Exception/panic");
      break;
    case ESP_RST_INT_WDT:
      return shortView ? F("WdtINT") : F("Watchdog reset (Interrupt)");
      break;
    case ESP_RST_TASK_WDT:
      return shortView ? F("WdtTSK") : F("Watchdog reset (Task)");
      break;
    case ESP_RST_WDT:
      return shortView ? F("WdtRST") : F("Watchdog reset");
      break;
    case ESP_RST_DEEPSLEEP:
      return shortView ? F("DepSlp") : F("Deep sleep reset");
      break;
    case ESP_RST_BROWNOUT:
      return shortView ? F("BwnOut") : F("Brownout reset");
      break;
    case ESP_RST_SDIO:
      return shortView ? F("SDIOrs") : F("SDIO reset");
      break;
    default:
      return shortView ? F("NA") : F("Unknown reason");
      break;
  }
}

const String GetResetReason(int &resetReason, const bool &shortView)
{
  #ifdef ESP8266
  resetReason = ESP.getResetInfoPtr()->reason;
  return ESP.getResetReason();  
  #else //ESP32
  // Get the reset reason
  /*esp_reset_reason_t*/ resetReason = esp_reset_reason();
  return ResetReasonToString(resetReason, shortView);
  #endif
}

const String GetResetReason(int &resetReason)
{
  return GetResetReason(resetReason, /*shortView:*/false);
}

const String GetResetReason(const bool &shortView)
{
  int reasonCode = 0;
  return GetResetReason(reasonCode, shortView);
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
  fsInfo = String(F("SPIFFS: ")) + F("Total: ") + String(total) + F(" ") + F("Used: ") + String(used) + F(" ") + F("Left: ") + String(total - used);  
}

#endif //ESP_HELPER_H