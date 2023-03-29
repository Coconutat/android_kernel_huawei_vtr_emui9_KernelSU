ccflags-y += -Idrivers/media/huawei/camera
ccflags-y += -Idrivers/rpmsg

obj-y += hisp.o
CLT_CAMERA := n
ifeq ($(CLT_IV), true)
CLT_CAMERA := n
endif

ifeq ($(CLT_CAMERA), y)
obj-y += simulated_fw/

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV150), y)
subdir-ccflags-y += -DHISP150_CAMERA
endif

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV160), y)
subdir-ccflags-y += -DHISP160_CAMERA
endif

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV120), y)
subdir-ccflags-y += -DHISP120_CAMERA
endif

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV110), y)
	obj-y += hisp110/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV150), y)
EXTRA_CFLAGS += -DHISP150_CAMERA
	obj-y += hisp150/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV160), y)
EXTRA_CFLAGS += -DHISP160_CAMERA
	obj-y += hisp160/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV120), y)
EXTRA_CFLAGS += -DHISP120_CAMERA
	obj-y += hisp120/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV200), y)
EXTRA_CFLAGS += -DHISP200_CAMERA
	obj-y += hisp200/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV210), y)
EXTRA_CFLAGS += -DHISP210_CAMERA
	obj-y += hisp210/

else ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV250), y)
EXTRA_CFLAGS += -DHISP250_CAMERA
	obj-y += hisp250/
endif



ifneq ($(CLT_IV), true)
ifneq ($(CONFIG_RPMSG),y)
	obj-y += fakefw/
endif
endif


