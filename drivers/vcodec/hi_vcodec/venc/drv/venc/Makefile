# Add your debugging flag (or not) to CFLAGS
include drivers/vcodec/hi_vcodec/venc/drv/venc/drv_venc_make.cfg

EXTRA_CFLAGS += $(VENC_CFLAGS) -fno-pic
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc/drv/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc/drv/venc

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DUSER_DISABLE_VENC_PROC
endif

ifeq ($(TARGET_BOARD_PLATFORM), hi3660)
EXTRA_CFLAGS += -DPLATFORM_HI3660
endif

obj-$(CONFIG_HI_VCODEC_VENC_HI3660) += hi_omxvenc.o
hi_omxvenc-objs :=  venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    drv_omxvenc.o        \
                    drv_omxvenc_efl.o    \
                    drv_venc_buf_mng.o   \
                    drv_venc_queue_mng.o \
                    drv_venc_proc.o      \
                    hal_venc.o           \
                    hi_drv_mem.o


#obj-m += venc.o
#venc-y :=           venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    drv_omxvenc.o        \
                    drv_omxvenc_efl.o    \
                    drv_venc_buf_mng.o   \
                    drv_venc_queue_mng.o \
                    drv_venc_proc.o      \
                    hal_venc.o           \
                    hi_drv_mem.o

