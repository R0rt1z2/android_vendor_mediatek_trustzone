#
# Copyright (C) 2015 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.panda), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

###
# keystore.trusty is the HAL used by keystore on Trusty devices.
###

include $(CLEAR_VARS)
ifneq (,$(filter 7.0 7.1.1 8.0 8.0.0 8.1.0,$(PLATFORM_VERSION)))
LOCAL_MODULE := keystore.v1.trustkernel
LOCAL_POST_INSTALL_CMD = \
	$(hide) ln -sf $(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))keystore.v1.$(TARGET_DEVICE).so ; \
	ln -sf $(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))keystore.v1.$(TARGET_BOARD_PLATFORM).so
else ifneq (,$(filter 6.0 6.1,$(PLATFORM_VERSION)))
LOCAL_MODULE := keystore.trustkernel
LOCAL_POST_INSTALL_CMD = \
	$(hide) ln -sf $(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))keystore.$(TARGET_DEVICE).so ; \
	ln -sf $(notdir $(LOCAL_INSTALLED_MODULE)) $(dir $(LOCAL_INSTALLED_MODULE))keystore.$(TARGET_BOARD_PLATFORM).so
else
$(error Android $(PLATFORM_VERSION) doesn\'t support Keymaster V1)
endif
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_OWNER := trustkernel

LOCAL_SRC_FILES := module.cpp
LOCAL_C_INCLUDES := system/security/keystore
LOCAL_CLFAGS = -fvisibility=hidden -Wall -Werror
LOCAL_SHARED_LIBRARIES := \
	libteec \
	libcrypto \
	liblog \
	libcutils \
	libkeystore_binder
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

# Symlink keystore.trusty.so -> keystore.<device>.so so libhardware can find it.

include $(BUILD_SHARED_LIBRARY)
