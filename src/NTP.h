#ifndef MY_NTP_H
#define MY_NTP_H

#include <Arduino.h>
#include <time.h>

void initNTP(const char* server);
String getTimeStr(); // Повертає час як "12:30:05"
bool isTimeSynced();
#endif