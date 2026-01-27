#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>

// Константи для графіків
#define MAX_HOUR_POINTS 60   // 60 точок (кожна хвилина)
#define MAX_DAY_POINTS 96    // 96 точок (кожні 15 хв: 24г * 4)

struct SensorHistory {
    float hourData[MAX_HOUR_POINTS];
    float dayData[MAX_DAY_POINTS];
    int hourIdx = 0;
    int dayIdx = 0;
    bool hourFull = false;
    bool dayFull = false;
};

struct TemperatureSensor {
    DeviceAddress address;
    String name;
    float currentTemp;
    float lastValues[5]; // Останні 5 значень для фільтра
    int filterIdx = 0;
    String color; // Для графіків
    SensorHistory history;
};

// Функції модуля
void initSensors(uint8_t pin);
void updateTemperatures();
void addHourPoint(); // Додати точку для графіка години
void addDayPoint();  // Додати точку для графіка доби
std::vector<TemperatureSensor>& getSensors();

#endif