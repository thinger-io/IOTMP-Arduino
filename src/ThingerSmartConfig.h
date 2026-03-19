// Convenience header — includes the IOTMP SmartConfig client and compatibility helpers
#ifndef THINGER_SMARTCONFIG_COMPAT_H
#define THINGER_SMARTCONFIG_COMPAT_H

#include <thinger/iotmp/arduino/ThingerSmartConfig.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP8266
using thinger::iotmp::ThingerSmartConfig;
#endif

#endif
