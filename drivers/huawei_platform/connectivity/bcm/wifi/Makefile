#=============================================================================
#=============================================================================

obj-$(CONFIG_BCMDHD) += device/
obj-$(CONFIG_BCMDHD) += driver/

ifneq ($(CONFIG_BCM_VIO),)
    obj-$(CONFIG_BCMDHD) += vio/
endif
ifneq ($(CONFIG_HW_WLANFTY_STATUS),)
    obj-$(CONFIG_BCMDHD) += wlanfty/
endif
