# Makefile for the Headset Debug Audio drivers.
#
# Each configuration option enables a list of files.

headset-debug-objs :=headset_debug.o headset_debug_input.o
ifneq ($(strip $(CONFIG_SWITCH)),)
	EXTRA_CFLAGS += -DCONFIG_HUAWEI_HEADSET_DEBUG_SWITCH
endif
ifneq ($(TARGET_BUILD_VARIANT),user)
	EXTRA_CFLAGS += -DCONFIG_HUAWEI_HEADSET_DEBUG
	obj-y  += headset-debug.o
endif
