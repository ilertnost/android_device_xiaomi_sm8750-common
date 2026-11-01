/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "BTAudioProviderShim"

#include <dlfcn.h>
#include <log/log.h>

#include <cerrno>
#include <cstdint>
#include <mutex>

namespace {

// binder_status_t is an int32_t. Keep the bridge independent from the AIDL
// version used by either proprietary side of this call.
using CreateProviderFactory = int32_t (*)();

constexpr const char *kQtiProviderLibrary =
    "android.hardware.bluetooth.audio-impl-qti.so";
constexpr const char *kQtiProviderPath =
    "/vendor/lib64/hw/android.hardware.bluetooth.audio-impl-qti.so";
constexpr const char *kQtiCreateSymbol =
    "_Z36createIBluetoothAudioProviderFactoryv";

void *gProviderHandle = nullptr;
CreateProviderFactory gCreateProviderFactory = nullptr;
std::once_flag gLoadProviderOnce;

void loadQtiProvider() {
  gProviderHandle = dlopen(kQtiProviderLibrary, RTLD_NOW | RTLD_LOCAL);
  if (gProviderHandle == nullptr) {
    gProviderHandle = dlopen(kQtiProviderPath, RTLD_NOW | RTLD_LOCAL);
  }
  if (gProviderHandle == nullptr) {
    ALOGE("Failed to load QTI Bluetooth audio provider: %s", dlerror());
    return;
  }

  gCreateProviderFactory = reinterpret_cast<CreateProviderFactory>(
      dlsym(gProviderHandle, kQtiCreateSymbol));
  if (gCreateProviderFactory == nullptr) {
    ALOGE("Failed to find QTI provider factory: %s", dlerror());
  }
}

} // namespace

extern "C" __attribute__((visibility("default"))) int32_t
createIBluetoothAudioProviderFactory() {
  std::call_once(gLoadProviderOnce, loadQtiProvider);
  if (gCreateProviderFactory == nullptr) {
    return -ENOENT;
  }
  return gCreateProviderFactory();
}
