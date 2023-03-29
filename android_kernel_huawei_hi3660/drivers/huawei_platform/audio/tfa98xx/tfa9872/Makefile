# tfa9872 smartPA driver

ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DNXP_FACTORY_MODE
endif
ccflags-$(CONFIG_TFA9872) += -Idrivers/devkit/audiokit/
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DTFA9872_DEBUG
endif
obj-$(CONFIG_TFA9872)	+= tfa9872.o
