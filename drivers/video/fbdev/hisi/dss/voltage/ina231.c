/*
 * Driver for ina230 or ina231 power monitor chips
 *
 * Copyright (c) 2013- Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include "ina231.h"

struct class *ina231_class;
struct ina231_data *g_idata = NULL;


/*
** for debug, S_IRUGO
** /sys/module/hisifb/parameters
*/
unsigned ina231_msg_level = INA231_DEBUG_LEVEL;
module_param_named(debug_ina231_msg_level, ina231_msg_level, int, 0640);
MODULE_PARM_DESC(debug_ina231_msg_level, "ADC ina231 msg level");

static const struct ina231_config ina231_config[] = {
	[0] = {
		.config_sleep_in = 0x4D34,
		.config_reset = 0x1000,
		.config_work = 0x4D37,
		.calibrate_content = 0x2800,
		.mask_enable_content = 0x0,
		.alert_limit_content = 0x0,

		.shunt_lsb = 2500,
		.bus_voltage_lsb = 1250,
		.current_lsb = 50,
		.power_lsb = 1250,
	},
	[1] = {
		.config_sleep_in = 0x4D34,
		.config_reset = 0x1000,
		.config_work = 0x4D37,
		.calibrate_content = 0x2800,
		.mask_enable_content = 0x0,
		.alert_limit_content = 0x0,

		.shunt_lsb = 2500,
		.bus_voltage_lsb = 1250,
		.current_lsb = 50,
		.power_lsb = 1250,
	},
};

static ssize_t ina231_show_value(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ina231_data *idata = NULL;
	struct i2c_client *client = NULL;
	u64 acc_power = 0;
	u64 acc_current = 0;
	int monitor_num = 0;

	if (NULL == buf){
		INA231_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	if (!dev) {
		INA231_ERR("dev is NULL!\n");
		return snprintf(buf, PAGE_SIZE, "dev is null\n");
	}
	idata = dev_get_drvdata(dev);
	if (!idata) {
		INA231_ERR("data is NULL!\n");
		return snprintf(buf, PAGE_SIZE, "data is null\n");
	}
	client = idata->client;
	if(!client) {
		INA231_ERR("client is NULL!\n");
		return snprintf(buf, PAGE_SIZE, "client is null\n");
	}
	if (idata->flag & ALREADY_READ) {
		INA231_INFO("Data already read!\n");
		return snprintf(buf, PAGE_SIZE, "Data already read!\n");
	}

	mutex_lock(&idata->mutex_lock);
	if (idata->num > 1) {
		monitor_num = idata->num - 1;
		acc_power = idata->acc_power;
		acc_current = idata->acc_current;
	}

	if (idata->flag == 0) {
		idata->num = 1;
		idata->acc_power = 0;
		idata->acc_current = 0;
		idata->flag |= ALREADY_READ;
		i2c_smbus_write_word_swapped(idata->client, INA231_CONFIG, idata->config->config_sleep_in);
	}
	mutex_unlock(&idata->mutex_lock);

	INA231_INFO("Power: %lld, Current:%lld, Num:%d\n", acc_power, (acc_current * idata->config->current_lsb), monitor_num);
	return snprintf(buf, PAGE_SIZE, "Power:%lld, Current:%lld, Num:%d\n",
				acc_power,
				(acc_current * idata->config->current_lsb),
				monitor_num);
}

static ssize_t ina231_store_debug(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	struct i2c_client *client = NULL;
	struct ina231_data *idata = NULL;
	u16 config = 0;
	u16 calibration = 0;
	u16 mask_en = 0;
	u16 alert_limit = 0;

	if (NULL == buf){
		INA231_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}

	ret = sscanf(buf, "config=0x%x, calibration=0x%x, mask_en=0x%x, alert_limit=0x%x", 
				(unsigned int *)&config, (unsigned int *)&calibration, (unsigned int *)&mask_en, (unsigned int *)&alert_limit);
	if (ret < 0) {
		INA231_ERR("check your input!\n");
		return count;
	}

	if (!dev) {
		INA231_ERR("dev is null\n!\n");
		return -1;
	}

	idata = dev_get_drvdata(dev);
	if (!idata) {
		INA231_ERR("idata is null\n!\n");
		return -1;
	}

	client = idata->client;
	if(!client) {
		INA231_ERR("client is null\n!\n");
		return -1;
	}

	INA231_INFO("[client addr=0x%x] config=0x%x, calibration=0x%x, mask_en=0x%x, alert_limit=0x%x\n",
			client->addr, config, calibration, mask_en, alert_limit);

	/* configuration */
	i2c_smbus_write_word_swapped(client, INA231_CONFIG, config);
	/* set calibrate*/
	i2c_smbus_write_word_swapped(client, INA231_CALIBRATION, calibration);
	/*Mask_enable*/
	i2c_smbus_write_word_swapped(client, INA231_MASK_ENABLE, mask_en);
	/*set alert limit*/
	i2c_smbus_write_word_swapped(client, INA231_ALERT_LIMIT, alert_limit);

	return count;
}

static ssize_t ina231_store_set(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct i2c_client *client = NULL;
	struct ina231_data *idata = NULL;
	unsigned int value;
	ssize_t ret = count;

	if (!dev) {
		INA231_ERR("dev is NULL!!!\n");
		return ret;
	}
	if (NULL == buf){
		INA231_ERR("NULL_PTR ERROR!\n");
		return -EINVAL;
	}
	ret = sscanf(buf, "%u", &value);
	if (ret < 0) {
		INA231_ERR("check your input!!!\n");
		ret = count;
		return ret;
	}

	idata = dev_get_drvdata(dev);
	if (!idata) {
		INA231_ERR("idata is NULL!!!\n");
		return ret;
	}
	client = idata->client;
	if(!client) {
		INA231_ERR("client is NULL!!!\n");
		return ret;
	}

	INA231_INFO("[client addr=0x%x]config_sleep_in:0x%x, config_work:0x%x, calibration:0x%x, mask_en:0x%x, alert_limit:0x%x\n",
			client->addr,
			idata->config->config_sleep_in,
			idata->config->config_work,
			idata->config->calibrate_content,
			idata->config->mask_enable_content,
			idata->config->alert_limit_content);

	if (value) {
		INA231_INFO("goto work mode, starting testing\n");
		/* configuration */
		i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_work);
		/* set calibrate*/
		i2c_smbus_write_word_swapped(client, INA231_CALIBRATION, idata->config->calibrate_content);
		/*Mask_enable*/
		i2c_smbus_write_word_swapped(client, INA231_MASK_ENABLE, idata->config->mask_enable_content);
		/*set alert limit*/
		i2c_smbus_write_word_swapped(client, INA231_ALERT_LIMIT, idata->config->alert_limit_content);
	} else {
		INA231_INFO("goto sleep mode, ending testing\n");
		/* configuration to sleep */
		i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_sleep_in);
	}
	return count;
}

