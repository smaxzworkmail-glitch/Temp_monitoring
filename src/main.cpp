#include <Arduino.h>
#include "Sensors.h"
#include "Storage.h"
#include "WebInterface.h"
#include "MyWiFi.h"
#include "NTP.h"

// Змінні для відстеження часу (таймери)
unsigned long lastMinuteTick = 0;
unsigned long last15MinTick = 0;
unsigned long lastSensorUpdate = 0;

void setup() {
    Serial.begin(115200);

    initWiFi("TempMon", "TempMon1pass");
    initNTP("pool.ntp.org");

    // Ініціалізація файлової системи
    if (!LittleFS.begin(true)) {
        Serial.println("Помилка LittleFS!");
    } else {
        Serial.println("LittleFS запущено.");
    }

    // Ініціалізація датчиків на GPIO 4
    initSensors(4);
    Serial.printf("Знайдено датчиків: %d\n", getSensors().size());

    // Запускаємо WiFi та Веб-сервер
    initWebInterface(); 
}

void loop() {
    unsigned long currentMillis = millis(); // Використовуємо одну назву всюди

    // 1. Опитування датчиків раз на 30 секунд
    if (currentMillis - lastSensorUpdate >= 30000) {
        updateTemperatures(); // Тут тепер працює твоя логіка з фільтрацією
        lastSensorUpdate = currentMillis;
    
        // Вивід для контролю в термінал
        Serial.printf("Замір: ", getTimeStr().c_str());
        for (auto &s : getSensors()) {
            Serial.printf("%s: %.2f°C | ", s.name.c_str(), s.currentTemp);
        }
        Serial.println();
    }

    if (isTimeSynced()) {
        // 2. Таймер: кожну хвилину (60 000 мс)
        if (currentMillis - lastMinuteTick >= 60000) {
            addHourPoint(); // Записуємо в масив "Остання година"
            lastMinuteTick = currentMillis;
            Serial.println("-- Точка додана в годинний графік");
        }

        // 3. Таймер: кожні 15 хвилин (900 000 мс)
        if (currentMillis - last15MinTick >= 900000) {
            addDayPoint(); // Записуємо в масив "Доба"
            last15MinTick = currentMillis;
            Serial.println("-- Точка додана в добовий графік");
        }
    } else {
        // Якщо час ще не синхронізовано, оновлюємо таймери, 
        // щоб вони не "вистрілили" купою точок одразу після синхронізації
        lastMinuteTick = currentMillis;
        last15MinTick = currentMillis;
    }

    delay(100); // Мінімальна пауза для стабільності фонових процесів
}