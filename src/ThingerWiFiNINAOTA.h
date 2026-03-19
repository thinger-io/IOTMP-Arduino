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

#ifndef THINGER_IOTMP_ARDUINO_WIFININA_OTA_H
#define THINGER_IOTMP_ARDUINO_WIFININA_OTA_H

#include "ThingerOTA.h"
#include <WiFiNINA.h>
#include <SNU.h>

namespace thinger::iotmp {

    class ThingerWiFiNINAOTA : public ThingerOTA {

        static constexpr const char* UPDATE_FILE_NAME      = "/fs/UPDATE.BIN";
        static constexpr const char* UPDATE_FILE_NAME_LZSS = "/fs/UPDATE.BIN.LZSS";

    public:
        ThingerWiFiNINAOTA(arduino_client& client)
            : ThingerOTA(client)
        {
            set_block_size(512);
        }

        virtual ~ThingerWiFiNINAOTA() {}

    protected:
        void fill_options(output& options) override {
            options["platform"] = "atmelsam";
        }

        bool begin_ota(input& options, output& state) override {
            reset_ota();
            return true;
        }

        bool reset_ota() override {
            if(WiFiStorage.exists(UPDATE_FILE_NAME)) {
                WiFiStorage.remove(UPDATE_FILE_NAME);
            }
            if(WiFiStorage.exists(UPDATE_FILE_NAME_LZSS)) {
                WiFiStorage.remove(UPDATE_FILE_NAME_LZSS);
            }
            return true;
        }

        bool write_ota(input& data, output& state) override {
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerWiFiNINAOTA;

#endif // THINGER_IOTMP_ARDUINO_WIFININA_OTA_H
