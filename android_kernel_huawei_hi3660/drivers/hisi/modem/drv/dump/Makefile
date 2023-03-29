#
# Makefile for the memory technology device drivers.
#

# Core functionality.
-include $(srctree)/drivers/hisi/modem/config/product/$(OBB_PRODUCT_NAME)/config/balong_product_config.mk

obj-y				+= apr/
ifeq ($(strip $(CFG_ENABLE_BUILD_OM)),YES)
obj-y				+= comm/
obj-y				+= mdmcp/
obj-y				+= mdmap/
obj-y				+= mem/
endif

KBUILD_CPPFLAGS += -Idrivers/hisi/modem/drv/dump/apr \
                   -Idrivers/hisi/modem/drv/dump/comm \
                   -Idrivers/hisi/modem/drv/dump/mdmcp \
                   -Idrivers/hisi/modem/drv/dump/mdmap \
				   -Idrivers/hisi/modem/drv/dump/mem


