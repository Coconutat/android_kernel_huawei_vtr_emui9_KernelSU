# define_trace.h needs to know how to find our header
CFLAGS_trace.o				:= -I$(src)

ccflags-$(CONFIG_USB_DWC3_FEB)	+= -DDWC3_OTG_FORCE_RESET_GCTL
ccflags-$(CONFIG_USB_DWC3_MAR)	+= -DDWC3_USB_HOST_IN_IMMRETRY_EN -DDWC3_DEV_P4_PG_EN -DDWC3_RX_TRES_CFG_DIS -DDWC3_OTG_FORCE_RESET_GCTL
ccflags-$(CONFIG_USB_DWC3_APR)	+= -DDWC3_OTG_FORCE_RESET_GCTL

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DHISI_PLUGUSB_TEST
else
ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DHISI_PLUGUSB_TEST
endif
endif

obj-$(CONFIG_USB_DWC3)			+= dwc3.o

dwc3-y					:= core.o debug.o trace.o

ifneq ($(filter y,$(CONFIG_USB_DWC3_HOST) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= host.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_GADGET) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= gadget.o ep0.o
endif

ifneq ($(CONFIG_USB_DWC3_ULPI),)
	dwc3-y				+= ulpi.o
endif

ifneq ($(CONFIG_DEBUG_FS),)
	dwc3-y				+= debugfs.o
endif

dwc3-$(CONFIG_USB_DWC3_OTG)		+= dwc3-otg.o

##
# Platform-specific glue layers go here
#
# NOTICE: Make sure your glue layer doesn't depend on anything
# which is arch-specific and that it compiles on all situations.
#
# We want to keep this requirement in order to be able to compile
# the entire driver (with all its glue layers) on several architectures
# and make sure it compiles fine. This will also help with allmodconfig
# and allyesconfig builds.
##

#obj-$(CONFIG_USB_DWC3_OMAP)		+= dwc3-omap.o
#obj-$(CONFIG_USB_DWC3_EXYNOS)		+= dwc3-exynos.o
#obj-$(CONFIG_USB_DWC3_PCI)		+= dwc3-pci.o
#obj-$(CONFIG_USB_DWC3_KEYSTONE)		+= dwc3-keystone.o
#obj-$(CONFIG_USB_DWC3_OF_SIMPLE)	+= dwc3-of-simple.o
#obj-$(CONFIG_USB_DWC3_ST)		+= dwc3-st.o


usb-vbus-y  := hisi_usb_vbus.o
ifeq ($(CONFIG_USB_DWC3_VBUS_PLT),y)
	usb-vbus-y    += hisi_usb_vbus_plt.o
endif

ifeq ($(CONFIG_USB_DWC3_VBUS_SPMI),y)
	usb-vbus-y    += hisi_usb_vbus_spmi.o
endif

obj-$(CONFIG_USB_DWC3_NOV)		+= dwc3-nov.o hisi_usb2_bc.o
obj-$(CONFIG_USB_DWC3_OCT)		+= dwc3-oct.o hisi_usb2_bc.o
# hisi_usb3_31phy must before dwc3-plat(e.g mar,apr), module_init called base on the order in Makefile
obj-$(CONFIG_USB_DWC3_APR)		+= hisi_usb3_31phy_v2.o dwc3-apr.o hisi_usb3_bc.o hisi_usb3_misctrl.o
obj-$(CONFIG_USB_DWC3_MAR)		+= hisi_usb3_31phy.o dwc3-mar.o hisi_usb3_bc.o hisi_usb3_misctrl.o
obj-$(CONFIG_USB_DWC3_FEB)		+= hisi_usb3_31phy.o dwc3-feb.o hisi_usb3_bc.o hisi_usb3_misctrl.o
obj-$(CONFIG_USB_DWC3_JAN)		+= dwc3-jan.o hisi_usb3_bc.o
obj-$(CONFIG_USB_DWC3_HISI)		+= dwc3-hisi.o usb-vbus.o

ifneq ($(CFG_CONFIG_HISI_FAMA),true)
obj-$(CONFIG_USB_DWC3_NYET_ABNORMAL)		+= dwc3-hifi-usb.o
endif

ifeq ($(CONFIG_HIFI_MAILBOX), y)
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_dsp
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_MAR) $(CONFIG_USB_DWC3_APR)),)
EXTRA_CFLAGS += -DCOMBOPHY_VERSION_1_1
EXTRA_CFLAGS += -DCOMBOPHY_FW_UPDATE
endif

obj-$(CONFIG_CONTEXTHUB_PD) += pd.o
