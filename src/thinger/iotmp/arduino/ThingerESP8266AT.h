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

#ifndef THINGER_IOTMP_ARDUINO_ESP8266AT_H
#define THINGER_IOTMP_ARDUINO_ESP8266AT_H

#define TINY_GSM_MODEM_ESP8266
#include <TinyGsmClient.h>

#include "client.hpp"

namespace thinger::iotmp {

    class ThingerESP8266AT : public arduino_client {
    public:
        ThingerESP8266AT(const char* user, const char* device, const char* credential, Stream& serial)
            : arduino_client(gsm_client_, user, device, credential),
              serial_(serial),
              gsm_client_(serial_)
        {
            // No TLS support via AT modem
            port_ = 25200;
        }

        void add_wifi(const char* ssid, const char* password = nullptr) {
            wifi_ssid_     = ssid;
            wifi_password_ = password;
        }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        TinyGsm serial_;
        TinyGsm::GsmClient gsm_client_;
        const char* wifi_ssid_     = nullptr;
        const char* wifi_password_ = nullptr;

        bool network_connected() {
            return serial_.isNetworkConnected();
        }

        bool connect_network() {
            return serial_.networkConnect(wifi_ssid_, wifi_password_);
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_ESP8266AT_H
