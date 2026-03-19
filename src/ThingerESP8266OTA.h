// Convenience header — includes the IOTMP ESP8266 OTA extension and compatibility helpers
#ifndef THINGER_ESP8266OTA_COMPAT_H
#define THINGER_ESP8266OTA_COMPAT_H

#include <thinger/iotmp/arduino/ThingerESP8266OTA.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP8266
using thinger::iotmp::ThingerESP8266OTA;
#endif

#endif
