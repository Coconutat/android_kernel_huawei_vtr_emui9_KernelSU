ifeq ($(CONFIG_ONEWIRE_PHY_PL061_GPIO), y)
ccflags-y += -I$(srctree)/drivers/gpio
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
ccflags-y += -DONEWIRE_STABILITY_DEBUG
endif

ifneq ($(TARGET_VERSION_MODE),factory)
ccflags-y += -DGPIO_POWER_FOR_ONEWIRE
endif

obj-$(CONFIG_ONEWIRE_PHY_PL061_GPIO) += onewire_phy_pl061_gpio.o
obj-$(CONFIG_CERTIFICATION_IC_DS28EL15) += ds28el15.o maxim_onewire.o onewire_common.o
