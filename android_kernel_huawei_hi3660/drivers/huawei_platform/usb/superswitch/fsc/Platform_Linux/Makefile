#
# Makefile for the usb FUSB3601 driver (if driver is inside kernel tree).
#
ccflags-y += -DDEBUG
ccflags-y += -DFSC_PLATFORM_LINUX
ccflags-y += -DFSC_DEBUG
ccflags-y += -DFSC_LOGGING
ccflags-y += -DFSC_HAVE_SRC
ccflags-y += -DFSC_HAVE_SNK
ccflags-y += -DFSC_HAVE_DRP
ccflags-y += -DFSC_HAVE_ACC
ccflags-y += -DFSC_HAVE_VDM
ccflags-y += -DFSC_HAVE_DP

obj-y += fusb3601_whole.o
fusb3601_whole-objs := \
			fusb3601_driver.o \
			fusb3601_global.o \
			platform.o \
			platform_helpers.o \
			dfs.o \
			hostcomm.o \
			strings.o \
			callbacks.o \
			../core/core.o \
			../core/log.o \
			../core/policy.o \
			../core/protocol.o \
			../core/typec.o \
			../core/port.o \
			../core/registers.o \
			../core/systempolicy.o \
			../core/timer.o \
			../core/vdm.o \
			../core/display_port.o \
			../core/bc12.o \
			../core/moisture_detection.o \
			../core/hw_scp.o
