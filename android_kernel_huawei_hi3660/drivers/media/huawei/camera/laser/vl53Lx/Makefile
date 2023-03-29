#
# Makefile for the vl53L0 drivers.
#

# Each configuration option enables a list of files.

ccflags-y	+= -I$(src)/vl53L0/inc -I$(src)/vl53L0 -Idrivers/media/huawei/camera -DSTM_TEST
ccflags-y	+= -I$(src)/vl53L1/inc -I$(src)/vl53L1/ipp -I$(src)/vl53L1 -I$(src)

obj-$(CONFIG_HISI_CAMERA_VL53L0_970)               += stmvl53lx.o

# module
stmvl53lx-objs := laser_module.o

# vl53l0
stmvl53lx-objs				+= vl53L0/gp2ap01vt.o vl53L0/hw_laser_i2c.o vl53L0/stmvl53l0_module.o vl53L0/src/vl53l0_api_calibration.o
stmvl53lx-objs				+= vl53L0/src/vl53l0_api_core.o vl53L0/src/vl53l0_api_histogram.o vl53L0/src/vl53l0_api_ranging.o
stmvl53lx-objs				+= vl53L0/src/vl53l0_api_strings.o vl53L0/src/vl53l0_api.o vl53L0/src/vl53l0_platform.o
stmvl53lx-objs				+= vl53L0/src/vl53l0_i2c_platform.o vl53L0/src/vl53l0_port_i2c.o


# vl53L1
ifdef OLD_NETLINK_API
ccflags-y	+= -DOLD_NETLINK_API
endif

ifdef VL53L1_LOG_ENABLE
ccflags-y	+= -DVL53L1_LOG_ENABLE
endif

stmvl53lx-objs		+= vl53L1/stmvl53l1_module.o vl53L1/stmvl53l1_module-i2c.o
stmvl53lx-objs		+= vl53L1/stmvl53l1_i2c.o vl53L1/stmvl53l1_ipp_nl.o vl53L1/stmvl53l1_log.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_api.o vl53L1/src/vl53l1_api_core.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_api_strings.o vl53L1/src/vl53l1_error_strings.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_core.o vl53L1/src/vl53l1_register_funcs.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_api_preset_modes.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_api_calibration.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_fpga_core.o vl53L1/src/vl53l1_silicon_core.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_zone_presets.o vl53L1/src/vl53l1_nvm.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_api_debug.o vl53L1/src/vl53l1_core_support.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_wait.o vl53L1/ipp/ipp_linux.o
stmvl53lx-objs		+= vl53L1/src/vl53l1_nvm_debug.o


