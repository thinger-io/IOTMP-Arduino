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
    // Inherits all common protocol logic from iotmp_client_base via CRTP.
    // Only contains Arduino-specific transport and connection management.
    //
    // Takes an Arduino Client& for transport.  Call handle() from the
    // Arduino loop() — it is cooperative and non-blocking.
    // ----------------------------------------------------------------
    class arduino_client : public iotmp_client_base<arduino_client> {
    public:
        arduino_client(Client& client, const char* user, const char* device, const char* credential)
            : iotmp_client_base(user, device, credential),
              client_(client) {}

        virtual ~arduino_client() = default;

        // ----- CRTP transport implementation -------------------------

        bool send_bytes_impl(const void* data, size_t len) {
            size_t written = client_.write(static_cast<const uint8_t*>(data), len);
            return written == len;
        }

        bool recv_bytes_impl(void* buf, size_t len) {
            auto* ptr = static_cast<uint8_t*>(buf);
            size_t remaining = len;
            unsigned long timeout_ms = 10000;
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
                    if(millis() - start >= timeout_ms) return false;
                    yield();
                }
            }
            return true;
        }

        bool is_connected_impl() const {
            return client_.connected();
        }

        // ----- Milliseconds provider (used by base for streams) ------

        unsigned long get_millis() const {
            return millis();
        }

        // ----- Disconnect handler (called by base on DISCONNECT msg) -

        void on_disconnect() {
            disconnect();
        }

        // ----- Main loop (call from Arduino loop()) -----------------

        virtual void handle() {
            unsigned long now = millis();

            if(!connected_) {
                if(!should_reconnect(now)) return;
                last_connection_attempt_ = now;
                if(!connect_socket()) {
                    update_backoff();
                    return;
                }

                notify_state(client_state::AUTHENTICATING);
                if(!authenticate()) {
                    notify_state(client_state::AUTH_FAILED);
                    disconnect();
                    update_backoff();
                    return;
                }
                notify_state(client_state::AUTHENTICATED);
                connected_ = true;
                last_keepalive_ = millis();
                reset_backoff();
            }

            // --- We are connected from here on ---

            if(!is_connected_impl()) {
                disconnect();
                return;
            }

            // Process all available incoming data
            while(client_.available()) {
                if(!process_incoming()) {
                    disconnect();
                    return;
                }
            }

            // Keep-alive and periodic streams
            process_keepalive(millis());
            check_streams();
        }

    protected:

        // Transport
        Client& client_;

    private:

        // ----- Connection management ---------------------------------

        bool connect_socket() {
            notify_state(client_state::SOCKET_CONNECTING);
            THINGER_LOG_INFO("Connecting to %s:%u", host_, port_);
            bool ok = client_.connect(host_, port_);
            if(ok) {
                THINGER_LOG_INFO("Connected");
                notify_state(client_state::SOCKET_CONNECTED);
            } else {
                THINGER_LOG_ERROR("Connection failed");
                notify_state(client_state::SOCKET_CONNECTION_ERROR);
            }
            return ok;
        }

        void disconnect() {
            THINGER_LOG_INFO("Disconnected");
            client_.stop();
            connected_ = false;
            notify_state(client_state::SOCKET_DISCONNECTED);
            clear_streams();
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_CLIENT_HPP
