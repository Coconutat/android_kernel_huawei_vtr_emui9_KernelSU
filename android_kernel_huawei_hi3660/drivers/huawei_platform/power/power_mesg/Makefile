obj-$(CONFIG_HUAWEI_POWER_MESG_INTERFACE) += power_genl.o power_mesg.o

ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DPOWER_MESG_DEBUG
endif