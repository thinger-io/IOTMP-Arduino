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

#ifndef THINGER_IOTMP_ARDUINO_LINKITONE_GPRS_H
#define THINGER_IOTMP_ARDUINO_LINKITONE_GPRS_H

#include <LGPRS.h>
#include <LGPRSClient.h>

#include "client.hpp"
#include "compat.hpp"

namespace thinger::iotmp {

    class ThingerLinkItOneGPRS : public arduino_client {
    public:
        ThingerLinkItOneGPRS(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
            // LinkIt ONE GPRS does not support TLS
            port_ = 25200;
        }

        void set_apn(const char* apn, const char* user = nullptr, const char* password = nullptr) {
            apn_      = apn;
            user_     = user;
            password_ = password;
        }

        void set_pin(const char* pin) {
            pin_ = pin;
        }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        LGPRSClient client_;
        bool connected_        = false;
        const char* apn_       = nullptr;
        const char* user_      = nullptr;
        const char* password_  = nullptr;
        const char* pin_       = nullptr;

        bool network_connected() {
            return connected_;
        }

        bool connect_network() {
            unsigned long gprs_timeout = millis();
            while(!attach_gprs()) {
                if(millis() - gprs_timeout > 30000) return false;
                delay(500);
            }
            connected_ = true;
            return connected_;
        }

        bool attach_gprs() {
            if(apn_ != nullptr) {
                return LGPRS.attachGPRS(apn_, user_, password_) == 1;
            } else {
                return LGPRS.attachGPRS() == 1;
            }
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerLinkItOneGPRS;

#endif
