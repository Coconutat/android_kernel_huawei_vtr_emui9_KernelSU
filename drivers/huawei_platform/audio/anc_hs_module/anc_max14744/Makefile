ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DANC_MAX14744_DEBUG
endif
obj-$(CONFIG_ANC_MAX14744)	:= anc_max14744.o
ccflags-$(CONFIG_ANC_MAX14744) += -Idrivers/devkit/audiokit/
