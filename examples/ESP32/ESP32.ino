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

#include <thinger/iotmp/arduino/ThingerESP32.h>

thinger::iotmp::ThingerESP32 thing("username", "device_id", "credential");

void setup() {
    Serial.begin(115200);
    thing.add_wifi("SSID", "PASSWORD");

    // Output resource — streams a temperature reading
    thing["temperature"] = [](thinger::iotmp::output& out) {
        out["celsius"] = 23.5;
    };

    // Input resource — controls a GPIO
    thing["led"] = [](thinger::iotmp::input& in) {
        bool state = in["state"].get<bool>();
        digitalWrite(2, state); // GPIO2 — built-in LED on most ESP32 boards
    };
}

void loop() {
    thing.handle();
}
