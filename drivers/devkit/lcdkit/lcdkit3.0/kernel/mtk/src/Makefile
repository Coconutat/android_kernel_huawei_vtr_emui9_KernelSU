obj-$(CONFIG_LCD_KIT_MTK)	+= lcd_kit_disp.o
obj-$(CONFIG_LCD_KIT_MTK)	+= lcd_kit_power.o
obj-$(CONFIG_LCD_KIT_MTK) 	+= lcd_kit_utils.o
obj-$(CONFIG_LCD_KIT_MTK) 	+= lcd_kit_panel.o
obj-$(CONFIG_MT6370_PMU_DSV) += bias/
obj-$(CONFIG_LCD_KIT_MTK) 	+= backlight/

EXTRA_CFLAGS +=-Idrivers/staging/android \
		-Idrivers/atfdriver \
		-Idrivers/devkit/lcdkit/lcdkit3.0 \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/mtk/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/mtk/adapt \
		-Idrivers/misc/mediatek/lcm/inc
