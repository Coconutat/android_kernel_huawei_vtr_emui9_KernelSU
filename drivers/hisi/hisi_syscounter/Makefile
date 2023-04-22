ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
ccflags-y  += -I$(srctree)/drivers/hisi/tzdriver
obj-$(CONFIG_HISI_SYSCOUNTER) += hisi_syscounter.o

