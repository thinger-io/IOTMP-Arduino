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

#include <map>
#include <string>

namespace thinger::iotmp {

    // ----------------------------------------------------------------
    // Writer adapter that buffers small writes and flushes them to an
    // Arduino Client in a single TCP write.  This is critical because
    // the encoder emits many tiny fragments (varint bytes, tags).
    // ----------------------------------------------------------------
    class buffered_writer {
    public:
        buffered_writer(Client& client, uint8_t*& buf, size_t& size, size_t& capacity)
            : client_(client), buf_(buf), size_(size), capacity_(capacity) {}

        bool write(const void* data, size_t len) {
            if(size_ + len > capacity_) {
                size_t new_cap = capacity_;
                while(new_cap < size_ + len) {
                    new_cap += GROW;
                }
                auto* p = static_cast<uint8_t*>(realloc(buf_, new_cap));
                if(!p) return false;
                buf_ = p;
                capacity_ = new_cap;
            }
            memcpy(buf_ + size_, data, len);
            size_ += len;
            return true;
        }

        size_t bytes_written() const { return size_; }

    private:
        Client& client_;
        uint8_t*& buf_;
        size_t& size_;
        size_t& capacity_;
        static constexpr size_t GROW = 64;
    };

    // ----------------------------------------------------------------
    // Stream configuration stored per active stream.
    // ----------------------------------------------------------------
    struct stream_config {
        const char* resource_name = nullptr;
        unsigned long interval_ms = 0;
        unsigned long last_sample  = 0;
    };

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
    // Takes an Arduino Client& for transport.  Call handle() from the
    // Arduino loop() — it is cooperative and non-blocking.
    // ----------------------------------------------------------------
    class arduino_client {
    public:
        arduino_client(Client& client, const char* user, const char* device, const char* credential)
            : client_(client),
              username_(user),
              device_id_(device),
              credential_(credential) {}

        virtual ~arduino_client() {
            free_output_buffer();
        }

        // ----- State listener ----------------------------------------

        void set_state_listener(std::function<void(THINGER_STATE)> listener) {
            state_listener_ = std::move(listener);
        }

        // ----- Resource registration --------------------------------

        iotmp_resource& operator[](const char* name) {
            return resources_[std::string(name)];
        }

        // ----- Main loop (call from Arduino loop()) -----------------

