# Hisilicon IPP Makefile

obj-$(CONFIG_HISPIPP_V210) += adapter_common.o
obj-$(CONFIG_HISPIPP_V210) += adapter_slam.o
obj-$(CONFIG_HISPIPP_V210) += adapter_slamtest.o
obj-$(CONFIG_HISPIPP_V210) += ipp_core.o
obj-$(CONFIG_HISPIPP_V210) += ipp_com.o
obj-$(CONFIG_HISPIPP_V210) += smc.o
obj-$(CONFIG_HISPIPP_V210) += memory.o

obj-$(CONFIG_HISPIPP_V210) += v210/
obj-$(CONFIG_HISPIPP_V210) += device/

subdir-ccflags-$(CONFIG_HISPIPP_V210) += -Wall -Werror
subdir-ccflags-$(CONFIG_HISPIPP_V210) += -I$(srctree)/drivers/hisi/ipp/v210 \
                                    -I$(srctree)/drivers/hisi/ipp/v210/device \
                                    -I$(srctree)/drivers/hisi/ipp/v210/v210 \
                                    -I$(srctree)/drivers/hisi/ipp/v210/include \
                                    -I$(srctree)/drivers/hisi/ipp/v210/v210/include \
                                    -I$(srctree)/drivers/hisi/ipp/v210/device/include \
                                    -I$(srctree)/drivers/hisi/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V210) += -DCONFIG_IPP_DEBUG
endif

