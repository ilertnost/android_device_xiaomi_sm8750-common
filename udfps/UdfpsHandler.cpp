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
#include <android/binder_parcel_utils.h>
#include <android/binder_status.h>

#include <atomic>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <linux/input.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

#include "UdfpsHandler.h"

#define COMMAND_FOD_PRESS_STATUS 1
#define COMMAND_FOD_PRESS_X 2
#define COMMAND_FOD_PRESS_Y 3
#define COMMAND_DISPLAY_STATUS 14
#define COMMAND_FILM_MODE 15
#define PARAM_FOD_PRESSED 1
#define PARAM_FOD_RELEASED 0

#define TOUCH_FEATURE_SERVICE                                                  \
  "vendor.xiaomi.hw.touchfeature.ITouchFeature/default"
#define TOUCH_FEATURE_DESCRIPTOR "vendor.xiaomi.hw.touchfeature.ITouchFeature"
#define TOUCH_FEATURE_SET_MODE_TRANSACTION 9
#define TOUCH_FEATURE_GET_MODE_STRING_TRANSACTION 10
#define TOUCH_ID_PRIMARY 0
#define TOUCH_MODE_FOD_ENABLE 10
#define TOUCH_MODE_FOD_AUTH 41
#define TOUCH_MODE_FILM 1015

#define FOD_PRESS_STATUS_PATH "/sys/class/touch/touch_dev/fod_press_status"
#define TOUCH_RESOLUTION_FACTOR_PATH                                           \
  "/sys/class/touch/touch_dev/resolution_factor"
#define TOUCH_INPUT_NAME "Xiaomi_Touch_Input_0"
#define QCOM_US_INTR2_PATH "/sys/class/xiaomi_fp/xiaomi-fp/device/intr2"
#define DISPLAY_PRE_POWER_PATH                                                 \
  "/sys/bus/platform/devices/soc:xiaomi_fingerprint/fingerdown"

#define DISPLAY_STATUS_ON 2
#define FILM_MODE_OFF 0

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

