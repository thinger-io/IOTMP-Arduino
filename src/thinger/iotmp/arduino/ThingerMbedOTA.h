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

#ifndef THINGER_IOTMP_ARDUINO_MBEDOTA_H
#define THINGER_IOTMP_ARDUINO_MBEDOTA_H

#include "ThingerOTA.h"
#include <mbed.h>
#include <SFU.h>
#include <FlashIAPBlockDevice.h>
#include <FATFileSystem.h>

#define SD_MOUNT_PATH           "ota"
#define FULL_UPDATE_FILE_PATH   "/" SD_MOUNT_PATH "/" "UPDATE.BIN"

namespace thinger::iotmp {

    class ThingerMbedOTA : public ThingerOTA {
    public:
        ThingerMbedOTA(arduino_client& client)
            : ThingerOTA(client),
              fs_(SD_MOUNT_PATH)
        {
            set_block_size(2048);
        }

        virtual ~ThingerMbedOTA() {}

    protected:
        void fill_options(output& options) override {
            options["platform"] = "atmelsam";
        }

        bool begin_ota(input& options, output& state) override {
            static FlashIAPBlockDevice bd_(XIP_BASE + 0xF00000, 0x100000);
            int err = fs_.mount(&bd_);
            if(err != 0) {
                err = fs_.reformat(&bd_);
                if(err != 0) {
                    state["error"] = "cannot mount partition";
                    return false;
                }
            }
            reset_ota();
            f_ = fopen(FULL_UPDATE_FILE_PATH, "w+");
            if(f_ == nullptr) {
                state["error"] = "cannot create file: " FULL_UPDATE_FILE_PATH;
                return false;
            }
            return true;
        }

        bool reset_ota() override {
            if(f_) {
                fclose(f_);
                f_ = nullptr;
            }
            fs_.remove(FULL_UPDATE_FILE_PATH);
            return true;
        }

        bool write_ota(input& data, output& state) override {
            return true;
        }

        bool end_ota(output& state) override {
            if(f_) {
                int err = fclose(f_);
                f_ = nullptr;
                if(err < 0) {
                    state["error"] = "error while closing file";
                    return false;
                }
            }
            return true;
        }

    private:
        mbed::FATFileSystem fs_;
        FILE* f_ = nullptr;
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_MBEDOTA_H
