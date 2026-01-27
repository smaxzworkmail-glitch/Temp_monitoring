#ifndef STORAGE_H
#define STORAGE_H

#include <LittleFS.h>
#include <ArduinoJson.h>

bool initStorage();
void saveConfig(const JsonDocument& doc);
JsonDocument loadConfig();

#endif