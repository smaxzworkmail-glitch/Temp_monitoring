#include "WebInterface.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

void initWebInterface() {
    // Налаштування WiFi (замініть на свої дані або зробіть точку доступу)
    WiFi.begin("drugabulka", "2bulka2023");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi підключено!");
    Serial.print("IP адреса: ");
    Serial.println(WiFi.localIP());

    // Головна сторінка
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // API для отримання даних датчиків (JSON)
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        auto& sensors = getSensors();
        
        for (auto &s : sensors) {
            JsonObject sObj = doc.add<JsonObject>();
            sObj["name"] = s.name;
            sObj["current"] = s.currentTemp;
            
            // Збираємо годинні дані
            JsonArray hArr = sObj["hour"].to<JsonArray>();
            int hStart = s.history.hourFull ? s.history.hourIdx : 0;
            int hCount = s.history.hourFull ? MAX_HOUR_POINTS : s.history.hourIdx;
            for (int i = 0; i < hCount; i++) {
                hArr.add(s.history.hourData[(hStart + i) % MAX_HOUR_POINTS]);
            }
            
            // Збираємо добові дані
            JsonArray dArr = sObj["day"].to<JsonArray>();
            int dStart = s.history.dayFull ? s.history.dayIdx : 0;
            int dCount = s.history.dayFull ? MAX_DAY_POINTS : s.history.dayIdx;
            for (int i = 0; i < dCount; i++) {
                dArr.add(s.history.dayData[(dStart + i) % MAX_DAY_POINTS]);
            }
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.begin();
}