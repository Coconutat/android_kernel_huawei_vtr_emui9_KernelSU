# hisilicon ivp ivpv250
ifeq ($(es_low_freq),true)
ccflags-$(CONFIG_IVP_IVPV250) += -DCONFIG_ES_LOW_FREQ
endif

ccflags-$(CONFIG_IVP_IVPV250) += -Idrivers/hisi/ivp/

obj-$(CONFIG_IVP_IVPV250) += ivp_platform.o
obj-$(CONFIG_IVP_IVPV250) += ivp_atf_subsys.o
