obj-$(CONFIG_LCD_KIT_MTK)	+= lcd_kit_adapt.o
obj-$(CONFIG_LCD_KIT_MTK)	+= lcd_kit_sysfs_mtk.o

EXTRA_CFLAGS += -Idrivers/staging/android \
		-Idrivers/atfdriver \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/common/include \
		-Idrivers/devkit/lcdkit/lcdkit3.0/kernel/mtk/include \
		-Idrivers/misc/mediatek/lcm/inc
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/lcm/devkit/lcdkit/lcdkit3.0/kernel/common/include \
#       -I$(srctree)/drivers/misc/mediatek/lcm/devkit/lcdkit/lcdkit3.0/kernel/mtk/include