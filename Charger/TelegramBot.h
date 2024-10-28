#pragma once
#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include "FastBot.h"
#include <vector>

#ifdef ENABLE_INFO_BOT
#define BOT_INFO(...) SS_TRACE("[BOT INFO] ", __VA_ARGS__)
#else
#define BOT_INFO(...) {}
#endif

#ifdef ENABLE_TRACE_BOT
#define BOT_TRACE(...) SS_TRACE("[BOT TRACE] ", __VA_ARGS__)
#else
#define BOT_TRACE(...) {}
#endif

class TelegramBot : public FastBot
{  
  public:
  String OTAVersion;
  virtual uint8_t tickManual() {
        if (!*_callback) return 7;
        String req;
        req.reserve(120);
        _addToken(req);
        req += F("/getUpdates?limit=");
        req += ovfFlag ? 1 : _limit;    // берём по 1 сообщению если переполнен
        req += F("&offset=");
        req += ID;
        //req += F("&allowed_updates=[\"update_id\",\"message\",\"edited_message\",\"channel_post\",\"edited_channel_post\",\"callback_query\"]");
        
        #ifdef ESP8266
        #ifdef FB_DYNAMIC
        BearSSL::WiFiClientSecure client;
        client.setInsecure();
        #endif
        if (!_http->begin(client, req)) return 4;  // ошибка подключения
        #else
        if (!_http->begin(req)) return 4;   // ошибка подключения
        #endif
		
		#ifdef HTTP_TIMEOUT
			_http->setTimeout(HTTP_TIMEOUT);
		#endif
        int answ = _http->GET();
        if (answ != HTTP_CODE_OK) {
            _http->end();
            if (answ == -1 && _http) {      // заплатка для есп32
                delete _http;
                _http = new HTTPClient;
            }
            return 3;   // ошибка сервера телеграм
        }
        
        #ifndef FB_NO_OTA
        // была попытка OTA обновления. Обновляемся после ответа серверу!
        if (OTAstate >= 0) {
            String ota;
            if (OTAstate == 0) ota = F("OTA Error");
            else if (OTAstate == 1) ota = F("No updates");
            else if (OTAstate == 2) ota = String(F("OTA OK")) + F(": ") + VER + F(" -> ") + OTAVersion;
            OTAVersion.clear();
            sendMessage(ota, _otaID);
            if (OTAstate == 2){
              #ifdef ESP32
              MFS.end();
              #endif
              ESP.restart();
            }
            OTAstate = -1;
        }
        #endif
        
        int size = _http->getSize();
        #ifdef BOT_MAX_INCOME_MSG_SIZE
        BOT_INFO(F("BOT INCOME MESSAGE SIZE: "), size);
            ovfFlag = size > BOT_MAX_INCOME_MSG_SIZE;         
        #else
          ovfFlag = size > 25000;							// 1 полное сообщение на русском языке или ~5 на английском
        #endif
        uint8_t status = 1;             // OK
        if (size) {                     // не пустой ответ?
            StreamString sstring;
            if (!ovfFlag && sstring.reserve(size + 1)) {    // не переполнен и хватает памяти
                _http->writeToStream(&sstring);             // копируем
                _http->end();  
                status = parseMessages(sstring);
                #ifdef ESP32
                BOT_TRACE(F("Bot Status: "), status, F(" msg: "), sstring);                             // завершаем
                #endif
                return  status;             // парсим
            } else status = 2;                              // переполнение
        } else status = 3;                                  // пустой ответ        
        _http->end();
        BOT_TRACE(F("Bot Status: "), status);
        return status;
    }

    #ifdef FS_H
    void (*_sendFilesCallback)(const int& fileNumber, const int &filesCount, const String& filePath) = nullptr;    
    public:
    void attachSendFilesCallback(void (*handler)(const int& fileNumber, const int &filesCount, const String& filePath)) {
        _sendFilesCallback = handler;
    }

    // отключение обработчика сообщений
    void detachSendFilesCallback() {
        _sendFilesCallback = nullptr;
    }
 
    private:
    void _sendFilesRoutine(FB_SECURE_CLIENT& client, const std::vector<String> &files) {        
        // Start MFS if not started
        if (!MFS.begin()) {
            BOT_TRACE("Failed to mount FS");
            return;
        }

        int fileNumber = 1;
        for (const auto &filename : files) {            
            BOT_TRACE(F("Sending file: "), filename);   

            if (*_sendFilesCallback){
                _sendFilesCallback(fileNumber, files.size(), filename);
            }

            yield(); // watchdog
            // Open the file for reading
            File file = MFS.open(filename.c_str(), FILE_READ);
            if (!file) {
                BOT_TRACE(F("Failed to open: "), filename);                
                continue;
            }

            _sendFileRoutine(client, file);
          
            file.close();  // Close the file after finished transmitting
            fileNumber++;
        }
    }    
   
    public:
    uint8_t sendFile(const std::vector<String> &files, const uint32_t size, const String& name, const String& id) {
        FB_DECLARE_CLIENT();        
        if (!_multipartSend(client, size, FB_DOC, name, id)) return 4;
          _sendFilesRoutine(client, files);
        return _multipartEnd(client);
    }
    #endif
};

#endif //TELEGRAM_BOT_H