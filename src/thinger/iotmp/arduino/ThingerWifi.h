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

#ifndef THINGER_IOTMP_ARDUINO_WIFI_H
#define THINGER_IOTMP_ARDUINO_WIFI_H

#include "client.hpp"

namespace thinger::iotmp {

    template <class ClientType>
    class ThingerWifiClient : public arduino_client {
    public:
        ThingerWifiClient(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {}

        void add_wifi(const char* ssid, const char* password = nullptr) {
            wifi_ssid_     = ssid;
            wifi_password_ = password;
        }

        void handle() override {
            if(!wifi_connected()) {
                connect_wifi();
                return;
            }
            arduino_client::handle();
        }

    protected:
        ClientType client_;
        const char* wifi_ssid_     = nullptr;
        const char* wifi_password_ = nullptr;

        bool wifi_connected() {
            return (WiFi.status() == WL_CONNECTED) && !(WiFi.localIP() == (IPAddress)INADDR_NONE);
        }

        bool connect_wifi() {
            if(wifi_ssid_ != nullptr) {
                WiFi.begin((char*)wifi_ssid_, (char*)wifi_password_);
            }
#if defined(ESP8266) || defined(ESP32)
            else {
                WiFi.begin();
            }
#else
            else {
                return false;
            }
#endif
            return WiFi.status() == WL_CONNECTED;
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_WIFI_H
