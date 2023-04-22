ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif
-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk 

EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
ifeq ($(strip $(CFG_CONFIG_SEC_RFILE)),YES)
obj-y += sec_rfile_balong.o
endif
obj-$(CONFIG_RFILE_SUPPORT)			+= rfile_balong.o
obj-$(CONFIG_ENABLE_TEST_CODE)         += rfile_api_test.o
