 OS_NAME = $(shell uname -s)

 LC_OS_NAME = $(shell echo $(OS_NAME) | tr '[A-Z]' '[a-z]')

 last_file_name := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

 ifeq ($(LC_OS_NAME), linux)
 mkfilepath := $(abspath $(last_file_name))
 else
 mkfilepath := $(last_file_name) 
 endif

 current_dir :=$(dir $(mkfilepath))

 include $(current_dir)/balong_product_config_drv.mk
 include $(current_dir)/balong_product_config_pam.mk
 include $(current_dir)/balong_product_config_gucas.mk
 include $(current_dir)/balong_product_config_gucnas.mk
 include $(current_dir)/balong_product_config_gucphy.mk
 include $(current_dir)/balong_product_config_tlps.mk
 include $(current_dir)/balong_product_config_audio.mk
 include $(current_dir)/balong_product_config_tool.mk

 
ifeq ($(strip $(MODEM_FULL_DUMP)),true)
 include $(current_dir)/balong_product_config_modem_full_dump.mk
endif

ifeq ($(strip $(MODEM_DDR_MINI_DUMP)),true)
 include $(current_dir)/balong_product_config_modem_ddr_mini_dump.mk
endif


