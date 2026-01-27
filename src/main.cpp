#include <Arduino.h>
#include "Sensors.h"
#include "Storage.h"
#include "WebInterface.h" // 1. Додали новий модуль

// Змінні для відстеження часу (таймери)
unsigned long lastMinuteTick = 0;
unsigned long last15MinTick = 0;

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
    
    unsigned long currentMillis = millis();

    // 2. Таймер: кожну хвилину (60 000 мс)
    if (currentMillis - lastMinuteTick >= 60000) {
        addHourPoint(); // Записуємо в масив "Остання година"
        lastMinuteTick = currentMillis;
        Serial.println("Точка додана в годинний графік");
    }

    // 3. Таймер: кожні 15 хвилин (900 000 мс)
    if (currentMillis - last15MinTick >= 900000) {
        addDayPoint(); // Записуємо в масив "Доба"
        last15MinTick = currentMillis;
        Serial.println("Точка додана в добовий графік");
    }
    
    // Вивід у Serial залишаємо для контролю, 
    // але тепер дані будуть доступні і через браузер
    for (auto &s : getSensors()) {
        Serial.printf("%s: %.2f°C | ", s.name.c_str(), s.currentTemp);
    }
    Serial.println();
    
    delay(5000);
}