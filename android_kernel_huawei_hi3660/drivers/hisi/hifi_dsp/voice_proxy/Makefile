# hifi dsp device driver
ifeq ($(CONFIG_HIFI_IPC_3650),y)
EXTRA_CFLAGS += -DPLATFORM_HI3XXX
EXTRA_CFLAGS += -DPLATFORM_HI365X
endif

ifeq ($(CONFIG_HIFI_IPC_6250),y)
EXTRA_CFLAGS += -DPLATFORM_HI3XXX
EXTRA_CFLAGS += -DPLATFORM_HI6250
#todo fixme: hi6250(PLATFORM_FPGA should be deleted on asic)
EXTRA_CFLAGS += -DPLATFORM_FPGA
endif

EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/ipcm

ifeq ($(CONFIG_HISI_AUDIO_DEBUG),y)
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif

EXTRA_CFLAGS += -Idrivers/hisi/hifi_dsp

obj-$(CONFIG_VOICE_PROXY) += voice_proxy.o
obj-$(CONFIG_VOICE_PROXY_VOLTE) += voice_proxy_volte.o
obj-$(CONFIG_VOICE_PROXY_VOWIFI) += voice_proxy_vowifi.o
obj-$(CONFIG_VOICE_PROXY_RTT) += voice_proxy_rtt.o
