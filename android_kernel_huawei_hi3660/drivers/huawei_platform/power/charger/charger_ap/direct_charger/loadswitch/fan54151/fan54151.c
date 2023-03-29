/*
 * fan54151.c
 *
 * fan54151 driver
 *
 * Copyright (C) 2016 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/raid/pq.h>
#include <huawei_platform/power/direct_charger.h>
#include "fan54151.h"

#define HWLOG_TAG fan54151

HWLOG_REGIST();

static struct fan54151_device_info *g_fan54151_dev;
/**********************************************************
*  Function:       fan54151_write_block
*  Discription:    register write block interface
*  Parameters:   di:fan54151_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_write_block(struct fan54151_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	*value = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = value;
	msg[0].len = num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1)
	{
		hwlog_err("i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else
	{
		return 0;
	}
}

/**********************************************************
*  Function:       fan54151_read_block
*  Discription:    register read block interface
*  Parameters:   di:fan54151_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_read_block(struct fan54151_device_info *di,u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

	buf = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = &buf;
	msg[0].len = 1;

	msg[1].addr = di->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = value;
	msg[1].len = num_bytes;

	ret = i2c_transfer(di->client->adapter, msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 2)
	{
		hwlog_err("i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else
	{
		return 0;
	}
}

/**********************************************************
*  Function:       fan54151_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_write_byte(u8 reg, u8 value)
{
	struct fan54151_device_info *di = g_fan54151_dev;
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = { 0 };

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return fan54151_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       fan54151_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_read_byte(u8 reg, u8 *value)
{
	struct fan54151_device_info *di = g_fan54151_dev;

	return fan54151_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       fan54151_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = fan54151_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = fan54151_write_byte(reg, val);

	return ret;
}

/**********************************************************
*  Function:       fan54151_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 *value)
{
	int ret = 0;
	u8 val = 0;

	ret = fan54151_read_byte(reg, &val);
	if (ret < 0)
		return ret;
	val &= MASK;
	val >>= SHIFT;
	*value = val;

	return 0;
}

#define CONFIG_SYSFS_BQ
#ifdef CONFIG_SYSFS_BQ
/*
 * There are a numerous options that are configurable on the fan54151
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define FAN54151_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
    .attr = __ATTR(_name, m, fan54151_sysfs_show, store),    \
    .reg = FAN54151_REG_##r,                      \
    .mask = FAN54151_REG_##r##_##f##_MASK,                       \
    .shift = FAN54151_REG_##r##_##f##_SHIFT,                     \
}

#define FAN54151_SYSFS_FIELD_RW(_name, r, f)                     \
	FAN54151_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, \
		fan54151_sysfs_store)

#define FAN54151_SYSFS_FIELD_RO(_name, r, f)                         \
	FAN54151_SYSFS_FIELD(_name, r, f, S_IRUGO, NULL)

static ssize_t fan54151_sysfs_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf);
static ssize_t fan54151_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

struct fan54151_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
	u8 mask;
	u8 shift;
};

static struct fan54151_sysfs_field_info fan54151_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	FAN54151_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
	FAN54151_SYSFS_FIELD_RW(reg_value, NONE, NONE),
};

static struct attribute *fan54151_sysfs_attrs[ARRAY_SIZE(fan54151_sysfs_field_tbl) + 1];

static const struct attribute_group fan54151_sysfs_attr_group = {
	.attrs = fan54151_sysfs_attrs,
};

/**********************************************************
*  Function:       fan54151_sysfs_init_attrs
*  Discription:    initialize fan54151_sysfs_attrs[] for fan54151 attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void fan54151_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(fan54151_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		fan54151_sysfs_attrs[i] =
		    &fan54151_sysfs_field_tbl[i].attr.attr;

	fan54151_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       fan54151_sysfs_field_lookup
*  Discription:    get the current device_attribute from fan54151_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  fan54151_sysfs_field_tbl[]
**********************************************************/
static struct fan54151_sysfs_field_info
*fan54151_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(fan54151_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		if (!strcmp
		    (name, fan54151_sysfs_field_tbl[i].attr.attr.name))
			break;

	if (i >= limit)
		return NULL;

	return &fan54151_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       fan54151_sysfs_show
*  Discription:    show the value for all fan54151 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t fan54151_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct fan54151_sysfs_field_info *info;
	struct fan54151_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = fan54151_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
		return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", info->reg);
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = fan54151_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}

	ret = fan54151_read_mask(info->reg, info->mask, info->shift, &v);
	if (ret)
		return ret;

	return scnprintf(buf, PAGE_SIZE, "%hhx\n", v);
}

