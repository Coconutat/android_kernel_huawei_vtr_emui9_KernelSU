# Makefile for the DSM_KIRIN_CHIPSETS drivers.
#
# Each configuration option enables a list of files.

ifeq ($(TARGET_VERSION_MODE),factory)
ccflags-y += -DFACTORY_TEST
endif

obj-$(CONFIG_HUAWEI_DSM_KIRIN_CHIPSETS)  += dsm_kirin_chipsets.o
