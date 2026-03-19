#define THINGER_SERIAL_DEBUG
#include <ThingerESP32.h>

#include "arduino_secrets.h"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    // open serial for debugging
    Serial.begin(115200);

    // configure wifi network
    thing.add_wifi(SSID, SSID_PASSWORD);

    // digital pin control
    thing["GPIO_2"] << digitalPin(2);

    // resource output example (i.e. reading a sensor value, a variable, etc)
    thing["millis"] >> outputValue(millis());

    // more details at http://docs.thinger.io/arduino/
}

void loop() {
    thing.handle();
}
