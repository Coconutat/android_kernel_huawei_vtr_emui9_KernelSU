
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
	obj-y += lcd_kit_debug.o
endif

obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_sysfs.o
obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_common.o
obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_parse.o
obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_core.o
obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_bl.o
obj-$(CONFIG_LCD_KIT_COMMON)	+= lcd_kit_bias.o
EXTRA_CFLAGS += -Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
