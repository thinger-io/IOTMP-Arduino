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

#ifndef THINGER_IOTMP_ARDUINO_MBED_H
#define THINGER_IOTMP_ARDUINO_MBED_H

#include <mbed.h>

// Load WiFi library according to board
#if defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#else
#include <WiFi.h>
#endif

#ifdef _DISABLE_TLS_
#include <WiFiClient.h>
typedef WiFiClient MBEDClient;
#else
#include <WiFiSSLClient.h>
typedef WiFiSSLClient MBEDClient;
#endif

#include "ThingerWifi.h"

namespace thinger::iotmp {

    class ThingerMbed : public ThingerWifiClient<MBEDClient> {
    public:
        ThingerMbed(const char* user, const char* device, const char* credential)
            : ThingerWifiClient(user, device, credential)
        {
#ifndef _DISABLE_TLS_
            port_ = 25206;
#endif
        }

        bool start() {
            if(running_) return false;
            running_ = true;
            thread_.start([this]() {
                while(running_) {
                    rtos::ThisThread::yield();
                    this->handle();
                }
            });
            return true;
        }

        bool stop() {
            if(running_) {
                running_ = false;
                thread_.join();
            }
            return true;
        }

        bool is_running() {
            return running_;
        }

    private:
        rtos::Thread thread_;
        bool running_ = false;
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerMbed;

#endif
