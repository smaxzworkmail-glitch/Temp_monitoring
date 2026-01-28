#include <Arduino.h>
#include "Sensors.h"
#include "Storage.h"
#include "WebInterface.h"
#include "MyWiFi.h"
#include "NTP.h"
#include "SystemService.h"

// Глобальна конфіґурація
AppConfig appConfig;

void setup() {
    Serial.begin(115200);
    delay(500);


    // Діагностика причини рестарту
    esp_reset_reason_t r0 = esp_reset_reason();
    Serial.printf("Reset reason (CPU0): %d\n", (int)r0);
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Chip revision: %u\n", ESP.getChipRevision());
    // Продовжити штатну ініціалізацію...
    // Ініціалізація файлової системи
    if (!initStorage()) {
        Serial.println("Помилка LittleFS!");
    } else {
        Serial.println("LittleFS запущено.");
    }

    // Завантажити конфіґурацію з флешу
    appConfig = loadAppConfig();
    Serial.printf("Конфіґ завантажена: %s\n", appConfig.title.c_str());

    // Ініціалізація WiFi з конфіґом
    initWiFi(appConfig.wifiSSID.c_str(), appConfig.wifiPassword.c_str());
    
    // Ініціалізація NTP з конфіґом
    initNTP(appConfig.ntpServer.c_str());

    // Ініціалізація датчиків на GPIO 4
    initSensors(4);
    Serial.printf("Знайдено датчиків: %d\n", getSensors().size());

    // Запускаємо Веб-сервер
    AsyncWebServer* serverPtr = initWebInterface(); 
    initSystemService(serverPtr);
}

void loop() {
    handleSensors();
    delay(100); // Мінімальна пауза для стабільності фонових процесів
}