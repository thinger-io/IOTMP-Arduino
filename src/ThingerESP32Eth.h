// Convenience header — includes the IOTMP ESP32 Ethernet client and compatibility helpers
#ifndef THINGER_ESP32ETH_COMPAT_H
#define THINGER_ESP32ETH_COMPAT_H

#include <thinger/iotmp/arduino/ThingerESP32Eth.h>
#include <thinger/iotmp/arduino/compat.hpp>

#ifdef ESP32
using thinger::iotmp::ThingerESP32Eth;
#endif

#endif
