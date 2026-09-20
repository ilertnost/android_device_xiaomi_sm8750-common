#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Call the proprietary setup
$(call inherit-product, vendor/xiaomi/sm8750-common/sm8750-common-vendor.mk)

# Enable virtual AB with vendor ramdisk
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota/vabc_features.mk)

# Enable project quotas and casefolding for emulated storage
$(call inherit-product, $(SRC_TARGET_DIR)/product/emulated_storage.mk)

# Setup dalvik vm configs
$(call inherit-product, frameworks/native/build/phone-xhdpi-6144-dalvik-heap.mk)

# Enforce generic ramdisk allow list
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_ramdisk.mk)

# Virtualization service
$(call inherit-product, packages/modules/Virtualization/apex/product_packages.mk)

# Add common definitions for Qualcomm
$(call inherit-product, hardware/qcom-caf/common/common.mk)

# SHIPPING API
PRODUCT_SHIPPING_API_LEVEL := 35

# AAPT
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xxhdpi

# A/B
AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_system=true \
    POSTINSTALL_PATH_system=system/bin/otapreopt_script \
    FILESYSTEM_TYPE_system=ext4 \
    POSTINSTALL_OPTIONAL_system=true

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_vendor=true \
    POSTINSTALL_PATH_vendor=bin/checkpoint_gc \
    FILESYSTEM_TYPE_vendor=ext4 \
    POSTINSTALL_OPTIONAL_vendor=true

PRODUCT_PACKAGES += \
    checkpoint_gc \
    otapreopt_script

PRODUCT_VIRTUAL_AB_COMPRESSION_METHOD := lz4

# ANT+
PRODUCT_PACKAGES += \
    com.dsi.ant@1.0.vendor

# Audio
PRODUCT_PACKAGES += \
    audio.primary.default \
    audio.r_submix.default \
    audio.usb.default \
    libaudiopreprocessing \
    libbundleaidl \
    libldnhncr \
    libdownmixaidl \
    libdynproc \
    libdynamicsprocessingaidl \
    libeffectproxy \
    libloudnessenhanceraidl \
    libreverbaidl \
    libreverbwrapper \
    libvisualizeraidl \
    libaudioutils_shim \
    libalsautilsv2.vendor \
    libtinyalsav2.vendor \
    libmediautils_vendor.vendor \
    libaudiohalvendorextn \
    qtiaudiohalvendorextn

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_ODM)/etc/audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml

# Bluetooth
PRODUCT_PACKAGES += \
    audio.bluetooth.default

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

# Boot control
PRODUCT_PACKAGES += \
    android.hardware.boot-service.qti \
    android.hardware.boot-service.qti.recovery

# Camera
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.concurrent.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.concurrent.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.camera.full.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.full.xml \
    frameworks/native/data/etc/android.hardware.camera.raw.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.raw.xml

# Camera - Aperture privapp permissions
PRODUCT_COPY_FILES += \
    device/xiaomi/sm8750-common/permissions/privapp-permissions-aperture.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/privapp-permissions-aperture.xml

# Phone state bridge (syncs Java audio mode to native AudioPolicyManager)
PRODUCT_COPY_FILES += \
    device/xiaomi/sm8750-common/audio/phone_state_bridge.sh:$(TARGET_COPY_OUT_VENDOR)/bin/phone_state_bridge.sh \
    device/xiaomi/sm8750-common/audio/phone_state_bridge.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/phone_state_bridge.rc

# DRM
PRODUCT_PACKAGES += \
    android.hardware.drm-service.clearkey

# Fastboot
PRODUCT_PACKAGES += \
    android.hardware.fastboot-service.example_recovery \
    fastbootd

# Fingerprint
PRODUCT_PACKAGES += \
    android.hardware.biometrics.fingerprint-service.xiaomi \
    libudfpshandler

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.fingerprint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.fingerprint.xml

# Face
# The stock AIDL IFace/default HAL is packaged from the HyperOS image, so the
# AOSP feature declaration stays in the vendor partition. crDroid's Sense based
# FaceUnlock and its system_ext copy of this file are disabled on dada through
# TARGET_FACE_UNLOCK_SUPPORTED.
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.biometrics.face.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.biometrics.face.xml

