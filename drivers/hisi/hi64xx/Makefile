subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -Idrivers/hisi/hi64xx_dsp/
EXTRA_CFLAGS += -Idrivers/hisi/soundtrigger/
EXTRA_CFLAGS += -Isound/soc/codecs/hisi/hi6xxx_fpga_test/
EXTRA_CFLAGS += -Isound/soc/codecs/hisi/hs_auto_calib/

ifeq ($(CLT_IV), true)
EXTRA_CFLAGS += -DCLT_AUDIO
endif

ifeq ($(CONFIG_HI6402_MBHC_ANC),y)
EXTRA_CFLAGS += -DANC_HEADSET
EXTRA_CFLAGS += -DINVERT_HEADSET
endif

ifeq ($(CONFIG_HISI_AUDIO_DEBUG),y)
EXTRA_CFLAGS += -DCONFIG_HUAWEI_HEADSET_DEBUG
EXTRA_CFLAGS += -Idrivers/devkit/audiokit/headset_debug/
ifeq ($(kcov),true)
EXTRA_CFLAGS += -DKCOV_AUDIO
else
EXTRA_CFLAGS += -DENABLE_HI64XX_CODEC_DEBUG
endif
#ifeq ($(CONFIG_SND_SOC_CODEC_STUB),y)
#EXTRA_CFLAGS += -DHI6XXX_FPGA_CODEC_STUB
#endif
endif

ccflags-y += -Idrivers/devkit/audiokit/

obj-y += hi_cdc_ctrl.o
obj-y += hi_cdc_ssi.o
obj-y += hi64xx_resmgr.o
obj-y += hi64xx_mbhc.o
obj-y += hi64xx_irq.o
obj-y += hi64xx_compat.o
obj-y += hi64xx_utils.o
obj-y += hi64xx_vad.o
obj-y += asp_dma.o
