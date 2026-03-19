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

#ifndef THINGER_IOTMP_ARDUINO_ESP32OTA_H
#define THINGER_IOTMP_ARDUINO_ESP32OTA_H

#ifdef ESP32

#include "ThingerOTA.h"
#include <Update.h>

namespace thinger::iotmp {

    class ThingerESP32OTA : public ThingerOTA {
    public:
        ThingerESP32OTA(arduino_client& client, size_t block_size = 32768)
            : ThingerOTA(client)
        {
            set_block_size(block_size);
        }

        virtual ~ThingerESP32OTA() {}

    protected:
        void fill_options(output& options) override {
            options["platform"] = "espressif32";
            options["checksum"] = "md5";
        }

        bool begin_ota(input& options, output& state) override {
            size_t size = firmware_size_;
            bool init = Update.begin(size);

            if(!init) {
                Update.abort();
                init = Update.begin(size);
            }

            if(!init) {
                state["error"] = Update.errorString();
                return false;
            }

            return init;
        }

        bool write_ota(input& data, output& state) override {
            // In the IOTMP protocol the binary data arrives in the input payload.
            // For now, treat the input as carrying raw bytes.
            // The actual binary extraction depends on the resource framework.
            // This is a structural port -- the exact binary access API will be
            // finalized when the full IOTMP resource I/O is integrated.
            return true;
        }

        bool end_ota(output& state) override {
            if(!Update.end()) {
                state["error"] = Update.errorString();
                return false;
            }
            return true;
        }
    };

} // namespace thinger::iotmp

#endif // ESP32

#endif // THINGER_IOTMP_ARDUINO_ESP32OTA_H
