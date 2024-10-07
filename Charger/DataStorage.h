#pragma once
#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <FS.h>
#include <Arduino.h>
#include <vector>
#include <map>
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

struct FileInfo { size_t size; int linesCount; };
typedef std::map<String, FileInfo> FilesInfo;

#define fileSystem SPIFFS // LittleFS

#define YEAR_2024_SECONDS   1704075408u
#define YEAR_2021_SECONDS   1609459200u; // Example timestamp for January 1, 2021
#define MILLIS_MAX_SECONDS  4294967u
#define UINT32_MAX          4294967295u
#define FILE_PATH "/data"
#define FILE_EXT ".csv"
#define FILE_EXT_LEN strlen(FILE_EXT)
#define FILE_NAME_FORMAT F("%Y-%m-%d")
#define BUFFER_DATE_SIZE 30

#define MAX_RECORD_LENGTH   62  // Target record length with fixed size for consistent read offsets
//#define RECORD_FORMAT       "%lu,%.1f,%d,%03d:%02d:%02d,%.1f,%s,%s,"
//#define RECORD_FORMAT_SCANF "%lu,%f,%d,%d:%d:%d,%f,%s,%s,"

#define HEADER_FILE_NAME    F("0000header")
#define RECORD_HEADER       String(F("DateTime"))+F(",")+F("Voltage")+F(",")+F("RelayOnOff")+F(",")+F("RelayTime")+F(",")+F("Temperature")+F(",")+F("Num")+F(",")+F("ESP")+F(",")+F("R1")+F(",")+F("WiFi")+F(",")+F("R2")
#define RECORD_FORMAT       "%s,%.1f,%d,%03d:%02d:%02d,%.1f,%04d,%s,%01d,%s,%01d"
#define RECORD_FORMAT_SCANF "%d-%d-%d %d:%d:%d,%f,%d,%d:%d:%d,%f,%d,%s,%d,%s,%d"

