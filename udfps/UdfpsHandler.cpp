/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8750"

#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_parcel.h>
#include <android/binder_status.h>

#include <fstream>

#include "UdfpsHandler.h"

#define COMMAND_FOD_PRESS_STATUS 1
#define COMMAND_FOD_PRESS_X 2
#define COMMAND_FOD_PRESS_Y 3
#define PARAM_FOD_PRESSED 1
#define PARAM_FOD_RELEASED 0

#define TOUCH_FEATURE_SERVICE "vendor.xiaomi.hw.touchfeature.ITouchFeature/default"
#define TOUCH_FEATURE_SET_MODE_TRANSACTION 9
#define TOUCH_ID_PRIMARY 0
#define TOUCH_MODE_FOD_ENABLE 10

constexpr binder_flags_t kPrivateVendorFlag = static_cast<binder_flags_t>(0x10000000);

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

namespace {

static bool setTouchFodEnabled(bool enabled) {
    ndk::SpAIBinder service(AServiceManager_checkService(TOUCH_FEATURE_SERVICE));
    if (service.get() == nullptr) {
        LOG(ERROR) << "TouchFeature service is unavailable";
        return false;
    }

    AParcel* in = nullptr;
    AParcel* out = nullptr;
    binder_status_t status = AIBinder_prepareTransaction(service.get(), &in);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, TOUCH_ID_PRIMARY);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, TOUCH_MODE_FOD_ENABLE);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, enabled ? 1 : 0);
    if (status == STATUS_OK) {
        status = AIBinder_transact(service.get(), TOUCH_FEATURE_SET_MODE_TRANSACTION, &in, &out,
                                   kPrivateVendorFlag);
    }

    AStatus* callStatus = nullptr;
    int32_t result = -1;
    if (status == STATUS_OK) status = AParcel_readStatusHeader(out, &callStatus);
    if (status == STATUS_OK && callStatus != nullptr && AStatus_isOk(callStatus)) {
        status = AParcel_readInt32(out, &result);
    }

    AStatus_delete(callStatus);
    AParcel_delete(out);
    AParcel_delete(in);

    if (status != STATUS_OK || result != 0) {
        LOG(ERROR) << "Failed to set Touch FOD mode to " << enabled << ": status=" << status
                   << " result=" << result;
        return false;
    }

    LOG(INFO) << "Touch FOD mode " << (enabled ? "enabled" : "disabled");
    return true;
}

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

}  // anonymous namespace

class SM8750UdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) {
        mDevice = device;
    }

    void onFingerDown(uint32_t x, uint32_t y, float /*minor*/, float /*major*/) {
        LOG(DEBUG) << __func__ << "x: " << x << ", y: " << y;
        // Ultrasonic sensors do not receive the optical onUiReady callback.
        // Arm the touch controller when the framework reports actual contact.
        setTouchFodEnabled(true);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_X, x);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_Y, y);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_PRESSED);
    }

    void onFingerUp() {
        LOG(DEBUG) << __func__;
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_X, 0);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_Y, 0);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_RELEASED);
    }

    void onUiReady() {
        LOG(DEBUG) << __func__;
        setTouchFodEnabled(true);
    }

    void onAcquired(int32_t result, int32_t vendorCode) {
        LOG(DEBUG) << __func__ << " result: " << result << " vendorCode: " << vendorCode;
        if (static_cast<AcquiredInfo>(result) == AcquiredInfo::VENDOR && 
           (vendorCode == 201 || vendorCode == 202)) {
            onFingerUp();
        }
    }

    void cancel() {
        LOG(INFO) << __func__;
        setTouchFodEnabled(false);
    }

    void onAuthenticationSucceeded() {
        setTouchFodEnabled(false);
    }

    void onAuthenticationFailed() {
        setTouchFodEnabled(false);
    }

  private:
    fingerprint_device_t* mDevice;
};

static UdfpsHandler* create() {
    return new SM8750UdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
