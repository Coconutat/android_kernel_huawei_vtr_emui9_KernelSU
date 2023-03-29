# hifi dsp device driver
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -Idrivers/hisi/slimbus
EXTRA_CFLAGS += -Idrivers/hisi/hifi_dsp
EXTRA_CFLAGS +=	-Iinclude/linux/hisi/hi64xx/
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_dsp

obj-$(CONFIG_SOUND_TRIGGER)	+= mlib_ringbuffer.o
obj-$(CONFIG_SOUND_TRIGGER)	+= mlib_static_ringbuffer.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_dma_drv.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_event.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_socdsp_mailbox.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_socdsp_pcm.o

