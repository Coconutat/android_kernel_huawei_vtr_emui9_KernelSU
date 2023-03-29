-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk

ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

obj-y       += applog_balong.o  
ifeq ($(strip $(CFG_CONFIG_MODULE_BUSSTRESS)),YES)
#obj-y       += applog_balong_test.o
else
obj-$(CONFIG_ENABLE_TEST_CODE) += applog_balong_test.o
endif

subdir-ccflags-y := -Werror
