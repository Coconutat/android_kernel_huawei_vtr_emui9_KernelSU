# hisilicon ivp ivpv200
ifeq ($(es_low_freq),true)
ccflags-$(CONFIG_IVP_IVPV200) += -DCONFIG_ES_LOW_FREQ
endif

ccflags-$(CONFIG_IVP_IVPV200) += -Idrivers/hisi/ivp/

obj-$(CONFIG_IVP_IVPV200) += ivp_platform.o
obj-$(CONFIG_IVP_IVPV200) += ivp_atf_subsys.o