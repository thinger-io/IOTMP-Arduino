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

#ifndef THINGER_IOTMP_ARDUINO_LINKITONE_WIFI_H
#define THINGER_IOTMP_ARDUINO_LINKITONE_WIFI_H

#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>

#include "client.hpp"
#include "compat.hpp"

namespace thinger::iotmp {

    class ThingerLinkItOneWifi : public arduino_client {
    public:
        ThingerLinkItOneWifi(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
            // LinkIt ONE does not support TLS
            port_ = 25200;
        }

        void add_wifi(const char* ssid, const char* password) {
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

    private:
        LWiFiClient client_;
        const char* wifi_ssid_     = nullptr;
        const char* wifi_password_ = nullptr;

        bool wifi_connected() {
            return LWiFi.status() == LWIFI_STATUS_CONNECTED && !(LWiFi.localIP() == INADDR_NONE);
        }

        bool connect_wifi() {
            if(!wifi_ssid_) return false;
            unsigned long wifi_timeout = millis();
            LWiFi.begin();
            while(LWiFi.connect((char*)wifi_ssid_, LWiFiLoginInfo(LWIFI_WPA, wifi_password_)) <= 0) {
                if(millis() - wifi_timeout > 30000) return false;
                delay(100);
            }
            wifi_timeout = millis();
            while(LWiFi.localIP() == INADDR_NONE) {
                if(millis() - wifi_timeout > 30000) return false;
            }
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerLinkItOneWifi;

#endif
