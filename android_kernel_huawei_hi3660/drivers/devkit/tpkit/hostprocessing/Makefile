EXTRA_CFLAGS += -Idrivers/hwspinlock

obj-$(CONFIG_HUAWEI_THP) += thp.o
thp-objs += huawei_thp_attr.o huawei_thp_core.o huawei_thp_mt_wrapper.o
obj-$(CONFIG_HUAWEI_THP_NOVATECH) += novatek_thp.o
obj-$(CONFIG_HUAWEI_THP_SYNAPTICS) += synaptics_thp.o
obj-$(CONFIG_JDI_HOST_TS_KIT) += bu21150_thp.o
obj-$(CONFIG_HUAWEI_THP_HIMAX) += himax_thp.o
obj-$(CONFIG_HUAWEI_THP_GOODIX) += goodix_thp.o
obj-$(CONFIG_HUAWEI_THP_SOLOMON) += ssl_thp.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Idrivers/tzdriver
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/core/common
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
EXTRA_CFLAGS += -Idrivers/huawei_platform/inputhub/default
else
EXTRA_CFLAGS += -Idrivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)
endif