        virtual void handle() {
            unsigned long now = millis();

            if(state_ == DISCONNECTED) {
                if(now - last_connection_attempt_ < RECONNECT_MS) return;
                THINGER_LOG_INFO("Reconnecting in %lu ms", RECONNECT_MS);
                last_connection_attempt_ = now;
                if(!connect_socket()) return;
                if(!authenticate()) {
                    disconnect();
                    return;
                }
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

        // ----- Configuration ----------------------------------------

        void set_host(const char* host, uint16_t port = 0) {
            host_ = host;
            if(port != 0) port_ = port;
        }

        // ----- Server API -------------------------------------------

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

        // ----- Manual streaming -------------------------------------

        bool stream(const char* resource_name) {
            if(state_ != AUTHENTICATED) return false;
            auto it = resources_.find(resource_name);
            if(it == resources_.end()) return false;
            auto& res = it->second;
            if(!res.stream_enabled()) return false;
            return stream_resource(res, res.get_stream_id());
        }

        // ----- Connection state -------------------------------------

        bool is_connected() const {
            return state_ == AUTHENTICATED;
        }

    protected:

        // Transport
        Client& client_;

        // Credentials
        const char* username_;
        const char* device_id_;
        const char* credential_;
        const char* host_ = "iot.thinger.io";
        uint16_t port_ = 25204;

        // Connection state
        enum state_t { DISCONNECTED, AUTHENTICATED };
        state_t state_ = DISCONNECTED;

        // State listener
        std::function<void(THINGER_STATE)> state_listener_;

        // Resources
        std::map<std::string, iotmp_resource> resources_;

        // Active streams  (stream_id -> config)
        std::map<uint16_t, stream_config> streams_;

        // Timing
        unsigned long last_keepalive_ = 0;
        unsigned long last_connection_attempt_ = 0;
        static constexpr unsigned long KEEPALIVE_MS = 60000;
        static constexpr unsigned long RECONNECT_MS = 5000;

    private:

        // ----- Output buffer for write coalescing -------------------

        uint8_t* out_buffer_   = nullptr;
        size_t   out_size_     = 0;
        size_t   out_capacity_ = 0;

        void free_output_buffer() {
            if(out_buffer_) {
                free(out_buffer_);
                out_buffer_ = nullptr;
            }
            out_size_ = 0;
            out_capacity_ = 0;
        }

        void flush_output() {
            if(out_size_ > 0 && out_buffer_) {
                client_.write(out_buffer_, out_size_);
                client_.flush();
            }
            // Release the buffer after flush
            free_output_buffer();
        }

        // ----- I/O helpers ------------------------------------------

        bool io_read(void* buf, size_t len) {
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

        bool io_write(const void* buf, size_t len, bool flush_flag = false) {
            // Accumulate in output buffer
            if(len > 0) {
                if(out_size_ + len > out_capacity_) {
                    size_t new_cap = out_capacity_;
                    while(new_cap < out_size_ + len) new_cap += 64;
                    auto* p = static_cast<uint8_t*>(realloc(out_buffer_, new_cap));
                    if(!p) return false;
                    out_buffer_ = p;
                    out_capacity_ = new_cap;
                }
                memcpy(out_buffer_ + out_size_, buf, len);
                out_size_ += len;
            }
            if(flush_flag) flush_output();
            return true;
        }

        // ----- Varint I/O -------------------------------------------

        bool read_varint(uint32_t& value) {
            value = 0;
            uint8_t byte;
            uint8_t bit_pos = 0;
            do {
                if(!io_read(&byte, 1) || bit_pos >= 32) return false;
                value |= static_cast<uint32_t>(byte & 0x7F) << bit_pos;
                bit_pos += 7;
            } while(byte & 0x80);
            return true;
        }

        bool write_varint(uint32_t value) {
            do {
                uint8_t byte = value & 0x7F;
                value >>= 7;
                if(value > 0) byte |= 0x80;
                if(!io_write(&byte, 1)) return false;
            } while(value > 0);
            return true;
        }

        // ----- Message read / write ---------------------------------

        bool read_message(iotmp_message& msg) {
            // Read message type varint
            uint32_t msg_type = 0;
            if(!read_varint(msg_type)) return false;
            msg.set_message_type(static_cast<message::type>(msg_type));

            // Read body size varint
            uint32_t body_size = 0;
            if(!read_varint(body_size)) return false;

            if(body_size == 0) return true;

            // Read body into buffer, then decode from memory
            std::vector<uint8_t> body(body_size);
            if(!io_read(body.data(), body_size)) return false;

            iotmp_memory_decoder decoder(body.data(), body_size);
            return decoder.decode(msg, body_size);
        }

        bool write_message(iotmp_message& msg) {
            // Use the core encode_message which does two-pass (size, then encode)
            std::string encoded = encode_message(msg);
            return io_write(encoded.data(), encoded.size(), true);
        }

        void send_message(iotmp_message& msg) {
            if(msg.get_message_type() != message::STREAM_DATA) {
                THINGER_LOG_DEBUG("TX: %s (stream=%u)", msg.message_type_str(), msg.get_stream_id());
            }
            write_message(msg);
        }

        void send_keepalive() {
            THINGER_LOG_DEBUG("Keep-alive sent");
            std::string encoded = encode_message(message::KEEP_ALIVE);
            io_write(encoded.data(), encoded.size(), true);
        }

        // ----- Connection -------------------------------------------

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

        bool authenticate() {
            notify_state(THINGER_AUTHENTICATING);
            THINGER_LOG_INFO("Authenticating as %s@%s", device_id_, username_);

            iotmp_message msg(message::CONNECT);
            msg.set_random_stream_id();
            msg[message::field::PAYLOAD] = json_t::array({
                json_t(username_),
                json_t(device_id_),
                json_t(credential_)
            });

            if(!write_message(msg)) {
                THINGER_LOG_ERROR("Failed to send CONNECT");
                notify_state(THINGER_AUTH_FAILED);
                return false;
            }

            // Wait for response
            iotmp_message response(message::RESERVED);
            if(!read_message(response)) {
                THINGER_LOG_ERROR("No CONNECT response");
                notify_state(THINGER_AUTH_FAILED);
                return false;
            }

            bool ok = response.get_message_type() == message::OK;
            if(ok) {
                THINGER_LOG_INFO("Authenticated!");
                notify_state(THINGER_AUTHENTICATED);
            } else {
                THINGER_LOG_ERROR("Authentication failed");
                notify_state(THINGER_AUTH_FAILED);
            }
            return ok;
        }

        void disconnect() {
            THINGER_LOG_INFO("Disconnected");
            client_.stop();
            state_ = DISCONNECTED;
            notify_state(SOCKET_DISCONNECTED);
            streams_.clear();
            for(auto& [name, res] : resources_) {
                res.set_stream_id(0);
            }
        }

        // ----- Message handling -------------------------------------

        void handle_message(iotmp_message& msg) {
            if(msg.get_message_type() != message::STREAM_DATA) {
                THINGER_LOG_DEBUG("RX: %s (stream=%u)", msg.message_type_str(), msg.get_stream_id());
            }

            switch(msg.get_message_type()) {
                case message::RUN:
                    handle_resource_request(msg);
                    break;
                case message::DESCRIBE:
                    handle_describe(msg);
                    break;
                case message::START_STREAM:
                    handle_start_stream(msg);
                    break;
                case message::STOP_STREAM:
                    handle_stop_stream(msg);
                    break;
                case message::KEEP_ALIVE:
                    // Server keep-alive, nothing to do
                    break;
                case message::DISCONNECT:
                    disconnect();
                    break;
                default:
                    break;
            }
        }

        void handle_resource_request(iotmp_message& request) {
            // Resource path is in RESOURCE field
            std::string path;
            if(request.has_field(message::field::RESOURCE)) {
                const json_t& res_field = request[message::field::RESOURCE];
                if(res_field.is_string()) {
                    path = res_field.get<std::string>();
                } else if(res_field.is_array()) {
                    // Concatenate array elements with '/'
                    for(size_t i = 0; i < res_field.size(); ++i) {
                        if(i > 0) path += '/';
                        path += res_field[i].get<std::string>();
                    }
                }
            }

            iotmp_resource* resource = find_resource(path);
            iotmp_message response(request.get_stream_id(), message::OK);

            if(resource) {
                bool success = resource->run_resource(request, response);
                if(!success) {
                    response.set_message_type(message::ERROR);
                }
            } else {
                response.set_message_type(message::ERROR);
            }

            send_message(response);

            // If the resource has stream echo enabled and a stream is active
            if(resource && resource->stream_enabled() && resource->stream_echo()) {
                stream_resource(*resource, resource->get_stream_id());
            }
        }

        void handle_describe(iotmp_message& request) {
            iotmp_message response(request.get_stream_id(), message::OK);

            // Check if asking for a specific resource or the API
            if(request.has_field(message::field::RESOURCE)) {
                std::string path;
                const json_t& res_field = request[message::field::RESOURCE];
                if(res_field.is_string()) {
                    path = res_field.get<std::string>();
                } else if(res_field.is_array()) {
                    for(size_t i = 0; i < res_field.size(); ++i) {
                        if(i > 0) path += '/';
                        path += res_field[i].get<std::string>();
                    }
                }

                iotmp_resource* resource = find_resource(path);
                if(resource) {
                    resource->describe(response);
                } else {
                    response.set_message_type(message::ERROR);
                }
            } else {
                // Describe full API: list all resources
                json_t& payload = response[message::field::PAYLOAD];
                for(auto& [name, res] : resources_) {
                    res.fill_api(payload[name]);
                }
            }

            send_message(response);
        }

        void handle_start_stream(iotmp_message& request) {
            std::string path;
            if(request.has_field(message::field::RESOURCE)) {
                const json_t& res_field = request[message::field::RESOURCE];
                if(res_field.is_string()) {
                    path = res_field.get<std::string>();
                } else if(res_field.is_array()) {
                    for(size_t i = 0; i < res_field.size(); ++i) {
                        if(i > 0) path += '/';
                        path += res_field[i].get<std::string>();
                    }
                }
            }

            uint16_t stream_id = request.get_stream_id();

            iotmp_resource* resource = find_resource(path);
            if(!resource) {
                iotmp_message response(stream_id, message::ERROR);
                send_message(response);
                return;
            }

            // Set the stream id on the resource
            resource->set_stream_id(stream_id);

            // Check if there is an interval in parameters
            unsigned long interval_ms = 0;
            if(request.has_params()) {
                const json_t& params = request.params();
                if(params.is_number()) {
                    interval_ms = params.get<uint64_t>();
                }
            }

            // Register stream
            stream_config cfg;
            cfg.resource_name = nullptr;
            cfg.interval_ms = interval_ms;
            cfg.last_sample = millis();

            // We need to store the path — find the key in resources_ map
            for(auto& [name, res] : resources_) {
                if(&res == resource) {
                    cfg.resource_name = name.c_str();
                    break;
                }
            }

            streams_[stream_id] = cfg;

            THINGER_LOG_DEBUG("Stream started: %s (id=%u)", cfg.resource_name ? cfg.resource_name : "?", stream_id);

            // Send OK
            iotmp_message response(stream_id, message::OK);
            send_message(response);

            // Send initial stream data
            stream_resource(*resource, stream_id);
        }

        void handle_stop_stream(iotmp_message& request) {
            uint16_t stream_id = request.get_stream_id();
            THINGER_LOG_DEBUG("Stream stopped (id=%u)", stream_id);

            // Find and clean up the stream
            auto it = streams_.find(stream_id);
            if(it != streams_.end()) {
                // Clear the stream id on the resource
                iotmp_resource* resource = find_resource(it->second.resource_name);
                if(resource) {
                    resource->set_stream_id(0);
                }
                streams_.erase(it);
            }

            // Send OK
            iotmp_message response(stream_id, message::OK);
            send_message(response);
        }

        bool stream_resource(iotmp_resource& resource, uint16_t stream_id) {
            if(resource.get_io_type() != iotmp_resource::output_wrapper &&
               resource.get_io_type() != iotmp_resource::input_output_wrapper) {
                return false;
            }

            iotmp_message dummy_request(stream_id, message::STREAM_DATA);
            iotmp_message stream_msg(stream_id, message::STREAM_DATA);

            resource.run_resource(dummy_request, stream_msg);

            send_message(stream_msg);
            return true;
        }

        void check_streams() {
            unsigned long now = millis();
            for(auto& [stream_id, cfg] : streams_) {
                if(cfg.interval_ms == 0) continue;
                if(now - cfg.last_sample >= cfg.interval_ms) {
                    cfg.last_sample = now;
                    if(cfg.resource_name) {
                        iotmp_resource* resource = find_resource(cfg.resource_name);
                        if(resource) {
                            stream_resource(*resource, stream_id);
                        }
                    }
                }
            }
        }

        // ----- Resource lookup --------------------------------------

        iotmp_resource* find_resource(const char* path) {
            if(!path) return nullptr;
            return find_resource(std::string(path));
        }

        iotmp_resource* find_resource(const std::string& path) {
            auto it = resources_.find(path);
            if(it != resources_.end()) return &it->second;
            return nullptr;
        }

        // ----- Blocking send + wait for OK --------------------------

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
