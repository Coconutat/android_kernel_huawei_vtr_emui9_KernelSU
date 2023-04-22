# hisilicon ivp

ccflags-$(CONFIG_IVP_IVPV160) += -Idrivers/hisi/ivp/ivpv160
ccflags-$(CONFIG_IVP_IVPV200) += -Idrivers/hisi/ivp/ivpv200
ccflags-$(CONFIG_IVP_IVPV150) += -Idrivers/hisi/ivp/ivpv150
ccflags-$(CONFIG_IVP_IVPV100) += -Idrivers/hisi/ivp/ivpv100
ccflags-$(CONFIG_IVP_IVPV120) += -Idrivers/hisi/ivp/ivpv120
ccflags-$(CONFIG_IVP_IVPV210) += -Idrivers/hisi/ivp/ivpv210
ccflags-$(CONFIG_IVP_IVPV250) += -Idrivers/hisi/ivp/ivpv250

obj-$(CONFIG_IVP_SMMU)      += ivp_smmu.o
obj-$(CONFIG_HISI_IVP) += ivp_ipc.o
obj-$(CONFIG_HISI_IVP) += ivp_core.o
obj-$(CONFIG_IVP_IVPV200) += ivp_sec.o
obj-$(CONFIG_IVP_IVPV250) += ivp_sec.o
obj-$(CONFIG_IVP_IVPV160) += ivpv160/
obj-$(CONFIG_IVP_IVPV200) += ivpv200/
obj-$(CONFIG_IVP_IVPV150) += ivpv150/
obj-$(CONFIG_IVP_IVPV100) += ivpv100/
obj-$(CONFIG_IVP_IVPV120) += ivpv120/
obj-$(CONFIG_IVP_IVPV210) += ivpv210/
obj-$(CONFIG_IVP_IVPV250) += ivpv250/

ifneq (, $(filter eng, $(TARGET_BUILD_VARIANT)))
	EXTRA_CFLAGS += -DENG_BUILDMODE
endif
ifeq ($(CONFIG_IVP_IVPV200), y)
subdir-ccflags-y += -DSEC_IVP_ENABLE
endif
ifeq ($(CONFIG_IVP_IVPV250), y)
subdir-ccflags-y += -DIVP_QOS_SUPPORT
endif
subdir-ccflags-y += -I$(srctree)/drivers/hisi/tzdriver \
                    -I$(srctree)/drivers/hisi/load_image
