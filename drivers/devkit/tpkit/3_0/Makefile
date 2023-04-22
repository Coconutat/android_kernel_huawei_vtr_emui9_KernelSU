# Makefile for the irda drivers.
#

# Each configuration option enables a list of files.
CFLAGS_huawei_ts_kit_core.o := -I$(src)
CFLAGS_huawei_ts_kit_api.o := -I$(src)

ifeq ($(CONFIG_HUAWEI_DEVKIT_HISI_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o hisi_adapter/tpkit_platform_adapter.o huawei_ts_kit_misc.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
endif

ifeq ($(CONFIG_HUAWEI_DEVKIT_QCOM_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o huawei_ts_kit_misc.o qcom_adapter/tpkit_cmdline_parse.o qcom_adapter/tpkit_platform_adapter.o
endif

ifeq ($(CONFIG_HUAWEI_DEVKIT_MTK_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o huawei_ts_kit_misc.o mtk_adapter/tpkit_platform_adapter.o
endif

obj-$(CONFIG_HUAWEI_TS_KIT_COMMON_3_0) += huawei_tp_common.o
obj-$(CONFIG_HUAWEI_TP_KIT_COLOR_INFO_3_0) += tp_color.o
obj-$(CONFIG_HUAWEI_TS_KIT_3_0)  += panel/
EXTRA_CFLAGS += -Idrivers/hwspinlock
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include

ifeq ($(CONFIG_HUAWEI_TS_KIT_TEST_3_0),y)
EXTRA_CFLAGS += -DHUAWEI_TS_KIT_TEST
endif
