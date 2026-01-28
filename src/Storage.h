#ifndef STORAGE_H
#define STORAGE_H

#include <LittleFS.h>
#include <ArduinoJson.h>

// Структура для зберігання конфіґурації в RAM
struct AppConfig {
    String title = "Система Моніторингу";
    String ntpServer = "pool.ntp.org";
    long tzOffset = 7200;  // UTC+2 в секундах
    String wifiSSID = "TempMon";
    String wifiPassword = "TempMon1pass";
    int updateInterval = 30000;  // мс
    int version = 1;
};

bool initStorage();
void saveConfig(const JsonDocument& doc);
JsonDocument loadConfig();
AppConfig loadAppConfig();
void saveAppConfig(const AppConfig& config);

#endif