/* config for debug*/
static DEVICE_ATTR(ina231_debug, S_IWUSR, NULL, ina231_store_debug);

/*test*/
static DEVICE_ATTR(ina231_set, S_IWUSR, NULL, ina231_store_set);

/* get value */
static DEVICE_ATTR(ina231_value, S_IRUGO, ina231_show_value, NULL);


/* pointers to created device attributes */
static struct attribute *ina231_attributes[] = {
	&dev_attr_ina231_debug.attr,
	&dev_attr_ina231_value.attr,
	&dev_attr_ina231_set.attr,
	NULL,
};

static const struct attribute_group ina231_group = {
	.attrs = ina231_attributes,
};

static void ina231_monitor_wq_handler(struct work_struct *work)
{
	int battery_voltage = 0;
	struct ina231_data *idata = NULL;
	static int s_battery_voltage = 4000;
	int current_data = 0;
	unsigned long expires = 0;
	int ret = 0;

	idata = container_of(work, struct ina231_data, wq);
	if (NULL == idata) {
		INA231_ERR("idata is NULL\n");
		return;
	}

	INA231_DEBUG("enter!\n");
	if (idata->flag & LCD_RESUME) {
		/* communication check and reset device*/
		ret = i2c_smbus_write_word_swapped(idata->client, INA231_CONFIG, idata->config->config_reset);
		if (ret < 0) {
			INA231_ERR("Reset failed\n");
			return;
		}
		msleep(20);
		/* configuration */
		ret = i2c_smbus_write_word_swapped(idata->client, INA231_CONFIG, idata->config->config_work);
		if (ret < 0) {
			INA231_ERR("Setting configuration failed\n");
			return;
		}
		/* set calibrate*/
		ret = i2c_smbus_write_word_swapped(idata->client, INA231_CALIBRATION, idata->config->calibrate_content);
		if (ret < 0) {
			INA231_ERR("Setting calibrate failed\n");
			return;
		}

		mutex_lock(&idata->mutex_lock);
		idata->num = 1;
		idata->flag &= ~LCD_RESUME;
		idata->flag &= ~ALREADY_READ;
		idata->acc_current = 0;
		idata->acc_power = 0;
		mutex_unlock(&idata->mutex_lock);
		expires = jiffies + INA231_SAMPLE_TIME * HZ;
		mod_timer(&(idata->ina321_timer), expires);
	}else {
		current_data = i2c_smbus_read_word_swapped(idata->client, INA231_CURRENT);
		if (current_data < 0) {
			INA231_ERR("Reading i2c data failed!\n");
			return;
		}
		INA231_DEBUG("current_data = %d\n", current_data);
		mutex_lock(&idata->mutex_lock);
		battery_voltage = hisi_battery_voltage();
		if (battery_voltage) {
			s_battery_voltage = battery_voltage;
		} else {
			INA231_ERR("Read hisi_battery_voltage failed!\n");
		}
		idata->acc_power += ((u64)s_battery_voltage * current_data * idata->config->current_lsb) / INA231_POWER_UNIT_CONVERSION;
		idata->acc_current += current_data;
		INA231_DEBUG("idata->acc_current = %lld\n", idata->acc_current);
		idata->num++;
		mutex_unlock(&idata->mutex_lock);
	}
}

