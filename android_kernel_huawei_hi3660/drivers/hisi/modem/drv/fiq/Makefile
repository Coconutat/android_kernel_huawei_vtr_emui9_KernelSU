-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk

ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

ifeq ($(strip $(CFG_CONFIG_CCPU_FIQ_SMP)),YES)
obj-y			+= fiq_smp.o
else
obj-y			+= fiq.o
endif
subdir-ccflags-y := -Werror

