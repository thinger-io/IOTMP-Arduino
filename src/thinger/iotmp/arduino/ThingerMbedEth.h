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

#ifndef THINGER_IOTMP_ARDUINO_MBED_ETH_H
#define THINGER_IOTMP_ARDUINO_MBED_ETH_H

#include <mbed.h>
#include <Ethernet.h>
#include <PortentaEthernet.h>

#ifdef _DISABLE_TLS_
typedef EthernetClient MBEDEthClient;
#else
#include <EthernetSSLClient.h>
typedef EthernetSSLClient MBEDEthClient;
#endif

#include "client.hpp"

namespace thinger::iotmp {

    class ThingerMbedEth : public arduino_client {
    public:
        ThingerMbedEth(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential)
        {
#ifndef _DISABLE_TLS_
            port_ = 25206;
#endif
        }

        bool start() {
            if(running_) return false;
            running_ = true;
            get_thread().start([this]() {
                while(running_) {
                    rtos::ThisThread::yield();
                    this->handle();
                }
            });
            return true;
        }

        bool stop() {
            if(running_) {
                running_ = false;
                get_thread().join();
            }
            return true;
        }

        bool is_running() {
            return running_;
        }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        MBEDEthClient client_;
        bool running_     = false;
        bool initialized_ = false;

        rtos::Thread& get_thread() {
            static rtos::Thread thread;
            return thread;
        }

        bool network_connected() {
            return initialized_ ? Ethernet.linkStatus() == LinkON : false;
        }

        bool connect_network() {
            if(!initialized_) {
                if(Ethernet.hardwareStatus() == EthernetNoHardware) {
                    return false;
                }

                if(Ethernet.begin() == 0) {
                    return false;
                }

                if(Ethernet.linkStatus() == LinkOFF) {
                    return false;
                }

                initialized_ = true;
                delay(1000);
            }
            return initialized_;
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_MBED_ETH_H
