#include "Storage.h"

bool initStorage() {
    return LittleFS.begin(true);
}

void saveConfig(const JsonDocument& doc) {
    File file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file) return;
    serializeJson(doc, file);
    file.close();
}

JsonDocument loadConfig() {
    JsonDocument doc;
    File file = LittleFS.open("/config.json", FILE_READ);
    if (!file) return doc;
    deserializeJson(doc, file);
    file.close();
    return doc;
}

// Завантажити конфіґ з JSON у структуру AppConfig
AppConfig loadAppConfig() {
    AppConfig config;
    JsonDocument doc = loadConfig();
    
    if (doc.containsKey("title")) config.title = doc["title"].as<String>();
    if (doc.containsKey("ntpServer")) config.ntpServer = doc["ntpServer"].as<String>();
    if (doc.containsKey("tzOffset")) config.tzOffset = doc["tzOffset"].as<long>();
    if (doc.containsKey("updateInterval")) config.updateInterval = doc["updateInterval"].as<int>();
    if (doc["wifi"].containsKey("ssid")) config.wifiSSID = doc["wifi"]["ssid"].as<String>();
    if (doc["wifi"].containsKey("password")) config.wifiPassword = doc["wifi"]["password"].as<String>();
    
    return config;
}

// Зберегти структуру AppConfig назад у JSON
void saveAppConfig(const AppConfig& config) {
    JsonDocument doc;
    doc["version"] = config.version;
    doc["title"] = config.title;
    doc["ntpServer"] = config.ntpServer;
    doc["tzOffset"] = config.tzOffset;
    doc["updateInterval"] = config.updateInterval;
    doc["wifi"]["ssid"] = config.wifiSSID;
    doc["wifi"]["password"] = config.wifiPassword;
    
    saveConfig(doc);
}