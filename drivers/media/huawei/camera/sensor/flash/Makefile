#
# Makefile for hisilicon camera flash driver
#
EXTRA_CFLAGS += -Idrivers/media/huawei/camera/sensor/flash
EXTRA_CFLAGS += -Idrivers/media/huawei/camera/
EXTRA_CFLAGS += -fno-pic

ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DCAMERA_FLASH_FACTORY_TEST
endif 

obj-y += hw_flash_i2c.o
obj-y += hw_flash.o

#obj-y += tps61310.o

obj-$(CONFIG_HISI_CAMERA_LM3642) += lm3642.o
obj-$(CONFIG_HISI_CAMERA_MP3331) += mp3331.o
obj-$(CONFIG_HISI_CAMERA_LM3642_FRONT) += lm3642_front.o
obj-$(CONFIG_HISI_CAMERA_MP3331_FRONT) += mp3331_front.o
obj-$(CONFIG_HISI_CAMERA_LM3642_IR) += aux_flash_ldo.o
obj-$(CONFIG_HISI_CAMERA_LM3642_IR) += lm3642_ir.o
obj-$(CONFIG_HISI_CAMERA_MP3331_IR) += aux_flash_ldo.o
obj-$(CONFIG_HISI_CAMERA_MP3331_IR) += mp3331_ir.o
# obj-y += lm3642.o
#obj-y += += pmu_led.o
obj-$(CONFIG_HISI_CAMERA_LM3646) += lm3646.o
obj-$(CONFIG_HISI_CAMERA_MP3336) += mp3336.o
obj-$(CONFIG_HISI_CAMERA_LM3646_MIX) += lm3646_mix.o
obj-$(CONFIG_HISI_CAMERA_MP3336_MIX) += mp3336_mix.o
obj-$(CONFIG_HISI_CAMERA_MP3336_MIX_IR) += mp3336_mix_ir.o
obj-$(CONFIG_HISI_CAMERA_LM3646_MIX_IR) += lm3646_mix_ir.o
#obj-y += lm3646_front.o
obj-$(CONFIG_HISI_CAMERA_SCHARGER) += scharger_flash.o
obj-$(CONFIG_HISI_CAMERA_AW3642) += aw3642.o