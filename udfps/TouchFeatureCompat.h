/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android/binder_interface_utils.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace aidl::vendor::xiaomi::hw::touchfeature {

class ITouchFeatureCallback;

// Client declaration matching Xiaomi's stable V1 TouchFeature AIDL ABI.
// The generated headers are not shipped with the stock interface library.
class ITouchFeature : public ::ndk::ICInterface {
public:
  static const char *descriptor;
  ITouchFeature();
  virtual ~ITouchFeature();

  static std::shared_ptr<ITouchFeature>
  fromBinder(const ::ndk::SpAIBinder &binder);

  virtual ::ndk::ScopedAStatus getModeCurValue(int32_t touchId, int32_t mode,
                                               int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  getModeDefaultValue(int32_t touchId, int32_t mode, int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus getModeMaxValue(int32_t touchId, int32_t mode,
                                               int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus getModeMinValue(int32_t touchId, int32_t mode,
                                               int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  getModeValue(int32_t touchId, int32_t mode,
               std::vector<int32_t> *result) = 0;
  virtual ::ndk::ScopedAStatus getTouchEvent(std::string *result) = 0;
  virtual ::ndk::ScopedAStatus modeReset(int32_t touchId, int32_t mode,
                                         int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  setModeLongValue(int32_t touchId, int32_t mode, int32_t value,
                   const std::vector<int32_t> &data, int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus setModeValue(int32_t touchId, int32_t mode,
                                            int32_t value,
                                            int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  getModeCurValueString(int32_t touchId, int32_t mode,
                        std::string *result) = 0;
  virtual ::ndk::ScopedAStatus
  getModeWhiteList(int32_t touchId, const std::vector<int32_t> &modes,
                   std::string *result) = 0;
  virtual ::ndk::ScopedAStatus
  registerCallback(int32_t touchId,
                   const std::shared_ptr<ITouchFeatureCallback> &callback,
                   const std::string &clientName, int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  unregisterCallback(int32_t touchId,
                     const std::shared_ptr<ITouchFeatureCallback> &callback,
                     const std::string &clientName, int32_t *result) = 0;
  virtual ::ndk::ScopedAStatus
  setModePackageName(int32_t touchId, int32_t mode,
                     const std::string &packageName, std::string *result) = 0;
  virtual ::ndk::ScopedAStatus getInterfaceVersion(int32_t *version) = 0;
  virtual ::ndk::ScopedAStatus getInterfaceHash(std::string *hash) = 0;
};

} // namespace aidl::vendor::xiaomi::hw::touchfeature
