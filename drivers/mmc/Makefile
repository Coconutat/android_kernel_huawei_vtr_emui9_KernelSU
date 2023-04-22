#
# Makefile for the kernel mmc device drivers.
#

subdir-ccflags-$(CONFIG_MMC_DEBUG) := -DDEBUG

obj-$(CONFIG_MMC)		+= core/
obj-$(CONFIG_MMC)		+= card/
obj-$(subst m,y,$(CONFIG_MMC))	+= host/

ifeq ($(HLTHERM_RUNTEST),true)
CFLAGS_KERNEL += -DCONFIG_HLTHERM_RUNTEST
endif
