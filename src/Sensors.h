#ifndef SENSORS_H
#define SENSORS_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>

struct TemperatureSensor {
    DeviceAddress address;
    String name;
    float currentTemp;
    String color; // Для графіків
};

void initSensors(uint8_t pin);
void updateTemperatures();
std::vector<TemperatureSensor>& getSensors();

#endif