static void ina231_monitor_timer_func(unsigned long data)
{
	unsigned long expires = 0;
	struct ina231_data *idata = NULL;

	idata = (struct ina231_data *)data;
	if (NULL == idata) {
		INA231_ERR("idata is NULL\n");
		return;
	}

	INA231_DEBUG("enter!\n");
	schedule_work(&(idata->wq));
	expires = jiffies + INA231_SAMPLE_TIME * HZ;
	mod_timer(&(idata->ina321_timer), expires);
}

int ina231_power_monitor_on(void)
{
	if (NULL == g_idata) {
		INA231_DEBUG("ina231 not register!");
		return INA231_OK;
	}

	INA231_DEBUG("enter!\n");
	mutex_lock(&g_idata->mutex_lock);
	g_idata->flag = 0;
	g_idata->flag |= LCD_ON | LCD_RESUME | ALREADY_READ;
	mutex_unlock(&g_idata->mutex_lock);
	schedule_work(&(g_idata->wq));
	return INA231_OK;
}

int ina231_power_monitor_off(void)
{
	if (NULL == g_idata) {
		INA231_DEBUG("ina231 not register!");
		return INA231_OK;
	}

	INA231_DEBUG("enter!\n");
	mutex_lock(&g_idata->mutex_lock);
	g_idata->flag &= ~LCD_ON;
	mutex_unlock(&g_idata->mutex_lock);

	cancel_work_sync(&g_idata->wq);
	del_timer_sync(&(g_idata->ina321_timer));
	return INA231_OK;
}

static int ina231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct ina231_data *idata;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	idata = devm_kzalloc(&client->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	/* set the device type */
	idata->type = id->driver_data;
	idata->config = &ina231_config[idata->type];
	idata->client = client;

	/* communication check and reset device*/
	ret = i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_reset);
	if (ret < 0) {
		INA231_ERR("Reset failed\n");
		return -ENODEV;
	}
	mdelay(20);

	/* configuration */
	ret = i2c_smbus_write_word_swapped(idata->client, INA231_CONFIG, idata->config->config_work);
	if (ret < 0) {
		INA231_ERR("Setting configuration failed\n");
		return -ENODEV;
	}
	/* set calibrate*/
	ret = i2c_smbus_write_word_swapped(idata->client, INA231_CALIBRATION, idata->config->calibrate_content);
	if (ret < 0) {
		INA231_ERR("Setting calibrate failed\n");
		return -ENODEV;
	}

	i2c_set_clientdata(client, idata);
	idata->dev = device_create(ina231_class, NULL, 0, NULL, "%s", client->name);
	if (IS_ERR(idata->dev)) {
		/* Not fatal */
		INA231_ERR("Unable to create device; errno = %ld\n", PTR_ERR(idata->dev));
		idata->dev = NULL;
	} else {
		dev_set_drvdata(idata->dev, idata);
		ret = sysfs_create_group(&idata->dev->kobj, &ina231_group);
		if (ret)
			return ret;
	}

	INIT_WORK(&(idata->wq), ina231_monitor_wq_handler);
	init_timer(&(idata->ina321_timer));
	idata->ina321_timer.function = ina231_monitor_timer_func;
	idata->ina321_timer.data = (unsigned long)idata;
	idata->ina321_timer.expires = jiffies + INA231_SAMPLE_TIME * HZ;
	add_timer(&(idata->ina321_timer));
	idata->num = 1;
	idata->flag |= LCD_ON;
	mutex_init(&idata->mutex_lock);
	g_idata = idata;
	dev_info(&client->dev, "name:%s(address:0x%x) probe successfully\n", client->name, client->addr);
	return 0;
}

static int ina231_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &ina231_group);
	return 0;
}

static const struct i2c_device_id ina231_id[] = {
	{"ina231_0", 0},
	{"ina231_1", 1},
	{ },
};
MODULE_DEVICE_TABLE(i2c, ina231_id);

static struct of_device_id ina231_match_table[] = {
	{
		.compatible = "huawei,ina231_0",
		.data = NULL,
	},
	{
		.compatible = "huawei,ina231_1",
		.data = NULL,
	},
	{ },
};

MODULE_DEVICE_TABLE(of, ina231_of_match);

static struct i2c_driver ina231_driver = {
	.probe		= ina231_probe,
	.remove		= ina231_remove,
	.shutdown = NULL,
	.driver = {
		.name = "huawei_ina231",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ina231_match_table),
	},
	.id_table = ina231_id,
};

static int __init ina231_module_init(void)
{
	int ret = 0;

	ina231_class = class_create(THIS_MODULE, "ina231");
	if (IS_ERR(ina231_class)) {
		INA231_ERR("Unable to create ina231 class; errno = %ld\n", PTR_ERR(ina231_class));
		ina231_class = NULL;
	}

	ret = i2c_add_driver(&ina231_driver);
	if (ret)
		INA231_ERR("Unable to register ina231 driver\n");

	return ret;
}

late_initcall(ina231_module_init);

MODULE_DESCRIPTION("ina231 driver");
MODULE_LICENSE("GPL");
