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

#ifndef THINGER_IOTMP_ARDUINO_PICO_W_H
#define THINGER_IOTMP_ARDUINO_PICO_W_H

#include <WiFi.h>

#include "client.hpp"
#include "compat.hpp"

namespace thinger::iotmp {

    class ThingerPicoW : public arduino_client {
    public:
        ThingerPicoW(const char* user, const char* device, const char* credential)
            : arduino_client(wifi_client_, user, device, credential)
        {
            // Plain TCP port (no TLS by default on Pico W)
            port_ = 25200;
        }

        // Store WiFi credentials
        void add_wifi(const char* ssid, const char* password) {
            wifi_ssid_     = ssid;
            wifi_password_ = password;
        }

        // Override handle() to manage WiFi before the protocol
        void handle() override {
            if(!wifi_connected()) {
                connect_wifi();
                return; // Let the next loop() iteration proceed
            }
            arduino_client::handle();
        }

    private:
        WiFiClient wifi_client_;
        const char* wifi_ssid_     = nullptr;
        const char* wifi_password_ = nullptr;

        bool wifi_connected() {
            return WiFi.status() == WL_CONNECTED;
        }

        bool connect_wifi() {
            if(!wifi_ssid_) return false;

            if(WiFi.status() != WL_CONNECTED) {
                WiFi.mode(WIFI_STA);
                WiFi.begin(wifi_ssid_, wifi_password_);

                // Non-blocking: we just kick off the connection and
                // return.  handle() will be called again from loop()
                // and will re-check wifi_connected().
            }
            return WiFi.status() == WL_CONNECTED;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerPicoW;

#endif // THINGER_IOTMP_ARDUINO_PICO_W_H
