#
# Makefile for the modem drivers.
#
-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk 
ifneq ($(strip $(CFG_ATE_VECTOR)),YES)
obj-y				+= sysctrl/
obj-y				+= s_memory/
obj-y				+= fiq/
ifeq ($(strip $(CFG_CONFIG_MODULE_TIMER)),YES)
obj-y				+= balong_timer/
endif
ifeq ($(strip $(CFG_CONFIG_MODULE_IPC)),YES)
obj-y				+= ipc/
endif
obj-y				+= icc/
obj-y				+= reset/
obj-y				+= sec_call/
obj-$(CONFIG_RFILE_SUPPORT)	+= rfile/
ifeq ($(strip $(CFG_FEATURE_TDS_WCDMA_DYNAMIC_LOAD)),FEATURE_ON)
obj-y				+= load_ps/
endif
obj-y				+= sync/
ifeq ($(strip $(CFG_ENABLE_BUILD_SOCP)),YES)
obj-y			+= socp/
endif
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
ifeq ($(strip $(CFG_CONFIG_MODEM_MINI_DUMP)),YES)
obj-y				+= dump_eng/
else
obj-y                           += dump/
endif
endif

obj-y				+= nvim/
ifeq ($(strip $(CFG_CONFIG_FAULT_INFLOOD)),YES)
obj-y				+= fault_inflood/
endif
obj-$(CONFIG_IMAGE_LOAD) += loadm/
ifeq ($(strip $(CFG_CONFIG_ZSTD_DECOMPRESS)),YES)
obj-y			+= zstd/
endif

obj-y			+= adp/
obj-y			+= hwadp/
obj-y			+= version/
#obj-$(CONFIG_BALONG_EASY_SHELL) += easy_shell/
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
obj-y			+= log/
endif

obj-y			+= hds/

ifeq ($(strip $(CFG_CONFIG_SYSBUS)),YES)
obj-y			+= sys_bus/
endif
ifeq ($(strip $(CFG_CONFIG_NEW_PLATFORM)),YES)
obj-y			+= n_ipf/
obj-y 			+= n_psam/
else
obj-y			+= ipf/
obj-y 			+= psam/
endif
obj-y			+= udi/
obj-y			+= thermal/
 
obj-$(CONFIG_MEM_BALONG)	+= mem/

obj-y 	+= onoff/

ifeq ($(strip $(CFG_CONFIG_EFUSE)),YES)
obj-y 	+= efuse/
endif

ifeq ($(strip $(CFG_CONFIG_SC)),YES)
obj-y  				+= sc/
endif
obj-y			+= pm_om/
 
obj-$(CONFIG_PMIC_OCP) += ocp/

ifeq ($(LLTMDRV_HUTAF_COV),true)
obj-y			+= lltcov/
endif

ifeq ($(strip $(llt_gcov)),y)
subdir-ccflags-y = -fno-inline
endif

ifeq ($(strip $(CFG_CONFIG_MODULE_BUSSTRESS)),YES)
obj-y                   += busstress/$(OBB_PRODUCT_NAME)/
endif

obj-$(CONFIG_HISI_SIM_HOTPLUG)          += sim_hotplug/
obj-$(CONFIG_HISI_SIM_HOTPLUG_SPMI)     += sim_hotplug/

else
obj-y				+= sysctrl/
obj-y				+= s_memory/
ifeq ($(strip $(CFG_CONFIG_MODULE_TIMER)),YES)
obj-y				+= balong_timer/
endif
obj-y				+= icc/
obj-y				+= reset/
obj-y				+= adp/
obj-y				+= hwadp/
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
obj-y				+= om/
endif
obj-y				+= pm_om/
endif
ifeq ($(strip $(CFG_CONFIG_APPLOG)),YES)
obj-y				+= applog/
endif
obj-y				+= diag_frame/
ifeq ($(strip $(CFG_CONFIG_DLOCK)),YES)
obj-y			    += dlock/
endif
obj-$(CONFIG_BBP_ACORE)			    += bbp/
obj-y				+= modem_avs/
