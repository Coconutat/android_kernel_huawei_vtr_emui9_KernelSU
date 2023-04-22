
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCDKIT_DEBUG_ENABLE
endif

obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_panel.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_fb_util.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_debug.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_parse.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_common.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_bias_ic_common.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_tp.o
obj-$(CONFIG_LCDKIT_DRIVER)	+= lcdkit_backlight_ic_common.o

ifdef CONFIG_HUAWEI_DEVKIT_HISI
obj-$(CONFIG_LCDKIT_DRIVER) += hisi/
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/core/common/hisi
EXTRA_CFLAGS += -Idrivers/video/fbdev/hisi/dss
EXTRA_CFLAGS += -Idrivers/staging/android
#$(info find lcdkit hisi folder, include it now!)
endif

ifndef CONFIG_HUAWEI_DEVKIT_HISI
obj-$(CONFIG_LCDKIT_DRIVER) += qcom/
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/core/common/qcom
#$(info find lcdkit qcom folder, include it now! )
endif

#for debug!!
#$(error stop! $(mkfile_path))

EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/include	\
		-Idrivers/devkit/tpkit
