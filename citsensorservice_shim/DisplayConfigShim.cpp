/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "citsensorservice_shim"

#include <aidl/vendor/qti/hardware/display/config/BpDisplayConfig.h>
#include <android/binder_status.h>
#include <dlfcn.h>
#include <log/log.h>

using aidl::vendor::qti::hardware::display::config::Attributes;
using aidl::vendor::qti::hardware::display::config::BpDisplayConfig;
using aidl::vendor::qti::hardware::display::config::DisplayType;
using aidl::vendor::qti::hardware::display::config::IDisplayConfig;

namespace {

// The stock CIT sensor service caches a physical config index, while SDM exposes
// deduplicated client config IDs. Retry an index rejected by the composer with the
// active config, whose attributes are the current ones the service is after.
class ActiveConfigFallbackProxy : public BpDisplayConfig {
  public:
    using BpDisplayConfig::BpDisplayConfig;

    ndk::ScopedAStatus getDisplayAttributes(int32_t configIndex, DisplayType dpy,
                                            Attributes* attributes) override {
        ndk::ScopedAStatus status =
                BpDisplayConfig::getDisplayAttributes(configIndex, dpy, attributes);
        if (status.getExceptionCode() != EX_ILLEGAL_ARGUMENT) {
            return status;
        }

        int32_t activeConfig = -1;
        if (!getActiveConfig(dpy, &activeConfig).isOk() || activeConfig == configIndex) {
            return status;
        }

        ALOGW("Remapped invalid config %d to active config %d for display %d", configIndex,
              activeConfig, static_cast<int32_t>(dpy));
        return BpDisplayConfig::getDisplayAttributes(activeConfig, dpy, attributes);
    }
};

}  // namespace

// Interposes the NDK proxy factory used by the stock service. Local binders and
// failures keep the original result; remote services get the fallback proxy, which
// keeps the original binder so death notifications still work.
std::shared_ptr<IDisplayConfig> IDisplayConfig::fromBinder(const ndk::SpAIBinder& binder) {
    using FromBinder = std::shared_ptr<IDisplayConfig> (*)(const ndk::SpAIBinder&);
    static const auto realFromBinder = reinterpret_cast<FromBinder>(
            dlsym(RTLD_NEXT,
                  "_ZN4aidl6vendor3qti8hardware7display6config14IDisplayConfig10fromBinderERKN3n"
                  "dk10SpAIBinderE"));
    if (realFromBinder == nullptr) {
        ALOGE("Failed to resolve IDisplayConfig::fromBinder: %s", dlerror());
        return nullptr;
    }

    std::shared_ptr<IDisplayConfig> config = realFromBinder(binder);
    if (config == nullptr || !config->isRemote()) {
        return config;
    }
    return ndk::SharedRefBase::make<ActiveConfigFallbackProxy>(binder);
}
