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
            s.currentTemp = 0.0;
            s.color = "#ff0000"; // Колір за замовчуванням
            for(int j=0; j<MAX_HOUR_POINTS; j++) s.history.hourData[j] = 0; // Зануляємо масиви
            for(int j=0; j<MAX_DAY_POINTS; j++) s.history.dayData[j] = 0; // Зануляємо масиви
            sensorList.push_back(s);
        }
    }
}

void updateTemperatures() {
    dallasSensors.requestTemperatures();
    for (auto &s : sensorList) {
        float t = dallasSensors.getTempC(s.address);
        if (t != DEVICE_DISCONNECTED_C) {
            s.currentTemp = t;
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