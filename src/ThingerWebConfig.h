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

#ifndef THINGER_IOTMP_ARDUINO_WEBCONFIG_H
#define THINGER_IOTMP_ARDUINO_WEBCONFIG_H

#include "client.hpp"
#include <DNSServer.h>
#include <WiFiManager.h>

#ifndef THINGER_DEVICE_SSID
#define THINGER_DEVICE_SSID "Thinger-Device"
#endif

#ifndef THINGER_DEVICE_SSID_PSWD
#define THINGER_DEVICE_SSID_PSWD "thinger.io"
#endif

#define MAX_ADDITIONAL_PARAMETERS 5

namespace thinger::iotmp {

    template <class ThingerDevice>
    class ThingerWebConfig : public ThingerDevice {
    public:
        ThingerWebConfig(const char* user = "", const char* device = "", const char* credential = "")
            : ThingerDevice(user_, device_, credential_),
              custom_params_(0),
              config_callback_(nullptr),
              wifi_callback_(nullptr),
              captive_portal_callback_(nullptr)
        {
            strncpy(user_, user, sizeof(user_) - 1);
            strncpy(device_, device, sizeof(device_) - 1);
            strncpy(credential_, credential, sizeof(credential_) - 1);
        }

        virtual ~ThingerWebConfig() {}

        virtual bool clean_credentials() = 0;

        bool add_setup_parameter(const char* id, const char* placeholder, const char* defaultValue, int length, const char* custom = "") {
            if(custom_params_ < MAX_ADDITIONAL_PARAMETERS) {
                parameters_[custom_params_++] = new WiFiManagerParameter(id, placeholder, defaultValue, length, custom);
                return true;
            }
            return false;
        }

        void set_on_config_callback(void (*callback)(json_t&)) {
            config_callback_ = callback;
        }

        void set_on_wifi_config(void (*callback)(bool)) {
            wifi_callback_ = callback;
        }

        void set_on_captive_portal_run(void (*callback)(WiFiManager&)) {
            captive_portal_callback_ = callback;
        }

        void set_user(const char* user) { strncpy(user_, user, sizeof(user_) - 1); }
        void set_device(const char* device) { strncpy(device_, device, sizeof(device_) - 1); }
        void set_credential(const char* credential) { strncpy(credential_, credential, sizeof(credential_) - 1); }

    protected:
        virtual bool load_configuration(json_t& configuration) = 0;
        virtual bool save_configuration(json_t& configuration) = 0;

    private:
        char user_[40]       = {};
        char device_[40]     = {};
        char credential_[40] = {};
        WiFiManagerParameter* parameters_[MAX_ADDITIONAL_PARAMETERS];
        int custom_params_;
        void (*config_callback_)(json_t&);
        void (*wifi_callback_)(bool);
        void (*captive_portal_callback_)(WiFiManager&);
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerWebConfig;

#endif // THINGER_IOTMP_ARDUINO_WEBCONFIG_H
