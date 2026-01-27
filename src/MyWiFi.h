#ifndef MY_WIFI_H
#define MY_WIFI_H

#include <WiFi.h>

void initWiFi(const char* ssid, const char* password);
bool isWiFiConnected();

#endif