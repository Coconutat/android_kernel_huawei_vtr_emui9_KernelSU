#hi151 Makefile，支持编译分离模式与AIO模式的KO:加速
#分离模式下所有KO
SEP_MOD=alg oal oam plat sdt frw hal dmac hmac wal

#AIO模式下ko名称
AIO_MOD=hi1151

MOD ?= $(SEP_MOD)
#MOD ?= $(AIO_MOD)

export ROOT ?= ../../../..
export ROOT_DEPTH ?= $(shell pwd)/$(ROOT)

MISC=.ko .mod.c .mod.o .o
ALL_KO=$(MOD:%=%.ko)
ALL_MOD_CLEAN=$(SEP_MOD) $(AIO_MOD)

# include the config file to setup the compiling environment
include $(ROOT_DEPTH)/build/1151_host_wifi/env.config
include $(ROOT_DEPTH)/build/1151_host_wifi/caps.config

ifeq ($(MOD),hi1151)
EXTRA_CFLAGS += -D_PRE_WLAN_AIO
endif

ifeq ($(DUMP),n)
OBJDUMP_ENABLE = n
endif

MAIN_PATH = drivers/wifi/main/hi1151
OAL_PATH  = drivers/platform/oal
OAM_PATH  = drivers/platform/oam
PLAT_PATH = drivers/common/customize
SDT_PATH  = drivers/platform/sdt
FRW_PATH  = drivers/platform/frw
HAL_PATH  = device/driver/wifi/hal
DMAC_PATH = drivers/wifi/dmac
HMAC_PATH = drivers/wifi/hmac
WAL_PATH  = drivers/wifi/wal
ALG_PATH  = device/driver/wifi/alg

DMAC_TEST_PATH=$(DMAC_PATH)/dmac_verify
HMAC_TEST_PATH=$(HMAC_PATH)/hmac_verify
HAL_TEST_PATH=$(HAL_PATH)/hal_verify
ALG_TEST_PATH=$(ALG_PATH)/alg_verify

