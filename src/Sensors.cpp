#include "Sensors.h"
#include <algorithm>

OneWire oneWire;
DallasTemperature dallasSensors(&oneWire);
std::vector<TemperatureSensor> sensorList;

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
        
        // 1. Розраховуємо середнє лише з валідних значень у фільтрі
        for(int j=0; j<5; j++) {
            if(s.lastValues[j] > -50.0) { // Ігноруємо порожні комірки
                avg += s.lastValues[j];
                count++;
            }
        }
        if(count > 0) avg /= count;

        float newTemp = -127.0;
        int attempts = 0;

        // 2. Спроби верифікації
        while (attempts < 3) {
            dallasSensors.requestTemperaturesByAddress(s.address);
            newTemp = dallasSensors.getTempC(s.address);

            // ЗАХИСТ: Якщо датчик відключено - негайно припиняємо опитування цього слота
            if (newTemp == DEVICE_DISCONNECTED_C) break;

            // Перевірка адекватності:
            // Якщо це старт (avg == 0) АБО значення близько до середнього (+/- 10 градусів)
            if (avg <= 0.1 || abs(newTemp - avg) < 10.0) {
                break; 
            }
            
            attempts++;
            delay(150); // Пауза для стабілізації шини
            Serial.printf("Підозріле значення на %s: %.2f (avg: %.2f). Спроба %d\n", 
                          s.name.c_str(), newTemp, avg, attempts);
        }

        // 3. Фінальний запис лише якщо дані валідні
        if (newTemp != DEVICE_DISCONNECTED_C && newTemp != 85.0) {
            s.currentTemp = newTemp;
            // Оновлюємо фільтр для наступних замірів
            s.lastValues[s.filterIdx] = newTemp;
            s.filterIdx = (s.filterIdx + 1) % 5;
        } else if (newTemp == DEVICE_DISCONNECTED_C) {
            Serial.printf("Помилка: %s не відповідає!\n", s.name.c_str());
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

std::vector<TemperatureSensor>& getSensors() {
    return sensorList;
}