/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "paths.h"

using misys::resolve;
using Comps = std::vector<std::string>;

// Calls the app really makes (MiuiCamera Ro.b callers).
TEST(MiSysPaths, AllowsAppFiles) {
    Comps c;
    EXPECT_TRUE(resolve("/data/vendor/camera/", "Test.txt", &c));
    EXPECT_EQ(c, (Comps{"camera", "Test.txt"}));
    EXPECT_TRUE(resolve("/data/vendor/camera/", "lut_normal_blackice.png", &c));
    EXPECT_TRUE(resolve("/data/vendor/camera/", "123.cube", &c));
    EXPECT_TRUE(resolve("/data/vendor/camera/", "MiCameraSDKXMS", &c));
    EXPECT_TRUE(resolve("/data/vendor/camera//fonts", "fonts_copied.txt", &c));
    EXPECT_EQ(c, (Comps{"camera", "fonts", "fonts_copied.txt"}));
    EXPECT_TRUE(resolve("/data/vendor/camera/watermarks/leica/1", "bg.webp", &c));
    EXPECT_TRUE(resolve("/data/vendor/camera", "video_watermarks", &c));
    EXPECT_TRUE(resolve("/data/vendor", "camera", &c));
    EXPECT_EQ(c, (Comps{"camera"}));
}

TEST(MiSysPaths, RejectsEverythingElse) {
    Comps c;
    EXPECT_FALSE(resolve("/data/vendor/camera", "com.xiaomi.dcal.wt.data", &c));  // HAL calibration copy
    EXPECT_FALSE(resolve("/data/vendor/camera/offlinelog", "x.png", &c));
    EXPECT_FALSE(resolve("/mnt/vendor/persist/camera/", "x.png", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera/watermarks/..", "x.png", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera", "../camera/x.png", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera/watermarks", "../../../../system/x", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera/./fonts", "a.ttf", &c));
    EXPECT_FALSE(resolve("data/vendor/camera", "x.png", &c));
    EXPECT_FALSE(resolve("", "x.png", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera", std::string("x.png\0", 6), &c));
    EXPECT_FALSE(resolve("/data/vendor/camera", ".png", &c));
    EXPECT_FALSE(resolve("/data/vendor", "cameraX", &c));
    EXPECT_FALSE(resolve("/data/vendor/camera/fonts/1/2/3/4/5/6/7", "8", &c));  // deeper than kMaxDepth
}
