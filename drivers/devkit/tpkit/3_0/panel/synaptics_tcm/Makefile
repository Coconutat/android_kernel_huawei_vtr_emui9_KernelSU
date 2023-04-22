#
# Makefile for the Synaptics TCM touchscreen driver.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SYNAPTICS_TCM_TS_KIT_3_0) += synaptics_tcm_driver.o
synaptics_tcm_driver-objs += synaptics_tcm_core.o synaptics_tcm_zeroflash.o synaptics_tcm_touch.o synaptics_tcm_device.o synaptics_tcm_cap_check.o  synaptics_tcm_reflash.o

EXTRA_CFLAGS += -Iarch/arm/mach-hisi
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