# Qualcomm Configuration and Connectivity
PRODUCT_PACKAGES += \
    QCC \
    qccsyshal_aidl-service \
    libqcc \
    libqcc_file_agent_sys \
    libqcc_netstats \
    libqccdme \
    libqccfileservice \
    vendor.qti.hardware.qccsyshal@1.0 \
    vendor.qti.hardware.qccsyshal@1.1 \
    vendor.qti.hardware.qccsyshal@1.2-halimpl \
    vendor.qti.hardware.qccsyshal@1.2 \
    vendor.qti.hardware.qccvndhal@1.0 \
    vendor.qti.qccsyshal_aidl-V1-ndk_system_ext \
    vendor.qti.qccsyshal_aidl-halimpl \
    vendor.qti.qccvndhal_aidl-V1-ndk_system_ext

# GPS
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml

# Display / Graphics (source-built from hardware/qcom-caf/sm8750/display)
PRODUCT_PACKAGES += \
    android.hardware.graphics.mapper@4.0-impl-qti-display \
    libgralloc.qti \
    vendor.qti.hardware.display.composer-service \
    vendor.qti.hardware.display.config-V12-ndk.vendor \
    vendor.qti.hardware.memtrack-service

ifneq ($(TARGET_USES_PREBUILT_DISPLAY_ALLOCATOR),true)
PRODUCT_PACKAGES += \
    vendor.qti.hardware.display.allocator-service
endif

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.opengles.aep.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.opengles.aep.xml \
    frameworks/native/data/etc/android.hardware.vulkan.compute-0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.compute-0.xml \
    frameworks/native/data/etc/android.hardware.vulkan.level-1.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.level-1.xml \
    frameworks/native/data/etc/android.hardware.vulkan.version-1_1.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.version-1_1.xml \
    frameworks/native/data/etc/android.hardware.vulkan.version-1_3.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.vulkan.version-1_3.xml \
    frameworks/native/data/etc/android.software.opengles.deqp.level-2023-03-01.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.opengles.deqp.level.xml \
    frameworks/native/data/etc/android.software.vulkan.deqp.level-2023-03-01.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.vulkan.deqp.level.xml

# Health
PRODUCT_PACKAGES += \
    android.hardware.health-service.qti \
    android.hardware.health-service.qti_recovery

# Hypsy
PRODUCT_PACKAGES += \
    xiaomi.system.hypsys.common-service

# IR Blaster
PRODUCT_PACKAGES += \
    android.hardware.ir-service.example

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.consumerir.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.consumerir.xml

# Keymint
PRODUCT_PACKAGES += \
    android.hardware.hardware_keystore.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.keystore.app_attest_key.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.keystore.app_attest_key.xml \
    frameworks/native/data/etc/android.software.device_id_attestation.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.device_id_attestation.xml

# Linker config
PRODUCT_VENDOR_LINKER_CONFIG_FRAGMENTS += \
    $(LOCAL_PATH)/configs/linker.config.json

# Lineage Health - charging control via mca_business_charger debug_ctrl
# Primary: soc_limit format "soc_limit <enable> <pct>" - needs smart_batt=1 to work
# Secondary: smart_batt enables/disables smart charge limit enforcement
$(call soong_config_set,lineage_health,charging_control_charging_path,/sys/devices/platform/soc/soc:mca_business_charger/debug_ctrl)
$(call soong_config_set,lineage_health,charging_control_charging_enabled,soc_limit 0 0)
$(call soong_config_set,lineage_health,charging_control_charging_disabled,soc_limit 1 15)
$(call soong_config_set,lineage_health,charging_control_charging_path2,/sys/devices/platform/soc/soc:smart_charge/smart_batt)
$(call soong_config_set,lineage_health,charging_control_charging_enabled2,0)
$(call soong_config_set,lineage_health,charging_control_charging_disabled2,1)
$(call soong_config_set_bool,lineage_health,charging_control_supports_toggle,true)
$(call soong_config_set_bool,lineage_health,charging_control_supports_bypass,false)
$(call soong_config_set_bool,lineage_health,charging_control_supports_limit,false)

