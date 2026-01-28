#include "WebInterface.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "NTP.h"
#include "Storage.h"
#include <deque>
#include <time.h>

AsyncWebServer server(80);

std::deque<String> systemLogs;

// Extern для доступу до глобальної конфіґурації з main.cpp
extern AppConfig appConfig;
extern void initNTP(const char* server);

// Функція для додавання нового логу
void addLog(String msg) {
    String timestampedMsg = "[" + getTimeStr() + "] " + msg;
    systemLogs.push_back(timestampedMsg);
    if (systemLogs.size() > 100) systemLogs.pop_front();
    Serial.println(timestampedMsg);
}

void generateFakeData() {
    time_t now;
    time(&now);
    
    // Генеруємо 60 точок (одна на хвилину)
    for (int i = 60; i > 0; i--) {
        time_t fakeTime = now - (i * 60);
        struct tm * tm_info = localtime(&fakeTime);
        char tStr[10];
        strftime(tStr, 10, "%H:%M", tm_info);

        // Математика для плавних ліній
        float t1 = 45.0 + 15.0 * sin(i * 0.2);
        float t2 = 30.0 + 10.0 * sin(i * 0.3);

        // Формуємо рядок так, як його чекає твій фронтенд
        // (Приклад: "14:20 | 45.5 | 31.2")
        String fakeLog = String(tStr) + " | " + String(t1, 1) + " | " + String(t2, 1);
        
        // Додаємо в твою чергу deque<String> systemLogs
        systemLogs.push_back(fakeLog);
        if (systemLogs.size() > 100) systemLogs.pop_front();
    }
    Serial.println("Seed data generated for charts.");
}

