#ifdef CONFIG_CONTEXTHUB_SHMEM
ccflags-y  += -I$(srctree)/drivers/contexthub/
#endif

# contexthub Driver
obj-$(CONFIG_INPUTHUB_20)		+= contexthub_boot.o
obj-$(CONFIG_INPUTHUB_20)		+= contexthub_route.o
obj-$(CONFIG_INPUTHUB_20)		+= sensor_config.o
obj-$(CONFIG_INPUTHUB_20)       += contexthub_logbuff.o
obj-$(CONFIG_INPUTHUB_20)       += contexthub_debug.o
obj-$(CONFIG_INPUTHUB_20)	+= sensor_channel.o
obj-$(CONFIG_INPUTHUB_20)  += motion_channel.o
obj-$(CONFIG_INPUTHUB_20)  += sensor_detect.o
obj-$(CONFIG_INPUTHUB_20)  += sensor_sysfs.o
obj-$(CONFIG_INPUTHUB_20)  += sensor_feima.o
obj-$(CONFIG_INPUTHUB_20)  += contexthub_recovery.o
obj-$(CONFIG_INPUTHUB_20)  += contexthub_pm.o
obj-$(CONFIG_INPUTHUB_20)  += ca_channel.o
obj-$(CONFIG_INPUTHUB_20)  += fingerprint_channel.o
obj-$(CONFIG_INPUTHUB_20)  += contexthub_ext_log.o
obj-$(CONFIG_INPUTHUB_20)  += fingerprint_ud_channel.o
obj-$(CONFIG_INPUTHUB_20)  += vibrator_channel.o
obj-$(CONFIG_INPUTHUB_20)  += big_data_channel.o