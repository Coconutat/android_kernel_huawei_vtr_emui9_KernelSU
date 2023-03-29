TOP      := drivers/../..
VFMW_DIR := drivers/vcodec/hi_vcodec/vdec_hivna

ifeq ($(TARGET_BOARD_PLATFORM), kirin710)
EXTRA_CFLAGS += -DOMXVDEC_TVP_CONFLICT
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
EXTRA_CFLAGS += -DOMXVDEC_TVP_CONFLICT
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin980)
EXTRA_CFLAGS += -DOMXVDEC_TVP_CONFLICT
endif

ifneq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV500), y)
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/HiVCodecV200/NOR_CFG/vfmw_make.cfg
else
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/HiVCodecV500/NOR_CFG/vfmw_make.cfg
EXTRA_CFLAGS += -DSECURE_VS_NOR_SECURE
endif

ifeq ($(CONFIG_VCODEC_PLATFORM_HIVCODECV300), y)
EXTRA_CFLAGS += -DSECURE_VS_NOR_SECURE
endif

EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna/omxvdec/platform
EXTRA_CFLAGS += $(VFMW_CFLAGS) -fno-pic

#build in
obj-$(CONFIG_HI_VCODEC_VDEC_HIVNA) += hi_vfmw.o
hi_vfmw-objs := $(VFMW_CFILES)

#module
#obj-m += hi_vfmw.o
#hi_vfmw-y := $(VFMW_CFILES)
