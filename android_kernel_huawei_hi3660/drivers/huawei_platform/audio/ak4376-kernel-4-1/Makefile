# ak4376 driver
ccflags-$(CONFIG_AK4376_KERNEL_4_1) += -Idrivers/devkit/audiokit/
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DAK4376_KERNEL_4_1_DEBUG
endif
obj-$(CONFIG_AK4376_KERNEL_4_1)	+= ak4376.o
