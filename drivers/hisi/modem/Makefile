-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/ccore/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/dsp/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/acore/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/acore/$(CFG_PLATFORM)/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/common/$(CFG_PLATFORM)/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/platform/common/$(CFG_PLATFORM)/soc/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/adrv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/drv/acore/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/drv/common/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/drv/common/include/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/drv/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/product/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/oam/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/nv/tl/lps/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/phy/lphy/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/config/nvim/include/gu/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/taf/common/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/include/taf/acore/
subdir-ccflags-y += -I$(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config

obj-$(CONFIG_HISI_BALONG_MODEM) +=drv/

ifeq ($(strip $(MODEM_FULL_DUMP)),true)
subdir-ccflags-y += -DMODEM_FULL_DUMP_INUSER
endif

ifeq ($(strip $(MODEM_DDR_MINI_DUMP)),true)
subdir-ccflags-y += -DMODEM_DDR_MINI_DUMP_INUSER
endif

ifeq ($(strip $(MODEM_LOGCAT_INUSER)),true)
subdir-ccflags-y += -DMODEM_LOGCAT_INUSER
endif

ifeq ($(strip $(MODEM_LOG_FILTER_NVCONTROL)),true)
subdir-ccflags-y += -DMODEM_LOG_FILTER_INUSER
endif

ifeq ($(strip $(MODEM_CHR_INUSER)),true)
subdir-ccflags-y += -DMODEM_CHR_INUSER
endif

ifeq ($(strip $(MODEM_PM_OM_INUSER)),true)
subdir-ccflags-y += -DMODEM_PM_OM_INUSER
endif

ifneq ($(chip_type),es)
KBUILD_CFLAGS += -DCHIP_TYPE_CS=FEATURE_ON
endif

ifeq ($(secdog),rsa2048)
KBUILD_CFLAGS += -DSECDOG_SUPPORT_RSA_2048=FEATURE_ON
endif


ifeq ($(ota_netlock),true)
KBUILD_CFLAGS += -DFEATURE_OTA_NETLOCK=FEATURE_ON
endif

ifeq ($(simlock_cust),true)
KBUILD_CFLAGS += -DFEATURE_SIMLOCK_CUST=FEATURE_ON
endif

ifneq ($(OBB_SEPARATE),true)
obj-$(CONFIG_HISI_BALONG_MODEM) +=oam/ ps/ taf/ 

#ifeq ($(CFG_FEATURE_IMS),FEATURE_ON)
obj-$(CONFIG_HISI_BALONG_MODEM) +=ims/
#endif

endif