namespace {

static void *touchFeatureOnCreate(void *args) { return args; }

static void touchFeatureOnDestroy(void *) {}

static binder_status_t touchFeatureOnTransact(AIBinder *, transaction_code_t,
                                              const AParcel *, AParcel *) {
  return STATUS_UNKNOWN_TRANSACTION;
}

static AIBinder_Class *touchFeatureClass() {
  static AIBinder_Class *clazz =
      AIBinder_Class_define(TOUCH_FEATURE_DESCRIPTOR, touchFeatureOnCreate,
                            touchFeatureOnDestroy, touchFeatureOnTransact);
  return clazz;
}

static bool setTouchMode(int32_t mode, int32_t value) {
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

  AParcel *in = nullptr;
  AParcel *out = nullptr;
  binder_status_t status = AIBinder_prepareTransaction(service.get(), &in);
  if (status == STATUS_OK)
    status = AParcel_writeInt32(in, TOUCH_ID_PRIMARY);
  if (status == STATUS_OK)
    status = AParcel_writeInt32(in, mode);
  if (status == STATUS_OK)
    status = AParcel_writeInt32(in, value);
  if (status == STATUS_OK) {
    status = AIBinder_transact(
        service.get(), TOUCH_FEATURE_SET_MODE_TRANSACTION, &in, &out, 0);
  }

  AStatus *callStatus = nullptr;
  int32_t result = -1;
  if (status == STATUS_OK)
    status = AParcel_readStatusHeader(out, &callStatus);
  if (status == STATUS_OK && callStatus != nullptr &&
      AStatus_isOk(callStatus)) {
    status = AParcel_readInt32(out, &result);
  }

  AStatus_delete(callStatus);
  AParcel_delete(out);
  AParcel_delete(in);

  if (status != STATUS_OK || result != 0) {
    LOG(ERROR) << "Failed to set touch mode " << mode << " to " << value
               << ": status=" << status << " result=" << result;
    return false;
  }

  LOG(INFO) << "Set touch mode " << mode << " to " << value;
  return true;
}

static int32_t getTouchFilmMode() {
  ndk::SpAIBinder service(
      AServiceManager_checkService(TOUCH_FEATURE_SERVICE));
  if (service.get() == nullptr ||
      !AIBinder_associateClass(service.get(), touchFeatureClass())) {
    return FILM_MODE_OFF;
  }

  AParcel *in = nullptr;
  AParcel *out = nullptr;
  binder_status_t status = AIBinder_prepareTransaction(service.get(), &in);
  if (status == STATUS_OK)
    status = AParcel_writeInt32(in, TOUCH_ID_PRIMARY);
  if (status == STATUS_OK)
    status = AParcel_writeInt32(in, TOUCH_MODE_FILM);
  if (status == STATUS_OK) {
    status = AIBinder_transact(service.get(),
                               TOUCH_FEATURE_GET_MODE_STRING_TRANSACTION, &in,
                               &out, 0);
  }

  AStatus *callStatus = nullptr;
  std::string value;
  if (status == STATUS_OK)
    status = AParcel_readStatusHeader(out, &callStatus);
  if (status == STATUS_OK && callStatus != nullptr &&
      AStatus_isOk(callStatus)) {
    status = AParcel_readString(out, &value, ndk::AParcel_stdStringAllocator);
  }

  AStatus_delete(callStatus);
  AParcel_delete(out);
  AParcel_delete(in);

  if (status != STATUS_OK) {
    LOG(WARNING) << "Failed to read touch film mode: status=" << status;
    return FILM_MODE_OFF;
  }
  if (value == "1")
    return 1;
  if (value == "2")
    return 2;
  return FILM_MODE_OFF;
}

static int openTouchInput() {
  DIR *inputDir = opendir("/dev/input");
  if (inputDir == nullptr) {
    PLOG(ERROR) << "Failed to open /dev/input";
    return -1;
  }

  int inputFd = -1;
  while (dirent *entry = readdir(inputDir)) {
    if (strncmp(entry->d_name, "event", 5) != 0)
      continue;

    std::string path = std::string("/dev/input/") + entry->d_name;
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0)
      continue;

    char name[128] = {};
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0 &&
        name == std::string(TOUCH_INPUT_NAME)) {
      LOG(INFO) << "Monitoring FOD coordinates from " << path;
      inputFd = fd;
      break;
    }
    close(fd);
  }
  closedir(inputDir);
  return inputFd;
}

static bool writeState(const char *path, bool enabled) {
  android::base::unique_fd fd(open(path, O_WRONLY | O_CLOEXEC));
  if (fd.get() < 0) {
    PLOG(ERROR) << "Failed to open " << path;
    return false;
  }

  const char value[] = {enabled ? '1' : '0', '\n'};
  if (TEMP_FAILURE_RETRY(write(fd.get(), value, sizeof(value))) !=
      sizeof(value)) {
    PLOG(ERROR) << "Failed to write " << path;
    return false;
  }
  return true;
}

} // anonymous namespace

class SM8750UdfpsHandler : public UdfpsHandler {
public:
  ~SM8750UdfpsHandler() override {
    mStop = true;
    if (mStopPipe[1] >= 0) {
      char value = 1;
      TEMP_FAILURE_RETRY(write(mStopPipe[1], &value, sizeof(value)));
    }
    if (mMonitorThread.joinable())
      mMonitorThread.join();
    if (mStopPipe[0] >= 0)
      close(mStopPipe[0]);
    if (mStopPipe[1] >= 0)
      close(mStopPipe[1]);
  }

  void init(fingerprint_device_t *device) {
    mDevice = device;

    std::ifstream factorFile(TOUCH_RESOLUTION_FACTOR_PATH);
    factorFile >> mResolutionFactor;
    if (mResolutionFactor <= 0)
      mResolutionFactor = 1;

    if (pipe2(mStopPipe, O_CLOEXEC | O_NONBLOCK) != 0) {
      PLOG(ERROR) << "Failed to create touch monitor stop pipe";
      return;
    }
    mMonitorThread = std::thread(&SM8750UdfpsHandler::monitorTouch, this);
  }