AsyncWebServer* initWebInterface() {

    // Головна сторінка
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // API для отримання поточної конфіґурації
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["version"] = appConfig.version;
        doc["title"] = appConfig.title;
        doc["ntpServer"] = appConfig.ntpServer;
        doc["tzOffset"] = appConfig.tzOffset;
        doc["updateInterval"] = appConfig.updateInterval;
        doc["wifi"]["ssid"] = appConfig.wifiSSID;
        doc["wifi"]["password"] = appConfig.wifiPassword;
        
        // Додаємо інформацію про датчики
        JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
        auto& sensors = getSensors();
        for (size_t i = 0; i < sensors.size(); i++) {
            JsonObject sObj = sensorsArr.add<JsonObject>();
            sObj["id"] = i;
            sObj["name"] = sensors[i].name;
            sObj["color"] = sensors[i].color;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json; charset=utf-8", response);
    });

    // API для оновлення конфіґурації
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\": \"pending\"}");
    }, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        static JsonDocument updateDoc;
        
        if (index == 0) {
            updateDoc.clear();
        }
        
        // Парсимо JSON
        DeserializationError error = deserializeJson(updateDoc, data, len);
        
        if (index + len == total) {
            // Оновлюємо конфіґ в пам'яті
            if (updateDoc.containsKey("title")) {
                appConfig.title = updateDoc["title"].as<String>();
            }
            if (updateDoc.containsKey("ntpServer")) {
                String newNTP = updateDoc["ntpServer"].as<String>();
                appConfig.ntpServer = newNTP;
                initNTP(newNTP.c_str());
                addLog("NTP сервер змінено на: " + newNTP);
            }
            if (updateDoc.containsKey("updateInterval")) {
                appConfig.updateInterval = updateDoc["updateInterval"].as<int>();
            }
            
            // Оновлюємо датчики
            if (updateDoc.containsKey("sensors")) {
                auto sensorsUpd = updateDoc["sensors"].as<JsonArray>();
                auto& sensors = getSensors();
                int idx = 0;
                for (auto sensor : sensorsUpd) {
                    if (idx < (int)sensors.size()) {
                        if (sensor.containsKey("name")) {
                            sensors[idx].name = sensor["name"].as<String>();
                        }
                        if (sensor.containsKey("color")) {
                            sensors[idx].color = sensor["color"].as<String>();
                        }
                    }
                    idx++;
                }
            }
            
            // Зберігаємо на флеш
            saveAppConfig(appConfig);
            addLog("Конфіґурація оновлена і збережена");
            
            // Відправляємо відповідь
            request->send(200, "application/json", "{\"status\": \"ok\", \"message\": \"Конфіґурація оновлена\"}");
        }
    });

    server.on("/api/terminal", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        
        JsonArray logs = doc["last_logs"].to<JsonArray>();
        for (auto const& log : systemLogs) {
            logs.add(log);
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/api/terminal/raw", HTTP_GET, [](AsyncWebServerRequest *request){
        String raw = "";
        for (auto const& log : systemLogs) {
            raw += log + "\n";
        }
        request->send(200, "text/plain", raw);
    });

    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
        String out = "";
        for (auto const& msg : systemLogs) {
            out += msg + "\r\n";
        }
        request->send(200, "text/plain; charset=utf-8", out);
    });

    // API для Zabbix - окремі слоти
    server.on("/slot", HTTP_GET, [](AsyncWebServerRequest *request){
        int slotId = 0;
        if (request->hasParam("id")) {
            slotId = request->getParam("id")->value().toInt();
        }
        
        auto& sensors = getSensors();
        if (slotId >= 0 && slotId < (int)sensors.size()) {
            JsonDocument doc;
            doc["id"] = slotId;
            doc["name"] = sensors[slotId].name;
            
            if (sensors[slotId].currentTemp > -50.0) {
                doc["temp"] = sensors[slotId].currentTemp;
            } else {
                doc["temp"] = nullptr;
            }
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json; charset=utf-8", response);
        } else {
            request->send(404, "application/json", "{\"error\": \"Slot not found\"}");
        }
    });

    // API для отримання всіх слотів
    server.on("/api/slots", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        auto& sensors = getSensors();
        
        JsonArray slots = doc["slots"].to<JsonArray>();
        for (size_t i = 0; i < sensors.size(); i++) {
            JsonObject sObj = slots.add<JsonObject>();
            sObj["id"] = i;
            sObj["name"] = sensors[i].name;
            if (sensors[i].currentTemp > -50.0) {
                sObj["temp"] = sensors[i].currentTemp;
            } else {
                sObj["temp"] = nullptr;
            }
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json; charset=utf-8", response);
    });

    // API для отримання даних датчиків з часовими мітками
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        auto& sensors = getSensors();
        time_t now = time(nullptr);
        
        for (auto &s : sensors) {
            JsonObject sObj = doc.add<JsonObject>();
            sObj["name"] = s.name;
            sObj["color"] = s.color;
            sObj["current"] = s.currentTemp;
            
            // Збираємо годинні дані (60 точок за останню годину)
            JsonArray hArr = sObj["hour"].to<JsonArray>();
            JsonArray hTimeArr = sObj["hourTime"].to<JsonArray>();
            int hStart = s.history.hourFull ? s.history.hourIdx : 0;
            int hCount = s.history.hourFull ? MAX_HOUR_POINTS : s.history.hourIdx;
            
            for (int i = 0; i < hCount; i++) {
                hArr.add(s.history.hourData[(hStart + i) % MAX_HOUR_POINTS]);
                // Час у хвилинах від початку
                hTimeArr.add(i);
            }
            
            // Збираємо добові дані (96 точок за останню добу)
            JsonArray dArr = sObj["day"].to<JsonArray>();
            JsonArray dTimeArr = sObj["dayTime"].to<JsonArray>();
            int dStart = s.history.dayFull ? s.history.dayIdx : 0;
            int dCount = s.history.dayFull ? MAX_DAY_POINTS : s.history.dayIdx;
            
            for (int i = 0; i < dCount; i++) {
                dArr.add(s.history.dayData[(dStart + i) % MAX_DAY_POINTS]);
                // Час у хвилинах (15-хвилинні інтервали)
                dTimeArr.add(i * 15);
            }
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.begin();
    return &server;
}