/**********************************************************
*  Function:       fan54151_sysfs_store
*  Discription:    set the value for all fan54151 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t fan54151_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct fan54151_sysfs_field_info *info;
	struct fan54151_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = fan54151_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0)
		return ret;
	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = fan54151_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}
	if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
		if (v < (u8) FAN54151_MAIN_REG_TOTAL_NUM && v >= (u8) 0x00) {
			info->reg = v;
			return count;
		} else {
			return -EINVAL;
		}
	}

	ret = fan54151_write_mask(info->reg, info->mask, info->shift, v);
	if (ret)
		return ret;

	return count;
}

/**********************************************************
*  Function:       fan54151_sysfs_create_group
*  Discription:    create the fan54151 device sysfs group
*  Parameters:   di:fan54151_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_sysfs_create_group(struct fan54151_device_info *di)
{
	fan54151_sysfs_init_attrs();

	return sysfs_create_group(&di->dev->kobj, &fan54151_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the fan54151 device sysfs group
*  Parameters:   di:fan54151_device_info
*  return value:  NULL
**********************************************************/
static void fan54151_sysfs_remove_group(struct fan54151_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &fan54151_sysfs_attr_group);
}
#else
static int fan54151_sysfs_create_group(struct fan54151_device_info *di)
{
	return 0;
}

static inline void fan54151_sysfs_remove_group(struct fan54151_device_info *di)
{
}
#endif

/*static int fan54151_device_check(void)
{
}*/

