#pragma once
#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include "FastBot.h"

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
            if (OTAstate == 2) ESP.restart();
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
                _http->end();                               // завершаем
                return parseMessages(sstring);              // парсим
            } else status = 2;                              // переполнение
        } else status = 3;                                  // пустой ответ        
        _http->end();
        return status;
    }
};

#endif //TELEGRAM_BOT_H