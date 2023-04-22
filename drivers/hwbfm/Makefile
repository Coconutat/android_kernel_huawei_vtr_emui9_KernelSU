# Makefile for the BFMR moduls.

obj-$(CONFIG_HUAWEI_BFM)  += main/
obj-$(CONFIG_HUAWEI_BFM)  += common/
obj-$(CONFIG_HUAWEI_BFM)  += abns/

ifeq ($(CONFIG_HW_BFMR_HISI),y)
obj-$(CONFIG_HUAWEI_BFM)  += common/chipsets/hisi/
endif

ifeq ($(CONFIG_HW_BFMR_QCOM),y)
obj-$(CONFIG_HUAWEI_BFM)  += common/chipsets/qcom/
endif

ifeq ($(CONFIG_HW_BFMR_MTK),y)
obj-$(CONFIG_HUAWEI_BFM)  += common/chipsets/mtk/
endif

obj-$(CONFIG_HUAWEI_BFM)  += bfm/core/
ifeq ($(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION),y)
obj-$(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION)  += bfr/core/
endif

ifeq ($(CONFIG_HW_BFMR_HISI),y)
obj-$(CONFIG_HUAWEI_BFM)  += bfm/chipsets/hisi/
ifeq ($(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION),y)
obj-$(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION)  += bfr/chipsets/hisi/
endif
endif

ifeq ($(CONFIG_HW_BFMR_QCOM),y)
obj-$(CONFIG_HUAWEI_BFM)  += bfm/chipsets/qcom/
ifeq ($(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION),y)
obj-$(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION)  += bfr/chipsets/qcom/
endif
endif

ifeq ($(CONFIG_HW_BFMR_MTK),y)
obj-$(CONFIG_HUAWEI_BFM)  += bfm/chipsets/mtk/
ifeq ($(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION),y)
obj-$(CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION)  += bfr/chipsets/mtk/
endif
endif