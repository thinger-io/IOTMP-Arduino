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

#ifndef THINGER_IOTMP_ARDUINO_CC3000_H
#define THINGER_IOTMP_ARDUINO_CC3000_H

#include <Adafruit_CC3000.h>

#include "client.hpp"

#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

namespace thinger::iotmp {

    class ThingerCC3000 : public arduino_client {
    public:
        ThingerCC3000(const char* user, const char* device, const char* credential,
                      const uint8_t cc3000csPin  = ADAFRUIT_CC3000_CS,
                      const uint8_t cc3000irqPin = ADAFRUIT_CC3000_IRQ,
                      const uint8_t cc3000vbatPin = ADAFRUIT_CC3000_VBAT)
            : arduino_client(client_, user, device, credential),
              cc3000_(cc3000csPin, cc3000irqPin, cc3000vbatPin, SPI_CLOCK_DIVIDER)
        {
            // CC3000 does not support TLS
            port_ = 25200;
        }

        void add_wifi(char* ssid, char* password) {
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
        Adafruit_CC3000 cc3000_;
        Adafruit_CC3000_Client client_;
        char* wifi_ssid_     = nullptr;
        char* wifi_password_ = nullptr;

        bool network_connected() {
            return cc3000_.checkConnected();
        }

        bool connect_network() {
            if(!cc3000_.begin()) {
                return false;
            }

            unsigned long aucDHCP       = 14400;
            unsigned long aucARP        = 3600;
            unsigned long aucKeepalive  = 30;
            unsigned long aucInactivity = 0;
            netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);

            if(cc3000_.connectToAP(wifi_ssid_, wifi_password_, WLAN_SEC_WPA2)) {
                unsigned long wifi_timeout = millis();
                while(!cc3000_.checkDHCP()) {
                    if(millis() - wifi_timeout > 30000) return false;
                }
                return true;
            } else {
                cc3000_.stop();
                return false;
            }
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_CC3000_H
