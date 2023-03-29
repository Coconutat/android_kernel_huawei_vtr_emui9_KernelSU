
obj-$(CONFIG_LCDKIT_DRIVER) += lcdkit_fb_util_qcom.o
obj-$(CONFIG_LCDKIT_DRIVER) += lcdkit_panel_adaptor_qcom.o
obj-$(CONFIG_LCDKIT_DRIVER) += lcdkit_debug_msm.o
obj-$(CONFIG_LCDKIT_DRIVER) += lcdkit_dsm.o
obj-$(CONFIG_LCDKIT_DRIVER) += lcdkit_bias_bl_utility.o

ifneq ($(CONFIG_ARCH_SDM660)$(CONFIG_ARCH_SDM632),)
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/include	\
                -Idrivers/devkit/lcdkit/lcdkit1.0/core/common/qcom \
                -Idrivers/video/fbdev/msm	\
                -Iinclude
else
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit1.0/include	\
                -Idrivers/devkit/lcdkit/lcdkit1.0/core/common/qcom \
                -Idrivers/video/msm/mdss	\
                -Iinclude
endif
