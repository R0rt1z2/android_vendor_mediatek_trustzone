LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libpl
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES_arm := ../../../ca/libs/armeabi-v7a/libpl.so
LOCAL_SRC_FILES_arm64 := ../../../ca/libs/arm64-v8a/libpl.so


LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := both

LOCAL_EXPORT_C_INCLUDE_DIRS := \
		$(LOCAL_PATH)/include

include $(BUILD_PREBUILT)
