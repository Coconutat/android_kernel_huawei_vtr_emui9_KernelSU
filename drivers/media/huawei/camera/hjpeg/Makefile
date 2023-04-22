ccflags-y += -Idrivers/media/huawei/camera
ccflags-y += -Idrivers/media/huawei/jpeg

obj-y = hjpgenc.o 

# CFLAGS_hwisp_stream.o = -I$(src)

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV150), y)
obj-y += hjpeg150/
else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV160), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV120), y)
subdir-ccflags-y += -DHISP120_CAMERA
obj-y += hjpegenc/
else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV200), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV250), y)
obj-y += hjpegenc/
else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV210), y)
obj-y += hjpegenc/
endif
