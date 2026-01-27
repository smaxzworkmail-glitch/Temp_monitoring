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

std::vector<TemperatureSensor>& getSensors() {
    return sensorList;
}