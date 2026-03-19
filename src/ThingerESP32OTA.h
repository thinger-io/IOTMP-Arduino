// Convenience header — includes the IOTMP ESP32 OTA extension and compatibility helpers
#ifndef THINGER_ESP32OTA_COMPAT_H
#define THINGER_ESP32OTA_COMPAT_H

#include <thinger/iotmp/arduino/ThingerESP32OTA.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP32
using thinger::iotmp::ThingerESP32OTA;
#endif

#endif
