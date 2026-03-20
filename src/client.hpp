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
    // Connection state enum — compatible with classic library
    // ----------------------------------------------------------------
    enum THINGER_STATE {
        NETWORK_CONNECTING,
        NETWORK_CONNECTED,
        NETWORK_CONNECT_ERROR,
        SOCKET_CONNECTING,
        SOCKET_CONNECTED,
        SOCKET_CONNECTION_ERROR,
        SOCKET_DISCONNECTED,
        SOCKET_TIMEOUT,
        SOCKET_ERROR,
        THINGER_AUTHENTICATING,
        THINGER_AUTHENTICATED,
        THINGER_AUTH_FAILED,
        THINGER_STOP_REQUEST
    };

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

        // ----- State listener ----------------------------------------

        void set_state_listener(std::function<void(THINGER_STATE)> listener) {
            state_listener_ = std::move(listener);
        }

        // ----- Main loop (call from Arduino loop()) -----------------

        virtual void handle() {
            unsigned long now = millis();

            if(state_ == DISCONNECTED) {
                if(now - last_connection_attempt_ < RECONNECT_MS) return;
                THINGER_LOG_INFO("Reconnecting in %lu ms", RECONNECT_MS);
                last_connection_attempt_ = now;
                if(!connect_socket()) return;

                notify_state(THINGER_AUTHENTICATING);
                if(!authenticate()) {
                    notify_state(THINGER_AUTH_FAILED);
                    disconnect();
                    return;
                }
                notify_state(THINGER_AUTHENTICATED);
                state_ = AUTHENTICATED;
                last_keepalive_ = millis();
            }

            // --- We are AUTHENTICATED from here on ---

            if(!client_.connected()) {
                disconnect();
                return;
            }

            // Process all available incoming data
            while(client_.available()) {
                iotmp_message msg(message::RESERVED);
                if(read_message(msg)) {
                    handle_message(msg);
                } else {
                    disconnect();
                    return;
                }
            }

            // Keep-alive
            now = millis();
            if(now - last_keepalive_ >= KEEPALIVE_MS) {
                send_keepalive();
                last_keepalive_ = now;
            }

            // Periodic streams
            check_streams();
        }

        // ----- Connection state --------------------------------------

        bool is_connected() const {
            return state_ == AUTHENTICATED;
        }

        // ----- Server API --------------------------------------------

        bool set_property(const char* id, json_t data) {
            if(state_ != AUTHENTICATED) return false;
            iotmp_message msg(message::RUN);
            msg.set_random_stream_id();
            msg[message::field::PARAMETERS] = static_cast<uint64_t>(server::SET_DEVICE_PROPERTY);
            msg[message::field::RESOURCE] = id;
            msg[message::field::PAYLOAD] = std::move(data);
            return send_and_wait_ok(msg);
        }

        bool get_property(const char* id, json_t& data) {
            if(state_ != AUTHENTICATED) return false;
            iotmp_message msg(message::RUN);
            msg.set_random_stream_id();
            msg[message::field::PARAMETERS] = static_cast<uint64_t>(server::READ_DEVICE_PROPERTY);
            msg[message::field::RESOURCE] = id;
            uint16_t sid = msg.get_stream_id();
            send_message(msg);

            // Wait for response (blocking with timeout)
            unsigned long start = millis();
            while(millis() - start < 10000) {
                if(!client_.connected()) return false;
                if(client_.available()) {
                    iotmp_message reply(message::RESERVED);
                    if(!read_message(reply)) return false;
                    if(reply.get_stream_id() == sid) {
                        if(reply.get_message_type() == message::OK) {
                            if(reply.has_payload()) data = std::move(reply.payload());
                            return true;
                        }
                        return false;
                    }
                    // Not our reply — handle normally
                    handle_message(reply);
                }
                yield();
            }
            return false;
        }

        bool write_bucket(const char* id, json_t data) {
            if(state_ != AUTHENTICATED) return false;
            iotmp_message msg(message::RUN);
            msg.set_random_stream_id();
            msg[message::field::PARAMETERS] = static_cast<uint64_t>(server::WRITE_BUCKET);
            msg[message::field::RESOURCE] = id;
            msg[message::field::PAYLOAD] = std::move(data);
            return send_and_wait_ok(msg);
        }

        bool call_endpoint(const char* name) {
            if(state_ != AUTHENTICATED) return false;
            iotmp_message msg(message::RUN);
            msg.set_random_stream_id();
            msg[message::field::PARAMETERS] = static_cast<uint64_t>(server::CALL_ENDPOINT);
            msg[message::field::RESOURCE] = name;
            return send_and_wait_ok(msg);
        }

        bool call_endpoint(const char* name, json_t data) {
            if(state_ != AUTHENTICATED) return false;
            iotmp_message msg(message::RUN);
            msg.set_random_stream_id();
            msg[message::field::PARAMETERS] = static_cast<uint64_t>(server::CALL_ENDPOINT);
            msg[message::field::RESOURCE] = name;
            msg[message::field::PAYLOAD] = std::move(data);
            return send_and_wait_ok(msg);
        }

    protected:

        // Transport
        Client& client_;

        // Connection state
        enum state_t { DISCONNECTED, AUTHENTICATED };
        state_t state_ = DISCONNECTED;

        // State listener
        std::function<void(THINGER_STATE)> state_listener_;

        // Timing
        unsigned long last_keepalive_ = 0;
        unsigned long last_connection_attempt_ = 0;
        static constexpr unsigned long KEEPALIVE_MS = 60000;
        static constexpr unsigned long RECONNECT_MS = 5000;

    private:

        // ----- Connection management ---------------------------------

        void notify_state(THINGER_STATE state) {
            if(state_listener_) state_listener_(state);
        }

        bool connect_socket() {
            notify_state(SOCKET_CONNECTING);
            THINGER_LOG_INFO("Connecting to %s:%u", host_, port_);
            bool ok = client_.connect(host_, port_);
            if(ok) {
                THINGER_LOG_INFO("Connected");
                notify_state(SOCKET_CONNECTED);
            } else {
                THINGER_LOG_ERROR("Connection failed");
                notify_state(SOCKET_CONNECTION_ERROR);
            }
            return ok;
        }

        void disconnect() {
            THINGER_LOG_INFO("Disconnected");
            client_.stop();
            state_ = DISCONNECTED;
            notify_state(SOCKET_DISCONNECTED);
            clear_streams();
        }

        // ----- Blocking send + wait for OK ---------------------------

        bool send_and_wait_ok(iotmp_message& msg) {
            uint16_t sid = msg.get_stream_id();
            send_message(msg);

            unsigned long start = millis();
            while(millis() - start < 10000) {
                if(!client_.connected()) return false;
                if(client_.available()) {
                    iotmp_message reply(message::RESERVED);
                    if(!read_message(reply)) return false;
                    if(reply.get_stream_id() == sid) {
                        return reply.get_message_type() == message::OK;
                    }
                    // Not our reply — handle normally
                    handle_message(reply);
                }
                yield();
            }
            return false;
        }
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_CLIENT_HPP
