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

#ifndef THINGER_IOTMP_ARDUINO_COMPAT_HPP
#define THINGER_IOTMP_ARDUINO_COMPAT_HPP

// Compatibility helpers for migrating from the classic Arduino library.
// These macros and operators provide a familiar API:
//
//   thing["led"] << digitalPin(LED_PIN);
//   thing["millis"] >> outputValue(millis());
//   thing["temp"] >> [](thinger::iotmp::output& out){ out = read_temp(); };
//   thing["relay"] << [](thinger::iotmp::input& in){ ... };

#include <Arduino.h>
#include <thinger/iotmp/core/iotmp_resource.hpp>

namespace thinger::iotmp {

// ---- operator>> (output resource) ----

inline void operator>>(iotmp_resource& res, std::function<void(output&)> fn) {
    res = std::move(fn);
}

// ---- operator<< (input resource or input/output resource) ----

inline void operator<<(iotmp_resource& res, std::function<void(input&)> fn) {
    res = std::move(fn);
}

inline void operator<<(iotmp_resource& res, std::function<void(input&, output&)> fn) {
    res = std::move(fn);
}

} // namespace thinger::iotmp

// ---- Convenience macros ----

// outputValue: captures a value expression and sends it as output
// Usage: thing["millis"] >> outputValue(millis());
#define outputValue(value) [](thinger::iotmp::output& out){ out = value; }

// outputString: same as outputValue but semantically for strings
#define outputString(value) [](thinger::iotmp::output& out){ out = value; }

// digitalPin: bidirectional digital pin control (read/write)
// Usage: thing["led"] << digitalPin(LED_PIN);
#define digitalPin(PIN) [](thinger::iotmp::input& in, thinger::iotmp::output& out) { \
    if(in.is_empty()) {                                                               \
        out = (bool)digitalRead(PIN);                                                 \
    } else {                                                                          \
        digitalWrite(PIN, in.payload().get<bool>() ? HIGH : LOW);                     \
    }                                                                                 \
}

// analogPin: read an analog pin
// Usage: thing["sensor"] >> analogPin(A0);
#define analogPin(PIN) [](thinger::iotmp::output& out){ out = analogRead(PIN); }

#endif // THINGER_IOTMP_ARDUINO_COMPAT_HPP
