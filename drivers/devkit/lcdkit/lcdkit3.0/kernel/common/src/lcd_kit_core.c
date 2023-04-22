#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/raid/pq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/backlight.h>
#include <linux/pwm.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/syscalls.h>

#include <linux/spi/spi.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>

#include "lcd_kit_core.h"

/*lcd kit ops*/
struct lcd_kit_ops *g_lcd_kit_ops = NULL;
/*lcd kit ops register function*/
int lcd_kit_ops_register(struct lcd_kit_ops * ops)
{
	if (!g_lcd_kit_ops) {
		g_lcd_kit_ops = ops;
		printk(KERN_INFO "[LCD_KIT]%s: ops register success!\n", __func__);
		return 0;
	}
	printk(KERN_ERR "[LCD_KIT]%s: ops have been registered!\n", __func__);
	return -1;
}

/*lcd kit ops unregister function*/
int lcd_kit_ops_unregister(struct lcd_kit_ops * ops)
{
	if (g_lcd_kit_ops == ops) {
		g_lcd_kit_ops = NULL;
		printk(KERN_INFO "[LCD_KIT]%s: ops unregister success!\n", __func__);
		return 0;
	}
	printk(KERN_ERR "[LCD_KIT]%s: ops unregister fail!\n", __func__);
	return -1;
}

/*get lcd kit ops function*/
struct lcd_kit_ops *lcd_kit_get_ops(void)
{
	return g_lcd_kit_ops;
}
/////////////////////////////////////////////////////////////
/////////////////////for ts kit///////////////////////
/*ts kit ops*/
struct ts_kit_ops *g_ts_kit_ops = NULL;
/*lcd kit ops register function*/
int ts_kit_ops_register(struct ts_kit_ops * ops)
{
	if (!g_ts_kit_ops) {
		g_ts_kit_ops = ops;
		printk(KERN_INFO "[TS_KIT]%s: ops register success!\n", __func__);
		return 0;
	}
	printk(KERN_ERR "[TS_KIT]%s: ops have been registered!\n", __func__);
	return -1;
}

/*lcd kit ops unregister function*/
int ts_kit_ops_unregister(struct ts_kit_ops * ops)
{
	if (g_ts_kit_ops == ops) {
		g_ts_kit_ops = NULL;
		printk(KERN_INFO "[TS_KIT]%s: ops unregister success!\n", __func__);
		return 0;
	}
	printk(KERN_ERR "[TS_KIT]%s: ops unregister fail!\n", __func__);
	return -1;
}

/*get lcd kit ops function*/
struct ts_kit_ops *ts_kit_get_ops(void)
{
	return g_ts_kit_ops;
}


