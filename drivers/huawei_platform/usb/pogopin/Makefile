# Switch Class Driver
DWCPATH= drivers/usb/susb/dwc_common_port
DWCSUSBPATH= drivers/usb/susb/
SCHARGERPATH= drivers/power/hisi/charger/schargerV300/
ccflags-y	+= -DDEBUG

ccflags-y	+= -Dlinux -DDWC_HS_ELECT_TST
ccflags-y   += -I$(DWCPATH)
ccflags-y   += -I$(DWCSUSBPATH)
ccflags-y   += -I$(SCHARGERPATH)
ccflags-y   += -DDWC_LINUX
ccflags-y	+= -DLM_INTERFACE

obj-y   += huawei_pogopin.o
obj-y 	+= pogo_otg_id.o
