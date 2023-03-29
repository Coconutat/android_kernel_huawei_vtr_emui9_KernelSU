ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk
EXTRA_CFLAGS += -Idrivers/tzdriver \
                -Idrivers/hisi/tzdriver \
		-Iscripts/dtc/libfdt

obj-y		    += load_image.o

ifeq ($(strip $(CFG_FEATURE_DELAY_MODEM_INIT)),FEATURE_ON)
obj-y		    += loadm_phone.o
else
obj-y		    += loadm_mbb.o
endif

#subdir-ccflags-y := -Werror
