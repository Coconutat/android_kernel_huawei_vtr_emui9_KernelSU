/* drivers/input/touchscreen/sec_ts_fw.c
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>
#include <linux/time.h>
#include <linux/vmalloc.h>

#include <linux/uaccess.h>
/*#include <asm/gpio.h>*/

#include "sec_ts.h"
struct class *sec_class;
EXPORT_SYMBOL(sec_class);

u8 lv1cmd;
u8 *read_lv1_buff;
static int lv1_readsize;
static int lv1_readremain;
static int lv1_readoffset;

static ssize_t sec_ts_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_INFO("%s: Power off state\n", __func__);
		return -EIO;
	}

	if (size > 0)
		ts->sec_ts_i2c_write_burst(ts, (u8 *)buf, size);

	TS_LOG_INFO("%s: 0x%x, 0x%x, size %d\n", __func__, buf[0], buf[1], (int)size);
	return size;
}

static ssize_t sec_ts_regread_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	int ret = NO_ERR;
	int length = 0;
	int remain = 0;
	int offset = 0;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: Power off state\n", __func__);
		return -EIO;
	}

	if ((lv1_readsize < 0) || (lv1_readsize > SEC_TS_DEBUGBUF_SIZE_MAX)) {
		TS_LOG_ERR("%s: wrong buffer size\n", __func__);
		return -EIO;
	}

	disable_irq(ts->client->irq);

	mutex_lock(&ts->device_mutex);

	read_lv1_buff = kzalloc(lv1_readsize, GFP_KERNEL);
	if (!read_lv1_buff)
		goto malloc_err;

	remain = lv1_readsize;
	offset = 0;
	do {
		if (remain >= ts->i2c_burstmax)
			length = ts->i2c_burstmax;
		else
			length = remain;

		if (offset == 0)
			ret = ts->sec_ts_i2c_read(ts, lv1cmd, &read_lv1_buff[offset], length);
		else
			ret = ts->sec_ts_i2c_read_bulk (ts, &read_lv1_buff[offset], length);

		if (ret < 0) {
			TS_LOG_ERR("%s: i2c read %x command, remain =%d\n", __func__, lv1cmd, remain);
			goto i2c_err;
		}

		remain -= length;
		offset += length;
	} while (remain > 0);

	TS_LOG_INFO("%s: lv1_readsize = %d\n", __func__, lv1_readsize);
	memcpy(buf, read_lv1_buff + lv1_readoffset, lv1_readsize);

i2c_err:
	kfree(read_lv1_buff);
malloc_err:
	mutex_unlock(&ts->device_mutex);
	lv1_readremain = 0;
	enable_irq(ts->client->irq);

	return lv1_readsize;
}

static ssize_t sec_ts_gesture_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->device_mutex);
	memcpy(buf, ts->gesture_status, sizeof(ts->gesture_status));
	TS_LOG_INFO("%s: GESTURE STATUS %x %x %x %x %x %x\n", __func__,
				ts->gesture_status[0], ts->gesture_status[1], ts->gesture_status[2],
				ts->gesture_status[3], ts->gesture_status[4], ts->gesture_status[5]);
	mutex_unlock(&ts->device_mutex);

	return sizeof(ts->gesture_status);
}

static ssize_t sec_ts_regreadsize_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->device_mutex);

	lv1cmd = buf[0];
	lv1_readsize = ((unsigned int)buf[4] << 24) |
			((unsigned int)buf[3] << 16) | ((unsigned int) buf[2] << 8) | ((unsigned int)buf[1] << 0);
	lv1_readoffset = 0;
	lv1_readremain = 0;

	mutex_unlock(&ts->device_mutex);

	return size;
}