raw-main-objs = hi1151_main.o
raw-oal-objs  = oal_bus_if.o oal_main.o oal_mem.o oal_net.o oal_hardware.o oal_schedule.o oal_pci_if.o oal_aes.o oal_kernel_file.o oal_cfg80211.o oal_profiling.o oal_workqueue.o
raw-oam-objs  = oam_main.o oam_alarm.o oam_event.o oam_log.o oam_trace.o oam_statistics.o oam_config.o oam_register.o oam_linux_netlink.o
raw-plat-objs = hisi_ini.o hisi_customize_wifi.o
raw-sdt-objs  = sdt_drv.o
raw-frw-objs  = frw_event_deploy.o frw_event_main.o frw_event_sched.o frw_ipc_msgqueue.o frw_main.o frw_task.o frw_timer.o
raw-hal-objs  = hal_chip.o hal_device.o hal_witp_irq.o hal_main.o hal_to_dmac_if.o hal_witp_mac.o hal_witp_phy.o hsimu_reg.o hal_witp_rf.o hal_witp_cali.o hal_witp_reset.o hal_witp_debug.o
raw-dmac-objs = dmac_main.o dmac_mgmt_classifier.o dmac_psm_ap.o dmac_rx_data.o dmac_tid.o dmac_tx_bss_comm.o dmac_11w.o dmac_tx_complete.o dmac_user.o dmac_vap.o mac_device.o mac_frame.o mac_user.o mac_vap.o dmac_data_acq.o mac_ie.o dmac_uapsd.o dmac_mgmt_bss_comm.o dmac_beacon.o dmac_alg.o dmac_blockack.o mac_resource.o dmac_mgmt_ap.o dmac_mgmt_sta.o dmac_wep.o dmac_11i.o mac_regdomain.o dmac_scan.o dmac_dfx.o dmac_device.o dmac_resource.o dmac_reset.o dmac_config.o dmac_config_debug.o dmac_stat.o dmac_fcs.o dmac_acs.o dmac_chan_mgmt.o dmac_user_track.o dmac_rx_filter.o dmac_txopps.o dmac_dft.o dmac_ap_pm.o mac_pm.o dmac_hcc_adapt.o mac_board.o mac_data.o dmac_arp_offload.o dmac_auto_adjust_freq.o dmac_green_ap.o dmac_auto_cali.o
raw-hmac-objs = hmac_main.o hmac_rx_data.o hmac_tx_amsdu.o hmac_tx_data.o hmac_user.o hmac_vap.o hmac_mgmt_bss_comm.o hmac_mgmt_classifier.o hmac_fsm.o hmac_mgmt_ap.o hmac_mgmt_sta.o hmac_encap_frame_ap.o hmac_encap_frame_sta.o hmac_encap_frame.o hmac_sme_sta.o hmac_11i.o hmac_uapsd.o hmac_config.o hmac_protection.o hmac_frag.o hmac_crypto_tkip.o hmac_chan_mgmt.o hmac_m2u.o hmac_smps.o hmac_data_acq.o hmac_rx_filter.o hmac_proxy_arp.o  hmac_hcc_adapt.o hmac_dfs.o hmac_reset.o hmac_edca_opt.o hmac_blockack.o hmac_device.o hmac_resource.o hmac_wapi_wpi.o hmac_wapi_sms4.o hmac_wapi.o hmac_blacklist.o hmac_isolation.o hmac_scan.o hmac_dfx.o hmac_arp_offload.o  hmac_tcp_opt.o hmac_fbt_main.o hmac_acs.o hmac_proxysta.o hmac_traffic_classify.o hmac_btcoex.o hmac_isolation.o
raw-wal-objs  = wal_config.o wal_config_debug.o wal_linux_bridge.o wal_linux_cfg80211.o wal_linux_ioctl.o wal_linux_ioctl_debug.o wal_main.o wal_linux_scan.o wal_linux_rx_rsp.o wal_linux_event.o wal_regdb.o wal_config_acs.o wal_linux_flowctl.o wal_dfx.o wal_linux_atcmdsrv.o
raw-alg-objs  = alg_main.o alg_schedule.o alg_schedule_log.o alg_schedule_traffic_ctl.o alg_autorate.o alg_autorate_log.o alg_smartant.o alg_dbac.o alg_txbf.o alg_anti_interference.o alg_tpc.o alg_tpc_log.o alg_edca_opt.o  alg_mwo_detection.o alg_cca_optimize.o alg_cca_opt_log.o
raw-dmac-test-objs = dmac_acs_test.o dmac_frame_filter_test.o dmac_lpm_test.o dmac_scan_test.o dmac_test_main.o dmac_test_sch.o dmac_wmm_test.o
raw-hmac-test-objs = hmac_test_main.o
raw-hal-test-objs  = hal_lpm_test.o hal_test_main.o
raw-alg-test-objs  = alg_autorate_test.o alg_dbac_test.o alg_rssi_test.o alg_sch_test.o alg_smartant_test.o alg_test_main.o alg_tpc_test.o alg_txbf_log.o

abs-main-objs = $(addprefix $(ROOT)/$(MAIN_PATH)/, $(raw-main-objs))
abs-oal-objs  = $(addprefix $(ROOT)/$(OAL_PATH)/,  $(raw-oal-objs))
abs-oam-objs  = $(addprefix $(ROOT)/$(OAM_PATH)/,  $(raw-oam-objs))
abs-plat-objs = $(addprefix $(ROOT)/$(PLAT_PATH)/, $(raw-plat-objs))
abs-sdt-objs  = $(addprefix $(ROOT)/$(SDT_PATH)/,  $(raw-sdt-objs))
abs-frw-objs  = $(addprefix $(ROOT)/$(FRW_PATH)/,  $(raw-frw-objs))
abs-hal-objs  = $(addprefix $(ROOT)/$(HAL_PATH)/,  $(raw-hal-objs))
abs-dmac-objs = $(addprefix $(ROOT)/$(DMAC_PATH)/, $(raw-dmac-objs))
abs-hmac-objs = $(addprefix $(ROOT)/$(HMAC_PATH)/, $(raw-hmac-objs))
abs-wal-objs  = $(addprefix $(ROOT)/$(WAL_PATH)/,  $(raw-wal-objs))
abs-alg-objs  = $(addprefix $(ROOT)/$(ALG_PATH)/,  $(raw-alg-objs))
abs-dmac-test-objs = $(addprefix $(ROOT)/$(DMAC_TEST_PATH)/, $(raw-dmac-test-objs))
abs-hmac-test-objs = $(addprefix $(ROOT)/$(HMAC_TEST_PATH)/, $(raw-hmac-test-objs))
abs-hal-test-objs  = $(addprefix $(ROOT)/$(HAL_TEST_PATH)/, $(raw-hal-test-objs))
abs-alg-test-objs  = $(addprefix $(ROOT)/$(ALG_TEST_PATH)/, $(raw-alg-test-objs))

