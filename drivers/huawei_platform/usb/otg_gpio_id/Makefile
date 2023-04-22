# Switch Class Driver
DWCPATH= drivers/usb/susb/dwc_common_port
DWCSUSBPATH= drivers/usb/susb/
ccflags-y	+= -DDEBUG

ccflags-y	+= -Dlinux -DDWC_HS_ELECT_TST
ccflags-y   += -I$(DWCPATH)
ccflags-y   += -I$(DWCSUSBPATH)
ccflags-y   += -DDWC_LINUX
ccflags-y	+= -DLM_INTERFACE

obj-$(CONFIG_OTG_GPIO_ID)	+= hw_otg_id.o
