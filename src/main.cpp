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

    handleSensors();

    delay(100); // Мінімальна пауза для стабільності фонових процесів
}