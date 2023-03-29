ccflags-$(CONFIG_SND_SOC_TAS2560) += -Idrivers/devkit/audiokit/
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DTAS2560_DEBUG
endif
snd-soc-tas2560-objs := tas2560_core.o tas2560_misc.o tas2560_i2c.o
obj-$(CONFIG_SND_SOC_TAS2560) += snd-soc-tas2560.o
