#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import extract_utils.tools
from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/sm8750-common',
    'hardware/qcom-caf/wlan',
    'hardware/qcom-caf/sm8750',
    'hardware/xiaomi',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
]

def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}-{partition}' if partition == 'vendor' else None

def lib_fixup_qcc_partition(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'system_ext' else None

lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'libc++_shared',
        'libmialgo',
        'libaudioserviceexampleimpl',
    ): lib_fixup_remove,
    (
        'vendor.qti.qccsyshal_aidl-V1-ndk',
        'vendor.qti.qccvndhal_aidl-V1-ndk',
    ): lib_fixup_qcc_partition,
}

blob_fixups: blob_fixups_user_type = {
    # AOSP does not ship Xiaomi's ODM audio module configuration. Carry its
    # four-microphone input layout into the extracted vendor configuration so
    # OZO spatial recording can open a genuine four-channel capture stream.
    'vendor/etc/audio/audio_module_config_primary.xml': blob_fixup()
        .regex_replace(
            r'(mixPort name="primary_in"[^>]*>\s*<profile[^>]*channelLayouts=")LAYOUT_MONO LAYOUT_STEREO LAYOUT_FRONT_BACK("[^>]*/>)',
            r'\1LAYOUT_MONO LAYOUT_STEREO LAYOUT_FRONT_BACK LAYOUT_2POINT0POINT2\2',
        ),
    # The framework consumes IVibrator/default. Xiaomi's stock service uses
    # the undeclared "vibratorfeature" instance even though it implements the
    # standard IVibrator interface, so align its registration with our VINTF
    # manifest while preserving the complete proprietary haptics HAL.
    'odm/bin/hw/vendor.xiaomi.hardware.vibratorfeature.service': blob_fixup()
        .binary_regex_replace(
            b'/vibratorfeature',
            b'/default\x00\x00\x00\x00\x00\x00\x00\x00',
        ),
    'odm/etc/init/vendor.xiaomi.hw.touchfeature-service.rc': blob_fixup()
        .regex_replace(
            r'(service touch-kmsg-init-sh .*\n    class main\n)',
            r'\1    user system\n'
        ),
    'vendor/etc/init/audiohalservice_qti.rc': blob_fixup()
        .regex_replace(
            r'(service set_diag_state /vendor/bin/set_diag_state\n)',
            r'\1    user system\n    group system\n'
        ),
    (
        'vendor/lib64/libwfdmmsrc_proprietary.so',
    ): blob_fixup()
        .replace_needed(
            'android.media.audio.common.types-V2-ndk.so', 
            'android.media.audio.common.types-V3-ndk.so'
        ),
     (
       'vendor/lib64/libsxrservice.so'
     ): blob_fixup()
        .replace_needed(
            'android.hardware.common-V2-ndk_platform.so',
            'android.hardware.common-V2-ndk.so'
        ),
    (
        'vendor/lib64/libqti-perfd.so',
    ): blob_fixup()
        .replace_needed('vendor.qti.hardware.display.config-V5-ndk.so', 'vendor.qti.hardware.display.config-V12-ndk.so'),
    (
        'system_ext/lib64/vendor.qti.hardware.qccsyshal@1.2-halimpl.so',
    ): blob_fixup()
        .replace_needed('libprotobuf-cpp-full.so', 'libprotobuf-cpp-qccx.so'),
    (
        'system_ext/lib64/libprotobuf-cpp-qccx.so',
    ): blob_fixup()
        .fix_soname(),
    (
        'vendor/lib64/libapengine.so',
    ): blob_fixup()
        .replace_needed('vendor.qti.hardware.display.config-V5-ndk.so', 'vendor.qti.hardware.display.config-V12-ndk.so')
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v34.so'
        ),
     (
       'odm/bin/hw/vendor.xiaomi.hw.touchfeature-service',
       'odm/lib64/hw/displayfeature.default.so',
       'odm/lib64/libadaptivehdr.so',
       'odm/lib64/libcolortempmode.so',
       'odm/lib64/libdither.so',
       'odm/lib64/libflatmode.so',
       'odm/lib64/libhistprocess.so',
       'odm/lib64/libmiBrightness.so',
       'odm/lib64/libmiSensorCtrl.so',
       'odm/lib64/libpaperMode.so',
       'odm/lib64/librhytheyecare.so',
       'odm/lib64/libsdr2hdr.so',
       'odm/lib64/libsre.so',
       'odm/lib64/libtruetone.so',
       'odm/lib64/libvideomode.so',
       'vendor/lib64/libgnss.so',
     ): blob_fixup()
        .replace_needed(
            'android.hardware.sensors-V2-ndk.so',
            'android.hardware.sensors-V3-ndk.so'
        ),
     (
       'odm/lib64/hw/displayfeature.default.so',
     ): blob_fixup()
        .replace_needed(
            'android.hardware.sensors-V2-ndk.so',
            'android.hardware.sensors-V3-ndk.so'
        )
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v34.so'
        ),
    (
       'vendor/bin/hw/vendor.qti.hardware.display.composer-service',
    ): blob_fixup()
        .replace_needed(
            'vendor.qti.hardware.display.composer3-V1-ndk.so',
            'vendor.qti.hardware.display.composer3-V3-ndk.so'
        )
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v34.so'
        ),
    (
       'vendor/lib64/libqcodec2_core.so',
    ): blob_fixup()
        .replace_needed(
            'android.hardware.graphics.common-V5-ndk.so',
            'android.hardware.graphics.common-V7-ndk.so'
        )
        .add_needed('libcodec2_shim.so'),
     (
       'odm/bin/hw/vendor.xiaomi.sensor.citsensorservice.aidl',
     ): blob_fixup()
        .replace_needed(
            'android.hardware.graphics.common-V5-ndk.so',
            'android.hardware.graphics.common-V7-ndk.so'
        )
        .replace_needed(
            'android.hardware.sensors-V2-ndk.so',
            'android.hardware.sensors-V3-ndk.so'
        )
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v34.so'
        ),
     (
       'odm/lib64/libmiXmlParser.so',
       'vendor/bin/hw/audiohalservice.qti',
       'vendor/bin/poweropt-service',
       'vendor/lib64/libaodoptfeature.so',
       'vendor/lib64/libaudiocloudctrl.so',
       'vendor/lib64/libcamerapoweroptfeature.so',
       'vendor/lib64/libgamepoweroptfeature.so',
       'vendor/lib64/liblearningmodule.so',
       'vendor/lib64/liboffscreenpoweroptfeature.so',
       'vendor/lib64/libpowercallback.so',
       'vendor/lib64/libpowercore.so',
       'vendor/lib64/libpsmoptfeature.so',
       'vendor/lib64/libsdmclient.so',
       'vendor/lib64/libstandbyfeature.so',
       'vendor/lib64/libvideooptfeature.so',
     ): blob_fixup()
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v34.so'
        ),
    (
        'vendor/lib64/libVoiceSdk.so',
        'vendor/lib64/libcapiv2uvvendor.so',
        'vendor/lib64/liblistensoundmodel2vendor.so',
    ): blob_fixup()
        .replace_needed('libtensorflowlite_c.so', 'libtensorflowlite_c_vendor.so'),
    'vendor/lib64/libtensorflowlite_c_vendor.so': blob_fixup()
        .fix_soname(),
}  # fmt: skip

module = ExtractUtilsModule(
    'sm8750-common',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    check_elf=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
