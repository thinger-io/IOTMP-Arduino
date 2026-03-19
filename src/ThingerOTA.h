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

#ifndef THINGER_IOTMP_ARDUINO_OTA_H
#define THINGER_IOTMP_ARDUINO_OTA_H

#include "client.hpp"
#include <functional>

#ifndef THINGER_OTA_MD5_VERIFICATION
#define THINGER_OTA_MD5_VERIFICATION 0
#endif

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#ifndef THINGER_OTA_VERSION
#define THINGER_OTA_VERSION
#endif

namespace thinger::iotmp {

    class ThingerOTA {
    public:
        ThingerOTA(arduino_client& client)
            : client_(client)
        {
            // OTA resources are registered on the client using the $ota path.
            // The actual resource registration uses the IOTMP resource API:
            //   client["$ota"]  -- options, begin, write, end, reboot, version
            //
            // In this port we set up the resources that the server will call
            // during an OTA update. The heavy lifting (begin_ota, write_ota,
            // end_ota) is delegated to platform-specific subclasses.

            auto& ota_options = client["$ota/options"];
            ota_options = [this](output& out) {
                out["enabled"]    = is_enabled();
                out["block_size"] = get_block_size();
                out["timeout"]    = timeout_;
            };

            auto& ota_begin = client["$ota/begin"];
            ota_begin = [this](input& in, output& out) {
                if(running_) {
                    out["success"] = false;
                    return;
                }

                firmware_size_   = in["size"];
                firmware_offset_ = 0;
                running_ = true;

                bool init = begin_ota(in, out);
                out["success"] = init;
                if(!init) running_ = false;
            };

            auto& ota_write = client["$ota/write"];
            ota_write = [this](input& in, output& out) {
                if(!running_) {
                    out["success"] = false;
                    return;
                }

                // The input carries the binary chunk
                bool success = write_ota(in, out);
                running_ = success;
                out["success"] = success;
            };

            auto& ota_end = client["$ota/end"];
            ota_end = [this](output& out) {
                if(!running_) {
                    out["success"] = false;
                    return;
                }

                bool result = end_ota(out);
                out["success"] = result;
                running_ = false;
            };
        }

        virtual ~ThingerOTA() {}

        void set_block_size(size_t size) { block_size_ = size; }
        void set_timeout(size_t timeout) { timeout_ = timeout; }
        bool is_enabled() { return enabled_; }
        void set_enabled(bool enabled) { enabled_ = enabled; }

        void on_start(std::function<void()> cb)    { on_start_ = cb; }
        void on_end(std::function<void()> cb)      { on_end_ = cb; }
        void on_progress(std::function<void(size_t, size_t)> cb) { on_progress_ = cb; }

    protected:
        virtual bool begin_ota(input& options, output& state) = 0;
        virtual bool write_ota(input& data, output& state) = 0;
        virtual bool end_ota(output& state) { return true; }
        virtual bool reset_ota() { return true; }
        virtual size_t get_block_size() const { return block_size_; }
        virtual void fill_options(output& options) = 0;

        bool enabled_           = true;
        bool running_           = false;
        uint16_t block_size_    = 8192;
        size_t timeout_         = 30000;
        size_t firmware_offset_ = 0;
        size_t firmware_size_   = 0;

        std::function<void()> on_start_;
        std::function<void(size_t, size_t)> on_progress_;
        std::function<void()> on_end_;

        arduino_client& client_;
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_OTA_H
