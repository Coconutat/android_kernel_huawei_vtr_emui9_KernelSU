KCOV_INSTRUMENT := n
CONFIG_TEE_SMP := y

ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DDEBUG_TUI
ccflags-y += -DDEF_ENG
endif

ccflags-y +=-DDYNAMIC_ION
ifeq ($(CONFIG_TEE_SMP), y)
ccflags-y += -DCONFIG_TEE_SMP
obj-$(CONFIG_TZDRIVER) += smc_smp.o
else
obj-$(CONFIG_TZDRIVER) += smc.o
endif

obj-$(CONFIG_TZDRIVER) += tc_client_driver.o teek_client_api.o mailbox_mempool.o
obj-$(CONFIG_TZDRIVER) += security_auth_enhance.o agent.o gp_ops.o mem.o cmdmonitor.o tzdebug.o tz_spi_notify.o  dynamic_mem.o
obj-$(CONFIG_TEELOG) += tlogger.o tee_rdr_register.o
ifeq ($(CONFIG_TZDRIVER), y)
obj-$(CONFIG_HISI_MMC_SECURE_RPMB) += agent_rpmb.o
endif

obj-$(CONFIG_TEE_TUI) += tui.o

obj-$(CONFIG_TEE_LIBHWSECUREC) += libhwsecurec/

obj-$(CONFIG_TEE_CFC) += cfc_data.o cfc_coresight.o
$(obj)/cfc_data.o: $(obj)/cfc_data.c
$(obj)/cfc_data.c: $(src)/cfc_graphgen.py $(src)/cfc_graph.py $(src)/cfc_codegen.py $(src)/cfc_rule_parser.py $(src)/template $(obj)/rule.out
	+python $(srctree)/$(src)/cfc_codegen.py $(obj)/rule.out > $@
$(obj)/rule.out: $(src)/rule.in
	$(CROSS_COMPILE)cpp $(srctree)/$(src)/rule.in > $@


ccflags-y += 	-Idrivers/video/hisi/dss \
		-Idrivers/video/fbdev/hisi/dss \
		-Idrivers/staging/android \
		-Idrivers/hisi/tzdriver/libhwsecurec

ifdef CONFIG_TEE_CFC
  rename-text-section-to := .cfc.text
endif
