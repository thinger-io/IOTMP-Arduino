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

#ifndef THINGER_IOTMP_ARDUINO_BC66_H
#define THINGER_IOTMP_ARDUINO_BC66_H

#ifndef _DISABLE_TLS_
#include <LClientSecure.h>
#else
#include <LClient.h>
#endif

#include "client.hpp"
#include "compat.hpp"

namespace thinger::iotmp {

    class ThingerBC66 : public arduino_client {
    public:
        ThingerBC66(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
#ifndef _DISABLE_TLS_
            port_ = 25206;
#else
            port_ = 25200;
#endif
        }

        static s32 callback(char* line, u32 len, void* userData) {
            return RIL_AT_SUCCESS;
        }

        static void send(const char* data, unsigned long timeout = 1000) {
            Dev.send(data, timeout, callback);
            Ql_Sleep(100);
        }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        uint8_t connection_errors_ = 0;

#ifndef _DISABLE_TLS_
        LClientSecure client_;
#else
        LClient client_;
#endif

        bool network_connected() {
            auto cereg = Dev.cereg(false);
            return cereg == 1 || cereg == 5;
        }

        bool connect_network() {
            unsigned long network_timeout = millis();
            while(true) {
                auto val = Dev.cereg(false);

                if(val == 1 || val == 5) {
                    break;
                }

                if(val == 3) {
                    Ql_Sleep(30000);
                    Dev.reset();
                    return false;
                }

                if(millis() - network_timeout > 300000) {
                    Dev.reset();
                    return false;
                }

                Ql_Sleep(500);
            }

            Ql_Sleep(100);
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerBC66;

#endif
