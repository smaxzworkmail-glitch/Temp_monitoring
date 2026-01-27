#include "NTP.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;      // UTC+2
const int   daylightOffset_sec = 3600; // Літній час

void initNTP(const char* server) {
    // Використовуємо аргумент 'server' замість жорстко прописаного тексту
    configTime(gmtOffset_sec, daylightOffset_sec, server);
    Serial.printf("NTP sync started using server: %s\n", server);
}

String getTimeStr() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return "Sync Error";
    
    char timeStringBuff[9];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

bool isTimeSynced() {
    struct tm timeinfo;
    // getLocalTime повертає false, якщо час ще не синхронізовано
    return getLocalTime(&timeinfo);
}