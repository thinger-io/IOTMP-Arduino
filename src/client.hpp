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

#ifndef THINGER_IOTMP_ARDUINO_CLIENT_HPP
#define THINGER_IOTMP_ARDUINO_CLIENT_HPP

// Serial debug logging — must be defined before including core headers
#ifdef THINGER_SERIAL_DEBUG
#include <Arduino.h>
#include <cstdarg>

inline void _thinger_log(const char* level, const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(level);
    Serial.println(buf);
}

#ifndef THINGER_LOG_ERROR
#define THINGER_LOG_ERROR(fmt, ...)   _thinger_log("[E][IOTMP] ", fmt, ##__VA_ARGS__)
#endif
#ifndef THINGER_LOG_WARNING
#define THINGER_LOG_WARNING(fmt, ...) _thinger_log("[W][IOTMP] ", fmt, ##__VA_ARGS__)
#endif
#ifndef THINGER_LOG_INFO
#define THINGER_LOG_INFO(fmt, ...)    _thinger_log("[I][IOTMP] ", fmt, ##__VA_ARGS__)
#endif
#ifndef THINGER_LOG_DEBUG
#define THINGER_LOG_DEBUG(fmt, ...)   _thinger_log("[D][IOTMP] ", fmt, ##__VA_ARGS__)
#endif

#endif // THINGER_SERIAL_DEBUG

#include <Arduino.h>
#include <Client.h>

#include "compat.hpp"
#include <thinger/iotmp/iotmp.hpp>

#include <functional>

namespace thinger::iotmp {

    // ----------------------------------------------------------------
    // Arduino IOTMP client.
    //
    // Inherits all protocol and connection-lifecycle logic from
    // iotmp_client_base via CRTP.  This class only provides the
    // transport primitives that the base dispatches through CRTP.
    //
    // Takes an Arduino Client& for transport.  Call handle() from the
    // Arduino loop() — it is cooperative and non-blocking.
    //
    // Subclasses (ThingerESP32, ThingerESP8266, …) may override
    // handle() (virtual) to add network management before delegating
    // to the base implementation.
    // ----------------------------------------------------------------
    class arduino_client : public iotmp_client_base<arduino_client> {
    public:
        arduino_client(Client& client, const char* user, const char* device, const char* credential)
            : iotmp_client_base(user, device, credential),
              client_(client) {}

        virtual ~arduino_client() = default;

        // ----- CRTP transport implementation -------------------------

        bool send_bytes_impl(const void* data, size_t len) {
            return client_.write(static_cast<const uint8_t*>(data), len) == len;
        }

        bool recv_bytes_impl(void* buf, size_t len) {
            auto* ptr = static_cast<uint8_t*>(buf);
            size_t remaining = len;
            unsigned long start = millis();
            while(remaining > 0) {
                if(!client_.connected()) return false;
                int avail = client_.available();
                if(avail > 0) {
                    size_t to_read = remaining < static_cast<size_t>(avail)
                                     ? remaining : static_cast<size_t>(avail);
                    int got = client_.read(ptr, to_read);
                    if(got <= 0) return false;
                    ptr += got;
                    remaining -= got;
                    start = millis(); // reset timeout on progress
                } else {
                    if(millis() - start >= 10000) return false;
                    yield();
                }
            }
            return true;
        }

        bool is_connected_impl() const { return client_.connected(); }
        bool data_available_impl() { return client_.available() > 0; }
        unsigned long get_millis() const { return millis(); }

        // ----- CRTP connection implementation ------------------------

        bool connect_impl() {
            return client_.connect(host_, port_);
        }

        void disconnect_impl() {
            client_.stop();
        }

        // ----- Virtual handle (allows subclass override) -------------
        //
        // The actual lifecycle logic lives in iotmp_client_base::handle().
        // This virtual wrapper lets WiFi/network subclasses add their
        // own pre-checks before delegating.

        virtual void handle() {
            iotmp_client_base::handle();
        }

    protected:

        // Transport
        Client& client_;
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_CLIENT_HPP
