# Add your debugging flag (or not) to CFLAGS

ifeq ($(CONFIG_VCODEC_PLATFORM_KIRIN970), y)
PLATFORM := HiVCodecV200
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV210), y)
EXTRA_CFLAGS += -DHIVCODEC_V210
EXTRA_CFLAGS += -DHIVCODEC_PLATFORM_ECONOMIC
PLATFORM := HiVCodecV210
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV300), y)
PLATFORM := HiVCodecV300
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV310), y)
EXTRA_CFLAGS += -DHIVCODEC_V310
EXTRA_CFLAGS += -DHIVCODEC_PLATFORM_ECONOMIC
PLATFORM := HiVCodecV310
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV500), y)
PLATFORM := HiVCodecV500
EXTRA_CFLAGS += -DHIVCODECV500
endif


include drivers/vcodec/hi_vcodec/venc_hivna/drv/venc/drv_venc_make.cfg

EXTRA_CFLAGS += $(VENC_CFLAGS) -fno-pic
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna/drv/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna/drv/venc
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna/drv/product/$(PLATFORM)

ifeq ($(es_low_freq),true)
EXTRA_CFLAGS += -DCONFIG_ES_VENC_LOW_FREQ
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
EXTRA_CFLAGS += -DUSER_DISABLE_VENC_PROC
endif

obj-$(CONFIG_HI_VCODEC_VENC_HIVNA) += hi_omxvenc.o
hi_omxvenc-objs :=  venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    hi_drv_mem.o         \
                    ../product/$(PLATFORM)/hal_venc.o  \

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV500), y)
hi_omxvenc-objs := $(hi_omxvenc-objs) ../product/$(PLATFORM)/smmu.o
endif


#obj-m += venc.o
#venc-y :=           venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    hal_venc.o           \
                    hi_drv_mem.o

