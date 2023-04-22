#
# Makefile for the Synaptics TCM touchscreen driver.
#

# Each configuration option enables a list of files.
obj-y += synaptics_tcm_driver.o
synaptics_tcm_driver-objs += synaptics_tcm_core.o synaptics_tcm_zeroflash.o synaptics_tcm_touch.o synaptics_tcm_device.o synaptics_tcm_cap_check.o

EXTRA_CFLAGS += -Iarch/arm/mach-hisi
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/core/common
