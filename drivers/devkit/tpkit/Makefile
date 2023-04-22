# Makefile for the irda drivers.
#

# Each configuration option enables a list of files.

ifeq ($(CONFIG_HUAWEI_DEVKIT_HISI),y)
obj-$(CONFIG_HUAWEI_TS_KIT) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o hisi_adapter/tpkit_platform_adapter.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
endif

ifeq ($(CONFIG_HUAWEI_DEVKIT_QCOM),y)
obj-$(CONFIG_HUAWEI_TS_KIT) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o qcom_adapter/tpkit_cmdline_parse.o qcom_adapter/tpkit_platform_adapter.o
endif

obj-$(CONFIG_HUAWEI_TS_KIT_COMMON) += huawei_tp_common.o
obj-$(CONFIG_HUAWEI_TP_KIT_COLOR_INFO) += tp_color.o
obj-$(CONFIG_HUAWEI_TS_KIT)  += panel/
obj-$(CONFIG_HUAWEI_HOST_TS_KIT) += hostprocessing/
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/core/common
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/include
EXTRA_CFLAGS += -Idrivers/hwspinlock

obj-$(CONFIG_HUAWEI_TS_KIT_3_0)    += 3_0/

ifeq ($(CONFIG_HUAWEI_TS_KIT_TEST),y)
EXTRA_CFLAGS += -DHUAWEI_TS_KIT_TEST
endif
