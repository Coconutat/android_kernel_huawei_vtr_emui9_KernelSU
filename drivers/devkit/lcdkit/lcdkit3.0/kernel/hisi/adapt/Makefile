obj-$(CONFIG_LCD_KIT_HISI)	+= lcd_kit_adapt.o
obj-$(CONFIG_LCD_KIT_HISI)	+= lcd_kit_sysfs_hs.o

ifneq ($(TARGET_BUILD_VARIANT),user)
	obj-y += lcd_kit_debug_hs.o
endif

EXTRA_CFLAGS += -Idrivers/video/fbdev/hisi/dss \
		-Idrivers/video/hisi/dss \
		-Idrivers/staging/android \
		-Idrivers/atfdriver \
		-Idrivers/hisi/tzdriver \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include
