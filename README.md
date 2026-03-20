# IOTMP-Arduino

[![Build](https://github.com/thinger-io/IOTMP-Arduino/actions/workflows/build.yml/badge.svg)](https://github.com/thinger-io/IOTMP-Arduino/actions/workflows/build.yml)

Arduino client library for connecting IoT devices to the [Thinger.io](https://thinger.io) platform using the [IOTMP](https://docs.thinger.io) protocol.

Built on the [IOTMP-Embedded](https://github.com/thinger-io/IOTMP-Embedded) protocol core with [PSON](https://www.mdpi.com/1424-8220/21/13/4559) binary encoding.

## Supported boards

| Platform | Board | OTA | Client |
|----------|-------|-----|--------|
| Espressif ESP32 | ESP32, ESP32-S3, ESP32-C3, Nano ESP32 | Yes | `ThingerESP32` |
| Espressif ESP8266 | ESP8266, Sonoff | Yes | `ThingerESP8266` |
| Arduino SAMD | MKR WiFi 1010, Nano 33 IoT, MKR 1000 | Yes | `ThingerWiFiNINA` / `ThingerWifi101` |
| Arduino Mbed | Portenta H7, Nano RP2040 Connect, GIGA R1 | Yes | `ThingerMbed` |
| Arduino Renesas | UNO R4 WiFi | — | `ThingerR4WiFi` |
| Arduino STM32 | Opta WiFi, Opta Ethernet | Yes | `ThingerMbed` / `ThingerMbedEth` |
| Raspberry Pi | Pico W | — | `ThingerPicoW` |
| Cellular | Any TinyGSM-compatible modem | — | `ThingerTinyGSM` |
| Cellular | MKR GSM 1400, MKR NB 1500 | — | `ThingerMKRGSM` / `ThingerMKRNB` |
| Ethernet | W5100/W5500 shields | — | `ThingerEthernet` |
| Ethernet | ENC28J60 | — | `ThingerENC28J60` |
| Ethernet | ESP32 LAN8720 | — | `ThingerESP32Eth` |

## Quick start

```cpp
#define THINGER_SERIAL_DEBUG
#include <ThingerESP32.h>
#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    Serial.begin(115200);
    thing.add_wifi(SSID, SSID_PASSWORD);

    // Digital pin control
    thing["led"] << digitalPin(2);

    // Sensor reading
    thing["millis"] >> outputValue(millis());

    // Custom output resource
    thing["sensor"] = [](thinger::iotmp::output& out) {
        out["temperature"] = 23.5;
        out["humidity"] = 65;
    };

    // Custom input resource
    thing["relay"] = [](thinger::iotmp::input& in) {
        bool state = in["state"].get<bool>();
        digitalWrite(4, state);
    };
}

void loop() {
    thing.handle();
}
```

## Installation

### PlatformIO

Add to `platformio.ini`:

```ini
lib_deps = thinger-io/IOTMP-Arduino
```

The `IOTMP-Embedded` dependency is resolved automatically.

### Arduino IDE

Install `IOTMP-Arduino` from the Library Manager. The `IOTMP-Embedded` dependency is resolved automatically.

## OTA (Over the Air)

Supported devices can be updated remotely over the Internet. Thinger.io provides a [Visual Studio Code extension](https://marketplace.visualstudio.com/items?itemName=thinger-io.thinger-io) for the OTA process — build, flash, and reboot remotely.

To enable OTA, include the corresponding OTA header after your client:

```cpp
#include <ThingerESP32.h>
#include <ThingerESP32OTA.h>

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
ThingerESP32OTA ota(thing);
```

Available OTA classes:

| Platform | Class |
|----------|-------|
| ESP32 | `ThingerESP32OTA` |
| ESP8266 | `ThingerESP8266OTA` |
| Portenta H7 | `ThingerPortentaOTA` |
| Nano RP2040 / Mbed | `ThingerMbedOTA` |
| MKR WiFi 1010 / WiFiNINA | `ThingerWiFiNINAOTA` |
| MKR NB 1500 | `ThingerMKRNBOTA` |

## WebConfig (WiFi provisioning)

ESP32 and ESP8266 support WiFi provisioning via a captive portal:

```cpp
#include <ThingerESP32.h>
#include <ThingerESP32WebConfig.h>

ThingerESP32WebConfig thing;
```

The device starts an access point for initial WiFi and Thinger.io credential configuration.

## State listener

Monitor connection lifecycle events:

```cpp
thing.set_state_listener([](thinger::iotmp::THINGER_STATE state) {
    switch(state) {
        case thinger::iotmp::THINGER_AUTHENTICATED:
            Serial.println("Connected to Thinger.io!");
            break;
        case thinger::iotmp::SOCKET_DISCONNECTED:
            Serial.println("Disconnected");
            break;
        default:
            break;
    }
});
```

Available states: `NETWORK_CONNECTING`, `NETWORK_CONNECTED`, `NETWORK_CONNECT_ERROR`, `SOCKET_CONNECTING`, `SOCKET_CONNECTED`, `SOCKET_CONNECTION_ERROR`, `SOCKET_DISCONNECTED`, `SOCKET_TIMEOUT`, `SOCKET_ERROR`, `THINGER_AUTHENTICATING`, `THINGER_AUTHENTICATED`, `THINGER_AUTH_FAILED`, `THINGER_STOP_REQUEST`.

## FreeRTOS (ESP32)

Run the client in a dedicated FreeRTOS task:

```cpp
#define THINGER_FREE_RTOS
#include <ThingerESP32.h>

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    thing.add_wifi(SSID, SSID_PASSWORD);
    thing["millis"] >> outputValue(millis());
    thing.start();  // Launches handle() on a FreeRTOS task
}

void loop() {
    // Free for other work
}
```

## Documentation

Full documentation: [docs.thinger.io](https://docs.thinger.io)

## License

MIT License — see [LICENSE](LICENSE) for details.
