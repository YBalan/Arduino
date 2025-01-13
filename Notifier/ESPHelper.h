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

const uint32_t GetCurrentTime(const int8_t &timeZone)
{
    // For GMT+1 with an additional hour for daylight saving time
    //configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
    // Configure time with NTP server (UTC time)
    configTime(timeZone * 3600, 0, "pool.ntp.org", "time.nist.gov");    

    // Wait for time to be set
    struct tm timeInfo;    
    int attempt = 1;
    while (!getLocalTime(&timeInfo)) {
        #ifdef SYNC_TIME_ATTEMPTS
        if (attempt >= SYNC_TIME_ATTEMPTS) break;
        #endif    
        Serial.print(F("Waiting for time...")); Serial.print(F("(")); Serial.print(attempt); Serial.print(F(")")); Serial.println();
        delay(1000);    
        attempt++;    
    }

    // Get the current time as epoch
    time_t now = time(NULL);    
    return static_cast<uint32_t>(now);
}

// Converts epoch time to a formatted date-time string
const String epochToDateTime(const time_t &epochTime, const String &format = F("%Y-%m-%d %H:%M:%S")) {
    struct tm *timeinfo;  // Pointer to struct tm which holds time values
    char buffer[30];      // Buffer to hold the formatted date-time string

    // Convert epoch time to calendar local time
    timeinfo = localtime(&epochTime);

    // Format time from struct tm into a readable format
    strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);

    // Return the formatted string
    return std::move(String(buffer));
}

// Converts a formatted date-time string to epoch time
const time_t dateTimeToEpoch(const String& dateTime, const String &format = F("%Y-%m-%d %H:%M:%S")) {
    struct tm tm;             // Struct to hold decomposed time
    memset(&tm, 0, sizeof(tm));  // Initialize tm structure

    // Populate tm structure with values extracted from string
    sscanf(dateTime.c_str(), format.c_str(), &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    // Adjust year and month values to fit struct tm conventions
    tm.tm_year -= 1900;   // Convert year to years since 1900
    tm.tm_mon--;          // Convert month from 1-12 to 0-11

    // Convert struct tm to time_t (epoch time)
    return mktime(&tm);
}

const String formatDuration(const uint32_t &start, const uint32_t &end, const String &format = F("%d %H:%M:%S")) {
  // Calculate the duration in seconds
  uint32_t duration = (end > start) ? (end - start) : (start - end);

  // Break down into days, hours, minutes, and seconds
  uint32_t days = duration / 86400;         // 1 day = 86400 seconds
  uint32_t hours = (duration % 86400) / 3600;
  uint32_t minutes = (duration % 3600) / 60;
  uint32_t seconds = duration % 60;

  // Format the string
  String result = "";
  if (days > 0) result += String(days) + " d, ";
  if (hours > 0 || days > 0) result += String(hours) + " h, ";
  if (minutes > 0 || hours > 0 || days > 0) result += String(minutes) + " min, ";
  result += String(seconds) + " sec";

  return result;
}

void PrintFSInfo(String &fsInfo)
{
  #ifdef ESP8266
  FSInfo fs_info;
  MFS.info(fs_info);   
  const auto &total = fs_info.totalBytes;
  const auto &used = fs_info.usedBytes;
  #else //ESP32
  const auto &total = MFS.totalBytes();
  const auto &used = MFS.usedBytes();
  #endif
  if(!fsInfo.isEmpty()) fsInfo += '\n';
  #ifdef LITTLEFS
  String fs = String(F("LittleFS"));
  #else
  String fs = String(F("SPIFFS"));
  #endif
  fsInfo += fs + F(": ") + F("Total: ") + String(total) + F(" ") + F("Used: ") + String(used) + F(" ") + F("Left: ") + String(total - used);  
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\n", file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1); // Recursively list subdirectories
      }
    } else {
      Serial.printf("  FILE: %s  SIZE: %d bytes\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}


#endif //ESP_HELPER_H