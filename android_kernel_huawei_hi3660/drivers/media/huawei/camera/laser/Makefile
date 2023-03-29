#
# Makefile for hisilicon camera laser driver
#

EXTRA_CFLAGS += -Idrivers/media/huawei/camera/
EXTRA_CFLAGS += -fno-pic

obj-y += laser_common.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_970) += hw_laser_subdev.o
obj-$(CONFIG_HISI_CAMERA_VL53L0) += vl53l0.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_KNT) += vl53l0_knt.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_GAL) += vl53l0_gal.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_ML) += vl53l0_ml.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_CMB) += vl53l0_cmb.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_DUKE) += vl53l0_duke.o
obj-$(CONFIG_HISI_CAMERA_GP2AP0XVT_STF) += gp2ap0xvt_stf.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_STF) += vl53l0_stf.o
obj-$(CONFIG_HISI_CAMERA_VL53L0_970) += vl53Lx/
