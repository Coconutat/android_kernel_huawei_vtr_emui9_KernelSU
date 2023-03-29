ifeq ($(CONFIG_HIFI_MAILBOX), y)
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_dsp
endif

ifeq ($(CFG_CONFIG_HISI_FAMA),true)
LOCAL_CFLAGS += -DCONFIG_HISI_FAMA
endif

ccflags-$(CONFIG_USB_PROXY_HCD_DEBUG)	:= -DDEBUG
ccflags-y += -Idrivers/devkit/audiokit/

obj-$(CONFIG_USB_PROXY_HCD)		+= usbaudio-monitor.o

obj-$(CONFIG_USB_PROXY_HCD)		+= hifiusb.o
hifiusb-y				+= hifi-usb.o hifi-usb-urb-buf.o
hifiusb-$(CONFIG_HISI_DEBUG_FS)		+= hifi-usb-debugfs.o
hifiusb-$(CONFIG_HIFI_MAILBOX)		+= hifi-usb-mailbox.o

obj-$(CONFIG_USB_PROXY_HCD)		+= phcd.o
phcd-y					:= proxy-hcd.o proxy.o
phcd-$(CONFIG_HISI_DEBUG_FS)		+= proxy-hcd-debugfs.o
