/*
* Simple driver for Texas Instruments LM3630 LED Flash driver chip
* Copyright (C) 2012 Texas Instruments
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/

#ifndef __LINUX_LP8556_H
#define __LINUX_LP8556_H

#include "../hisi_fb.h"
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory

#if defined(CONFIG_LCDKIT_DRIVER)
#include "lcdkit_panel.h"
#endif

#define LP8556_NAME "lp8556"
#define DTS_COMP_LP8556 "ti,lp8556"

/* base reg */
#define LP8556_DEVICE_CONTROL		0X01
#define LP8556_EPROM_CFG0			0XA0
#define LP8556_EPROM_CFG1			0XA1
#define LP8556_EPROM_CFG2			0XA2
#define LP8556_EPROM_CFG3			0XA3
#define LP8556_EPROM_CFG4			0XA4
#define LP8556_EPROM_CFG5			0XA5
#define LP8556_EPROM_CFG6			0XA6
#define LP8556_EPROM_CFG7			0XA7
#define LP8556_EPROM_CFG9			0XA9
#define LP8556_EPROM_CFGA			0XAA
#define LP8556_EPROM_CFGE			0XAE
#define LP8556_EPROM_CFG98			0X98
#define LP8556_EPROM_CFG9E			0X9E
#define LP8556_LED_ENABLE			0X16
#define LP8556_FUALT_FLAG			0X02

#define LP8556_ENABLE_ALL_LEDS      0x3f
#define LP8556_DISABLE_ALL_LEDS     0X00
#define LP8556_FAULT_SHORT_BIT      6
#define LP8556_FAULT_OPEN_BIT       7
#define LP8556_LED1_SHORT_ERR_BIT   10
#define LP8556_LED1_OPEN_ERR_BIT    4
#define LP8556_LED_NUM              6



#define TEST_OK                  0
#define TEST_ERROR_DEV_NULL      BIT(0)
#define TEST_ERROR_DATA_NULL     BIT(1)
#define TEST_ERROR_CLIENT_NULL   BIT(2)
#define TEST_ERROR_I2C           BIT(3)
#define TEST_ERROR_LED1_OPEN     BIT(4)
#define TEST_ERROR_LED2_OPEN     BIT(5)
#define TEST_ERROR_LED3_OPEN     BIT(6)
#define TEST_ERROR_LED4_OPEN     BIT(7)
#define TEST_ERROR_LED5_OPEN     BIT(8)
#define TEST_ERROR_LED6_OPEN     BIT(9)
#define TEST_ERROR_LED1_SHORT    BIT(10)
#define TEST_ERROR_LED2_SHORT    BIT(11)
#define TEST_ERROR_LED3_SHORT    BIT(12)
#define TEST_ERROR_LED4_SHORT    BIT(13)
#define TEST_ERROR_LED5_SHORT    BIT(14)
#define TEST_ERROR_LED6_SHORT    BIT(15)

#define TEST_ERROR_CHIP_INIT     BIT(16)

#define LP8556_BL_MIN            0
#define LP8556_BL_MAX            4095



#ifndef BIT
#define BIT(x)  (1<<(x))
#endif

#define LP8556_EMERG(msg, ...)    \
	do { if (lp8556_msg_level > 0)  \
		printk(KERN_EMERG "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_ALERT(msg, ...)    \
	do { if (lp8556_msg_level > 1)  \
		printk(KERN_ALERT "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_CRIT(msg, ...)    \
	do { if (lp8556_msg_level > 2)  \
		printk(KERN_CRIT "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_ERR(msg, ...)    \
	do { if (lp8556_msg_level > 3)  \
		printk(KERN_ERR "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_WARNING(msg, ...)    \
	do { if (lp8556_msg_level > 4)  \
		printk(KERN_WARNING "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_NOTICE(msg, ...)    \
	do { if (lp8556_msg_level > 5)  \
		printk(KERN_NOTICE "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_INFO(msg, ...)    \
	do { if (lp8556_msg_level > 6)  \
		printk(KERN_INFO "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LP8556_DEBUG(msg, ...)    \
	do { if (lp8556_msg_level > 7)  \
		printk(KERN_DEBUG "[lp8556]%s: "msg, __func__, ## __VA_ARGS__); } while (0)

struct lp8556_chip_data {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
	struct semaphore test_sem;
};

#define GPIO_LP8556_EN_NAME "lp8556_hw_en"
#define LP8556_RW_REG_MAX  15

struct lp8556_backlight_information {
	/* whether support lp8556 or not */
	int lp8556_support;
	/* which i2c bus controller lp8556 mount */
	int lp8556_i2c_bus_id;
	/* lp8556 hw_en gpio */
	int lp8556_hw_en_gpio;
	int lp8556_reg[LP8556_RW_REG_MAX];
	int bl_on_kernel_mdelay;
	int lp8556_level_lsb;
	int lp8556_level_msb;
	int bl_led_num;
};

ssize_t lp8556_set_backlight_init(uint32_t bl_level);

#endif /* __LINUX_LP8556_H */


