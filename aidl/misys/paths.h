/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

namespace misys {

// The app passes absolute (dir, name) pairs; they resolve below kBase and must start with "camera"
// (/data/vendor/camera). Header-only so the host test checks the exact code the service runs.
constexpr char kBase[] = "/data/vendor";
constexpr size_t kMaxDepth = 8;  // components below /data/vendor/camera

// Trees the app manages as a whole: watermark assets (photo + video) and fonts for the HAL renderer.
inline bool isAppTree(const std::string& n) {
    return n == "watermarks" || n == "video_watermarks" || n == "fonts";
}

// Files the app writes straight into /data/vendor/camera: filter LUTs (.png/.cube), AI/device watermarks
// (.png/.webp), MiVi SDK state and the testPermission probe. HAL files there (calibration copies such as
// com.xiaomi.dcal.*.data, dumps, logs) do not match.
inline bool isRootFile(const std::string& n) {
    if (n == "Test.txt" || n == "MiCameraSDKCloud" || n == "MiCameraSDKXMS") return true;
    for (const std::string ext : {".png", ".webp", ".cube"}) {
        if (n.size() > ext.size() && n.compare(n.size() - ext.size(), ext.size(), ext) == 0) return true;
    }
    return false;
}

// Splits dir + "/" + name into components below kBase. Tolerates "//" and trailing slashes (the app sends
// both); rejects relative paths, "." and "..", NUL bytes and anything off the allowlist.
// out->size() == 1 means /data/vendor/camera itself.
inline bool resolve(const std::string& dir, const std::string& name, std::vector<std::string>* out) {
    const std::string full = dir + "/" + name;
    if (full[0] != '/' || full.find('\0') != std::string::npos) return false;
    std::vector<std::string> parts;
    for (size_t pos = 0, end; pos < full.size(); pos = end + 1) {
        end = full.find('/', pos);
        if (end == std::string::npos) end = full.size();
        std::string part = full.substr(pos, end - pos);
        if (part == "." || part == "..") return false;
        if (!part.empty()) parts.push_back(std::move(part));
    }
    if (parts.size() < 3 || parts[0] != "data" || parts[1] != "vendor" || parts[2] != "camera") return false;
    out->assign(parts.begin() + 2, parts.end());
    const size_t depth = out->size() - 1;
    if (depth == 0) return true;
    if (depth > kMaxDepth) return false;
    return isAppTree((*out)[1]) || (depth == 1 && isRootFile((*out)[1]));
}

}  // namespace misys
