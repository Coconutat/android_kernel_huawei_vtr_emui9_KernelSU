# Hisilicon IPP Makefile

obj-$(CONFIG_HISPIPP_V200) += adapter_common.o
obj-$(CONFIG_HISPIPP_V200) += adapter_mcf.o
obj-$(CONFIG_HISPIPP_V200) += adapter_mfnr.o
obj-$(CONFIG_HISPIPP_V200) += adapter_slam.o
obj-$(CONFIG_HISPIPP_V200) += adapter_slamtest.o
obj-$(CONFIG_HISPIPP_V200) += ipp-core.o
obj-$(CONFIG_HISPIPP_V200) += ipp-com.o
obj-$(CONFIG_HISPIPP_V200) += smc.o
obj-$(CONFIG_HISPIPP_V200) += memory.o

obj-$(CONFIG_HISPIPP_V200) += v200/
obj-$(CONFIG_HISPIPP_V200) += device/


subdir-ccflags-$(CONFIG_HISPIPP_V200) += -Wall -Werror
subdir-ccflags-$(CONFIG_HISPIPP_V200) += -I$(srctree)/drivers/hisi/ippv200 \
                                    -I$(srctree)/drivers/hisi/ipp/v200/v200 \
                                    -I$(srctree)/drivers/hisi/ipp/v200/device \
                                    -I$(srctree)/drivers/hisi/ipp/v200/include \
                                    -I$(srctree)/drivers/hisi/ipp/v200/v200/include \
                                    -I$(srctree)/drivers/hisi/ipp/v200/device/include \
                                    -I$(srctree)/drivers/hisi/tzdriver
ifneq ($(TARGET_BUILD_VARIANT),user)
subdir-ccflags-$(CONFIG_HISPIPP_V200) += -DCONFIG_IPP_DEBUG
endif
