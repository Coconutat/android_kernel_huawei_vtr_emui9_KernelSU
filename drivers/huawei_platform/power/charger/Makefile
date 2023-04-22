obj-$(CONFIG_HUAWEI_CHARGER_AP)	+= charger_ap/
obj-$(CONFIG_HUAWEI_CHARGER_SENSORHUB)	+= charger_sensorhub/

ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif

obj-y += huawei_charger_common.o
obj-y += huawei_charger_power_interface.o