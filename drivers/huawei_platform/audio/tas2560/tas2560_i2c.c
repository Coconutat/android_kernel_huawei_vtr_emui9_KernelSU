/*
 * ALSA SoC Texas Instruments TAS2560 High Performance 4W Smart Amplifier
 *
 * Copyright (C) 2016 Texas Instruments, Inc.
 *
 * Author: saiprasad
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */



#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "tas2560.h"
#include "tas2560_core.h"
#include "tas2560_misc.h"
/*lint -e845 -e747 -e64 -e30 -e785 -e438 -e712 -e550 -e50 -e529 -e732*/
/*lint -e774 -e778 -e838 -e753 -e750 -e530 -e1564 -e142 -e528*/
static void tas2560_change_page(struct tas2560_priv *pTAS2560, unsigned int page)
{
	if (pTAS2560->mnCurrentPage == page)
		return;

	regmap_write(pTAS2560->regmap, TAS2560_BOOKCTL_PAGE, page);
	pTAS2560->mnCurrentPage = page;
}

static void tas2560_change_book(struct tas2560_priv *pTAS2560, unsigned int book)
{
	if (pTAS2560->mnCurrentBook == book)
		return;
	tas2560_change_page(pTAS2560,0);
	regmap_write(pTAS2560->regmap, TAS2560_BOOKCTL_REG, book);
	pTAS2560->mnCurrentBook = book;
}

static int tas2560_dev_read(struct tas2560_priv *pTAS2560,
				 unsigned int reg, unsigned int *pValue, unsigned int log_enable)
{
	int ret = -1;

	if(log_enable){
		dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u\n", __func__,
			TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
			TAS2560_PAGE_REG(reg));
	}

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	ret = regmap_read(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pValue);
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_write(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned int value)
{
	int ret = -1;

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	ret = regmap_write(pTAS2560->regmap, TAS2560_PAGE_REG(reg),
			   value);
	mutex_unlock(&pTAS2560->dev_lock);
	if(!ret){
		dev_dbg(pTAS2560->dev, "write reg %u,	val: 0x%02x\n",reg, value);
	}else{
		dev_dbg(pTAS2560->dev, "write reg %u,	val: 0x%02x failed\n",reg, value);
	}
	return ret;
}

static int tas2560_dev_bulk_write(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned char *pData, unsigned int nLength)
{
	int ret = -1;

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	ret = regmap_bulk_write(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pData, nLength);
	mutex_unlock(&pTAS2560->dev_lock);
	if(!ret){
		dev_dbg(pTAS2560->dev, "bulkwrite reg %u,	len: 0x%02x\n",reg,nLength);
	}else{
		dev_dbg(pTAS2560->dev, "bulkwrite reg %u,	len: 0x%02x failed\n",reg, nLength);
	}
	return ret;
}

static int tas2560_dev_bulk_read(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned char *pData, unsigned int nLength)
{
	int ret;

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, len: 0x%02x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), nLength);
	ret = regmap_bulk_read(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pData, nLength);
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_update_bits(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned int mask, unsigned int value)
{
	int ret = 0;
//	dev_info(pTAS2560->dev, "tas2560_dev_update_bits called\n");

       mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, mask: 0x%x, val=0x%x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), mask, value);

	ret = regmap_update_bits(pTAS2560->regmap, TAS2560_PAGE_REG(reg), mask, value);
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static bool tas2560_volatile(struct device *dev, unsigned int reg)
{
	UNUSED(dev);
	UNUSED(reg);
	return false;
}

static bool tas2560_writeable(struct device *dev, unsigned int reg)
{
	UNUSED(dev);
	UNUSED(reg);
	return true;
}

static const struct regmap_config tas2560_i2c_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.writeable_reg = tas2560_writeable,
	.volatile_reg = tas2560_volatile,
	.cache_type = REGCACHE_NONE,
	.max_register = 128,
};

static int tas2560_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct tas2560_priv *pTAS2560 = NULL;
	int ret = 0;
	int lret = 0;
