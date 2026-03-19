// Convenience header — includes the IOTMP ESP8266 WebConfig and compatibility helpers
#ifndef THINGER_ESP8266WEBCONFIG_COMPAT_H
#define THINGER_ESP8266WEBCONFIG_COMPAT_H

#include <thinger/iotmp/arduino/ThingerESP8266WebConfig.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP8266
using thinger::iotmp::ThingerESP8266WebConfig;
#endif

#endif
