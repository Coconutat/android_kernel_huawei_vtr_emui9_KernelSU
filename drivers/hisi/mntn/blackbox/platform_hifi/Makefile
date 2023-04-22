obj-y += rdr_hisi_audio_adapter.o
obj-y += rdr_hisi_audio_codec.o
obj-y += rdr_hisi_audio_soc.o

EXTRA_CFLAGS += -Idrivers/hisi/hifi_dsp/
EXTRA_CFLAGS += -Idrivers/hisi/hi64xx_dsp/
EXTRA_CFLAGS += -Idrivers/hisi/mntn/blackbox/
EXTRA_CFLAGS += -Isound/usb/hifi/

ccflags-y += -I$(srctree)/drivers/hisi/tzdriver

EXTRA_CFLAGS += -DSECOS_RELOAD_HIFI
EXTRA_CFLAGS += -DRDR_SOCHIFI
EXTRA_CFLAGS += -DRDR_CODECDSP

ccflags-y += -Idrivers/devkit/audiokit/
