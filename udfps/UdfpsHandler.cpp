/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_kona"

#include "UdfpsHandler.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <thread>
#include <unistd.h>

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF -1

#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_FOD_ENABLE 10
#define TOUCH_MAGIC 0x5400
#define TOUCH_IOC_SETMODE TOUCH_MAGIC + 0

#define DISPPARAM_PATH "/sys/devices/platform/soc/ae00000.qcom,mdss_mdp/drm/card0/card0-DSI-1/disp_param"
#define DISPPARAM_FOD_HBM_OFF "0xE0000"

static const char* kFodUiPaths[] = {
        "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_ui",
        "/sys/devices/platform/soc/soc:qcom,dsi-display/fod_ui",
};

namespace {

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

} // anonymous namespace

static bool readBool(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
}

class XiaomiKonaUdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t *device) {
        mDevice = device;
        touch_fd_ = android::base::unique_fd(open(TOUCH_DEV_PATH, O_RDWR));

        std::thread([this]() {
            int fd;
            for (auto& path : kFodUiPaths) {
                fd = open(path, O_RDONLY);
                if (fd >= 0) {
                    break;
                }
            }

            if (fd < 0) {
                LOG(ERROR) << "failed to open fd, err: " << fd;
                return;
            }

            struct pollfd fodUiPoll = {
                    .fd = fd,
                    .events = POLLERR | POLLPRI,
                    .revents = 0,
            };

            while (true) {
                int rc = poll(&fodUiPoll, 1, -1);
                if (rc < 0) {
                    LOG(ERROR) << "failed to poll fd, err: " << rc;
                    continue;
                }

                if (readBool(fd)) {
                    mDevice->extCmd(mDevice, COMMAND_NIT, PARAM_NIT_FOD);
                    int arg[2] = {TOUCH_FOD_ENABLE, FOD_STATUS_ON};
                    ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
                } else {
                    mDevice->extCmd(mDevice, COMMAND_NIT, PARAM_NIT_NONE);
                    set(DISPPARAM_PATH, DISPPARAM_FOD_HBM_OFF);
                    int arg[2] = {TOUCH_FOD_ENABLE, FOD_STATUS_OFF};
                    ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
                }
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        // nothing
    }

    void onFingerUp() {
        // nothing
    }

    void onAcquired(int32_t /*result*/, int32_t /*vendorCode*/) {
        // nothing
    }

    void cancel() {
        // nothing
    }
  private:
    fingerprint_device_t *mDevice;
    android::base::unique_fd touch_fd_;
};

static UdfpsHandler* create() {
    return new XiaomiKonaUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
    .create = create,
    .destroy = destroy,
};
