#pragma once
#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <FS.h>
#include <Arduino.h>
#include <vector>
#include <time.h>

#ifdef ENABLE_INFO_DS
#define DS_INFO(...) SS_TRACE(F("[DS INFO] "), __VA_ARGS__)
#else
#define DS_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_DS
#define DS_TRACE(...) SS_TRACE(F("[DS TRACE] "), __VA_ARGS__)
#else
#define DS_TRACE(...) {}
#endif

#define fileSystem SPIFFS // LittleFS

#define YEAR_2024_SECONDS 1704075408u
#define YEAR_2021_SECONDS 1609459200u; // Example timestamp for January 1, 2021
#define UINT32_MAX        4294967295u
#define FILE_PATH "/data"
#define FILE_EXT ".csv"
#define FILE_EXT_LEN strlen(FILE_EXT)
#define FILE_NAME_FORMAT "%Y-%m-%d"
#define BUFFER_DATE_SIZE 16

#define MAX_RECORD_LENGTH   48  // Target record length with fixed size for consistent read offsets
#define RECORD_FORMAT       "%lu,%00.1f,%d,%03d:%02d:%02d,%00.1f,%s,%s,"
#define RECORD_FORMAT_SCANF "%lu,%f,%d,%d:%d:%d,%f,%s,%s"

struct Data {
  private:
    uint32_t dateTime = YEAR_2024_SECONDS;
    String resetReason;
    String wifiStatus;
  public:
    float voltage;
    bool relayOn;
    uint16_t relayOnHH;
    uint8_t relayOnMM;
    uint8_t relayOnSS;
    float temperature = 22.1;  
    static constexpr size_t recordLength = MAX_RECORD_LENGTH;

    // Default constructor
    Data() = default;

    // Copy constructor
    Data(const Data& other)
        : dateTime(other.dateTime),
          voltage(other.voltage),
          relayOn(other.relayOn),
          relayOnHH(other.relayOnHH),
          relayOnMM(other.relayOnMM),
          relayOnSS(other.relayOnSS),
          resetReason(other.resetReason),
          temperature(other.temperature),
          wifiStatus(other.wifiStatus)
    {
        // Optionally, you can add some logging or custom logic here.
    }

    // Assignment operator
    Data& operator=(const Data& other) {
        if (this == &other) {
            // Handle self-assignment (e.g., a = a)
            return *this;
        }

        // Copy all members from 'other' to 'this'
        dateTime = other.dateTime;
        voltage = other.voltage;
        relayOn = other.relayOn;
        relayOnHH = other.relayOnHH;
        relayOnMM = other.relayOnMM;
        relayOnSS = other.relayOnSS;
        resetReason = other.resetReason;
        temperature = other.temperature;
        wifiStatus = other.wifiStatus;

        return *this;
    }

    void setDateTime(const uint32_t &datetime) { dateTime = datetime < YEAR_2024_SECONDS ? dateTime + datetime : datetime;  }
    const uint32_t &getDateTime() const { return dateTime; }

    void setResetReason(const String &resetreason) { resetReason = resetreason; }
    const String &getResetReason() const { return resetReason; }

    void setWiFiStatus(const String &wifistatus) { wifiStatus = wifistatus; }
    const String &getWiFiStatus() const { return wifiStatus; }

    const String writeToCsv(size_t &realDataSize) const {
        char buffer[MAX_RECORD_LENGTH];        
        snprintf(buffer, sizeof(buffer), RECORD_FORMAT, dateTime, voltage, relayOn ? 1 : 0, relayOnHH, relayOnMM, relayOnSS, temperature, resetReason, wifiStatus);
        auto res = String(buffer);
        realDataSize = res.length();
        for(uint8_t i = realDataSize; i < MAX_RECORD_LENGTH; i++){ res += ' '; }
        return res;
    }

    const String writeToCsv() const { size_t s; return writeToCsv(s); }

    void readFromCsv(const String& str) {
        char startReasonBuff[7];
        char wifiStatusBuff[7];
        sscanf(str.c_str(), RECORD_FORMAT_SCANF, &dateTime, &voltage, (int*)&relayOn, &relayOnHH, &relayOnMM, &relayOnSS, &temperature, startReasonBuff, wifiStatusBuff);
        resetReason = String(startReasonBuff);
        wifiStatus = String(wifiStatusBuff);
    }

    //00.3,00:00:00,CL
    void readFromXYDJ(const String &str){
      char relayOnBuff[3];
      sscanf(str.c_str(), "%f,%d:%d:%d,%s", &voltage, &relayOnHH, &relayOnMM, &relayOnSS, relayOnBuff);
      relayOn = !String(relayOnBuff).startsWith(F("CL"));
    }

};

class DataStorage {
public:
    String currentFileName;
    Data lastRecord;
    String startDate;
    String endDate;
    uint8_t filesCount;

public:
    void begin() {
        if (!fileSystem.begin()) {
            DS_INFO(F("fileSystem initialization failed"));
            return;
        }
        readLastFileRecord();
        fileSystem.mkdir(FILE_PATH);
        TraceToSerial();
    }

    void TraceToSerial() const
    {
      DS_TRACE(F("CurrentFile: "), currentFileName);
      DS_TRACE(F("Last Record: "), lastRecord.writeToCsv());
      DS_TRACE(F("startDate: "), startDate, F(" "), F("endDate: "), endDate, F(" "), F("Files: "), filesCount);
    }

