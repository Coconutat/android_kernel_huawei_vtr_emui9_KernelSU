HI110X_DRIVER_BUILTIN_PATH ?= drivers/connectivity/hi11xx
DRIVER_FLODER = platform

export _PRE_PRODUCT_VERSION ?= 1103
export HI110X_BOARD_VERSION ?= default
export PLAT_DEFCONFIG_FILE  ?= plat_$(_PRE_PRODUCT_VERSION)_$(HI110X_BOARD_VERSION)_defconfig

HI110X_COMM_DEFCONFIG := hi$(_PRE_PRODUCT_VERSION)_comm_defconfig

CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT ?= no

ifneq ($(KERNELRELEASE),)
ifeq ($(HI1102_MAKE_FLAG),)
KERNEL_DIR := $(srctree)
HI110X_DRIVER_SRC_PATH=$(KERNEL_DIR)/$(HI110X_DRIVER_BUILTIN_PATH)/hi$(_PRE_PRODUCT_VERSION)
endif

DRIVER_PATH=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER)

include $(DRIVER_PATH)/$(HI110X_COMM_DEFCONFIG)
include $(DRIVER_PATH)/$(PLAT_DEFCONFIG_FILE)

#$(warning defconfig: $(DRIVER_PATH)/$(PLAT_DEFCONFIG_FILE))

oal-objs := oal_bus_if.o oal_main.o oal_mem.o oal_net.o oal_hardware.o oal_schedule.o oal_pci_if.o oal_aes.o oal_kernel_file.o oal_cfg80211.o oal_sdio_host.o oal_profiling.o oal_hcc_host.o oal_hcc_bus.o oal_hcc_test.o oal_softwdt.o oal_dft.o oal_workqueue.o oal_pcie_host.o oal_pcie_linux.o oal_pcie_pm.o oal_fsm.o oal_pcie_firmware.o

oal-objs := $(addprefix oal/,$(oal-objs))

frw-objs := frw_event_deploy.o frw_event_main.o frw_event_sched.o frw_ipc_msgqueue.o frw_main.o frw_task.o frw_timer.o
frw-objs := $(addprefix frw/,$(frw-objs))

oam-objs := oam_main.o oam_alarm.o oam_event.o oam_log.o oam_trace.o oam_statistics.o oam_config.o oam_register.o oam_linux_netlink.o oam_rdr.o
oam-objs := $(addprefix oam/,$(oam-objs))

sdt-objs := sdt_drv.o
sdt-objs := $(addprefix sdt/,$(sdt-objs))

customize-objs := hisi_ini.o hisi_customize_wifi_hi110x.o
customize-objs := $(addprefix ../common/customize/,$(customize-objs))

chr_devs-objs := chr_devs.o
chr_devs-objs := $(addprefix ../common/chr_log/,$(chr_devs-objs))

main-objs := plat_main.o
main-objs := $(addprefix main/,$(main-objs))

pm-objs :=  plat_firmware.o plat_pm.o  plat_pm_wlan.o plat_cali.o plat_efuse.o

ifeq ($(BFGX_UART_DOWNLOAD_SUPPORT),yes)
KBUILD_CFLAGS += -DBFGX_UART_DOWNLOAD_SUPPORT
ifeq ($(GNSS_ONLY_ASIC_CHIP),yes)
$(warning *******GNSS ONLY IS ASIC  work*********)
KBUILD_CFLAGS += -DGNSS_ONLY_ASIC_CHIP
endif
$(warning *******BFGX_UART_DOWNLOAD_SUPPORT work*********)
pm-objs += wireless_patch.o
endif

ifeq ($(CONFIG_HI110X_GPS_REFCLK),y)
ifeq ($(CONFIG_HI110X_GPS_REFCLK_SRC_1),y)
pm-objs += gps_refclk_src_1.o
endif
ifeq ($(CONFIG_HI110X_GPS_REFCLK_SRC_2),y)
pm-objs += gps_refclk_src_2.o
endif
ifeq ($(CONFIG_HI110X_GPS_REFCLK_SRC_3),y)
pm-objs += gps_refclk_src_3.o
endif
endif