  void onFingerDown(uint32_t x, uint32_t y, float /*minor*/, float /*major*/) {
    LOG(DEBUG) << __func__ << "x: " << x << ", y: " << y;
    // Stock mfp-daemon does not trigger capture from the framework overlay.
    // Keep these coordinates only as a fallback if the touch input node is
    // absent.
    mFrameworkX = x;
    mFrameworkY = y;
    if (mInputUnavailable && mPhysicalPressed)
      maybeSendPress();
  }

  void onFingerUp() { LOG(DEBUG) << __func__; }

  void onUiReady() { LOG(DEBUG) << __func__; }

  void onEnrollStarted() {
    LOG(INFO) << __func__;
    startOperation();
  }

  void onAuthenticateStarted() {
    LOG(INFO) << __func__;
    startOperation();
  }

  void onAcquired(int32_t result, int32_t vendorCode) {
    LOG(DEBUG) << __func__ << " result: " << result
               << " vendorCode: " << vendorCode;
    if (static_cast<AcquiredInfo>(result) == AcquiredInfo::VENDOR &&
        (vendorCode == 201 || vendorCode == 202)) {
      // QFP has rejected this sample, but the physical contact is still down.
      // Do not synthesize pointer-up here: QFP uses the up transition to take
      // an air reference, which must happen only after the finger is gone.
      mContactConsumed = true;
    }
  }

  void onEnrollmentProgress(uint32_t remaining) {
    if (remaining == 0) {
      stopOperation();
    } else if (mOperationActive) {
      setTouchMode(TOUCH_MODE_FOD_AUTH, 1);
    }
  }

  void onError() { stopOperation(); }

  bool shouldForwardAcquired(int32_t result, int32_t vendorCode) {
    // QFP uses vendor acquired 28 to report an unusable ultrasonic sample.
    // Forward it so enrollment can give feedback instead of appearing inert.
    return static_cast<AcquiredInfo>(result) == AcquiredInfo::VENDOR &&
           vendorCode == 28;
  }

  void cancel() {
    LOG(INFO) << __func__;
    stopOperation();
  }

  void onAuthenticationSucceeded() { stopOperation(); }

  void onAuthenticationFailed() {
    if (mOperationActive)
      setTouchMode(TOUCH_MODE_FOD_AUTH, 1);
  }

private:
  void startOperation() {
    mTouchX = 0;
    mTouchY = 0;
    mCaptureSent = false;
    mContactConsumed = false;
    mPhysicalPressed = false;
    mOperationActive = true;
    mDevice->extCmd(mDevice, COMMAND_DISPLAY_STATUS, DISPLAY_STATUS_ON);
    setTouchMode(TOUCH_MODE_FOD_ENABLE, 1);
    setTouchMode(TOUCH_MODE_FOD_AUTH, 1);
  }

  void stopOperation() {
    if (!mOperationActive.exchange(false))
      return;
    sendRelease();
    setTouchMode(TOUCH_MODE_FOD_AUTH, 0);
    setTouchMode(TOUCH_MODE_FOD_ENABLE, 0);
  }

