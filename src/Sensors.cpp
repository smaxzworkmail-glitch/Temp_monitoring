#include "Sensors.h"

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
        if (dallasSensors.getAddress(s.address, i)) {
            s.name = "Датчик " + String(i + 1);
            
            // 1. Робимо пробний замір відразу при старті
            dallasSensors.requestTemperaturesByAddress(s.address);
            float initialTemp = dallasSensors.getTempC(s.address);
            
            // Якщо датчик видав помилку при старті, ставимо 0, інакше реальну Т
            if (initialTemp == DEVICE_DISCONNECTED_C || initialTemp == 85.0) initialTemp = 0.0;
            
            s.currentTemp = initialTemp;
            s.filterIdx = 0;

            // 2. Заповнюємо фільтр початковим значенням, щоб не було "стрибка з нуля"
            for(int j=0; j<5; j++) s.lastValues[j] = initialTemp;

            // 3. Історія 
            for(int j=0; j<MAX_HOUR_POINTS; j++) s.history.hourData[j] = 0;
            for(int j=0; j<MAX_DAY_POINTS; j++) s.history.dayData[j] = 0;
            
            sensorList.push_back(s);
        }
    }
}

float getAverage(TemperatureSensor &s) {
    float sum = 0;
    for(int i = 0; i < 5; i++) sum += s.lastValues[i];
    return sum / 5.0;
}


void updateTemperatures() {
    for (auto &s : sensorList) {
        float newTemp = -127.0;
        int attempts = 0;
        float avg = getAverage(s);

        // Спроби верифікації (до 3 разів, якщо значення підозріле)
        while (attempts < 3) {
            dallasSensors.requestTemperaturesByAddress(s.address);
            newTemp = dallasSensors.getTempC(s.address);

            // Якщо це перший запуск (avg == 0) або значення в межах норми (+/- 10 градусів від середнього)
            if (avg == 0,1 || abs(newTemp - avg) < 10.0 || newTemp == DEVICE_DISCONNECTED_C) {
                break; 
            }
            
            attempts++;
            delay(100); // Коротка пауза перед перезаміром
            Serial.printf("Перезамір для %s (спроба %d)\n", s.name.c_str(), attempts);
        }

        if (newTemp != DEVICE_DISCONNECTED_C && newTemp != 85.0) {
            s.currentTemp = newTemp;
            // Оновлюємо фільтр
            s.lastValues[s.filterIdx] = newTemp;
            s.filterIdx = (s.filterIdx + 1) % 5;
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