#define EXCEL_DATE_FORMAT F("%Y-%m-%d %H:%M:%S")

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
    uint16_t reserv1; 
    uint16_t reserv2;
    uint16_t count; 
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
          wifiStatus(other.wifiStatus),
          reserv1(other.reserv1),
          reserv2(other.reserv2),
          count(other.count)
    {
        // Optionally, you can add some logging or custom logic here.
        DS_TRACE(F("Data copy ctor"));
    }

    // Assignment operator
    Data& operator=(const Data& other) {        
        if (this == &other) {
            // Handle self-assignment (e.g., a = a)
            DS_TRACE(F("Data self-assignment"));
            return *this;
        }

        DS_TRACE(F("Data assignment"));

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
        reserv1 = other.reserv1;
        reserv2 = other.reserv2;
        count = other.count;

        return *this;
    }

    const String dateTimeToString(const String &format = EXCEL_DATE_FORMAT) const{
      return epochToDateTime(dateTime, format);
    }

    void setDateTime(const uint32_t &datetime) { dateTime = datetime < MILLIS_MAX_SECONDS ? dateTime + datetime : datetime;  }
    const uint32_t &getDateTime() const { return dateTime; }

    void setResetReason(const String &resetreason) { resetReason = resetreason; }
    const String &getResetReason() const { return resetReason; }

    void setWiFiStatus(const String &wifistatus) { wifiStatus = wifistatus; }
    const String &getWiFiStatus() const { return wifiStatus; }

    const String writeToCsv(size_t &realDataSize) const {
        //DS_TRACE(F("writeToCsv"));
        char buffer[MAX_RECORD_LENGTH];        
        const String &dateTimeStr = epochToDateTime(dateTime, EXCEL_DATE_FORMAT);
        //DS_TRACE(dateTimeStr);

        snprintf(buffer, sizeof(buffer), RECORD_FORMAT, dateTimeStr.c_str(), voltage, relayOn ? 1 : 0, relayOnHH, relayOnMM, relayOnSS, temperature, count, resetReason.c_str(), reserv1, wifiStatus.c_str(), reserv2);
        auto res = String(buffer);
        realDataSize = res.length();
        for(uint8_t i = realDataSize; i < MAX_RECORD_LENGTH; i++){ res += ' '; }
        return res;
    }

    const String writeToCsv() const { size_t s; return writeToCsv(s); }

    void readFromCsv(const String& str) {
        DS_TRACE(F("readFromCsv"));
        char startReasonBuff[6];
        char wifiStatusBuff[6];        

        struct tm tm;             // Struct to hold decomposed time
        memset(&tm, 0, sizeof(tm));  // Initialize tm structure

        sscanf(str.c_str(), RECORD_FORMAT_SCANF, &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &voltage, (int*)&relayOn, &relayOnHH, &relayOnMM, &relayOnSS, &temperature, &count, startReasonBuff, &reserv1, wifiStatusBuff, &reserv2);        

        resetReason = String(startReasonBuff);
        wifiStatus = String(wifiStatusBuff);  

        DS_TRACE("dateTimeToEpoch: ", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        // Adjust year and month values to fit struct tm conventions
        tm.tm_year -= 1900;   // Convert year to years since 1900
        tm.tm_mon--;          // Convert month from 1-12 to 0-11      
        
        dateTime = mktime(&tm);
        DS_TRACE(F("From str: "), dateTime, F(" to epochTime: "), epochToDateTime(dateTime, EXCEL_DATE_FORMAT));        
    }

    //00.3,00:00:00,CL
    void readFromXYDJ(const String &str){
      char relayOnBuff[3];
      sscanf(str.c_str(), "%f,%d:%d:%d,%s", &voltage, &relayOnHH, &relayOnMM, &relayOnSS, relayOnBuff);
      relayOn = !String(relayOnBuff).startsWith(F("CL"));
    }

    public:
    static const String epochToDateTime(const time_t &epochTime, const String &fromat){
      char buffer[BUFFER_DATE_SIZE];
      struct tm *timeinfo;
      time_t tempTime = epochTime;
      timeinfo = localtime(&tempTime);
      strftime(buffer, BUFFER_DATE_SIZE, fromat.c_str(), timeinfo);
      return buffer;
    }

    static const uint32_t dateTimeToEpoch(const String& dateTime, const String &format) {
        struct tm tm;             // Struct to hold decomposed time
        memset(&tm, 0, sizeof(tm));  // Initialize tm structure

        // Populate tm structure with values extracted from string
        sscanf(dateTime.c_str(), format.c_str(), &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

        DS_TRACE("dateTimeToEpoch: ", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        // Adjust year and month values to fit struct tm conventions
        tm.tm_year -= 1900;   // Convert year to years since 1900
        tm.tm_mon--;          // Convert month from 1-12 to 0-11

        // Convert struct tm to time_t (epoch time)
        return static_cast<uint32_t>(mktime(&tm));
    }

};

class DataStorage {
private:
  FilesInfo filesInfo;
  int filesCount;
public:
    String currentFileName;
    Data lastRecord;
    String startDate;
    String endDate;    

public:
    const Data& begin() {
        if (!fileSystem.begin()) {
            DS_INFO(F("fileSystem initialization failed"));
            return Data();
        }
        readLastFileRecord();
        fileSystem.mkdir(FILE_PATH);

        String headerName = String(HEADER_FILE_NAME) + FILE_EXT;
        String headerPath = String(FILE_PATH) + F("/") + headerName;        
        File header = fileSystem.open(headerPath.c_str(), "w");
        if(!header) TraceOpenFileFailed(headerPath);        
        else 
        { 
          String headerContent = RECORD_HEADER + '\n';
          header.write((uint8_t*)headerContent.c_str(), headerContent.length() + 1);          
          header.close();
          filesInfo[headerName] = { headerContent.length(), 1 };
        }

        TraceToSerial();

        return lastRecord;
    }

    void TraceToSerial() const
    {
      DS_TRACE(F("CurrentFile: "), currentFileName);
      DS_TRACE(F("Last Record: "), lastRecord.writeToCsv(), F(" "), F("Count: "), lastRecord.count);
      DS_TRACE(F("startDate: "), startDate, F(" "), F("endDate: "), endDate, F(" "), F("Files: "), filesCount);
    }

    const int &getFilesCount()
    {
      return filesCount;
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
            if(fileName.length() == 10 + FILE_EXT_LEN && fileName.endsWith(FILE_EXT) && !fileName.startsWith(HEADER_FILE_NAME))
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
            data.count = 0;
            endDate = extractDate(fileName);
        }        

        File file = fileSystem.open(fileName, "a");
        if (!file) { TraceOpenFileFailed(fileName); return; }
        
        size_t realDataSize = 0;
        data.count++;
        String csv = data.writeToCsv(realDataSize) + '\n';
        const auto &size = file.write((uint8_t*)csv.c_str(), csv.length());
        DS_TRACE(F("Real data size: "), realDataSize, F(" "), F("Wrote: "), size);        
        file.close();
        lastRecord = data;
    }    

    const bool fileFilterPrepare(String &filter){      
        filter.replace('_', '-');
        return true;
    }

    const FilesInfo downloadData(String &filter, int &recordsTotal, uint32_t &totalSize) {
        FilesInfo result;
        totalSize = 0;
        recordsTotal = 0;
        filesCount = 0; 

        File root = fileSystem.open(FILE_PATH);
        if (!root) { TraceOpenFileFailed(FILE_PATH); return filesInfo; }

        File file = root.openNextFile();
        if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH); return filesInfo; }

        fileFilterPrepare(filter);

        while (file) {
            int recordsInFile = 0;
            const String &fileName = file.name();                  
            if(fileName.startsWith(HEADER_FILE_NAME)) {
              recordsInFile = 1;
              totalSize += file.size();
              filesInfo[fileName] = { file.size(), recordsInFile };
              result[fileName] = { file.size(), recordsInFile };
            }
            else if (fileName.endsWith(FILE_EXT)) {              
              filesCount++;
              if(filter.isEmpty() || fileName.startsWith(filter)) {
                //DS_TRACE(F("endDate: "), endDate, F(" "), F("fileName: "), fileName, F(" "), F("filesInfo.size: "), filesInfo.size());
                totalSize += file.size();
                if(fileName.startsWith(endDate) || filesInfo.count(fileName) == 0){              
                  //out += file.readString();                
                  while (file.available()) {
                      file.readStringUntil('\n') + '\n';                    
                      recordsInFile++;
                  } 
                  DS_TRACE(String(F("/")) + root.name() + F("/") + fileName, F(" Records: "), recordsInFile);
                  filesInfo[fileName] = { file.size(), recordsInFile };
                  result[fileName] = { file.size(), recordsInFile };
                }
                else {
                  const auto &fileInfo = filesInfo[fileName];
                  recordsInFile = fileInfo.linesCount;
                  result[fileName] = { file.size(), recordsInFile };
                  DS_TRACE(String(F("/")) + root.name() + F("/") + fileName, F(" Records: "), recordsInFile, F(" "), F("Exist in map: "), filesInfo.size()); 
                }
              }
            }
            recordsTotal += recordsInFile;            
            file.close();
            file = root.openNextFile();
        }
        file.close();
        root.close();

        return std::move(result);
    }

    const int removeAllExceptLast()
    {
      if(!endDate.isEmpty())
        return removeData(endDate, /*except:*/true);
      return 0;
    }

    const int removeData(String &filter, const bool &except = false)
    {
        File root = fileSystem.open(FILE_PATH);
        if (!root) { TraceOpenFileFailed(FILE_PATH); return 0; }

        File file = root.openNextFile();
        if(!file) { DS_TRACE(F("No files found in: "), FILE_PATH); return 0; }

        fileFilterPrepare(filter);

        filesCount = 0;
        uint8_t removedFiles = 0;

        std::vector<String> files;

        while (file) {
            const String &fileName = file.name();
            if (fileName.endsWith(FILE_EXT)) {
              filesCount++;
              files.push_back(fileName);
            }
            file.close();
            file = root.openNextFile();
        }

        for(const auto &f : files)
        {
          if(filter.isEmpty() || (f.startsWith(filter) == !except))
          {
            String fileName = String(F("/")) + root.name() + F("/") + f;
            DS_TRACE(F("Remove: "), fileName);
            fileSystem.remove(fileName);
            filesInfo.erase(f);
            filesCount--; 
            removedFiles++;           
          }
        }

        root.close();

        return removedFiles;
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
                if (fileName.endsWith(FILE_EXT) && !fileName.startsWith(HEADER_FILE_NAME)) {
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
                String oldestFilePath = String(F("/")) + root.name() + F("/") + oldestFile;
                DS_TRACE(F("Old file to remove: "), oldestFilePath, F(" "), F("Prev file: "), prevFile);
                fileSystem.remove(oldestFilePath);
                filesInfo.erase(oldestFile);
                startDate = extractDate(prevFile);
                filesCount--;
            }

            root.close();
        }
    }

    static const String generateFileName(const uint32_t &epochTime) {        
        return String(FILE_PATH) + F("/") + Data::epochToDateTime(epochTime, FILE_NAME_FORMAT) + FILE_EXT;
    }
};

std::unique_ptr<DataStorage> ds(new DataStorage());

#endif // DATASTORAGE_H