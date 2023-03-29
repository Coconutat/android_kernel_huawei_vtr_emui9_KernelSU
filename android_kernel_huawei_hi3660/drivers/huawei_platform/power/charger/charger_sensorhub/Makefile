ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
obj-$(CONFIG_HUAWEI_CHARGER_SENSORHUB) += huawei_charger_sh.o charging_core_sh.o
obj-$(CONFIG_BQ25892_CHARGER_SENSORHUB) += bq25892/
