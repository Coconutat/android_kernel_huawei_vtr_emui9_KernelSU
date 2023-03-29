obj-$(CONFIG_HUAWEI_CHARGER_AP) += huawei_charger.o charging_core.o
obj-$(CONFIG_BQ2419X_CHARGER) += bq2419x/
obj-$(CONFIG_BQ2429X_CHARGER) += bq2429x/
obj-$(CONFIG_BQ25892_CHARGER) += bq25892/
obj-$(CONFIG_BQ2560X_CHARGER) += bq2560x/
obj-$(CONFIG_RT9466_CHARGER) += rt9466/
obj-$(CONFIG_DUAL_CHARGER) += dual_charger/
obj-$(CONFIG_DIRECT_CHARGER) += direct_charger/
obj-$(CONFIG_WIRELESS_CHARGER) += wireless_charger/
obj-$(CONFIG_WIRED_CHANNEL_SWITCH) += wired_channel_switch/
obj-$(CONFIG_BQ25882_CHARGER) += bq25882/

ifeq ($(HLTHERM_RUNTEST), true)
    CFLAGS_KERNEL += -DCONFIG_HLTHERM_RUNTEST
endif
