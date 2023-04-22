#ifneq ($(TARGET_BUILD_VARIANT),user)
#ifneq ($(kcov),true)
ifeq ($(CONFIG_HISI_ISP_DEBUG), y)
subdir-ccflags-y += -DDEBUG_HISI_ISP
#endif
endif

ifeq ($(es_low_freq),true)
subdir-ccflags-y += -DCONFIG_ES_ISP_LOW_FREQ
endif

ifneq ($(chip_type),es)
subdir-ccflags-y += -DCONFIG_CS_ISP
endif

obj-$(CONFIG_HISI_ISP_RDR) += mntn/

obj-$(CONFIG_HISI_HISTAR_ISP) += hisp_rpm.o nonsec.o sec.o \
                                cma.o smc.o  qos.o \
                                sensor.o isp.o

ifeq ($(CONFIG_HISI_HISTAR_ISP), y)
subdir-ccflags-y += -Wall -Werror
subdir-ccflags-y += -I$(srctree)/drivers/hisi/tzdriver \
                    -I$(srctree)/drivers/hisi/load_image \
                    -I$(srctree)/drivers/media/huawei/camera \
                    -I$(srctree)/drivers/media/huawei/camera/sensor \
                    -I$(srctree)/drivers/media/huawei/camera/platform
endif
