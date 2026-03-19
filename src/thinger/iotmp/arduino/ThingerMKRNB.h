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

#ifndef THINGER_IOTMP_ARDUINO_MKRNB_H
#define THINGER_IOTMP_ARDUINO_MKRNB_H

#include <MKRNB.h>

#include "client.hpp"

#define NB_CONNECTION_TIMEOUT   120000
#define GPRS_CONNECTION_TIMEOUT_NB 60000

namespace thinger::iotmp {

    class ThingerMKRNB : public arduino_client {
    public:
        ThingerMKRNB(const char* user, const char* device, const char* credential)
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
        NB& getNB()     { return nbAccess_; }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        const char* pin_           = nullptr;
        const char* apn_           = nullptr;
        const char* username_      = nullptr;
        const char* password_      = nullptr;
        bool nb_connected_         = false;
        bool modem_restart_        = false;

#ifndef _DISABLE_TLS_
        NBSSLClient client_;
#else
        NBClient client_;
#endif

        GPRS gprs_;
        NB nbAccess_;

        bool network_connected() {
            return nb_connected_ ? nbAccess_.isAccessAlive() : false;
        }

        bool connect_network() {
            nbAccess_.setTimeout(NB_CONNECTION_TIMEOUT);

            nb_connected_ = nbAccess_.begin(pin_, apn_, username_, password_, modem_restart_, true) == NB_READY;

            if(!nb_connected_) {
                modem_restart_ = true;
                return false;
            }

            modem_restart_ = false;

            gprs_.setTimeout(GPRS_CONNECTION_TIMEOUT_NB);
            bool gprs_connected = gprs_.attachGPRS(true) == GPRS_READY;
            if(!gprs_connected) return false;

            return true;
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_MKRNB_H