static int fan54151_config_ocp_threshold(int ocp_threshold)
{
	u8 value;
	int ret = 0;

	if (ocp_threshold >= OCP_LIMIT_6900_MA)
	{
		value = OCP_LIMIT_6900_MA_REG;
	}
	else if (ocp_threshold >= OCP_LIMIT_6200_MA)
	{
		value = OCP_LIMIT_6200_MA_REG;
	}
	else if (ocp_threshold >= OCP_LIMIT_5400_MA)
	{
		value = OCP_LIMIT_5400_MA_REG;
	}
	else
	{
		value = OCP_LIMIT_4600_MA_REG;
	}
	hwlog_info("ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
	ret = fan54151_write_mask(FAN54151_CONTROL0 , OCP_MASK, OCP_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: config ocp threshold fail\n", __func__);
		return -1;
	}

	return ret;
}
static int fan54151_config_sovp_threshold(int sovp_threshold)
{
	u8 value;
	int ret = 0;

	if (sovp_threshold < SOVP_LIMIT_4200_MV)
	{
		sovp_threshold = SOVP_LIMIT_4200_MV;
	}
	if (sovp_threshold > SOVP_LIMIT_4575_MV)
	{
		sovp_threshold = SOVP_LIMIT_4575_MV;
	}
	value = (u8) ((sovp_threshold - SOVP_LIMIT_4200_MV)/SOVP_STEP);
	hwlog_info("sovp_threshold = %d, value = 0x%x\n", sovp_threshold, value);
	ret = fan54151_write_mask(FAN54151_CONTROL0 , SOVP_MASK, SOVP_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: config sovp threshold fail\n", __func__);
		return -1;
	}

	return ret;
}

/**********************************************************
*  Function:        fan54151_charge_status
*  Discription:     return the status of cur module
*  Parameters:    void
*  return value:   0-sucess or others-fail
**********************************************************/
static int fan54151_charge_status(void)
{
	struct fan54151_device_info *di = g_fan54151_dev;
	if (NULL == di)
	{
		hwlog_err("%s: di is NULL\n", __func__);
		return -1;
	}

	if (di->chip_already_init == 1)
		return 0;

	hwlog_err("%s = %d\n", __func__, di->chip_already_init);
	return -1;
}

static int fan54151_charge_init(void)
{
	int ret = 0;
	struct fan54151_device_info *di = g_fan54151_dev;
	if (NULL == di)
	{
		hwlog_err("%s: di is NULL\n", __func__);
		return -1;
	}
	/*pull down reset pin to reset fan54151*/
	ret = gpio_direction_output(di->gpio_reset, 0);
	if (ret < 0)
	{
		hwlog_err("%s: reset pull down fail!\n", __func__);
		return -1;
	}
	msleep(10);
	ret = gpio_direction_output(di->gpio_reset, 1);
	if (ret < 0)
	{
		hwlog_err("%s: reset pull up fail!\n", __func__);
		return -1;
	}
	msleep(10);
	ret = fan54151_write_byte(FAN54151_PROTECT_ENABLE , 0);
	if (ret)
	{
		hwlog_err("%s: uvlo enable fail\n", __func__);
		return -1;
	}
	ret |= fan54151_config_ocp_threshold(6200);
	ret |= fan54151_config_sovp_threshold(4575);

	if (!ret)
		di->chip_already_init = 1;

	return ret;
}
static int fan54151_enable(int enable)
{
	int ret = 0;
	u8 value = enable ? SWITCH_ENABLE : SWITCH_DISABLE;

	ret = fan54151_write_mask(FAN54151_CONTROL0 , SWITCH_ENABLE_MASK, SWITCH_ENABLE_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: switch_enable fail\n", __func__);
		return -1;
	}
	msleep(2);
	/*from the datasheet, we know the max turn-on time is 1.5 ms, and the max turn-off time is 1ms, so we sleep 2ms here*/
	/*ret = fan54151_protection_uvlo_enable(1);
	if (ret)
	{
		hwlog_err("%s: uvlo enable fail\n", __func__);
		return -1;
	}*/

	return ret;
}
static int fan54151_charge_exit(void)
{
	int ret = 0;
	struct fan54151_device_info *di = g_fan54151_dev;
	if (NULL == di)
	{
		hwlog_err("%s: di is NULL\n", __func__);
		return -1;
	}
	di->chip_already_init = 0;
	ret = fan54151_enable(0);
	if (ret)
	{
		hwlog_err("%s: close fail\n", __func__);
		/*here do not return, cause reset pin can also close the switch*/
	}
	/*pull down reset pin to reset fan54151*/
	ret = gpio_direction_output(di->gpio_reset, 0);
	if (ret < 0)
	{
		hwlog_err("%s: reset pull down fail!\n", __func__);
		return -1;
	}
	msleep(10);

	return ret;
}

static struct loadswitch_ops fan54151_ops = {
	.ls_init = fan54151_charge_init,
	.ls_exit = fan54151_charge_exit,
	.ls_enable = fan54151_enable,
	.ls_status = fan54151_charge_status,
};

/**********************************************************
*  Function:       fan54151_irq_work
*  Discription:    handler for loadswitch fault irq in charging process
*  Parameters:   work:loadswitch fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void fan54151_irq_work(struct work_struct *work)
{
	struct fan54151_device_info *di = container_of(work, struct fan54151_device_info, irq_work);
	u8 reg_irq = 0;

	fan54151_read_byte(FAN54151_IRQ, &reg_irq);
	hwlog_info("reg_irq = 0x%x\n", reg_irq);
	/*clear irq*/
	fan54151_write_byte(FAN54151_IRQ, reg_irq);

	enable_irq(di->irq_int);

}

