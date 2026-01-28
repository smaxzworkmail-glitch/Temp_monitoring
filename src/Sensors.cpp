#include "Sensors.h"
#include "NTP.h"
#include <algorithm>


unsigned long lastReadTime = 0;
unsigned long lastMinTime = 0;
unsigned long last15MinTime = 0;

OneWire oneWire;
DallasTemperature dallasSensors(&oneWire);
std::vector<TemperatureSensor> sensorList;

extern void addLog(String msg);

void initSensors(uint8_t pin) {
    oneWire.begin(pin);
    dallasSensors.begin();
    
    int count = dallasSensors.getDeviceCount();
    sensorList.clear();

    for (int i = 0; i < count; i++) {
        TemperatureSensor s;
        // DallasTemperature автоматично сортує датчики за ID (адресою),
        // тому "Датчик 1" завжди буде мати найменший ID.
        if (dallasSensors.getAddress(s.address, i)) {
            s.name = "Датчик " + String(i + 1);
            
            // 1. Початковий замір, щоб фільтр не стартував з нуля
            dallasSensors.requestTemperaturesByAddress(s.address);
            float initialTemp = dallasSensors.getTempC(s.address);
            
            // Перевірка на валідність при старті
            if (initialTemp == DEVICE_DISCONNECTED_C || initialTemp == 85.0) {
                initialTemp = 0.0;
            }
            
            s.currentTemp = initialTemp;
            s.filterIdx = 0;

            // 2. Заповнюємо буфер фільтра (останні 5 значень) початковою температурою
            for(int j = 0; j < 5; j++) {
                s.lastValues[j] = initialTemp;
            }

            // 3. Ініціалізуємо масиви історії (зануляємо)
            for(int j = 0; j < MAX_HOUR_POINTS; j++) {
                s.history.hourData[j] = 0;
            }
            s.history.hourIdx = 0;
            s.history.hourFull = false;

            for(int j = 0; j < MAX_DAY_POINTS; j++) {
                s.history.dayData[j] = 0;
            }
            s.history.dayIdx = 0;
            s.history.dayFull = false;
            
            // Додаємо налаштований датчик у список
            sensorList.push_back(s);
        }
    }
    Serial.printf("Система ініціалізована. Слотів зайнято: %d\n", sensorList.size());
}

float getAverage(TemperatureSensor &s) {
    float sum = 0;
    for(int i = 0; i < 5; i++) sum += s.lastValues[i];
    return sum / 5.0;
}


void updateTemperatures() {
    if (sensorList.empty()) return;

    for (auto &s : sensorList) {
        float avg = 0;
        int count = 0;
        
        // 1. Рахуємо середнє з фільтра
        for(int j=0; j<5; j++) {
            if(s.lastValues[j] > -50.0) {
                avg += s.lastValues[j];
                count++;
            }
        }
        if(count > 0) avg /= count;

        float newTemp = -127.0;
        bool validMeasurement = false;

        // 2. Спроби вимірювання (макс 3)
        for (int attempts = 0; attempts < 3; attempts++) {
            dallasSensors.requestTemperaturesByAddress(s.address);
            newTemp = dallasSensors.getTempC(s.address);

            // Якщо датчик фізично відпав - далі намагатися нема сенсу
            if (newTemp == DEVICE_DISCONNECTED_C || newTemp == 85.0) {
                break; 
            }

            // Перевірка на адекватність (стрибок не більше 10 градусів)
            // Якщо це перший замір (avg == 0), то приймаємо будь-яке значення
            if (avg < -50.0 || count == 0 || abs(newTemp - avg) < 10.0) {
                validMeasurement = true;
                break; 
            }
            
            // Якщо значення підозріле
            addLog("Підозра на " + s.name + ": " + String(newTemp) + " (avg: " + String(avg) + ")");
            delay(200); 
        }

        // 3. Фінальне рішення за результатом спроб
        if (validMeasurement) {
            s.currentTemp = newTemp;
            s.lastValues[s.filterIdx] = newTemp;
            s.filterIdx = (s.filterIdx + 1) % 5;
        } else {
            // Якщо за 3 спроби не отримали адекватних даних АБО датчик відпав
            s.currentTemp = -127.0;
            addLog("Помилка: " + s.name + " недоступний або дає збій");
        }
    }
}

void addHourPoint() {
    for (auto &s : sensorList) {
        s.history.hourData[s.history.hourIdx] = s.currentTemp;
        s.history.hourIdx = (s.history.hourIdx + 1) % MAX_HOUR_POINTS;
        if (s.history.hourIdx == 0) s.history.hourFull = true;
    }
}

void addDayPoint() {
    for (auto &s : sensorList) {
        s.history.dayData[s.history.dayIdx] = s.currentTemp;
        s.history.dayIdx = (s.history.dayIdx + 1) % MAX_DAY_POINTS;
        if (s.history.dayIdx == 0) s.history.dayFull = true;
    }
}

void handleSensors() {
    unsigned long now = millis();

    // 1. Опитування датчиків (раз на 30 сек)
    if (now - lastReadTime >= 30000) {
        updateTemperatures(); 
        lastReadTime = now;

        String logMsg = "";
        for (auto &s : sensorList) {
            if (s.currentTemp > -50.0) {
                // Накопичуємо для обох графіків
                s.tempAccumulator += s.currentTemp;
                s.samplesCount++;
                s.dayAccumulator += s.currentTemp;
                s.daySamplesCount++;
            }
            // ВИВІД У ТЕРМІНАЛ:
            logMsg += s.name + ": " + String(s.currentTemp, 2) + "°C | ";
        }
        addLog(logMsg);
    }

    if (!isTimeSynced()) {
        lastMinTime = now;
        last15MinTime = now;
        return;
    }

    // 2. Хвилинний графік (60 точок)
    if (now - lastMinTime >= 60000) {
        for (auto &s : sensorList) {
            float avg = (s.samplesCount > 0) ? s.tempAccumulator / s.samplesCount : s.currentTemp;
            s.history.hourData[s.history.hourIdx] = avg;
            s.history.hourIdx = (s.history.hourIdx + 1) % MAX_HOUR_POINTS;
            if (s.history.hourIdx == 0) s.history.hourFull = true;
            
            s.tempAccumulator = 0; 
            s.samplesCount = 0;
        }
        lastMinTime = now;
        addLog("Система: Середнє за хвилину записано в графік.");
    }

    // 3. Добовий графік (96 точок по 15 хв) - ТЕПЕР ТЕЖ СЕРЕДНЄ
    if (now - last15MinTime >= 900000) {
        for (auto &s : sensorList) {
            float avgDay = (s.daySamplesCount > 0) ? s.dayAccumulator / s.daySamplesCount : s.currentTemp;
            s.history.dayData[s.history.dayIdx] = avgDay;
            s.history.dayIdx = (s.history.dayIdx + 1) % MAX_DAY_POINTS;
            if (s.history.dayIdx == 0) s.history.dayFull = true;
            
            s.dayAccumulator = 0;
            s.daySamplesCount = 0;
        }
        last15MinTime = now;
        addLog("Система: Середнє за 15 хв записано в добовий графік");
    }
}


std::vector<TemperatureSensor>& getSensors() {
    return sensorList;
}