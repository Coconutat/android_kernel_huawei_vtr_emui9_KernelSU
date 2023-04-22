ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver

obj-$(CONFIG_CONTEXTHUB) += inputhub_api.o common.o
obj-$(CONFIG_CONTEXTHUB_SHMEM) += shmem.o
obj-$(CONFIG_CONTEXTHUB_SHELL) += shell_dbg.o
obj-$(CONFIG_CONTEXTHUB_LOADMONITOR) += loadmonitor.o
obj-$(CONFIG_CONTEXTHUB_PLAYBACK) += playback.o
obj-$(CONFIG_CONTEXTHUB_CHRE) += chre.o
obj-$(CONFIG_HISI_FLP)   += flp/
obj-$(CONFIG_INPUTHUB_MOCK_SENSOR)  += mock_sensor_channel.o
