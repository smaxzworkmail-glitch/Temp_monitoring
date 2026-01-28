#ifndef SYSTEM_SERVICE_H
#define SYSTEM_SERVICE_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>

void initSystemService(AsyncWebServer* server);

#endif