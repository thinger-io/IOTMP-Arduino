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

#ifndef THINGER_IOTMP_ARDUINO_CONSOLE_H
#define THINGER_IOTMP_ARDUINO_CONSOLE_H

#include <Stream.h>
#include <functional>
#include <map>
#include <string>

#include "client.hpp"

#define CONSOLE_BUFFER_SIZE 128
#define MAX_CMD_ARGS        10

#ifndef THINGER_PROMPT_COLOR
#define THINGER_PROMPT_COLOR "\033[1;34m"
#endif

#ifndef TERMINAL_GREEN_COLOR
#define TERMINAL_GREEN_COLOR "\033[0;32m"
#endif

#ifndef THINGER_RED_COLOR
#define THINGER_RED_COLOR "\033[1;31m"
#endif

#ifndef THINGER_NORMAL_COLOR
#define THINGER_NORMAL_COLOR "\033[0m"
#endif

namespace thinger::iotmp {

    struct ThingerCommand {
        std::function<void(int argc, char* argv[])> callback;
        const char* description;
    };

    class ThingerConsole : public Stream {
    public:
        ThingerConsole(arduino_client& client)
            : client_(client),
              resource_(client["$console"]),
              prompt_(client.device_id_)
        {
            tx_buffer_[CONSOLE_BUFFER_SIZE] = 0;

            register_commands();

            // The console resource will be set up as a bidirectional stream.
            // Input data from the server is processed as terminal input,
            // and output data is sent back as terminal output.
            //
            // The resource registration for stream I/O will be connected
            // once the full IOTMP resource stream API is integrated.
        }

        virtual ~ThingerConsole() {}

        bool is_running() {
            return console_running_;
        }

        bool command_running() {
            client_.handle();
            return is_running() && running_;
        }

        size_t write(uint8_t value) override {
            tx_buffer_[tx_index_++] = value;
            if(value == '\n' || tx_index_ == CONSOLE_BUFFER_SIZE) {
                flush();
            }
            return 1;
        }

        int available() override {
            client_.handle();
            return rx_input_ ? rx_input_ - rx_read_ : 0;
        }

        int read() override {
            if(!available()) return -1;
            auto value = rx_buffer_[rx_read_++];
            if(rx_read_ >= rx_input_) {
                reset();
            }
            return value;
        }

        int peek() override {
            if(!available()) return -1;
            return rx_buffer_[rx_read_];
        }

        void flush() override {
            tx_index_ = 0;
        }

        void set_prompt(const char* prompt) {
            prompt_ = prompt;
        }

        operator bool() {
            return console_running_;
        }

        void command(const char* cmd, std::function<void(int argc, char* argv[])> callback, const char* desc = "") {
            commands_enabled_ = true;
            cmds_[cmd] = {callback, desc};
        }

        void error(const char* message) {
            printf(THINGER_RED_COLOR "%s\r\n", message);
        }

    protected:
        void register_commands() {
            command("clear", [&](int argc, char* argv[]) {
                clear_screen();
            }, "clear console");

            command("help", [&](int argc, char* argv[]) {
                for(auto& [name, cmd] : cmds_) {
                    printf(TERMINAL_GREEN_COLOR "%s\t" THINGER_NORMAL_COLOR " - %s\r\n", name.c_str(), cmd.description);
                }
            }, "show this help");

            commands_enabled_ = false;
        }

        void send_newline() {
            printf("\r\n");
        }

        void send_prompt_1() {
            if(prompt_ != nullptr && strlen(prompt_) > 0) {
                printf(THINGER_PROMPT_COLOR "%s", prompt_);
            }
            printf(THINGER_NORMAL_COLOR "$ ");
            flush();
        }

        void send_prompt_2() {
            printf(THINGER_NORMAL_COLOR "> ");
            flush();
        }

        void remove_last_char() {
            printf("\x08\x1b\x5b\x4a");
            flush();
        }

        void clear_screen() {
            printf("\x1b\x5b\x48\x1b\x5b\x4a");
            flush();
        }

        void reset() {
            rx_index_ = 0;
            rx_read_  = 0;
            rx_input_ = 0;
        }

    protected:
        arduino_client& client_;
        iotmp_resource& resource_;
        const char* prompt_ = nullptr;

        uint8_t tx_buffer_[CONSOLE_BUFFER_SIZE + 1];
        size_t tx_index_ = 0;

        uint8_t rx_buffer_[CONSOLE_BUFFER_SIZE + 1];
        size_t rx_index_  = 0;
        size_t rx_read_   = 0;
        size_t rx_input_  = 0;

        char cmd_buffer_[CONSOLE_BUFFER_SIZE];

        bool commands_enabled_ = false;
        bool running_ = false;
        bool console_running_ = false;

        std::map<std::string, ThingerCommand> cmds_;
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerCommand;
using thinger::iotmp::ThingerConsole;

#endif // THINGER_IOTMP_ARDUINO_CONSOLE_H
