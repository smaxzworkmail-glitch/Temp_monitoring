#include "SystemService.h"

void initSystemService(AsyncWebServer* server) {
    
    // Сторінка для завантаження (можна буде потім вбудувати в твій index.html)
    server->on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
    });

    // Обробник завантаження файлів та прошивки
    server->on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(shouldReboot ? 200 : 500, "text/plain", shouldReboot ? "OK. Rebooting..." : "Update Failed");
        response->addHeader("Connection", "close");
        request->send(response);
        if (shouldReboot) delay(1000), ESP.restart();
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        
        // Якщо це перший пакет (index == 0)
        if (!index) {
            Serial.printf("Update Start: %s\n", filename.c_str());
            
            // Перевіряємо, чи це прошивка
            if (filename.endsWith(".bin")) {
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            } else {
                // Якщо це просто файл (html, json), відкриваємо його на запис у LittleFS
                String path = "/" + filename;
                request->_tempFile = LittleFS.open(path, "w");
            }
        }

        // Записуємо дані
        if (filename.endsWith(".bin")) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
        } else {
            if (request->_tempFile) {
                request->_tempFile.write(data, len);
            }
        }

        // Якщо це останній пакет
        if (final) {
            if (filename.endsWith(".bin")) {
                if (Update.end(true)) {
                    Serial.printf("Update Success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            } else {
                if (request->_tempFile) {
                    request->_tempFile.close();
                    Serial.printf("File Saved: %s\n", filename.c_str());
                }
            }
        }
    });
}