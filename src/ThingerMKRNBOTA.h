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

#ifndef THINGER_IOTMP_ARDUINO_MKRNBOTA_H
#define THINGER_IOTMP_ARDUINO_MKRNBOTA_H

#include "ThingerOTA.h"
#include <NBFileUtils.h>
#include <SBU.h>

namespace thinger::iotmp {

    class ThingerMKRNBOTA : public ThingerOTA {

        static constexpr const char* UPDATE_FILE            = "UPDATE.BIN";
        static constexpr const char* UPDATE_FILE_OK         = "UPDATE.OK";
        static constexpr const char* UPDATE_FILE_COMPRESSED = "UPDATE.BIN.LZSS";

    public:
        ThingerMKRNBOTA(arduino_client& client)
            : ThingerOTA(client)
        {
            set_block_size(256);
        }

        virtual ~ThingerMKRNBOTA() {}

    protected:
        void fill_options(output& options) override {
            options["platform"] = "mkrnb";
        }

        bool begin_ota(input& options, output& state) override {
            if(file_utils_.begin(false)) {
                reset_ota();
                return true;
            }
            return false;
        }

        bool reset_ota() override {
            if(file_utils_.existFile(UPDATE_FILE))
                file_utils_.deleteFile(UPDATE_FILE);
            if(file_utils_.existFile(UPDATE_FILE_OK))
                file_utils_.deleteFile(UPDATE_FILE_OK);
            if(file_utils_.existFile(UPDATE_FILE_COMPRESSED))
                file_utils_.deleteFile(UPDATE_FILE_COMPRESSED);
            return true;
        }

        bool write_ota(input& data, output& state) override {
            return true;
        }

        bool end_ota(output& state) override {
            return file_utils_.createFile(UPDATE_FILE_OK, "OK", 2) == 2;
        }

    private:
        NBFileUtils file_utils_;
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerMKRNBOTA;

#endif // THINGER_IOTMP_ARDUINO_MKRNBOTA_H
