LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CLANG := true

LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/lib/egl
LOCAL_MODULE := libEGL_swiftshader

LOCAL_SRC_FILES += \
	../common/Object.cpp \
	../common/debug.cpp \
	Config.cpp \
	Display.cpp \
	Surface.cpp \
	libEGL.cpp \
	main.cpp

LOCAL_CFLAGS += -DLOG_TAG=\"libEGL_swiftshader\"

# Android's make system also uses NDEBUG, so we need to set/unset it forcefully
# Uncomment for ON:
LOCAL_CFLAGS += -UNDEBUG -g
# Uncomment for OFF:
#LOCAL_CFLAGS += -DANGLE_DISABLE_TRACE

LOCAL_CFLAGS += -std=c++11
LOCAL_CFLAGS += -fvisibility=protected

# These changes tie the build to Cloud Android. Do something else
# for other Android builds.
LOCAL_STATIC_LIBRARIES += libgceframebufferconfig libgcemetadata
LOCAL_C_INCLUDES += device/google/gce/include

LOCAL_SHARED_LIBRARIES += libdl liblog libandroid libutils libcutils $(GCE_STLPORT_LIBS)
LOCAL_LDFLAGS += -Wl,--version-script=$(LOCAL_PATH)/exports.map -Wl,--hash-style=sysv

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../../

include external/stlport/libstlport.mk

include $(BUILD_SHARED_LIBRARY)