main-objs = $(abs-main-objs)
oal-objs  = $(abs-oal-objs)
oam-objs  = $(abs-oam-objs)
plat-objs = $(abs-plat-objs)
wal-objs  = $(abs-wal-objs)
sdt-objs  = $(abs-sdt-objs)
frw-objs  = $(abs-frw-objs)
hal-objs  = $(abs-hal-objs)  $(abs-hal-test-objs)
dmac-objs = $(abs-dmac-objs) $(abs-dmac-test-objs)
hmac-objs = $(abs-hmac-objs) $(abs-hmac-test-objs)
alg-objs  = $(abs-alg-objs) $(abs-alg-test-objs)

hi1151-objs = $(main-objs) $(oal-objs) $(oam-objs) \
	$(plat-objs) $(sdt-objs) $(frw-objs) $(hal-objs)  \
	$(dmac-objs) $(hmac-objs) $(wal-objs) $(alg-objs)

obj-m := $(MOD:%=%.o)

ALL_STRIPPED_FILE=$(addprefix $(IMAGE_PATH_KO)/,$(MOD:%=%.ko))
ALL_DUMP_FILE=$(addprefix $(OBJDUMP_PATH)/,$(MOD:%=%.objdump))
ALL_DUMP_TXT_FILE=$(addprefix $(OBJDUMP_TXT_PATH)/,$(MOD:%=%.txt))
ALL_ORI_KO_FILE=$(addprefix $(ORIGINAL_KO_PATH)/,$(MOD:%=%.ko))
ALL_DUMP_ELF_HEADE_FILE=$(addprefix $(OBJDUMP_TXT_PATH)/,$(MOD:%=%.elf_header))

# build the module
ko: 
	@echo "Building module [$(MOD:%=%.ko)] img=$(IMAGE_PATH_KO)"
	@$(foreach d,$(IMAGE_PATH_KO) $(ORIGINAL_KO_PATH),$(shell [ -d $(d) ] || mkdir -p $(d)))
	make -j $(NJOBS) -C $(ROOT_DEPTH)/$(KERNEL_PATH) M=$(shell pwd) modules 
	make -j $(NJOBS) create_original_ko create_stipped
ifeq ($(OBJDUMP_ENABLE),y)
	$(foreach d,$(OBJDUMP_PATH) $(OBJDUMP_TXT_PATH),$(shell [ -d $(d) ] || mkdir -p $(d)))
	make -j $(NJOBS) create_objdump create_objdump_txt create_readelf_header
endif
	make create_pointtrans

