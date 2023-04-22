ifeq ($(es_low_freq),true)
EXTRA_CFLAGS += -DCONFIG_ES_VDEC_LOW_FREQ
endif

obj-$(CONFIG_HISI_ICS_IPU) += cambricon_ipu.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_smmu_drv.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_clock.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_mntn.o

obj-$(CONFIG_HISI_NPU_V200) += npu_main.o
obj-$(CONFIG_HISI_NPU_V200) += npu_task.o
obj-$(CONFIG_HISI_NPU_V200) += npu_task_wq.o
obj-$(CONFIG_HISI_NPU_V200) += npu_power.o
obj-$(CONFIG_HISI_NPU_V200) += npu_regulator.o
obj-$(CONFIG_HISI_NPU_V200) += npu_clock.o
obj-$(CONFIG_HISI_NPU_V200) += npu_smmu.o
obj-$(CONFIG_HISI_NPU_V200) += npu_mntn.o
