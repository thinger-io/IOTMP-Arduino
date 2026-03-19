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

#ifndef THINGER_IOTMP_ARDUINO_TINYGSM_H
#define THINGER_IOTMP_ARDUINO_TINYGSM_H

#include <TinyGsmClient.h>

#include "client.hpp"

namespace thinger::iotmp {

    template<typename ModemType>
    class ThingerTinyGSM : public arduino_client {
    public:
        ThingerTinyGSM(const char* user, const char* device, const char* credential, Stream& serial)
            : arduino_client(gsm_client_, user, device, credential),
              modem_(serial),
              gsm_client_(modem_)
        {
            // Plain TCP port (most GSM modems lack TLS support)
            port_ = 25200;
        }

        // Set APN credentials for GPRS data connection
        void add_apn(const char* apn, const char* apn_user = nullptr, const char* apn_password = nullptr) {
            apn_          = apn;
            apn_user_     = apn_user;
            apn_password_ = apn_password;
        }

        // Set SIM PIN if the card is locked
        void set_pin(const char* pin) {
            pin_ = pin;
        }

        // Access the underlying modem object
        ModemType& modem() { return modem_; }

        // Override handle() to manage cellular connectivity
        void handle() override {
            if(!network_connected()) {
                connect_network();
                return; // Let the next loop() iteration proceed
            }
            arduino_client::handle();
        }

    private:
        ModemType modem_;
        TinyGsmClient gsm_client_;

        const char* apn_          = nullptr;
        const char* apn_user_     = nullptr;
        const char* apn_password_ = nullptr;
        const char* pin_          = nullptr;

        bool network_connected() {
            return modem_.isNetworkConnected() && modem_.isGprsConnected();
        }

        bool connect_network() {
            if(!apn_) return false;

            if(!modem_.isNetworkConnected()) {
                modem_.restart();

                // Unlock SIM if a PIN was provided
                if(pin_ && modem_.getSimStatus() != 3) {
                    modem_.simUnlock(pin_);
                }

                if(!modem_.waitForNetwork(15000)) {
                    return false;
                }
            }

            if(!modem_.isGprsConnected()) {
                if(!modem_.gprsConnect(apn_, apn_user_, apn_password_)) {
                    return false;
                }
            }

            return true;
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_TINYGSM_H