PRODUCT_PACKAGES += \
    vendor.lineage.health-service.default

# PowerShare
$(call soong_config_set,lineage_powershare,powershare_path,/sys/class/qcom-battery/reverse_chg_mode)
$(call soong_config_set,lineage_powershare,powershare_enabled,1)
$(call soong_config_set,lineage_powershare,powershare_disabled,0)

PRODUCT_PACKAGES += \
    vendor.lineage.powershare-service.default

# Mountpoint
$(call soong_config_set,rfs,mpss_firmware_symlink_target,modem_firmware)

PRODUCT_PACKAGES += \
    product_vm-system_mountpoint \
    rfs_msm_mpss_readonly_mbnconfig_symlink \
    vendor_bt_firmware_mountpoint \
    vendor_dsp_mountpoint \
    vendor_firmware_mnt_mountpoint \
    vendor_modem_firmware_mountpoint \
    vendor_vm-system_mountpoint

# Network
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.ipsec_tunnels.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.ipsec_tunnels.xml

# NFC
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.nfc.ese.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.nfc.ese.xml \
    frameworks/native/data/etc/android.hardware.nfc.hcef.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.nfc.hcef.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.nfc.hce.xml \
    frameworks/native/data/etc/android.hardware.nfc.uicc.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.nfc.uicc.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/com.android.nfc_extras.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/com.android.nfc_extras.xml \
    frameworks/native/data/etc/com.nxp.mifare.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/com.nxp.mifare.xml

# Overlay
PRODUCT_PACKAGES += \
    CarrierConfigOverlayCommon \
    FrameworkResOverlayCommon \
    SettingsOverlayCommon \
    SystemUIOverlayCommon \
    TelephonyOverlayCommon \
    WifiOverlayCommon

# Partitions
PRODUCT_USE_DYNAMIC_PARTITIONS := true

# Power
PRODUCT_PACKAGES += \
    android.hardware.power-service-qti

PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/power/config/sun/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml

$(call soong_config_set,qtipower,mode_ext_lib,//$(LOCAL_PATH)/power:libpowermode-ext-xiaomi)

# QSPA
PRODUCT_PACKAGES += \
    qspa_vendor.rc \
    vendor.qti.qspa-service

# Rootdir
PRODUCT_PACKAGES += \
    fstab.qcom \
    init.recovery.qcom.rc \
    init.qcom.rc \
    init.target.rc \
    init.xiaomi.rc \
    init.haptics.rc \
    ueventd.qcom.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/etc/fstab.qcom:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.qcom \
    $(LOCAL_PATH)/rootdir/etc/fstab.qcom:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.default

# Sensors
PRODUCT_PACKAGES += \
    android.hardware.sensors-service.xiaomi-multihal

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.dynamic.head_tracker.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.dynamic.head_tracker.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.hifi_sensors.xml:$(TARGET_COPY_OUT_ODM)/etc/permissions/android.hardware.sensor.hifi_sensors.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.stepcounter.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.stepcounter.xml \
    frameworks/native/data/etc/android.hardware.sensor.stepdetector.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/sku_sun/android.hardware.sensor.stepdetector.xml

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH) \
    hardware/xiaomi \
    vendor/qcom/opensource/commonsys/audio/hal_adapter

# Vibrator - Xiaomi proprietary HAL (provides RichTap/CoolVibrator support).
# Its instance name is aligned with IVibrator/default during extraction.

# Dolby
PRODUCT_SOONG_NAMESPACES += hardware/dolby
PRODUCT_PACKAGES += LunarisDolby

# Vndservice manager
PRODUCT_PACKAGES += \
    vndservicemanager

# Telephony
PRODUCT_PACKAGES += \
    extphonelib \
    extphonelib-product \
    extphonelib.xml \
    extphonelib_product.xml \
    ims-ext-common \
    ims_ext_common.xml \
    qti-telephony-hidl-wrapper \
    qti-telephony-hidl-wrapper-prd \
    qti_telephony_hidl_wrapper.xml \
    qti_telephony_hidl_wrapper_prd.xml \
    qti-telephony-utils \
    qti-telephony-utils-prd \
    qti_telephony_utils.xml \
    qti_telephony_utils_prd.xml \
    telephony-ext \
    xiaomi-telephony-stub \
    imssettings \
    ImsDataChannelService

