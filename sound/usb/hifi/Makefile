subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS +=	-Isound/usb/
ccflags-y += -Idrivers/devkit/audiokit/
obj-y += usbaudio_dsp_client.o \
		usbaudio_ctrl.o
ifeq ($(CLT_IV), true)
obj-y += usbaudio_test.o
endif

ifeq ($(CLT_IV), true)
EXTRA_CFLAGS += -DCLT_AUDIO
endif

ifeq ($(CONFIG_USB_AUDIO_DTSI_PROPERTY), y)
obj-y += usbaudio_dtsi_property.o
endif
