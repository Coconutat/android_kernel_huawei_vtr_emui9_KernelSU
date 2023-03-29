/*
 * drivers/huawei/drivers/rt4801h.c
 *
 * rt4801h driver reffer to lcd
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <huawei_platform/devdetect/hw_dev_dec.h>

#include "rt4801h.h"
#include "../hisi_fb_def.h"

#define DTS_COMP_RT4801H "hisilicon,rt4801h_phy"
static int gpio_vsp_enable = 0;
static int gpio_vsn_enable = 0;
static bool fastboot_display_enable = true;

static u8 vpos_cmd = 0;
static u8 vneg_cmd = 0;
static struct rt4801h_device_info *rt4801h_client = NULL;
static bool is_rt4801h_device = false;

#define DTS_COMP_SHARP_DUKE_NT35597     "hisilicon,mipi_sharp_duke_NT35597"
#define DTS_COMP_JDI_DUKE_R63450_5P7    "hisilicon,mipi_jdi_duke_R63450_5P7"
#define DTS_COMP_TIANMA_DUKE_TD4302_5P7 "hisilicon,mipi_tianma_duke_TD4302_5P7"

#define VAL_5V5 0
#define VAL_5V8 1
#define VAL_5V6 2

#define VSP_ENABLE 1
#define VSN_ENABLE 1
#define VSP_DISABLE 0
#define VSN_DISABLE 0
#define GPIOS_NUM0 0
#define GPIOS_NUM1 1

static int get_lcd_type(void)
{
	struct device_node *np = NULL;
	int ret = 0;
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SHARP_DUKE_NT35597);
	ret = of_device_is_available(np);
	if (np && ret) {
		HISI_FB_INFO("device %s! set voltage 5.8V\n", DTS_COMP_SHARP_DUKE_NT35597);
		return VAL_5V8;
	}
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_JDI_DUKE_R63450_5P7);
	ret = of_device_is_available(np);
	if (np && ret) {
		HISI_FB_INFO("device %s! set voltage 5.8V\n", DTS_COMP_JDI_DUKE_R63450_5P7);
		return VAL_5V5;
	}
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_TIANMA_DUKE_TD4302_5P7);
	ret = of_device_is_available(np);
	if (np && ret) {
		HISI_FB_INFO("device %s! set voltage 5.8V\n", DTS_COMP_TIANMA_DUKE_TD4302_5P7);
		return VAL_5V5;
	}

	HISI_FB_INFO("not found device set vsp/vsn voltage 5.5V\n");
	return VAL_5V5;
}

static int rt4801h_reg_init(struct i2c_client *client, u8 vpos, u8 vneg)
{
	int nRet = 0;
	int app_dis = 0;

	if(client == NULL) {
		pr_err("[%s,%d]: NULL point for client\n",__FUNCTION__,__LINE__);
		goto exit;
	}

	nRet = i2c_smbus_read_byte_data(client, RT4801H_REG_APP_DIS);
	if (nRet < 0) {
		pr_err("%s read app_dis failed\n", __func__);
		goto exit;
	}
	app_dis = nRet;

	app_dis = app_dis | RT4801H_DISP_BIT | RT4801H_DISN_BIT | RT4801H_DISP_BIT;

	nRet = i2c_smbus_write_byte_data(client, RT4801H_REG_VPOS, vpos);
	if (nRet < 0) {
		pr_err("%s write vpos failed\n", __func__);
		goto exit;
	}

	nRet = i2c_smbus_write_byte_data(client, RT4801H_REG_VNEG, vneg);
	if (nRet < 0) {
		pr_err("%s write vneg failed\n", __func__);
		goto exit;
	}

	nRet = i2c_smbus_write_byte_data(client, RT4801H_REG_APP_DIS, (u8)app_dis);
	if (nRet < 0) {
		pr_err("%s write app_dis failed\n", __func__);
		goto exit;
	}

exit:
	return nRet;
}



static bool rt4801h_device_verify(void)
{
	int nRet = 0;
	nRet = i2c_smbus_read_byte_data(rt4801h_client->client, RT4801H_REG_APP_DIS);
	if (nRet < 0) {
		pr_err("%s read app_dis failed\n", __func__);
		return false;
	}
	HISI_FB_INFO("RT4801H verify ok, app_dis = 0x%x\n", nRet);
	return true;
}

bool check_rt4801h_device(void)
{
	return is_rt4801h_device;
}

int rt4801h_set_voltage(void)
{
	HISI_FB_INFO("RT4801H set vol reg, vpos = 0x%x, vneg = 0x%x\n", vpos_cmd, vneg_cmd);
	return rt4801h_reg_init(rt4801h_client->client, vpos_cmd, vneg_cmd);
}

static void rt4801h_get_target_voltage(int *vpos_target, int *vneg_target)
{
	int ret = 0;

	if((vpos_target == NULL) || (vneg_target == NULL)) {
		pr_err("%s: NULL point\n", __func__);
		return;
	}




	ret = get_lcd_type();
	if (ret == VAL_5V8) {
		HISI_FB_INFO("vpos and vneg target is 5.8V\n");
		*vpos_target = RT4801H_VOL_58;
		*vneg_target = RT4801H_VOL_58;
	} else if (ret == VAL_5V6) {
		HISI_FB_INFO("vpos and vneg target is 5.6V\n");
		*vpos_target = RT4801H_VOL_56;
		*vneg_target = RT4801H_VOL_56;
	}else {
		HISI_FB_INFO("vpos and vneg target is 5.5V\n");
		*vpos_target = RT4801H_VOL_55;
		*vneg_target = RT4801H_VOL_55;
	}
	return;
}

static int rt4801h_start_setting(void)
{
	int retval = 0;

	retval = gpio_request(gpio_vsp_enable, "gpio_lcd_p5v5_enable");
	if (retval != 0) {
		pr_err("failed to request gpio %d : gpio_lcd_p5v5_enable !\n", gpio_vsp_enable);
		return retval;
	}

	retval = gpio_request(gpio_vsn_enable, "gpio_lcd_n5v5_enable");
	if (retval != 0) {
		pr_err("failed to request gpio %d : gpio_lcd_n5v5_enable !\n", gpio_vsn_enable);
		return retval;
	}

	retval = gpio_direction_output(gpio_vsp_enable, VSP_ENABLE);
	if (retval != 0) {
		pr_err("failed to output gpio %d : gpio_lcd_p5v5_enable !\n", gpio_vsp_enable);
		return retval;
	}
	mdelay(5);

	retval = gpio_direction_output(gpio_vsn_enable, VSN_ENABLE);
	if (retval != 0) {
		pr_err("failed to output gpio %d : gpio_lcd_p5v5_enable !\n", gpio_vsn_enable);
		return retval;
	}
	mdelay(5);

	return retval;
}

static int rt4801h_finish_setting(void)
{
	int retval = 0;

	retval = gpio_direction_output(gpio_vsn_enable, VSP_DISABLE);
	if (retval != 0) {
		pr_err("failed to output gpio %d : gpio_lcd_n5v5_enable !\n", gpio_vsn_enable);
		return retval;
	}
	udelay(10);

	retval = gpio_direction_output(gpio_vsp_enable, VSP_DISABLE);
	if (retval != 0) {
		pr_err("failed to output gpio %d : gpio_lcd_p5v5_enable !\n", gpio_vsp_enable);
		return retval;
	}
	udelay(10);

	retval = gpio_direction_input(gpio_vsn_enable);
	if (retval != 0) {
		pr_err("failed to set gpio %d input: gpio_lcd_n5v5_enable !\n", gpio_vsn_enable);
		return retval;
	}
	udelay(10);

	retval = gpio_direction_input(gpio_vsp_enable);
	if (retval != 0) {
		pr_err("failed to set gpio %d input: gpio_lcd_p5v5_enable !\n", gpio_vsp_enable);
		return retval;
	}
	udelay(10);

	gpio_free(gpio_vsn_enable);
	gpio_free(gpio_vsp_enable);

	return retval;
}


static int rt4801h_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int retval = 0;
	int nRet = 0;
	int vpos_target = 0;
	int vneg_target = 0;
	struct device_node *np = NULL;

	if(client == NULL) {
		pr_err("[%s,%d]: NULL point for client\n",__FUNCTION__,__LINE__);
		retval = -ENODEV;
		goto failed_1;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_RT4801H);
	if (!np) {
		pr_err("NOT FOUND device node %s!\n", DTS_COMP_RT4801H);
		retval = -ENODEV;
		goto failed_1;
	}

	gpio_vsp_enable = of_get_named_gpio(np, "gpios", GPIOS_NUM0);
	gpio_vsn_enable = of_get_named_gpio(np, "gpios", GPIOS_NUM1);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[%s,%d]: need I2C_FUNC_I2C\n",__FUNCTION__,__LINE__);
		retval = -ENODEV;
		goto failed_1;
	}

	rt4801h_client = kzalloc(sizeof(*rt4801h_client), GFP_KERNEL);
	if (!rt4801h_client) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto failed_1;
	}

	i2c_set_clientdata(client, rt4801h_client);
	rt4801h_client->dev = &client->dev;
	rt4801h_client->client = client;

	if (!fastboot_display_enable) {
		rt4801h_start_setting();
	}

	rt4801h_get_target_voltage(&vpos_target, &vneg_target);
	vpos_cmd = (u8)vpos_target;
	vneg_cmd = (u8)vneg_target;

	if(rt4801h_device_verify()) {
		is_rt4801h_device = true;
	} else {
		is_rt4801h_device = false;
		retval = -ENODEV;
		pr_err("rt4801h_reg_verify failed\n");
		goto failed;
	}

	nRet = rt4801h_reg_init(rt4801h_client->client, (u8)vpos_target, (u8)vneg_target);
	if (nRet) {
		retval = -ENODEV;
		pr_err("rt4801h_reg_init failed\n");
		goto failed;
	}
	pr_info("rt4801h inited succeed\n");


	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_DC_DC);

	if (!fastboot_display_enable) {
		rt4801h_finish_setting();
	}
failed_1:
	return retval;
failed:
	if(rt4801h_client) {
		kfree(rt4801h_client);
	}
	return retval;
}

static const struct of_device_id rt4801h_match_table[] = {
	{
		.compatible = DTS_COMP_RT4801H,
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id rt4801h_i2c_id[] = {
	{ "rt4801h", 0 },
	{ }
};

MODULE_DEVICE_TABLE(of, rt4801h_match_table);

static struct i2c_driver rt4801h_driver = {
	.id_table = rt4801h_i2c_id,
	.probe = rt4801h_probe,
	.driver = {
		.name = "rt4801h",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt4801h_match_table),
	},
};

static int __init rt4801h_module_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&rt4801h_driver);
	if (ret)
		pr_err("Unable to register rt4801h driver\n");

	return ret;
}
static void __exit rt4801h_exit(void)
{
	if(rt4801h_client) {
		kfree(rt4801h_client);
	}
	i2c_del_driver(&rt4801h_driver);
}

late_initcall(rt4801h_module_init);
module_exit(rt4801h_exit);

MODULE_DESCRIPTION("RT4801H driver");
MODULE_LICENSE("GPL");
