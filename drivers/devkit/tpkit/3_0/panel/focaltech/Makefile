#drivers/input/touchscreen
#
#Makefile for the touch driver of Focaltech
obj-$(CONFIG_FOCAL_TS_KIT_3_0) += focaltech_ts.o
focaltech_ts-objs += focaltech_core.o focaltech_flash.o focaltech_apk_node.o focaltech_dts.o focaltech_test.o focaltech_spi.o focaltech_test_8201.o

EXTRA_CFLAGS += -Iarch/arm/mach-hisi
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
