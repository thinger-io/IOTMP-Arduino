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

#ifndef THINGER_IOTMP_ARDUINO_ESP32_H
#define THINGER_IOTMP_ARDUINO_ESP32_H

#ifdef ESP32

#include <WiFi.h>

#ifdef _DISABLE_TLS_
#include <WiFiClient.h>
typedef WiFiClient ESP32Client;
#else
#include <WiFiClientSecure.h>
typedef WiFiClientSecure ESP32Client;
#endif

#include "client.hpp"
#include "compat.hpp"

#ifdef THINGER_FREE_RTOS
#include "ThingerESP32FreeRTOS.h"
#endif

namespace thinger::iotmp {

    class ThingerESP32 : public arduino_client {
    public:
        ThingerESP32(const char* user, const char* device, const char* credential)
            : arduino_client(wifi_client_, user, device, credential)
#ifdef THINGER_FREE_RTOS
            , rtos_(*this)
#endif
        {
#ifdef _DISABLE_TLS_
            port_ = 25204;
#else
            port_ = 25206;
            wifi_client_.setInsecure();
#endif
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

#ifdef THINGER_FREE_RTOS
        bool start(unsigned core = ARDUINO_RUNNING_CORE, size_t stack_size = 8192) {
            return rtos_.start(core, stack_size);
        }

        bool stop() {
            return rtos_.stop();
        }

        bool is_running() {
            return rtos_.is_running();
        }
#endif

    private:
        ESP32Client wifi_client_;
        const char* wifi_ssid_     = nullptr;
        const char* wifi_password_ = nullptr;

#ifdef THINGER_FREE_RTOS
        ThingerESP32FreeRTOS rtos_;
#endif

        bool wifi_connected() {
            return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
        }

        bool connect_wifi() {
            if(!wifi_ssid_) return false;

            THINGER_LOG_INFO("Connecting to WiFi: %s", wifi_ssid_);
            WiFi.mode(WIFI_STA);
            WiFi.begin(wifi_ssid_, wifi_password_);

            // Wait for connection with timeout
            unsigned long start = millis();
            while(WiFi.status() != WL_CONNECTED) {
                if(millis() - start > 30000) {
                    THINGER_LOG_ERROR("WiFi connection timeout");
                    return false;
                }
                delay(1); // yield to WiFi task
            }

            THINGER_LOG_INFO("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerESP32;

#endif // ESP32

#endif // THINGER_IOTMP_ARDUINO_ESP32_H
