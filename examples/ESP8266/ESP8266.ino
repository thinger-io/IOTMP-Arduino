#define THINGER_SERIAL_DEBUG
#include <ThingerESP8266.h>

#include "arduino_secrets.h"

ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    Serial.begin(115200);
    thing.add_wifi(SSID, SSID_PASSWORD);

    // digital pin control
    thing["GPIO_2"] << digitalPin(2);

    // resource output example
    thing["millis"] >> outputValue(millis());
}

void loop() {
    thing.handle();
}
