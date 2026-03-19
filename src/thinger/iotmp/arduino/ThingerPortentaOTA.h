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

#ifndef THINGER_IOTMP_ARDUINO_PORTENTA_OTA_H
#define THINGER_IOTMP_ARDUINO_PORTENTA_OTA_H

#include "ThingerOTA.h"
#include <mbed.h>
#include <FATFileSystem.h>
#include <QSPIFBlockDevice.h>
#include <MBRBlockDevice.h>
#include <stm32h7xx_hal_rtc_ex.h>

#define PORTENTA_SD_MOUNT_PATH   "fs"
#define PORTENTA_UPDATE_FILE     "/" PORTENTA_SD_MOUNT_PATH "/" "UPDATE.BIN"

#define APOTA_QSPI_FLASH_FLAG   (1 << 2)
#define APOTA_SDCARD_FLAG        (1 << 3)
#define APOTA_RAW_FLAG           (1 << 4)
#define APOTA_FATFS_FLAG         (1 << 5)
#define APOTA_LITTLEFS_FLAG      (1 << 6)
#define APOTA_MBR_FLAG           (1 << 7)

extern RTC_HandleTypeDef RTCHandle;

namespace thinger::iotmp {

    class ThingerPortentaOTA : public ThingerOTA {
    public:
        ThingerPortentaOTA(arduino_client& client)
            : ThingerOTA(client)
        {
            set_block_size(8192);
        }

        virtual ~ThingerPortentaOTA() {}

    protected:

        enum StorageTypePortenta {
            QSPI_FLASH_FATFS        = APOTA_QSPI_FLASH_FLAG | APOTA_FATFS_FLAG,
            QSPI_FLASH_FATFS_MBR    = APOTA_QSPI_FLASH_FLAG | APOTA_FATFS_FLAG | APOTA_MBR_FLAG,
            SD_FATFS                = APOTA_SDCARD_FLAG | APOTA_FATFS_FLAG,
            SD_FATFS_MBR            = APOTA_SDCARD_FLAG | APOTA_FATFS_FLAG | APOTA_MBR_FLAG,
        };

        void fill_options(output& options) override {
            options["platform"] = "atmelsam";
        }

        bool begin_ota(input& options, output& state) override {
            bd_raw_qspi_ = mbed::BlockDevice::get_default_instance();

            if(bd_raw_qspi_ != nullptr && bd_raw_qspi_->init() != 0) {
                state["error"] = "QSPI init failure";
                return false;
            }

            bool mounted = false;

            if(storage_type_ == QSPI_FLASH_FATFS) {
                fs_qspi_ = new mbed::FATFileSystem(PORTENTA_SD_MOUNT_PATH);
                mounted = fs_qspi_->mount(bd_raw_qspi_) == 0;
            } else if(storage_type_ == QSPI_FLASH_FATFS_MBR) {
                bd_qspi_ = new mbed::MBRBlockDevice(bd_raw_qspi_, 2);
                fs_qspi_ = new mbed::FATFileSystem(PORTENTA_SD_MOUNT_PATH);
                mounted = fs_qspi_->mount(bd_qspi_) == 0;
            } else {
                state["error"] = "Unknown target destination";
                return false;
            }

            if(!mounted) {
                state["error"] = "Error while mounting the filesystem";
                return false;
            }

            f_ = fopen(PORTENTA_UPDATE_FILE, "wb");
            if(f_ == nullptr) {
                state["error"] = "Cannot create file: " PORTENTA_UPDATE_FILE;
                return false;
            }

            return true;
        }

        bool reset_ota() override {
            if(f_ != nullptr) {
                fclose(f_);
                f_ = nullptr;
            }
            if(fs_qspi_) {
                fs_qspi_->remove(PORTENTA_UPDATE_FILE);
            }
            return true;
        }

        bool write_ota(input& data, output& state) override {
            return true;
        }

        bool end_ota(output& state) override {
            if(f_) {
                int err = fclose(f_);
                f_ = nullptr;
                if(err < 0) {
                    state["error"] = "error while closing file";
                    return false;
                }
            }

            HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR0, 0x07AA);
            HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR1, storage_type_);
            HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR2, 2);
            HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR3, firmware_size_);

            return true;
        }

    private:
        mbed::BlockDevice*   bd_raw_qspi_ = nullptr;
        mbed::BlockDevice*   bd_qspi_     = nullptr;
        mbed::FATFileSystem* fs_qspi_     = nullptr;
        StorageTypePortenta  storage_type_ = QSPI_FLASH_FATFS_MBR;
        FILE* f_ = nullptr;
    };

} // namespace thinger::iotmp

#endif // THINGER_IOTMP_ARDUINO_PORTENTA_OTA_H
