// The MIT License (MIT)
//
// Copyright (c) INTERNET OF THINGER SL
// Author: alvarolb@gmail.com (Alvaro Luis Bustamante)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <SPI.h>
#include <Ethernet.h>
#include <thinger/iotmp/arduino/ThingerEthernet.h>

// MAC address for the Ethernet shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

thinger::iotmp::ThingerEthernet thing("username", "device_id", "credential");

void setup() {
    Serial.begin(115200);

    // Initialize Ethernet with DHCP
    Ethernet.begin(mac);

    // Output resource — streams a temperature reading
    thing["temperature"] = [](thinger::iotmp::output& out) {
        out["celsius"] = 23.5;
    };

    // Input resource — controls an LED
    thing["led"] = [](thinger::iotmp::input& in) {
        bool state = in["state"].get<bool>();
        digitalWrite(LED_BUILTIN, state);
    };
}

void loop() {
    thing.handle();
}
