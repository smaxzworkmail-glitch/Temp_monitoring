#include "Storage.h"
#include "Sensors.h"

// Forward declaration or include the header that defines getSensors()
// If getSensors() is defined elsewhere, make sure Storage.h includes the appropriate header

bool initStorage()
{
    return LittleFS.begin(true);
}

void saveConfig(const JsonDocument &doc)
{
    File file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file)
        return;
    serializeJson(doc, file);
    file.close();
}

JsonDocument loadConfig()
{
    JsonDocument doc;
    File file = LittleFS.open("/config.json", FILE_READ);
    if (!file)
        return doc;
    deserializeJson(doc, file);
    file.close();
    return doc;
}

// Завантажити конфіґ з JSON у структуру AppConfig
AppConfig loadAppConfig()
{
    AppConfig config;
    JsonDocument doc = loadConfig();

    if (doc.containsKey("title"))
        config.title = doc["title"].as<String>();
    if (doc.containsKey("ntpServer"))
        config.ntpServer = doc["ntpServer"].as<String>();
    if (doc.containsKey("tzOffset"))
        config.tzOffset = doc["tzOffset"].as<long>();
    if (doc.containsKey("updateInterval"))
        config.updateInterval = doc["updateInterval"].as<int>();
    if (doc["wifi"].containsKey("ssid"))
        config.wifiSSID = doc["wifi"]["ssid"].as<String>();
    if (doc["wifi"].containsKey("password"))
        config.wifiPassword = doc["wifi"]["password"].as<String>();

    // ВАЖЛИВО: Ми не заповнюємо датчики тут, бо список датчиків
    // формується динамічно в Sensors.cpp після сканування шини.

    return config;
}

// Функція для накладання збережених назв на знайдені датчики
void applySensorsConfig()
{
    JsonDocument doc = loadConfig();
    if (!doc.containsKey("sensors"))
        return;

    JsonArray sensorsArr = doc["sensors"];
    auto &sensors = getSensors();

    for (JsonObject sObj : sensorsArr)
    {
        int id = sObj["id"];
        if (id >= 0 && id < (int)sensors.size())
        {
            if (sObj.containsKey("name"))
                sensors[id].name = sObj["name"].as<String>();
            if (sObj.containsKey("color"))
                sensors[id].color = sObj["color"].as<String>();
            // Додаємо нові поля:
            if (sObj.containsKey("msg"))
                sensors[id].customMessage = sObj["msg"].as<String>();
            if (sObj.containsKey("target"))
                sensors[id].targetTemp = sObj["target"].as<float>();
        }
    }
}

// Зберегти структуру AppConfig назад у JSON
void saveAppConfig(const AppConfig &config)
{
    JsonDocument doc;
    doc["version"] = config.version;
    doc["title"] = config.title;
    doc["ntpServer"] = config.ntpServer;
    doc["tzOffset"] = config.tzOffset;
    doc["updateInterval"] = config.updateInterval;
    doc["wifi"]["ssid"] = config.wifiSSID;
    doc["wifi"]["password"] = config.wifiPassword;

    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    auto &sensors = getSensors();
    for (size_t i = 0; i < sensors.size(); i++)
    {
        JsonObject sObj = sensorsArr.add<JsonObject>();
        sObj["id"] = i;
        sObj["name"] = sensors[i].name;
        sObj["color"] = sensors[i].color;
        // Зберігаємо нові поля
        sObj["msg"] = sensors[i].customMessage;
        sObj["target"] = sensors[i].targetTemp;
    }

    saveConfig(doc);
}