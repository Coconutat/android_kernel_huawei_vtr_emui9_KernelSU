
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DLCD_KIT_DEBUG_ENABLE
endif

obj-$(CONFIG_LCD_KIT_HISI)	+= lcd_kit_disp.o
obj-$(CONFIG_LCD_KIT_HISI)	+= lcd_kit_power.o
obj-$(CONFIG_LCD_KIT_HISI) 	+= lcd_kit_utils.o
obj-$(CONFIG_LCD_KIT_HISI) 	+= lcd_kit_panel.o

EXTRA_CFLAGS += -Idrivers/video/fbdev/hisi/dss \
		-Idrivers/video/hisi/dss \
		-Idrivers/staging/android \
		-Idrivers/atfdriver \
		-Idrivers/hisi/tzdriver \
		-Idrivers/devkit/lcdkit/lcdkit3.0 \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/hisi/adapt
