#
# Main Makefile for building the corresponding module functioned target
#
#
#
# set the coorespondig path to the project root directory
# DO NOT change the variable value, or else if YOU know what you are changing
export ROOT_DEPTH_DMAC=../../..
export DRV_DMAC_ROM_DIR=../dmac_ec
export DRV_TEST_DIR=$(ROOT_DEPTH_DMAC)/drivers/wifi/dmac/dmac_verify
export KERNEL_PATH_DMAC=$(PWD)/$(ROOT_DEPTH_DMAC)/$(KERNEL_PATH)
export 1151_ROOT_PATH ?= $(PWD)/$(ROOT_DEPTH_DMAC)

# set the default make target
# NOTICE: make sure that the target name here is same with the module
# build target defined at the bottom
.PHONY:module_build

# include the config file to setup the compiling environment
include $(1151_ROOT_PATH)/build/1151_host_wifi/env.config

# set the module objects files
obj-m :=dmac.o
dmac-objs := dmac_main.o dmac_mgmt_classifier.o dmac_psm_ap.o dmac_rx_data.o dmac_tid.o dmac_tx_bss_comm.o dmac_11w.o dmac_tx_complete.o dmac_user.o dmac_vap.o mac_device.o mac_frame.o mac_user.o mac_vap.o dmac_data_acq.o\
             mac_ie.o dmac_uapsd.o dmac_mgmt_bss_comm.o dmac_beacon.o dmac_alg.o dmac_blockack.o mac_resource.o dmac_mgmt_ap.o dmac_mgmt_sta.o dmac_wep.o dmac_11i.o mac_regdomain.o dmac_scan.o dmac_dfx.o dmac_device.o dmac_resource.o dmac_smps.o dmac_opmode.o dmac_m2s.o\
			 dmac_reset.o dmac_config.o dmac_config_debug.o dmac_stat.o dmac_fcs.o dmac_acs.o dmac_chan_mgmt.o dmac_user_track.o dmac_rx_filter.o dmac_txopps.o dmac_dft.o dmac_ap_pm.o mac_pm.o dmac_hcc_adapt.o mac_board.o mac_data.o dmac_arp_offload.o dmac_auto_adjust_freq.o dmac_green_ap.o dmac_auto_cali.o dmac_user_extend.o dmac_pkt_capture.o dmac_wmmac.o dmac_crypto_comm.o dmac_crypto_wep.o dmac_crypto_tkip.o dmac_crypto_aes_ccm.o dmac_bsd.o dmac_radar.o dmac_power.o dmac_11v.o dmac_csa_sta.o


# set the module objects files in rom path
dmac-rom-objs := dmac_main_rom.o dmac_mgmt_classifier_rom.o dmac_psm_ap_rom.o dmac_rx_data_rom.o dmac_tid_rom.o dmac_tx_bss_comm_rom.o dmac_11w_rom.o dmac_tx_complete_rom.o \
			dmac_user_rom.o dmac_vap_rom.o dmac_data_acq_rom.o dmac_uapsd_rom.o dmac_mgmt_bss_comm_rom.o dmac_beacon_rom.o dmac_alg_rom.o dmac_blockack_rom.o dmac_mgmt_ap_rom.o dmac_mgmt_sta_rom.o  \
			dmac_wep_rom.o dmac_11i_rom.o dmac_scan_rom.o dmac_dfx_rom.o dmac_device_rom.o dmac_resource_rom.o dmac_smps_rom.o dmac_opmode_rom.o dmac_m2s_rom.o dmac_reset_rom.o dmac_config_rom.o \
	        dmac_config_debug_rom.o dmac_stat_rom.o dmac_fcs_rom.o dmac_acs_rom.o dmac_chan_mgmt_rom.o dmac_user_track_rom.o dmac_rx_filter_rom.o dmac_txopps_rom.o dmac_dft_rom.o dmac_ap_pm_rom.o \
			dmac_hcc_adapt_rom.o dmac_arp_offload_rom.o dmac_auto_adjust_freq_rom.o dmac_green_ap_rom.o dmac_auto_cali_rom.o dmac_user_extend_rom.o dmac_pkt_capture_rom.o \
			mac_data_rom.o mac_frame_rom.o mac_ie_rom.o mac_regdomain_rom.o  mac_device_rom.o mac_resource_rom.o mac_board_rom.o mac_user_rom.o mac_vap_rom.o dmac_power_rom.o dmac_csa_sta_rom.o

dmac-rom-objs := $(addprefix $(DRV_DMAC_ROM_DIR)/,$(dmac-rom-objs))
dmac-objs += $(dmac-rom-objs)