pm-objs := $(addprefix pm/,$(pm-objs))

driver-objs :=  plat_sdio.o plat_uart.o
driver-objs := $(addprefix driver/,$(driver-objs))

bfgx-objs := bfgx_low_power.o bfgx_exception_rst.o bfgx_data_parse.o bfgx_dev.o bfgx_user_ctrl.o
bfgx-objs := $(addprefix ../bfgx/,$(bfgx-objs))

board-objs := board.o board-hi1102.o board-hi1103.o
board-objs := $(addprefix board/,$(board-objs))

oneimage-objs := oneimage.o
oneimage-objs := $(addprefix ../common/oneimage/,$(oneimage-objs))

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oal-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(frw-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oam-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(sdt-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(pm-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(main-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(bfgx-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(chr_devs-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(board-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(oneimage-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(customize-objs)

plat_$(_PRE_PRODUCT_VERSION)-objs += $(driver-objs)

#plat ko
ifeq ($(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT), yes)
obj-m += plat_$(_PRE_PRODUCT_VERSION).o
else
obj-y += plat_$(_PRE_PRODUCT_VERSION).o
endif

HI110X_INCLUDES := -I$(DRIVER_PATH)/inc
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/frw
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oal
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oal/linux
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/oam
HI110X_INCLUDES += -I$(DRIVER_PATH)/inc/pm
HI110X_INCLUDES += -I$(DRIVER_PATH)/frw
HI110X_INCLUDES += -I$(DRIVER_PATH)/oal
HI110X_INCLUDES += -I$(DRIVER_PATH)/oam
HI110X_INCLUDES += -I$(DRIVER_PATH)/pm
HI110X_INCLUDES += -I$(DRIVER_PATH)/../bfgx
HI110X_INCLUDES += -I$(DRIVER_PATH)/sdt
HI110X_INCLUDES += -I$(DRIVER_PATH)/board
HI110X_INCLUDES += -I$(DRIVER_PATH)/driver
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/oneimage
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/customize
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/inc
HI110X_INCLUDES += -I$(DRIVER_PATH)/../common/chr_log

#This is not good
HI110X_INCLUDES += -I$(KERNEL_DIR)
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/inc
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/inc/hal
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/inc/dmac
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/inc/wal
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/hal/hal_verify/
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/hmac/hmac_verify/
HI110X_INCLUDES += -I$(HI110X_DRIVER_SRC_PATH)/wifi/1103/dmac/dmac_verify/

KBUILD_CFLAGS += -DPLATFORM_TYPE_FOR_HI110X=0
KBUILD_CFLAGS += -DPLATFORM_HI110X_k3v2oem1=1
KBUILD_CFLAGS += -DPLATFORM_HI110X_UEDGE=2

ifneq ($(TARGET_BUILD_VARIANT),user)
KBUILD_CFLAGS += -DPLATFORM_DEBUG_ENABLE
KBUILD_CFLAGS += -DPLATFORM_SSI_FULL_LOG
endif

KBUILD_CFLAGS += -DHI1XX_ANDROID_BUILD_VARIANT_ROOT=1
KBUILD_CFLAGS += -DHI1XX_ANDROID_BUILD_VARIANT_USER=2
ifneq ($(TARGET_BUILD_VARIANT),user)
KBUILD_CFLAGS += -DANDROID_HI1XX_BUILD_VERSION=HI1XX_ANDROID_BUILD_VARIANT_ROOT
else
KBUILD_CFLAGS += -DANDROID_HI1XX_BUILD_VERSION=HI1XX_ANDROID_BUILD_VARIANT_USER
endif

ifneq ($(HAVE_HISI_FEATURE_BT),false)
KBUILD_CFLAGS += -DHAVE_HISI_BT
endif

ifneq ($(HAVE_HISI_FEATURE_FM),false)
KBUILD_CFLAGS += -DHAVE_HISI_FM
endif

ifneq ($(HAVE_HISI_FEATURE_GNSS),false)
KBUILD_CFLAGS += -DHAVE_HISI_GNSS
endif

ifneq ($(HAVE_HISI_FEATURE_IR),false)
KBUILD_CFLAGS += -DHAVE_HISI_IR
endif

#ifneq ($(HAVE_HISI_FEATURE_NFC),false)
#KBUILD_CFLAGS += -DHAVE_HISI_NFC
#endif

HI110X_VER_COMMIT_ID = $(shell cd $(HI110X_DRIVER_SRC_PATH);if test -d .git;then git rev-parse --verify --short HEAD 2>/dev/null;fi)
ifneq ($(HI110X_VER_COMMIT_ID),)
HI110X_VER_COMMIT_TIME = $(shell cd $(HI110X_DRIVER_SRC_PATH);git log -1 --pretty=format:%ci)

HI110X_VERSION="\"commitId:$(HI110X_VER_COMMIT_ID), commitTime:$(HI110X_VER_COMMIT_TIME)\""

#ifeq ($(HI110X_KERNEL_MODULES_BUILD_VERSION), y)
KBUILD_CFLAGS += -DHI110X_DRV_VERSION=$(HI110X_VERSION)
#endif
endif
#$(warning HI110X_VER_COMMIT_ID: $(HI110X_VER_COMMIT_ID))
#$(warning HI110X_VER_COMMIT_TIME: $(HI110X_VER_COMMIT_TIME))
#$(warning HI110X_VERSION: $(HI110X_VERSION))
#$(warning INCLUDE: $(HI110X_INCLUDES))

EXTRA_CFLAGS = $(HI110X_INCLUDES)
EXTRA_CFLAGS += $(COMM_COPTS)
EXTRA_CFLAGS += $(COPTS) $(KBUILD_CFLAGS)
ifeq ($(CONN_NON_STRICT_CHECK),y)
$(warning hisi connectivity driver compile strict check disable)
else
#EXTRA_CFLAGS += -Werror -Wuninitialized -Wempty-body -Wclobbered -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wtype-limits
EXTRA_CFLAGS += -Wuninitialized -Wempty-body -Wclobbered -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wtype-limits
endif

EXTRA_CFLAGS +=  -fno-pic
MODFLAGS = -fno-pic
else

#make modules
export HI1102_MAKE_FLAG=MODULES
export HI110X_DRIVER_SRC_PATH ?= $(shell pwd)/..

ANDROID_PRODUCT=hi6210sft
ifeq ($(ARCH),arm64)
ANDROID_PATH ?= /home/dengwenhua/zhouxinfeng/k3v5
MODULE_PARAM ?= ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
else
ANDROID_PATH ?= /home/zhouxinfeng/1102/v8r2-checkin
MODULE_PARAM ?= ARCH=arm CROSS_COMPILE=arm-eabi-
endif

KERNEL_DIR ?= $(ANDROID_PATH)/out/target/product/$(ANDROID_PRODUCT)/obj/KERNEL_OBJ
CROSS_DIR ?= $(ANDROID_PATH)/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin:$(ANDROID_PATH)/prebuilts/gcc/linux-x86/arm/gcc-linaro-aarch64-linux-gnu-4.8/bin
PATH := $(CROSS_DIR):$(PATH)
CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT ?= yes

default:
	$(MAKE) -C $(KERNEL_DIR) $(MODULE_PARAM)  M=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(HI110X_DRIVER_SRC_PATH)/$(DRIVER_FLODER) clean
	-find ../bfgx/ -type f -name "*.o" -exec rm -f {} \;
	-find ../bfgx/ -type f -name "*.o.cmd" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o" -exec rm -f {} \;
	-find ../common/ -type f -name "*.o.cmd" -exec rm -f {} \;

endif