PRODUCT_BOOT_JARS += \
    telephony-ext \
    xiaomi-telephony-stub

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.cdma.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.cdma.xml \
    frameworks/native/data/etc/android.hardware.telephony.euicc.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/android.hardware.telephony.euicc.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.telephony.ims.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.ims.xml \
    frameworks/native/data/etc/android.hardware.telephony.mbms.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.mbms.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.sip.voip.xml

# Thermal
PRODUCT_PACKAGES += \
    android.hardware.thermal-service.qti

# Touchscreen
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml

# Update engine
PRODUCT_PACKAGES += \
    update_engine \
    update_engine_sideload \
    update_verifier

PRODUCT_PACKAGES_DEBUG += \
    update_engine_client

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb-service.qti \
    android.hardware.usb.gadget-service.qti \
    init.qcom.usb.rc \
    init.qcom.usb.sh \
    usb_compositions.conf

PRODUCT_SOONG_NAMESPACES += vendor/qcom/opensource/usb/etc

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml

# Verified boot
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.software.verified_boot.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.verified_boot.xml

# WiFi
PRODUCT_PACKAGES += \
    android.hardware.wifi-service \
    hostapd \
    hostapd_cli \
    libwifi-hal-qcom \
    libwpa_client \
    wpa_cli \
    wpa_supplicant \
    wpa_supplicant.conf \
    libpasn \
    libwifi-hal

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.wifi.aware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.aware.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.wifi.passpoint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.passpoint.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml

# WiFi firmware symlinks
PRODUCT_PACKAGES += \
    firmware_wlanmdsp.otaupdate_symlink \
    firmware_wlan_mac.bin_symlink \
    firmware_WCNSS_qcom_cfg.ini_symlink

# WiFi Display
PRODUCT_PACKAGES += \
    libwfdaac_vendor

# Override vendor_ramdisk bionic with ROM prebuilts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/vendor_ramdisk/system/bin/linker64:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/bin/linker64 \
    $(LOCAL_PATH)/vendor_ramdisk/system/bin/e2fsck:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/bin/e2fsck \
    $(LOCAL_PATH)/vendor_ramdisk/system/bin/fsck.f2fs:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/bin/fsck.f2fs \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libc.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libc.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/ld-android.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/ld-android.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libdl.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libdl.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libm.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libm.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libbase.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libbase.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/liblog.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/liblog.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libz.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libz.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libsparse.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libsparse.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2_blkid.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2_blkid.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2_com_err.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2_com_err.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2_e2p.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2_e2p.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2fs.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2fs.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2_quota.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2_quota.so \
    $(LOCAL_PATH)/vendor_ramdisk/system/lib64/libext2_uuid.so:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/system/lib64/libext2_uuid.so
PRODUCT_CHECK_PREBUILT_MAX_PAGE_SIZE := false

# Symlink for Xiaomi audio HAL compatibility
PRODUCT_PACKAGES += \

# Audio configs
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/../../../vendor/xiaomi/sm8750-common/proprietary/vendor/etc/audio/sku_sun/audio_effects.conf:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_sun/audio_effects.conf \
    $(LOCAL_PATH)/../../../vendor/xiaomi/sm8750-common/proprietary/vendor/etc/audio/sku_sun/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_sun/audio_effects.xml \
    $(LOCAL_PATH)/../../../vendor/xiaomi/sm8750-common/proprietary/vendor/etc/audio/sku_sun/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_sun/audio_policy_configuration.xml \
    $(LOCAL_PATH)/../../../vendor/xiaomi/sm8750-common/proprietary/vendor/etc/audio/sku_sun/mixer_paths_sun_mtp.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_sun/mixer_paths_sun_mtp.xml \
    $(LOCAL_PATH)/../../../vendor/xiaomi/sm8750-common/proprietary/vendor/etc/audio/sku_sun/resourcemanager_sun_mtp.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_sun/resourcemanager_sun_mtp.xml
