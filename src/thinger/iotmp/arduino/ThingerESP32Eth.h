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

#ifndef THINGER_IOTMP_ARDUINO_ESP32ETH_H
#define THINGER_IOTMP_ARDUINO_ESP32ETH_H

#ifdef ESP32

#include <ETH.h>

#ifdef _DISABLE_TLS_
typedef WiFiClient ESP32EthClient;
#else
#include <WiFiClientSecure.h>
typedef WiFiClientSecure ESP32EthClient;
#endif

#include "client.hpp"

namespace thinger::iotmp {

    class ThingerESP32Eth : public arduino_client {
    public:
        ThingerESP32Eth(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
#ifndef _DISABLE_TLS_
            port_ = 25206;
            client_.setInsecure();
#endif
        }

        void set_hostname(const char* hostname) {
            hostname_ = hostname;
        }

        void set_address(const char* ip, const char* gateway, const char* subnet,
                         const char* dns1 = "8.8.8.8", const char* dns2 = "8.8.4.4") {
            ip_      = ip;
            gateway_ = gateway;
            subnet_  = subnet;
            dns1_    = dns1;
            dns2_    = dns2;
        }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        ESP32EthClient client_;
        bool initialized_       = false;
        const char* hostname_   = "esp32-thinger";
        const char* ip_         = nullptr;
        const char* gateway_    = nullptr;
        const char* subnet_     = nullptr;
        const char* dns1_       = nullptr;
        const char* dns2_       = nullptr;

        bool init_address() {
            if(ip_ == nullptr) return true;
            bool result = true;
            IPAddress ip, gateway, subnet, dns1, dns2;
            result &= ip.fromString(ip_);
            result &= gateway.fromString(gateway_);
            result &= subnet.fromString(subnet_);
            result &= dns1.fromString(dns1_);
            result &= dns2.fromString(dns2_);
            return result && ETH.config(ip, gateway, subnet, dns1, dns2);
        }

        bool network_connected() {
            return initialized_ ? ETH.linkUp() : false;
        }

        bool connect_network() {
            if(!initialized_) {
                initialized_ = ETH.begin();
                if(initialized_) {
                    ETH.setHostname(hostname_);
                    init_address();
                }
            }
            return network_connected();
        }
    };

} // namespace thinger::iotmp

#endif // ESP32

#endif // THINGER_IOTMP_ARDUINO_ESP32ETH_H
