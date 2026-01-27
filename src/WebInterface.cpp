#include "WebInterface.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "NTP.h"
#include <deque>

AsyncWebServer server(80);


std::deque<String> systemLogs;

// Функція для додавання нового логу
void addLog(String msg) {
    String timestampedMsg = "[" + getTimeStr() + "] " + msg;
    systemLogs.push_back(timestampedMsg);
    if (systemLogs.size() > 20) systemLogs.pop_front(); // Тримаємо останні 20 записів
}

void initWebInterface() {

    // Головна сторінка
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
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


server.on("/api/slots", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    for (auto &s : getSensors()) {
        doc[s.name] = (s.currentTemp < -50) ? "error" : String(s.currentTemp, 2);
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
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