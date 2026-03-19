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

#ifndef THINGER_IOTMP_ARDUINO_NB_CORE_H
#define THINGER_IOTMP_ARDUINO_NB_CORE_H

#include <Arduino.h>
#include <EEPROM.h>

#define TINY_GSM_MODEM_BC660

#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 1536
#endif

#include <TinyGsmClient.h>

#include "client.hpp"
#include "compat.hpp"

#ifndef THINGER_NETWORK_REGISTER_TIMEOUT
#define THINGER_NETWORK_REGISTER_TIMEOUT 300000
#endif

#ifndef THINGER_NB_M2CORE
#define PIN_MODEM_RESET 13
#define PIN_MODEM_WAKEUP 12
#define PIN_MODEM_PWR_KEY 19
#define PIN_MODEM_RX 32
#define PIN_MODEM_TX 33
#else
#define PIN_MODEM_RESET 5
#define PIN_MODEM_WAKEUP 18
#define PIN_MODEM_PWR_KEY 23
#define PIN_MODEM_RX 4
#define PIN_MODEM_TX 13
#endif

#define EEPROM_CRC_ADDR 0

#ifdef THINGER_DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
#define SerialAT debugger
#else
#define SerialAT Serial1
#endif

namespace thinger::iotmp {

    class ThingerNBCore : public arduino_client {
    public:
        ThingerNBCore(const char* user, const char* device, const char* credential)
            : arduino_client(client_, user, device, credential),
              modem_(SerialAT),
              client_(modem_)
        {
            port_ = 25200;
        }

        bool init_modem(bool reset_modem = false) {
            pinMode(PIN_MODEM_RESET, OUTPUT);
            pinMode(PIN_MODEM_WAKEUP, OUTPUT);
            pinMode(PIN_MODEM_PWR_KEY, OUTPUT);

            Serial1.begin(115200, SERIAL_8N1, PIN_MODEM_RX, PIN_MODEM_TX);

            if(reset_modem) reset();

            bool init = modem_.init(pin_);
            if(!init) {
                reset();
                return false;
            }

            return initialize_defaults();
        }

        void set_apn(const char* APN, const char* user = "", const char* password = "") {
            apn_          = APN;
            apn_username_ = user;
            apn_password_ = password;
        }

        void set_preferred_operator(const char* operator_name) {
            preferred_operator_ = operator_name;
        }

        void set_preferred_bands(const char* bands) {
            preferred_bands_ = bands;
        }

        void set_pin(const char* pin) {
            pin_ = pin;
        }

        void set_power_save(bool power_save) {
            power_save_ = power_save;
        }

        void set_network_led(bool network_led) {
            network_led_ = network_led;
        }

        void reset() {
            digitalWrite(PIN_MODEM_RESET, 1);
            delay(100);
            digitalWrite(PIN_MODEM_RESET, 0);
            delay(100);
        }

        TinyGsm& getTinyGsm()             { return modem_; }
        TinyGsmClient& getTinyGsmClient()  { return client_; }

        int16_t get_batt_voltage() { return modem_.getBattVoltage(); }
        String get_modem_info()    { return modem_.getModemInfo(); }
        String get_sim_iccid()     { return modem_.getSimCCID(); }
        String get_modem_imei()    { return modem_.getIMEI(); }
        String get_modem_imsi()    { return modem_.getIMSI(); }
        String get_operator()      { return modem_.getOperator(); }

        void handle() override {
            if(!network_connected()) {
                connect_network();
                return;
            }
            arduino_client::handle();
        }

    private:
        const char* apn_                = "iot.1nce.net";
        const char* apn_username_       = nullptr;
        const char* apn_password_       = nullptr;
        const char* preferred_operator_ = nullptr;
        const char* preferred_bands_    = nullptr;
        bool power_save_                = false;
        bool network_led_               = true;
        const char* pin_                = nullptr;
        uint8_t connection_errors_      = 0;

        TinyGsm modem_;

#ifdef _DISABLE_TLS_
        TinyGsmClient client_;
#else
        TinyGsmClientSecure client_;
#endif

