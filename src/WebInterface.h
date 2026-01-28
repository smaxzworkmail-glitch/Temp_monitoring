#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESPAsyncWebServer.h>
#include "Sensors.h"
#include "Storage.h"

//void initWebInterface();
AsyncWebServer* initWebInterface();

#endif