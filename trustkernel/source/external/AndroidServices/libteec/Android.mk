LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libteec
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_SRC_FILES_arm := ../../../client/libs/armeabi-v7a/libteec.so
LOCAL_SRC_FILES_arm64 := ../../../client/libs/arm64-v8a/libteec.so

LOCAL_MODULE_SUFFIX := .so
LOCAL_MULTILIB := both

LOCAL_EXPORT_C_INCLUDE_DIRS := \
		$(LOCAL_PATH)/include

include $(BUILD_PREBUILT)
