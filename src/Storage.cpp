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