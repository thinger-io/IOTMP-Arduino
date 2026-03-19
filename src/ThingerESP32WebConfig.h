// Convenience header — includes the IOTMP ESP32 WebConfig and compatibility helpers
#ifndef THINGER_ESP32WEBCONFIG_COMPAT_H
#define THINGER_ESP32WEBCONFIG_COMPAT_H

#include <thinger/iotmp/arduino/ThingerESP32WebConfig.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP32
using thinger::iotmp::ThingerESP32WebConfig;
#endif

#endif
