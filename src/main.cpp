#include <Arduino.h>
#include "Sensors.h"
#include "Storage.h"
#include "WebInterface.h" // 1. Додали новий модуль

void setup() {
    Serial.begin(115200);

    // Ініціалізація файлової системи
    if (!LittleFS.begin(true)) {
        Serial.println("Помилка LittleFS!");
    } else {
        Serial.println("LittleFS запущено.");
    }

    // Ініціалізація датчиків на GPIO 4
    initSensors(4);
    Serial.printf("Знайдено датчиків: %d\n", getSensors().size());

    // 2. Запускаємо WiFi та Веб-сервер
    initWebInterface(); 
}

void loop() {
    updateTemperatures();
    
    // Вивід у Serial залишаємо для контролю, 
    // але тепер дані будуть доступні і через браузер
    for (auto &s : getSensors()) {
        Serial.printf("%s: %.2f°C | ", s.name.c_str(), s.currentTemp);
    }
    Serial.println();
    
    delay(5000);
}