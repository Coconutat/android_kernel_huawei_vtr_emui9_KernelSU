ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
obj-$(CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT)      += usb_short_circuit_protect.o
obj-$(CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT_SENSORHUB)      += usb_short_circuit_protect_sh.o
