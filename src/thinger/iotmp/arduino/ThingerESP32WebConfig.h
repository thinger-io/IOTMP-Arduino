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

#ifndef THINGER_IOTMP_ARDUINO_ESP32_WEBCONFIG_H
#define THINGER_IOTMP_ARDUINO_ESP32_WEBCONFIG_H

#ifdef ESP32

#include <FS.h>
#include <SPIFFS.h>
#include "ThingerESP32.h"
#include "ThingerWebConfig.h"

#define FORMAT_SPIFFS_IF_FAILED true
#define CONFIG_FILE "/config.json"

namespace thinger::iotmp {

    class ThingerESP32WebConfig : public ThingerWebConfig<ThingerESP32> {
    public:
        ThingerESP32WebConfig(const char* user = "", const char* device = "", const char* credential = "")
            : ThingerWebConfig<ThingerESP32>(user, device, credential)
        {}

        bool clean_credentials() override {
            if(SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
                if(SPIFFS.exists(CONFIG_FILE)) {
                    bool result = SPIFFS.remove(CONFIG_FILE);
                    SPIFFS.end();
                    return result;
                }
                SPIFFS.end();
            }
            return false;
        }

    protected:
        bool save_configuration(json_t& config) override {
            if(SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
                File configFile = SPIFFS.open(CONFIG_FILE, "w");
                if(configFile) {
                    std::string serialized = config.dump();
                    configFile.write((const uint8_t*)serialized.c_str(), serialized.size());
                    configFile.close();
                    SPIFFS.end();
                    return true;
                }
                SPIFFS.end();
            }
            return false;
        }

        bool load_configuration(json_t& config) override {
            if(SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
                if(SPIFFS.exists(CONFIG_FILE)) {
                    File configFile = SPIFFS.open(CONFIG_FILE, "r");
                    if(configFile) {
                        String content = configFile.readString();
                        configFile.close();
                        config = json_t::parse(content.c_str(), nullptr, false);
                        SPIFFS.end();
                        return !config.is_discarded();
                    }
                }
                SPIFFS.end();
            }
            return false;
        }
    };

} // namespace thinger::iotmp

#endif // ESP32

#endif // THINGER_IOTMP_ARDUINO_ESP32_WEBCONFIG_H
