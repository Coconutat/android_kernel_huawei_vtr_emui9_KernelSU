#
# Makefile for MMC/SD card drivers
#
ifeq ($(TARGET_VERSION_MODE),factory)
KBUILD_CFLAGS += -DEMMC_FMD_FACTORY_MODE
endif

obj-$(CONFIG_MMC_BLOCK)		+= mmc_block.o
mmc_block-objs			:= block.o queue.o
obj-$(CONFIG_MMC_TEST)		+= mmc_test.o

obj-$(CONFIG_SDIO_UART)		+= sdio_uart.o

obj-$(CONFIG_MMC_FFU)		+= ffu.o

obj-$(CONFIG_HW_SD_HEALTH_DETECT)	+= mmc_health_diag.o
obj-$(CONFIG_HW_MMC_TEST_MODULE)	+= hw_mmc_test.o
obj-$(CONFIG_HISI_MMC_SECURE_RPMB) += hisi_rpmb.o
obj-$(CONFIG_HW_SYSTEM_HW_WR_PROTECT) += emmc_system_wp.o
obj-$(CONFIG_HUAWEI_EMMC_DSM) += dsm_emmc.o
obj-$(CONFIG_HUAWEI_SDCARD_DSM) += dsm_sdcard.o
obj-$(CONFIG_MMC_CQ_HCI) += hisi_cmdq_card.o
