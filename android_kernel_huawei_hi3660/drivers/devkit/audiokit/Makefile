# Audiokit driver

obj-$(CONFIG_HUAWEI_DSM_AUDIO)	+= dsm_audio/
obj-$(CONFIG_HUAWEI_DEVICEBOX_AUDIO)	+= deviceboxID/
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO)	+= smartpakit/
ifneq ($(TARGET_BUILD_VARIANT),user)
	obj-y      += headset_debug/
endif

