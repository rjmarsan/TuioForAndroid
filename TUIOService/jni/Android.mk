LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := testmodule 

LOCAL_SRC_FILES := test.c suinput.c


LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

include $(NDK_ROOT)/sources/cpufeatures/Android.mk