/**********************************************************
*  Function:       fan54151_interrupt
*  Discription:    callback function for loadswitch fault irq in charging process
*  Parameters:   irq:loadswitch fault interrupt
*                      _di:fan54151_device_info
*  return value:  IRQ_HANDLED-sucess or others
**********************************************************/
static irqreturn_t fan54151_interrupt(int irq, void *_di)
{
	struct fan54151_device_info *di = _di;

	hwlog_info("fan54151 interrupt\n");
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static void parse_dts(struct device_node *np, struct fan54151_device_info *di)
{
	return;
}

/**********************************************************
*  Function:       fan54151_probe
*  Discription:    fan54151 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct fan54151_device_info *di = NULL;
	struct device_node *np = NULL;
	//struct class *power_class = NULL;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
	{
		hwlog_err("fan54151_device_info is NULL!\n");
		return -ENOMEM;
	}
	g_fan54151_dev = di;
	di->chip_already_init = 0;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	parse_dts(np, di);
	INIT_WORK(&di->irq_work, fan54151_irq_work);

	di->gpio_int = of_get_named_gpio(np, "loadswitch_int", 0);
	hwlog_err("loadswitch_int = %d\n", di->gpio_int);
	if (!gpio_is_valid(di->gpio_int))
	{
		hwlog_err("loadswitch_int is not valid\n");
		ret = -EINVAL;
		goto fan54151_fail_0;
	}

	di->gpio_reset = of_get_named_gpio(np, "loadswitch_reset", 0);
	hwlog_err("loadswitch_reset = %d\n", di->gpio_reset);
	if (!gpio_is_valid(di->gpio_reset))
	{
		hwlog_err("gpio_reset is not valid\n");
		ret = -EINVAL;
		goto fan54151_fail_0;
	}

	ret = gpio_request(di->gpio_reset, "loadswitch_reset");
	if (ret)
	{
		hwlog_err("could not request gpio_reset\n");
		ret = -ENOMEM;
		goto fan54151_fail_0;
	}

	ret = gpio_request(di->gpio_int, "loadswitch_int");
	if (ret)
	{
		hwlog_err("can not request gpio_int\n");
		goto fan54151_fail_1;
	}

	gpio_direction_input(di->gpio_int);
	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0)
	{
		hwlog_err("could not map gpio_int to irq\n");
		goto fan54151_fail_2;
	}

	ret = request_irq(di->irq_int, fan54151_interrupt, IRQF_TRIGGER_FALLING, "loadswitch_int_irq", di);
	if (ret)
	{
		hwlog_err("could not request irq_int\n");
		di->irq_int = -1;
		goto fan54151_fail_2;
	}

	ret = loadswitch_ops_register(&fan54151_ops);
	if (ret)
	{
		hwlog_err("register loadswitch ops failed!\n");
		goto fan54151_fail_3;
	}
	ret = fan54151_sysfs_create_group(di);
	if (ret)
	{
		hwlog_err("create sysfs entries failed!\n");
		goto fan54151_fail_3;
	}
/*
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev =
			    device_create(power_class, NULL, 0, NULL,
					  "charger");
		ret =
		    sysfs_create_link(&charge_dev->kobj, &di->dev->kobj,
				      "fan54151");
		if (ret)
			hwlog_err("create link to fan54151 fail.\n");
	}
*/
	hwlog_info("fan54151 probe ok!\n");
	return 0;

/*fan54151_fail_4:
	fan54151_sysfs_remove_group(di);*/
fan54151_fail_3:
	free_irq(di->irq_int, di);
fan54151_fail_2:
	gpio_free(di->gpio_int);
fan54151_fail_1:
	gpio_free(di->gpio_reset);
fan54151_fail_0:
	kfree(di);
	g_fan54151_dev = NULL;
	np = NULL;

	return ret;
}

/**********************************************************
*  Function:       fan54151_remove
*  Discription:    fan54151 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int fan54151_remove(struct i2c_client *client)
{
	struct fan54151_device_info *di = i2c_get_clientdata(client);

	fan54151_sysfs_remove_group(di);

	/*reset fan54151*/
	gpio_set_value(di->gpio_reset, 0);
	if (di->gpio_reset)
	{
		gpio_free(di->gpio_reset);
	}
	if (di->irq_int)
	{
		free_irq(di->irq_int, di);
	}
	if (di->gpio_int)
	{
		gpio_free(di->gpio_int);
	}
	return 0;
}

MODULE_DEVICE_TABLE(i2c, fan54151);
static struct of_device_id fan54151_of_match[] = {
	{
	 .compatible = "fan54151",
	 .data = NULL,
	 },
	{
	 },
};

static const struct i2c_device_id fan54151_i2c_id[] = {
	{"fan54151", 0}, {}
};

static struct i2c_driver fan54151_driver = {
	.probe = fan54151_probe,
	.remove = fan54151_remove,
	.id_table = fan54151_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "fan54151",
		   .of_match_table = of_match_ptr(fan54151_of_match),
		   },
};

/**********************************************************
*  Function:       fan54151_init
*  Discription:    fan54151 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init fan54151_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&fan54151_driver);
	if (ret)
		hwlog_err("%s: i2c_add_driver error!!!\n", __func__);

	return ret;
}

/**********************************************************
*  Function:       fan54151_exit
*  Discription:    fan54151 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit fan54151_exit(void)
{
	i2c_del_driver(&fan54151_driver);
}

module_init(fan54151_init);
module_exit(fan54151_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("fan54151 module driver");
MODULE_AUTHOR("HW Inc");