  void sendRelease() {
    if (mCaptureSent.exchange(false)) {
      mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_X, 0);
      mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_Y, 0);
      mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_RELEASED);
      LOG(INFO) << "Forwarded physical FOD release";
    }
  }

  void maybeSendPress() {
    if (!mOperationActive || !mPhysicalPressed || mCaptureSent ||
        mContactConsumed)
      return;

    int32_t x = mTouchX;
    int32_t y = mTouchY;
    if ((x <= 0 || y <= 0) && mInputUnavailable) {
      x = mFrameworkX;
      y = mFrameworkY;
    }
    if (x <= 0 || y <= 0)
      return;

    mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_X, x);
    mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_Y, y);
    mDevice->extCmd(mDevice, COMMAND_FILM_MODE, getTouchFilmMode());
    mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_PRESSED);
    mCaptureSent = true;
    LOG(INFO) << "Forwarded physical FOD press at " << x << "," << y;
  }

  void handlePressState(bool pressed) {
    if (pressed == mPhysicalPressed.exchange(pressed))
      return;

    if (pressed) {
      // Stock mfp-daemon arms both qcom_us pressure sensing and display
      // pre-power before it forwards the physical touch to QFP.
      writeState(QCOM_US_INTR2_PATH, true);
      writeState(DISPLAY_PRE_POWER_PATH, true);
      if (mOperationActive)
        setTouchMode(TOUCH_MODE_FOD_AUTH, 0);
      // Stock posts its QFP down notification asynchronously after changing
      // the touch-panel auth state. Our direct extCmd path is synchronous, so
      // arm the panel before starting capture.
      maybeSendPress();
    } else {
      writeState(QCOM_US_INTR2_PATH, false);
      writeState(DISPLAY_PRE_POWER_PATH, false);
      sendRelease();
      mTouchX = 0;
      mTouchY = 0;
      mContactConsumed = false;
      if (mOperationActive)
        setTouchMode(TOUCH_MODE_FOD_AUTH, 1);
    }
  }

  void handleInputEvents(int inputFd) {
    input_event events[16];
    ssize_t bytes;
    while ((bytes = TEMP_FAILURE_RETRY(read(inputFd, events, sizeof(events)))) >
           0) {
      size_t count = static_cast<size_t>(bytes) / sizeof(input_event);
      for (size_t i = 0; i < count; i++) {
        const auto &event = events[i];
        if (event.type != EV_ABS || event.value <= 0)
          continue;
        if (event.code == ABS_MT_POSITION_X) {
          mTouchX = event.value / mResolutionFactor;
        } else if (event.code == ABS_MT_POSITION_Y) {
          mTouchY = event.value / mResolutionFactor;
        }
      }
    }
    maybeSendPress();
  }

  void monitorTouch() {
    int statusFd =
        open(FOD_PRESS_STATUS_PATH, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (statusFd < 0) {
      PLOG(ERROR) << "Failed to open " << FOD_PRESS_STATUS_PATH;
      return;
    }

    int inputFd = openTouchInput();
    mInputUnavailable = inputFd < 0;
    if (mInputUnavailable)
      LOG(WARNING) << "Touch input unavailable; using overlay coordinates";

    char initialState = '0';
    TEMP_FAILURE_RETRY(pread(statusFd, &initialState, sizeof(initialState), 0));

    pollfd fds[] = {
        {.fd = mStopPipe[0], .events = POLLIN},
        {.fd = statusFd, .events = POLLERR | POLLPRI},
        {.fd = inputFd, .events = POLLIN},
    };

    while (!mStop) {
      int count = TEMP_FAILURE_RETRY(poll(fds, 3, -1));
      if (count <= 0)
        continue;
      if (fds[0].revents & POLLIN) {
        char value;
        TEMP_FAILURE_RETRY(read(mStopPipe[0], &value, sizeof(value)));
        break;
      }

      if (inputFd >= 0 && (fds[2].revents & POLLIN))
        handleInputEvents(inputFd);
      if (fds[1].revents & (POLLERR | POLLPRI)) {
        char state = '0';
        if (TEMP_FAILURE_RETRY(pread(statusFd, &state, sizeof(state), 0)) ==
            1) {
          handlePressState(state == '1');
        }
      }
    }

    if (inputFd >= 0)
      close(inputFd);
    close(statusFd);
  }

  fingerprint_device_t *mDevice = nullptr;
  std::thread mMonitorThread;
  int mStopPipe[2] = {-1, -1};
  int32_t mResolutionFactor = 1;
  std::atomic_bool mStop = false;
  std::atomic_bool mOperationActive = false;
  std::atomic_bool mPhysicalPressed = false;
  std::atomic_bool mCaptureSent = false;
  std::atomic_bool mContactConsumed = false;
  std::atomic_bool mInputUnavailable = true;
  std::atomic<int32_t> mTouchX = 0;
  std::atomic<int32_t> mTouchY = 0;
  std::atomic<int32_t> mFrameworkX = 0;
  std::atomic<int32_t> mFrameworkY = 0;
};

static UdfpsHandler *create() { return new SM8750UdfpsHandler(); }

static void destroy(UdfpsHandler *handler) { delete handler; }

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
    .create = create,
    .destroy = destroy,
};
