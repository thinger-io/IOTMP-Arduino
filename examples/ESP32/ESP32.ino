#include <ThingerESP32.h>
#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    Serial.begin(115200);
    thing.add_wifi(SSID, SSID_PASSWORD);

    // digital pin control
    thing["GPIO_2"] << digitalPin(2);

    // resource output example
    thing["millis"] >> outputValue(millis());

    // custom output resource
    thing["temperature"] = [](thinger::iotmp::output& out) {
        out["celsius"] = 23.5;
    };
}

void loop() {
    thing.handle();
}
