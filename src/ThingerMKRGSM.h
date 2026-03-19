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

#ifndef THINGER_IOTMP_ARDUINO_MKRGSM_H
#define THINGER_IOTMP_ARDUINO_MKRGSM_H

#include <MKRGSM.h>

#include "client.hpp"
#include "compat.hpp"

#define GPRS_CONNECTION_TIMEOUT 30000

namespace thinger::iotmp {

    class ThingerMKRGSM : public arduino_client {
    public:
        ThingerMKRGSM(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
#ifndef _DISABLE_TLS_
            port_ = 25206;
#endif
        }

        void set_apn(const char* apn, const char* apn_username = "", const char* apn_password = "") {
            apn_      = apn;
            username_ = apn_username;
            password_ = apn_password;
        }

        void set_pin(const char* pin) {
            pin_ = pin;
        }

        GPRS& getGPRS() { return gprs_; }
        GSM& getGSM()   { return gsmAccess_; }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        const char* pin_       = nullptr;
        const char* apn_       = nullptr;
        const char* username_  = nullptr;
        const char* password_  = nullptr;
        bool gsm_connected_    = false;
        bool gprs_connected_   = false;

#ifndef _DISABLE_TLS_
        GSMSSLClient client_;
#else
        GSMClient client_;
#endif

        GPRS gprs_;
        GSM gsmAccess_;

        bool network_connected() {
            return gsm_connected_ && gprs_connected_;
        }

        bool connect_network() {
            if(!gsm_connected_) {
                gsm_connected_ = gsmAccess_.begin(pin_, true, true) == GSM_READY;
            }

            if(gsm_connected_) {
                if(apn_ == nullptr) return false;

                unsigned long timeout = millis();
                gprs_.attachGPRS(apn_, username_, password_, false);
                while(gprs_.ready() == 0) {
                    if(millis() - timeout > GPRS_CONNECTION_TIMEOUT) {
                        return false;
                    }
                    delay(100);
                }
                gprs_connected_ = true;
            }

            return gsm_connected_ && gprs_connected_;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerMKRGSM;

#endif
