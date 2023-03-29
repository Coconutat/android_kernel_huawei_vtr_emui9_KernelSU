# Makefile for the smartpa devkit drivers.
#

# Each configuration option enables a list of files.
ifeq ($(TARGET_BUILD_VARIANT),eng)
	EXTRA_CFLAGS += -DSMARTPAKIT_MISC_RW_PERMISSION_ENABLE
endif
ifdef CONFIG_HUAWEI_SMARTPAKIT_AUDIO
	ccflags-y += -Idrivers/devkit/audiokit/
endif
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO) += smartpakit.o

smartpakit_i2c-objs := smartpakit_i2c_ops.o
ifneq ($(TARGET_BUILD_VARIANT),user)
	smartpakit_i2c-objs += smartpakit_info.o
endif
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO) += smartpakit_i2c.o
