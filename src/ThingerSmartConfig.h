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

#ifndef THINGER_IOTMP_ARDUINO_SMARTCONFIG_H
#define THINGER_IOTMP_ARDUINO_SMARTCONFIG_H

#ifdef ESP8266

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include "client.hpp"
#include "compat.hpp"

#define SMART_CONFIG_WAIT_MS 60000
#define WIFI_CONNECTION_TIMEOUT_MS 15000

namespace thinger::iotmp {

    class ThingerSmartConfig : public arduino_client {
    public:
        ThingerSmartConfig(const char* user, const char* device, const char* credential, bool use_led = true)
            : arduino_client(wifi_client_, user, device, credential),
              use_led_(use_led)
        {
            port_ = 25206;
            wifi_client_.setInsecure();
            if(use_led_) {
                pinMode(BUILTIN_LED, OUTPUT);
            }
        }

        void handle() override {
            if(!wifi_connected()) {
                connect_wifi();
                return;
            }
            arduino_client::handle();
        }

    private:
        BearSSL::WiFiClientSecure wifi_client_;
        bool use_led_;

        bool wifi_connected() {
            return WiFi.status() == WL_CONNECTED && !(WiFi.localIP() == INADDR_NONE);
        }

        bool connect_wifi() {
            if(use_led_) {
                digitalWrite(BUILTIN_LED, HIGH);
            }

            // try to connect to the last known WiFi network
            if(WiFi.SSID() != NULL) {
                WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
                unsigned long wifi_timeout = millis();
                while(WiFi.status() != WL_CONNECTED && (millis() - wifi_timeout < WIFI_CONNECTION_TIMEOUT_MS)) {
                    yield();
                }
            }

            // if not connected, start SmartConfig
            if(WiFi.status() != WL_CONNECTED) {
                unsigned long wifi_timeout = millis();
                WiFi.stopSmartConfig();
                WiFi.beginSmartConfig();
                while(!WiFi.smartConfigDone()) {
                    if(millis() - wifi_timeout > SMART_CONFIG_WAIT_MS) {
                        return false;
                    }
                    if(use_led_) {
                        digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
                        delay(500);
                    } else {
                        yield();
                    }
                }
            }

            // wait for WiFi connection
            unsigned long wifi_timeout = millis();
            while(WiFi.status() != WL_CONNECTED) {
                if(millis() - wifi_timeout > WIFI_CONNECTION_TIMEOUT_MS) {
                    WiFi.disconnect();
                    return false;
                }
                yield();
            }

            // blink to notify connection
            if(use_led_) {
                for(int i = 0; i < 10; i++) {
                    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
                    delay(100);
                }
            }

            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerSmartConfig;

#endif // ESP8266

#endif // THINGER_IOTMP_ARDUINO_SMARTCONFIG_H
