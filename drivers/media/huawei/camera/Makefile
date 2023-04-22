ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DDEBUG_HISI_CAMERA
endif

ccflags-y += -Idrivers/hisi/tzdriver/libhwsecurec
obj-y += hwcam_usrdev.o hwcam_cfgdev.o hwcam_cfgpipeline.o hwcam_cfgstream.o hwcam_compat32.o

CFLAGS_hwcam_cfgdev.o = -I$(src)


obj-y += sensor/
obj-y += hisp/
obj-y += cci/
obj-y += csi/
obj-y += io/
obj-y += pmic/
obj-y += platform/
obj-y += laser/
obj-y += hjpeg/
obj-y += clt/
obj-y += fd/

obj-$(CONFIG_HISI_CAMERA_FPGA) += fpga/
obj-$(CONFIG_HISI_CAMERA_DRIVER_IC) += driver_ic/
obj-$(CONFIG_HISI_CAMERA_DOT_PROJECTOR) += dot/

