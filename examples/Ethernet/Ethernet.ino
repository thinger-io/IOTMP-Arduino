#define THINGER_SERIAL_DEBUG
#include <ThingerEthernet.h>

#include "arduino_secrets.h"

ThingerEthernet thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
    Serial.begin(115200);

    // resource output example
    thing["millis"] >> outputValue(millis());

    // analog pin reading
    thing["A0"] >> analogPin(A0);
}

void loop() {
    thing.handle();
}
