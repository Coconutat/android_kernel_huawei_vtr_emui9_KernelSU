-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk
obj-$(CONFIG_PSAM_SUPPORT)           += psam_balong.o psam_hal32.o psam_hal64.o
subdir-ccflags-y := -Werror
ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE_psam_balong.o := y
endif

