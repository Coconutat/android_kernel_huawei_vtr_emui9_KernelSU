obj-$(CONFIG_HW_DP_SOURCE) += dp_source_switch.o

obj-$(CONFIG_HUAWEI_DSM) += dp_dsm.o
ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DDP_FACTORY_MODE_ENABLE
endif
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DDP_DEBUG_ENABLE
endif
obj-$(CONFIG_HW_DP_SOURCE) += dp_factory.o
obj-$(CONFIG_HW_DP_SOURCE) += dp_debug.o
