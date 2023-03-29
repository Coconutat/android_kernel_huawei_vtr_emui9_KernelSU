subdir-ccflags-y += -Wall -Werror

ifeq ($(CLT_IV),true)
EXTRA_CFLAGS += -DCLT_VOICE
endif

# hifi dsp device driver
ifeq ($(CONFIG_HIFI_IPC_3650),y)
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi365x
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi365x
endif

ifeq ($(CFG_CONFIG_HISI_FAMA),true)
LOCAL_CFLAGS += -DCONFIG_HISI_FAMA
endif

ifeq ($(CONFIG_HISI_FB_970),y)
EXTRA_CFLAGS += -DCONFIG_DP_MULTI_CHANNELS
endif

ifeq ($(CONFIG_HISI_FB_V501),y)
EXTRA_CFLAGS += -DCONFIG_DP_MULTI_CHANNELS
endif

ifeq ($(CONFIG_HIFI_IPC_3660),y)
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi365x
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi365x
endif
ifeq ($(CONFIG_HIFI_IPC_6250),y)
EXTRA_CFLAGS += -DPLATFORM_HI6250
EXTRA_CFLAGS += -DEXPAND_HIFI_RUNNING_REGION
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi62x0
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi62x0
endif

ifeq ($(CONFIG_SND_SOC_HI6402),y)
EXTRA_CFLAGS += -DMULTI_MIC
endif

EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/ipcm
EXTRA_CFLAGS += -Isound/usb/hifi/
ifeq ($(CONFIG_HISI_AUDIO_DEBUG),y)
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DENABLE_AUDIO_KCOV
else
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif
endif

obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= hifi_lpp.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= hifi_om.o

ifeq ($(CONFIG_HISI_AUDIO_DEBUG),y)
obj-$(CONFIG_SND_SOC_CODEC_STUB)	+= memcpy_opt.o
ifeq ($(CONFIG_SND_SOC_CODEC_STUB),y)
EXTRA_CFLAGS += -DDYNAMIC_LOAD_HIFIIMG
endif
endif

obj-y	+= voice_proxy/
ccflags-$(CONFIG_HIFI_DSP_ONE_TRACK) += -Idrivers/devkit/audiokit/