        uint8_t crc8(const uint8_t* data, size_t len, uint8_t crc = 0x00) {
            while(len--) {
                crc ^= *data++;
                for(uint8_t i = 0; i < 8; ++i)
                    crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
            }
            return crc;
        }

        uint8_t calc_crc_config() {
            uint8_t crc = 0xFF;
            if(apn_)                crc = crc8((const uint8_t*)apn_, strlen(apn_), crc);
            if(apn_username_)       crc = crc8((const uint8_t*)apn_username_, strlen(apn_username_), crc);
            if(apn_password_)       crc = crc8((const uint8_t*)apn_password_, strlen(apn_password_), crc);
            if(preferred_bands_)    crc = crc8((const uint8_t*)preferred_bands_, strlen(preferred_bands_), crc);
            if(preferred_operator_) crc = crc8((const uint8_t*)preferred_operator_, strlen(preferred_operator_), crc);
            crc = crc8((const uint8_t*)&power_save_, sizeof(power_save_), crc);
            crc = crc8((const uint8_t*)&network_led_, sizeof(network_led_), crc);
            return ~crc;
        }

        bool initialize_defaults() {
            EEPROM.begin(1);
            uint8_t stored_crc  = EEPROM.read(EEPROM_CRC_ADDR);
            uint8_t current_crc = calc_crc_config();

            if(stored_crc == current_crc) {
                EEPROM.end();
                return true;
            }

            modem_.sendAT("+CFUN=0");
            modem_.waitResponse();

            if(apn_username_ != nullptr && apn_password_ != nullptr) {
                modem_.sendAT("+QCGDEFCONT=\"IP\",", "\"", apn_, "\",", "\"", apn_username_, "\",", "\"", apn_password_, "\"");
            } else {
                modem_.sendAT("+QCGDEFCONT=\"IP\",", "\"", apn_, "\"");
            }
            modem_.waitResponse();

            if(preferred_bands_ == nullptr) {
                modem_.sendAT("+QBAND=0");
            } else {
                modem_.sendAT("+QBAND=", preferred_bands_);
            }
            modem_.waitResponse();

            if(preferred_operator_ == nullptr) {
                modem_.sendAT("+COPS=0");
            } else {
                modem_.sendAT("+COPS=4,2,\"", preferred_operator_, "\"");
            }
            modem_.waitResponse();

            modem_.sendAT("+CPSMS=", power_save_ ? "1" : "0");
            modem_.waitResponse();
            modem_.sendAT("+CEDRXS=", power_save_ ? "1" : "0");
            modem_.waitResponse();
            modem_.sendAT("+QEDRXCFG=", power_save_ ? "1" : "0");
            modem_.waitResponse();

            modem_.sendAT("+QLEDMODE=", network_led_ ? "1" : "0");
            modem_.waitResponse();

            modem_.sendAT("+CFUN=1");
            modem_.waitResponse();

            EEPROM.write(EEPROM_CRC_ADDR, current_crc);
            EEPROM.commit();

            reset();
            EEPROM.end();
            return modem_.init(pin_);
        }

        bool network_connected() {
            RegStatus s = modem_.getRegistrationStatus();
            return (s == REG_OK_HOME || s == REG_OK_ROAMING);
        }

        bool connect_network() {
            unsigned long network_timeout = millis();
            while(true) {
                RegStatus cereg = modem_.getRegistrationStatus();

                if(cereg == REG_OK_HOME || cereg == REG_OK_ROAMING) {
                    break;
                }

                if(cereg == REG_DENIED) {
                    delay(30000);
                    init_modem(true);
                    return false;
                }

                if(millis() - network_timeout > THINGER_NETWORK_REGISTER_TIMEOUT) {
                    init_modem(true);
                    return false;
                }

                delay(500);
            }

            delay(100);
            return true;
        }
    };

} // namespace thinger::iotmp

using thinger::iotmp::ThingerNBCore;

#endif