dmac-objs += $(DRV_TEST_DIR)/dmac_test_sch.o
dmac-objs += $(DRV_TEST_DIR)/dmac_test_main.o \
	     $(DRV_TEST_DIR)/dmac_lpm_test.o \
 	     $(DRV_TEST_DIR)/dmac_frame_filter_test.o \
		 $(DRV_TEST_DIR)/dmac_wmm_test.o  \
		 $(DRV_TEST_DIR)/dmac_scan_test.o \
		 $(DRV_TEST_DIR)/dmac_acs_test.o

# set the feature options
include $(1151_ROOT_PATH)/build/1151_host_wifi/caps.config

# for example:
# ifeq ($(SUPPORT_VIDEO_OPTIMIZE),1)
#	alg-objs += alg_video.o
# endif

# set the module name
MODULE_NAME=$(subst .o,.ko,$(obj-m))

BACKUP_OBJFILE_FOLDER=$(strip $(subst .o,,$(obj-m)))_objfile

BACKUP_OBJFILE_DIR=$(PWD)/$(BACKUP_OBJFILE_FOLDER)

OBJDUMP_FILE=$(subst .o,.objdump,$(obj-m))
OBJDUMP_TXT_FILE=$(subst .o,.txt,$(obj-m))

# module's elf header file name
MODULE_ELF_HEADER_FILE=$(subst .o,.elf_header,$(obj-m))

# build the module
all:
	@echo Building module $(MODULE_NAME)
	make -j $(NJOBS) -C $(KERNEL_PATH_DMAC) M=$(PWD) modules
	$(shell [ -d $(IMAGE_PATH_KO) ] || mkdir -p $(IMAGE_PATH_KO))
	cp ./$(MODULE_NAME) $(IMAGE_PATH_KO) -fv
	$(STRIP) $(IMAGE_PATH_KO)/$(MODULE_NAME) --strip-unneeded
	make backup_objfile
	make create_objdump
	make create_objdump_txt
	make create_original_ko
	make create_readelf_header

# clean the module
clean:
	@echo cleaning module $(MODULE_NAME)
	-find . -type f -name "*.o" -exec rm -f {} \;
	-find . -type f -name "*.o.uu" -exec rm -f {} \;
	-find . -type f -name "*.cmd" -exec rm -f {} \;
	-find . -type f -name "*.ko" -exec rm -f {} \;
	-find . -type f -name "*.out" -exec rm -f {} \;
	-find . -type f -name "*.order" -exec rm -f {} \;
	-find . -type f -name "*.mod.c" -exec rm -f {} \;
	-find . -type f -name "*.symvers" -exec rm -f {} \;
	rm .tmp_versions -frv
	rm -rf $(IMAGE_PATH_KO)/*.ko

install:
	@echo install the module to the root-file system

backup_objfile:
	@echo backup the temp files
	mkdir -p $(BACKUP_OBJFILE_FOLDER)
	-find . -type f -name "*.o.uu" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.cmd" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.ko" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.out" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.order" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.mod.c" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.symvers" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.c.swp" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	cp -fr .tmp_versions $(BACKUP_OBJFILE_FOLDER)
	rm .tmp_versions -frv
	-find $(DRV_TEST_DIR)/ -type f -name "*.o" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	$(foreach OBJFILE, $(dmac-objs) $(obj-m) , mv -v -f $(OBJFILE) $(BACKUP_OBJFILE_FOLDER);)
create_objdump:
ifeq ($(OBJDUMP_ENABLE),y)
	@echo start to create_objdump file
	$(shell [ -d $(OBJDUMP_PATH) ] ||(mkdir -p $(OBJDUMP_PATH)))
	find . -type f -name "*.o" |xargs -t -i $(OBJDUMP) -S {} >>  $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_FILE)
	@mv -f $(BACKUP_OBJFILE_DIR)/*.objdump $(OBJDUMP_PATH)	
endif

create_objdump_txt:
	$(shell [ -d $(OBJDUMP_TXT_PATH) ] ||(mkdir -p $(OBJDUMP_TXT_PATH)))
	$(foreach OBJFILE, $(obj-m),$(OBJDUMP) -t $(BACKUP_OBJFILE_DIR)/$(obj-m) >> $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_TXT_FILE);)
	@mv -f $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_TXT_FILE) $(OBJDUMP_TXT_PATH)
create_original_ko:
	$(shell [ -d $(ORIGINAL_KO_PATH) ] ||(mkdir -p $(ORIGINAL_KO_PATH)))
	@mv -f $(BACKUP_OBJFILE_DIR)/*.ko $(ORIGINAL_KO_PATH)

create_readelf_header:
	$(shell [ -d $(OBJDUMP_TXT_PATH) ] ||(mkdir -p $(OBJDUMP_TXT_PATH)))
	@echo $(MODULE_NAME) > $(OBJDUMP_TXT_PATH)/$(MODULE_ELF_HEADER_FILE)
	@$(READELF) -S $(ORIGINAL_KO_PATH)/$(MODULE_NAME) >> $(OBJDUMP_TXT_PATH)/$(MODULE_ELF_HEADER_FILE)