    void readLastFileRecord() {
        File root = fileSystem.open(FILE_PATH);
        if (!root) { TraceOpenFileFailed(FILE_PATH); return; }

        String lastFile, firstFile;
        File file = root.openNextFile();
        if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH) return; };
        while (file) {            
            const String &fileName = file.name();
            DS_TRACE(String(F("/")) + root.name() + F("/") + fileName);
            if(fileName.endsWith(FILE_EXT))
            {
              filesCount++;
              if (lastFile.isEmpty() || fileName > lastFile) {
                  lastFile = fileName;
              }
              if (firstFile.isEmpty() || fileName < firstFile) {
                  firstFile = fileName;
              }              
            }
            file.close();
            file = root.openNextFile();
        }

        file.close();

        if (!lastFile.isEmpty()) {
            lastFile = String(F("/")) + root.name() + F("/") + lastFile;            
            file = fileSystem.open(lastFile, "r");
            if (!file) { TraceOpenFileFailed(lastFile); return; }

            int seek = file.size() - ((Data::recordLength + 1) * 2);
            seek = seek < 0 ? 0 : seek;
            DS_TRACE(F("FileSize: "), file.size(), F(" "), F("Seek: "), seek);
            file.seek(seek);
            String lastString;
            while(file.available() > 0)
              lastString = file.readStringUntil('\n');
            DS_TRACE(F("Last Record: "), F("'"), lastString, F("'"), F(" "), F("Size: "), lastString.length());
            lastRecord.readFromCsv(lastString);
            currentFileName = lastFile;
            file.close();            
        }
        
        root.close();
        extractDates(firstFile, lastFile);
    }

    void appendData(Data &data, const uint32_t &dateTime) {
        data.setDateTime(dateTime);        
        const String &fileName = generateFileName(data.getDateTime());

        manageStorage();

        if (fileName != currentFileName || !fileSystem.exists(fileName)) {            
            currentFileName = fileName;
            filesCount++;
            endDate = extractDate(fileName);
        }        

        File file = fileSystem.open(fileName, "a");
        if (!file) { TraceOpenFileFailed(fileName); return; }
        
        size_t realDataSize = 0;
        String csv = data.writeToCsv(realDataSize) + '\n';
        const auto &size = file.write((uint8_t*)csv.c_str(), csv.length());
        DS_TRACE(F("Real data size: "), realDataSize, F(" "), F("Wrote: "), size);        
        file.close();
        lastRecord = data;
    }    

    const int extractAllData(String &out) {
        File root = fileSystem.open(FILE_PATH);
        if (!root) { TraceOpenFileFailed(FILE_PATH); return 0; }

        File file = root.openNextFile();
        if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH); return 0; }
        int recordsTotal = 0;        
        while (file) {
            int recordsInFile = 0;
            const String &fileName = file.name();            
            if (fileName.endsWith(FILE_EXT)) {
                //out += file.readString();
                while (file.available()) {
                    out += file.readStringUntil('\n') + '\n';
                    recordsInFile++;
                } 
                DS_TRACE(String(F("/")) + root.name() + F("/") + fileName, F(" Records: "), recordsInFile);
            }
            recordsTotal += recordsInFile;
            file.close();
            file = root.openNextFile();
        }
        file.close();
        root.close();

        return recordsTotal;
    }

    const int removeAllData()
    {
        File root = fileSystem.open(FILE_PATH);
        if (!root) { TraceOpenFileFailed(FILE_PATH); return 0; }

        File file = root.openNextFile();
        if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH); return 0; }

        filesCount = 0;

        std::vector<String> files;

        while (file) {
            const String &fileName = file.name();
            if (fileName.endsWith(FILE_EXT)) {
              files.push_back(fileName);
            }
            file.close();
            file = root.openNextFile();
        }

        for(const auto &f : files)
        {
          String fileName = String(F("/")) + root.name() + F("/") + f;
          DS_TRACE(F("Remove: "), fileName);
          fileSystem.remove(fileName);
        }

        root.close();

        return files.size();
    }

private:
    void extractDates(const String &firstFile, const String &lastFile) {
        startDate = extractDate(firstFile);
        endDate = extractDate(lastFile);
    }

    // Extract date part of the filename
    static String extractDate(const String &file) {
        return file.substring(file.lastIndexOf('/') + 1, file.length() - FILE_EXT_LEN);        
    }

    void TraceOpenFileFailed(const String &fileName) const {
        DS_TRACE(F("Failed to open file: "), fileName);
    }

private:
    void manageStorage() {
        // Check storage capacity and delete old files if necessary
        if (fileSystem.totalBytes() - fileSystem.usedBytes() < MAX_RECORD_LENGTH) {
            File root = fileSystem.open(FILE_PATH);
            if (!root) { TraceOpenFileFailed(FILE_PATH); return; }

            String oldestFile, prevFile;
            File file = root.openNextFile();
            if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH); return; }
            while (file) {
                const String &fileName = file.name();
                if (fileName.endsWith(FILE_EXT)) {
                  prevFile = fileName;                
                  if (oldestFile.isEmpty() || prevFile < oldestFile) {
                      oldestFile = prevFile;
                  }
                }
                file.close();
                file = root.openNextFile();
            }

            file.close();

            if (!oldestFile.isEmpty()) {
                oldestFile = String(F("/")) + root.name() + F("/") + oldestFile;
                DS_TRACE(F("Old file to remove: "), oldestFile, F(" "), F("Prev file: "), prevFile);
                fileSystem.remove(oldestFile);
                startDate = extractDate(prevFile);
                filesCount--;
            }

            root.close();
        }
    }

    String generateFileName(uint32_t epochTime) {
        char buffer[BUFFER_DATE_SIZE];
        struct tm *timeinfo;
        time_t tempTime = epochTime;
        timeinfo = localtime(&tempTime);
        strftime(buffer, BUFFER_DATE_SIZE, FILE_NAME_FORMAT, timeinfo);
        return String(FILE_PATH) + F("/") + buffer + FILE_EXT;
    }
};

#endif // DATASTORAGE_H