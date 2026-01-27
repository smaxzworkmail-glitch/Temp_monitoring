#include "WebInterface.h"
#include <WiFi.h>
#include <LittleFS.h>

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
        
        for (size_t i = 0; i < sensors.size(); i++) {
            doc[i]["name"] = sensors[i].name;
            doc[i]["temp"] = sensors[i].currentTemp;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.begin();
}