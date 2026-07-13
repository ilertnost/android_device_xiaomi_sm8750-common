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

#define COMMAND_FOD_PRESS_X 2
#define COMMAND_FOD_PRESS_Y 3

#define TOUCH_FEATURE_SERVICE "vendor.xiaomi.hw.touchfeature.ITouchFeature/default"
#define TOUCH_FEATURE_DESCRIPTOR "vendor.xiaomi.hw.touchfeature.ITouchFeature"
#define TOUCH_FEATURE_SET_MODE_TRANSACTION 9
#define TOUCH_ID_PRIMARY 0
#define TOUCH_MODE_FOD_ENABLE 10

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

namespace {

static void* touchFeatureOnCreate(void* args) {
    return args;
}

static void touchFeatureOnDestroy(void*) {}

static binder_status_t touchFeatureOnTransact(AIBinder*, transaction_code_t, const AParcel*,
                                               AParcel*) {
    return STATUS_UNKNOWN_TRANSACTION;
}

static AIBinder_Class* touchFeatureClass() {
    static AIBinder_Class* clazz =
            AIBinder_Class_define(TOUCH_FEATURE_DESCRIPTOR, touchFeatureOnCreate,
                                  touchFeatureOnDestroy, touchFeatureOnTransact);
    return clazz;
}

static bool setTouchFodEnabled(bool enabled) {
    ndk::SpAIBinder service(AServiceManager_checkService(TOUCH_FEATURE_SERVICE));
    if (service.get() == nullptr) {
        LOG(ERROR) << "TouchFeature service is unavailable";
        return false;
    }

    // AIBinder_prepareTransaction only writes the AIDL interface token after the
    // remote binder has been associated with its interface class.
    if (!AIBinder_associateClass(service.get(), touchFeatureClass())) {
        LOG(ERROR) << "Failed to associate the TouchFeature AIDL interface";
        return false;
    }

    AParcel* in = nullptr;
    AParcel* out = nullptr;
    binder_status_t status = AIBinder_prepareTransaction(service.get(), &in);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, TOUCH_ID_PRIMARY);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, TOUCH_MODE_FOD_ENABLE);
    if (status == STATUS_OK) status = AParcel_writeInt32(in, enabled ? 1 : 0);
    if (status == STATUS_OK) {
        status = AIBinder_transact(service.get(), TOUCH_FEATURE_SET_MODE_TRANSACTION, &in, &out, 0);
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
        // QFP waits for the physical ultrasonic IRQ itself. Injecting the
        // framework's touchscreen contact as a sensor-down event starts capture
        // before intr2 fires and consistently yields a zero-IQ sample.
    }

    void onFingerUp() {
        LOG(DEBUG) << __func__;
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_X, 0);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_Y, 0);
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

    bool shouldForwardAcquired(int32_t result, int32_t vendorCode) {
        // QFP uses vendor acquired 28 to report an unusable ultrasonic sample.
        // Forward it so enrollment can give feedback instead of appearing inert.
        return static_cast<AcquiredInfo>(result) == AcquiredInfo::VENDOR && vendorCode == 28;
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