static ssize_t sec_ts_enter_recovery_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
	struct sec_ts_plat_data *pdata = ts->plat_data;
	int ret = 0;
	unsigned long on = 0;

	ret = kstrtoul(buf, 10, &on);
	if (ret != 0) {
		TS_LOG_ERR("%s: failed to read:%d\n",
					__func__, ret);
		return -EINVAL;
	}

	if (on) {
		disable_irq(ts->client->irq);
		gpio_free(pdata->irq_gpio);

		TS_LOG_INFO("%s: gpio free\n", __func__);
		if (gpio_is_valid(pdata->irq_gpio)) {
			ret = gpio_request_one(pdata->irq_gpio, GPIOF_OUT_INIT_LOW, "sec,tsp_int");
			TS_LOG_INFO("%s: gpio request one\n", __func__);
			if (ret < 0)
				TS_LOG_ERR("%s: Unable to request tsp_int [%d]: %d\n", __func__, pdata->irq_gpio, ret);
		} else {
			TS_LOG_ERR("%s: Failed to get irq gpio\n", __func__);
			return -EINVAL;
		}

		pdata->power(ts, false);
		sec_ts_delay(100);
		pdata->power(ts, true);
	} else {
		gpio_free(pdata->irq_gpio);

		if (gpio_is_valid(pdata->irq_gpio)) {
			ret = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "sec,tsp_int");
			if (ret) {
				TS_LOG_ERR("%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
				return -EINVAL;
			}
		} else {
			TS_LOG_ERR("%s: Failed to get irq gpio\n", __func__);
			return -EINVAL;
		}

		pdata->power(ts, false);
		sec_ts_delay(500);
		pdata->power(ts, true);
		sec_ts_delay(500);

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CALIBRATION_AMBIENT, NULL, 0);
		if (ret < 0)
			TS_LOG_ERR("%s: fail to write AFE_CAL\n", __func__);

		sec_ts_delay(1000);
		enable_irq(ts->client->irq);
	}

	ret = sec_ts_read_information(ts);
	if (ret < 0) {
		TS_LOG_ERR("sec_ts_read_information error\n");
	}

	return size;
}

static DEVICE_ATTR(sec_ts_reg, (S_IWUSR | S_IWGRP), NULL, sec_ts_reg_store);
static DEVICE_ATTR(sec_ts_regreadsize, (S_IWUSR | S_IWGRP), NULL, sec_ts_regreadsize_store);
static DEVICE_ATTR(sec_ts_enter_recovery, (S_IWUSR | S_IWGRP), NULL, sec_ts_enter_recovery_store);
static DEVICE_ATTR(sec_ts_regread, S_IRUGO, sec_ts_regread_show, NULL);
static DEVICE_ATTR(sec_ts_gesture_status, S_IRUGO, sec_ts_gesture_status_show, NULL);

static struct attribute *cmd_attributes[] = {
	&dev_attr_sec_ts_reg.attr,
	&dev_attr_sec_ts_regreadsize.attr,
	&dev_attr_sec_ts_enter_recovery.attr,
	&dev_attr_sec_ts_regread.attr,
	&dev_attr_sec_ts_gesture_status.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

int sec_ts_raw_device_init(struct sec_ts_data *ts)
{
	int ret = NO_ERR;

	if (IS_ERR_OR_NULL(ts)) {
		TS_LOG_ERR("%s: fail - invalid param\n", __func__);
		return -EINVAL;
	}

	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class)) {
		TS_LOG_ERR("%s:fail - class_create\n", __func__);
		return -EIO;
	}
	ts->dev = device_create(sec_class, NULL, 0, ts, "sec_ts");

	ret = IS_ERR(ts->dev);
	if (ret) {
		TS_LOG_ERR("%s: fail - device_create\n", __func__);
		return ret;
	}

	ret = sysfs_create_group(&ts->dev->kobj, &cmd_attr_group);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail - sysfs_create_group\n", __func__);
		goto err_sysfs;
	}
	return ret;
err_sysfs:
	TS_LOG_ERR("%s: fail\n", __func__);
	return ret;
}
