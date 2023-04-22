obj-$(CONFIG_HISI_DEVFREQ)      += hisi_devfreq.o
obj-$(CONFIG_DEVFREQ_GOV_PM_QOS)    += governor_pm_qos.o
obj-$(CONFIG_HISI_DDR_DEVFREQ)      += ddr_devfreq.o
obj-$(CONFIG_DEVFREQ_GOV_MALI_ONDEMAND)      += governor_maliondemand.o
obj-$(CONFIG_DEVFREQ_GOV_GPU_SCENE_AWARE)    += governor_gpu_scene_aware.o
obj-$(CONFIG_DEVFREQ_GOV_NPU_PM_QOS)    += governor_npu_pm_qos.o
obj-$(CONFIG_ARM_MEMLAT_MON)    += arm-memlat-mon.o
obj-$(CONFIG_DEVFREQ_GOV_MEMLAT)    += governor_memlat.o
obj-$(CONFIG_HISI_DEVFREQ_DEVBW)    += devfreq_devbw.o

obj-$(CONFIG_ARM64_DSU_PCTRL_DEVFREQ)      += dsu_pctrl/
obj-$(CONFIG_HISI_L3C_DEVFREQ)      	   += l3c_devfreq/

obj-$(CONFIG_HISI_L3P_KARMA) += l3p_karma/

ccflags-$(CONFIG_HISI_DEVFREQ)  += -Idrivers/devfreq \
				   -I$(srctree)/drivers/hisi/tzdriver/libhwsecurec