clean:
	@echo [CLEAN]
	@rm -rf $(hi1151-objs)
	@rm -rf $(foreach m,$(ALL_MOD_CLEAN),$(addprefix $(m),$(MISC))) 
	@rm -rf $(foreach m,$(ALL_MOD_CLEAN),$(m:%=.%.*.cmd)) 
	@rm -rf $(addprefix $(IMAGE_PATH_KO)/,$(ALL_MOD_CLEAN:%=%.ko))
	@rm -rf modules.order Module.symvers .tmp_versions
	@rm -rf $(IMAGE_PATH_KO)/*

create_pointtrans:
	@echo [POINT]
	@rm -rf $(IMAGE_PATH_KO)/sdt/release
	@mkdir -p $(IMAGE_PATH_KO)/sdt/release
	@cd $(SCRIPT_PATH);$(FORMAT_TC_PATH)/1151/pointTrans.py wifi $(TOPDIR) $(CONFIG_TARGET_PRODUCT)
	@cd $(SCRIPT_PATH);$(FORMAT_TC_PATH)/1151/pointTrans.py $(IMAGE_PATH_KO)/sdt/release $(IMAGE_PATH_KO)/sdt/release $(CONFIG_TARGET_PRODUCT) merge
	@cp -rf $(IMAGE_PATH_KO)/sdt/release/Total.wdk $(IMAGE_PATH_KO)
	@rm -rf $(IMAGE_PATH_KO)/sdt

$(IMAGE_PATH_KO)/%.ko : %.ko
	cp $< $@
	$(STRIP) $@ --strip-unneeded

$(OBJDUMP_PATH)/%.objdump : %.ko
	$(OBJDUMP) -S $<  > $@ 

$(OBJDUMP_TXT_PATH)/%.txt : %.ko
	$(OBJDUMP) -t $<  > $@ 

$(ORIGINAL_KO_PATH)/%.ko : %.ko
	@cp $< $@

$(OBJDUMP_TXT_PATH)/%.elf_header : %.ko
	@$(READELF) -S $<  >> $@

create_stipped        : $(ALL_STRIPPED_FILE)
create_objdump        : $(ALL_DUMP_FILE)
create_objdump_txt    : $(ALL_DUMP_TXT_FILE)
create_original_ko    : $(ALL_ORI_KO_FILE)
create_readelf_header : $(ALL_DUMP_ELF_HEADE_FILE)

all_clean:clean app_clean

app: libnl openssl hostapd wpa_supplicant app_sdt app_vamp app_daq app_reg app_nlc app_acs libpcap tcpdump tcpdump_mgr
app_clean:libnl_clean openssl_clean hostapd_clean wpa_supplicant_clean app_sdt_clean app_vamp_clean app_daq_clean app_nlc_clean app_acs_clean libpcap_clean tcpdump_clean tcpdump_mgr_clean app_reg_clean

libnl:
	cd $(LIBNL_PATH);	\
	./configure CC=$(CROSS_COMPILE)gcc --prefix=$(LIBNL_INSTALL_PATH) --host=arm;	\
	make all;		\
	make install;		\
	$(CROSS_COMPILE)strip ./lib/libnl.so.1.1.4 ;	\
	$(shell [ -d $(IMAGE_PATH_LIB) ] || mkdir -p $(IMAGE_PATH_LIB)) \
	cp ./lib/libnl.so.1.1.4 $(IMAGE_PATH_LIB)
	
libnl_clean:
	cd $(LIBNL_PATH);       \
	make clean
	cd $(IMAGE_PATH_LIB);       \
	rm -f libnl.so.1.1.4
	
openssl:
	cd $(OPENSSL_PATH);	\
	touch  -c Makefile;	\
	make all;		\
	make install;		\
	$(shell [ -d $(IMAGE_PATH_LIB) ] || mkdir -p $(IMAGE_PATH_LIB)) \
	cp $(OPENSSL_INSTALL_PATH)/lib/libcrypto.a  $(IMAGE_PATH_LIB);\
	cp $(OPENSSL_INSTALL_PATH)/lib/libssl.a $(IMAGE_PATH_LIB)

openssl_clean:
	cd $(OPENSSL_PATH);     \
	make clean
	cd $(IMAGE_PATH_LIB);       \
	rm -f libcrypto.a libssl.a
	
hostapd:
	cd $(HOSTAPD_PATH)/hostapd;	\
	make all;			\
	$(CROSS_COMPILE)strip hostapd hostapd_cli;	\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) 	\
	cp hostapd hostapd_cli $(IMAGE_PATH_TOOL)

hostapd_clean:
	cd $(HOSTAPD_PATH)/hostapd;     \
    make clean
	cd $(IMAGE_PATH_TOOL);       \
	rm -f hostapd hostapd_cli
	
wpa_supplicant:
	cd $(WPA_SUPPLICANT_PATH)/wpa_supplicant;	\
	make all;					\
	$(CROSS_COMPILE)strip wpa_supplicant wpa_passphrase wpa_cli;	\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL))	\
	cp wpa_supplicant wpa_passphrase wpa_cli $(IMAGE_PATH_TOOL)

wpa_supplicant_clean:
	cd $(WPA_SUPPLICANT_PATH)/wpa_supplicant;       \
	make clean
	cd $(IMAGE_PATH_TOOL);       \
	rm -f wpa_supplicant wpa_passphrase wpa_cli
	
app_sdt:
	cd $(SDT_APP_PATH);		\
	echo $(SDT_APP_PATH);		\
	make;				\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) \
	$(CROSS_COMPILE)strip app_sdt;	\
	cp app_sdt  $(IMAGE_PATH_TOOL)

app_sdt_clean:
	cd $(SDT_APP_PATH);             \
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) \
	make clean;			
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_sdt

app_vamp:
	cd $(VAMP_APP_PATH);		\
	echo $(VAMP_APP_PATH);		\
	make;				\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) \
	$(CROSS_COMPILE)strip app_vamp vamp_cli;	\
	cp app_vamp vamp_cli $(IMAGE_PATH_TOOL)

app_vamp_clean:
	cd $(VAMP_APP_PATH);             \
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) \
	make clean;			
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_vamp vamp_cli
	
app_daq:
	cd $(DAQ_APP_PATH);		\
	echo $(DAQ_APP_PATH);		\
	make;				\
	$(CROSS_COMPILE)strip app_daq;	\
	cp app_daq  $(IMAGE_PATH_TOOL)

app_daq_clean:
	cd $(DAQ_APP_PATH);             \
	make clean;	
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_daq
	
app_nlc:
	cd $(NLC_APP_PATH);		\
	echo $(NLC_APP_PATH);		\
	make;				\
	$(CROSS_COMPILE)strip app_nlc;	\
	cp app_nlc  $(IMAGE_PATH_TOOL)

app_nlc_clean:
	cd $(NLC_APP_PATH);             \
	make clean;	
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_nlc	

app_acs:
	cd $(ACS_APP_PATH);		\
        echo $(ACS_APP_PATH);		\
	make;				\
	$(CROSS_COMPILE)strip app_acs;	\
	cp app_acs  $(IMAGE_PATH_TOOL)

app_acs_clean:
	cd $(ACS_APP_PATH);             \
	make clean;	
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_acs	

app_reg:
	cd $(REG_APP_PATH);		\
	echo $(REG_APP_PATH);		\
	make;				\
	$(CROSS_COMPILE)strip app_reg;	\
	cp app_reg  $(IMAGE_PATH_TOOL)

app_reg_clean:
	cd $(REG_APP_PATH);             \
	make clean;	
	cd $(IMAGE_PATH_TOOL);       \
	rm -f app_reg


libpcap:
	cd $(LIBPCAP_PATH);	\
	make all;		\
	make install;		

libpcap_clean:
	cd $(LIBPCAP_PATH);       \
	make clean
	cd $(LIBPCAP_INSTALL_PATH);       \
	rm -f ./lib/libpcap.a
	
tcpdump:
	cd $(TCPDUMP_PATH);	\
	make all;		\
	$(CROSS_COMPILE)strip tcpdump;	\
	make install;	\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) 	\
	cp $(TCPDUMP_INSTALL_PATH)/sbin/tcpdump $(IMAGE_PATH_TOOL)
	
tcpdump_clean:
	cd $(TCPDUMP_PATH);       \
	make clean
	cd $(TCPDUMP_INSTALL_PATH);       \
	rm -f ./sbin/tcpdump
	cd $(IMAGE_PATH_TOOL);		\
	rm -f tcpdump
	
tcpdump_mgr:
	cd $(TCPDUMP_MGR_PATH);	\
	make;		\
	$(CROSS_COMPILE)strip tcpdump_mgr;	\
	$(shell [ -d $(IMAGE_PATH_TOOL) ] || mkdir -p $(IMAGE_PATH_TOOL)) 	\
	cp tcpdump_mgr  $(IMAGE_PATH_TOOL)

tcpdump_mgr_clean:
	cd $(TCPDUMP_MGR_PATH);       \
	make clean
	cd $(IMAGE_PATH_TOOL);			\
	rm -f tcpdump_mgr
