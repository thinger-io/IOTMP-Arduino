// The MIT License (MIT)
//
// Copyright (c) 2017 THINK BIG LABS S.L.
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

#ifndef THINGER_CLIENT_H
#define THINGER_CLIENT_H

// Uses the shared IOTMP protocol core from iotmp-embedded
#include <thinger/iotmp/core/iotmp_value.hpp>
#include <thinger/iotmp/core/iotmp_message.hpp>
#include <thinger/iotmp/core/iotmp_encoder.hpp>
#include <thinger/iotmp/core/iotmp_decoder.hpp>
#include <thinger/iotmp/core/iotmp_resource.hpp>

#include <Arduino.h>
#include <Client.h>

namespace thinger::iotmp {

// TODO: Port from current Arduino-Library ThingerClient
// Key changes from current implementation:
// - Replace internal pson with iotmp_value from iotmp-embedded
// - Replace PSON v1 encoding with PSON v2 (pson_encoder/pson_decoder)
// - Replace signal_flag-based protocol with direct message types
// - Keep the Arduino Client-based transport (read/write via Client interface)
// - Keep connection management (connect, reconnect, timeout)
// - Keep output buffering for write coalescing

class ThingerClient {
public:
    ThingerClient(Client& client, const char* user, const char* device, const char* credential)
        : client_(client), username_(user), device_id_(device), credential_(credential) {}

    // Resource registration (same API as current library)
    iotmp_resource& operator[](const char* name) {
        return resources_[std::string(name)];
    }

    // Main loop — call from Arduino loop()
    void handle() {
        // TODO: implement connection management + message handling
        // Similar to current ThingerClient but using IOTMP message types
    }

private:
    Client& client_;
    const char* username_;
    const char* device_id_;
    const char* credential_;
    std::map<std::string, iotmp_resource> resources_;
};

} // namespace thinger::iotmp

#endif
