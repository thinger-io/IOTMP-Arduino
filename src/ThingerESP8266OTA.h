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

#ifndef THINGER_IOTMP_ARDUINO_ESP8266OTA_H
#define THINGER_IOTMP_ARDUINO_ESP8266OTA_H

#ifdef ESP8266

#include "ThingerOTA.h"
#include <Updater.h>

namespace thinger::iotmp {

    class ThingerESP8266OTA : public ThingerOTA {
    public:
        ThingerESP8266OTA(arduino_client& client)
            : ThingerOTA(client)
        {
            set_block_size(2048);
        }

        virtual ~ThingerESP8266OTA() {}

    protected:
        void fill_options(output& options) override {
            options["platform"] = "espressif8266";
            options["checksum"] = "md5";
        }

        bool begin_ota(input& options, output& state) override {
            size_t size = firmware_size_;
            bool init = Update.begin(size);

            if(!init) {
                Update.end(true);
                init = Update.begin(size);
            }

            if(!init) {
                state["error"] = "Update.begin failed";
                return false;
            }

            return init;
        }

        bool write_ota(input& data, output& state) override {
            return true;
        }

        bool end_ota(output& state) override {
            if(!Update.end()) {
                state["error"] = "Update.end failed";
                return false;
            }
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerESP8266OTA;

#endif // ESP8266

#endif // THINGER_IOTMP_ARDUINO_ESP8266OTA_H