//	static int nClient = 0;
	UNUSED(id);

	dev_info(&client->dev, "%s enter\n",__func__);

	pTAS2560 = devm_kzalloc(&client->dev, sizeof(struct tas2560_priv), GFP_KERNEL);
	if (pTAS2560 == NULL){
		dev_err(&client->dev, "%s, -ENOMEM \n",__func__);
		return -ENOMEM;
	}

	pTAS2560->dev = &client->dev;
	i2c_set_clientdata(client, pTAS2560);

	dev_set_drvdata(&client->dev, pTAS2560);

	pTAS2560->regmap = devm_regmap_init_i2c(client, &tas2560_i2c_regmap);
	if (IS_ERR(pTAS2560->regmap)) {
		ret = PTR_ERR(pTAS2560->regmap);
		dev_err(&client->dev, "Failed to allocate register map: %d\n",
					ret);
		return ret;
	}

	pTAS2560->mnCurrentBook = 0;
	pTAS2560->mnCurrentPage = 0;
	pTAS2560->read = tas2560_dev_read;
	pTAS2560->write = tas2560_dev_write;
	pTAS2560->bulk_read = tas2560_dev_bulk_read;
	pTAS2560->bulk_write = tas2560_dev_bulk_write;
	pTAS2560->update_bits = tas2560_dev_update_bits;
	INIT_WORK(&pTAS2560->tas_irq_handle_work,tas2560_handle_irq);

	if (client->dev.of_node){
		tas2560_parse_dt(&client->dev, pTAS2560);
		if(pTAS2560->mnResetGPIO > 0){
			ret = gpio_request(pTAS2560->mnResetGPIO, "tas2560_reset");
			if (ret) {
				dev_err(pTAS2560->dev, "%s: Failed to request gpio %d\n", __func__,
					pTAS2560->mnResetGPIO);
				pTAS2560->mnResetGPIO = 0;
			}else{
				gpio_direction_output(pTAS2560->mnResetGPIO, 0);
				mdelay(3);
				gpio_direction_output(pTAS2560->mnResetGPIO, 1);
				msleep(1);
			}
		}
	}

	mutex_init(&pTAS2560->dev_lock);

	ret = tas2560_dev_write(pTAS2560, TAS2560_SW_RESET_REG, 0x01);
	if(ret < 0){
		dev_err(pTAS2560->dev, "ERROR I2C comm, %d\n", ret);
		return ret;
	}

	mdelay(1);

	pTAS2560->write(pTAS2560,TAS2560_DR_BOOST_REG_1, 0x0c);
	pTAS2560->write(pTAS2560,TAS2560_DR_BOOST_REG_2, 0x33);
	if(_8OHM_LOAD == pTAS2560->load_type){
		pTAS2560->write(pTAS2560,TAS2560_LOAD, 0x83);
	}else{
		pTAS2560->write(pTAS2560,TAS2560_LOAD, 0x8b);
	}
	pTAS2560->write(pTAS2560,TAS2560_DEV_MODE_REG, 0x02);
	pTAS2560->update_bits(pTAS2560, TAS2560_SPK_CTRL_REG, 0x0f, (pTAS2560->gain)&0x0f);
	if(hi6402_24bit == pTAS2560->iv_type){
		tas2560_set_ASI_fmt(pTAS2560, SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_NB_NF|SND_SOC_DAIFMT_I2S);
		tas2560_dev_write(pTAS2560, ASI_OFFSET_1, 0x00);
		tas2560_set_bit_rate(pTAS2560, 24);
	}else{
		tas2560_set_ASI_fmt(pTAS2560, SND_SOC_DAIFMT_CBS_CFS|SND_SOC_DAIFMT_IB_IF|SND_SOC_DAIFMT_DSP_A);
		tas2560_dev_write(pTAS2560, ASI_OFFSET_1, 0x01);
		tas2560_set_bit_rate(pTAS2560, 16);
	}

	int ret_check = of_property_read_u32(client->dev.of_node, "gpio_irq", &pTAS2560->gpio_irq);
	if (ret_check) {
		dev_err(&client->dev,"get gpio_irq failed\n");
	}
	if (pTAS2560->gpio_irq == 0) {
		dev_err(&client->dev,"get gpio for tas irq failed\n");
	} else {
		dev_err(&client->dev,"got gpio irq for tas %d\n", pTAS2560->gpio_irq);
		if (!gpio_is_valid((int)pTAS2560->gpio_irq)) {
			dev_err(&client->dev,"registed gpio for irq is not valid\n");
		}
		lret = devm_gpio_request_one(&client->dev, pTAS2560->gpio_irq,
									(unsigned long)GPIOF_DIR_IN, "tas2560_gpio_irq");
		if (lret != 0) {
			dev_err(&client->dev,"request GPIO for tas irq fail %d\n", lret);
		}
		lret = devm_request_threaded_irq(&client->dev,
					(unsigned int)gpio_to_irq((unsigned int)pTAS2560->gpio_irq),
					NULL, tas2560_thread_irq, (unsigned long)(IRQF_TRIGGER_FALLING | IRQF_ONESHOT),
					"tas2560_irq", pTAS2560);
		if (lret != 0) {
			dev_err(&client->dev, "Failed to request IRQ %d: %d\n",
					gpio_to_irq((unsigned int)pTAS2560->gpio_irq), lret);
		}
	}

	pTAS2560->mnDBGCmd = 0;
	pTAS2560->mnCurrentReg = 0;
	mutex_init(&pTAS2560->file_lock);
	tas2560_register_misc(pTAS2560);
	tas2560_irq_enable(pTAS2560);

	return ret;
}


static int tas2560_i2c_remove(struct i2c_client *client)
{
	struct tas2560_priv *pTAS2560 = i2c_get_clientdata(client);

	dev_info(pTAS2560->dev, "%s\n", __FUNCTION__);

	tas2560_deregister_misc(pTAS2560);
	mutex_destroy(&pTAS2560->file_lock);

	return 0;
}


static const struct i2c_device_id tas2560_i2c_id[] = {
	{ "tas2560", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas2560_i2c_id);

static const struct of_device_id tas2560_of_match[] = {
	{ .compatible = "huawei,tas2560" },
	{},
};
MODULE_DEVICE_TABLE(of, tas2560_of_match);


static struct i2c_driver tas2560_i2c_driver = {
	.driver = {
		.name   = "tas2560",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(tas2560_of_match),
	},
	.probe      = tas2560_i2c_probe,
	.remove     = tas2560_i2c_remove,
	.id_table   = tas2560_i2c_id,
};

module_i2c_driver(tas2560_i2c_driver);
MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 I2C Smart Amplifier driver");
MODULE_LICENSE("GPLv2");

