#include <Arduino.h>
#include "Sensors.h"
#include "Storage.h"
#include "WebInterface.h"
#include "MyWiFi.h"
#include "NTP.h"

// Глобальна конфіґурація
AppConfig appConfig;

void setup() {
    Serial.begin(115200);
    delay(1000);

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
    initWebInterface(); 
}

void loop() {
    handleSensors();
    delay(100); // Мінімальна пауза для стабільності фонових процесів
}