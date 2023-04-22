#gcov
ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
GCOV_PROFILE_socp_balong_test.o := n
GCOV_PROFILE_socp_api_test.o := n
endif

-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk
obj-y       += socp_balong.o socp_api_test.o  socp_ind_delay.o
#socp_balong_test.o socp_balong_test_llt.o 

ifeq ($(strip $(CFG_CONFIG_DEFLATE)),YES)	
obj-y      += deflate.o 
endif
ifeq ($(strip $(CFG_CONFIG_MODULE_BUSSTRESS)),YES)
#obj-y       += socp_balong_test.o
else
obj-$(CONFIG_ENABLE_TEST_CODE) += socp_balong_test.o
endif

subdir-ccflags-y := -Werror
