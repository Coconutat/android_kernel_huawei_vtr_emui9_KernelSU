/*
 * deviceboxID.c -- deviceboxID driver
 *
 * Copyright (c) 2014 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/ioctl.h>

#include "deviceboxID.h"

#define LOG_TAG "deviceboxID"
/*lint -e528*/
#define DEVICEBOX_ID_NAME    "deviceboxID"
#define ADC_READ_COUNT       3
#define DEVICEBOX_ID_LIMIT   500
#define PAGE_SIZE_MAX        1024
#define NAME_LEN_MAX         32
#define VENDOR_DEFAULT       0
#define BOX_NUM_DEFAULT      0
#define ELECTRIC_LEVEL_LOW   0
#define ELECTRIC_LEVEL_HIGH  1
#define VENDOR               2
#define BOX_NUM_MAX          4
#define BOX_INDEX_DEFAULT    -1
#define GPIO_INDEX_DEFAULT   -1
#define DEVICEBOX_ID_GET_SPEAKER		_IOR('H', 0x01, __u32)
#define DEVICEBOX_ID_GET_RECEIVER		_IOR('H', 0x02, __u32)
#define DEVICEBOX_ID_GET_3rdBOX			_IOR('H', 0x03, __u32)
#define DEVICEBOX_ID_GET_4thBOX			_IOR('H', 0x04, __u32)
#define DEVICEBOX_ID_GET_BOX_NUM		_IOR('H', 0xff, __u32)

#define DEVICEBOX_ID_MODE_OF_GPIO       "gpio"
#define CODEC_GPIO_BASE                 (224)
#define SUPPORT_GPIO_NUM_MAX            2
#define GPIO_ID_FAIL                    -1
#define COEFF_OF_READ_ARRAY_NUM         2
#define BOXID_VDD_MIN_UV                1800000
#define BOXID_VDD_MAX_UV                1800000

enum Box_vendor {
	BOX_NAME_DEFAULT = 0,
	BOX_NAME_AAC,
	BOX_NAME_GOER,
	BOX_NAME_GD,
	BOX_NAME_LC,
	BOX_NAME_LX,
	BOX_NAME_XW,
	BOX_NAME_PU,
	BOX_NAME_PD,
	BOX_NAME_NP,
	BOX_NAME_QS,
	BOX_VENDOR_MAX
};

static char *boxtable[BOX_VENDOR_MAX]={"", "AAC", "GOER", "GD ", "LC ", "LX ", "XW ", "PU ", "PD ", "NP ", "QS "};

enum {
	DEVICEBOX_ID_MODE_USE_GPIO	= 0x0,
	DEVICEBOX_ID_MODE_USE_ADC	= 0x1,
};

enum {
	USE_ONE_GPIO	= 1, /*default value*/
	USE_TWO_GPIO	= 2,
};

enum {
	BOX_ID_DISABLED	= 0,
	BOX_ID_ENABLED	= 1,
};

enum {
	GPIO_REQ_FAIL	    = -1,
	GPIO_REQ_SUCCESS	= 0,
};


/*define gpio and map name,which is the same as dtsi config*/
static const char * const gpio_name_table[BOX_NUM_MAX] = {
	[SPEAKER_ID]  = "gpio_speakerID",
	[RECEIVER_ID] = "gpio_receiverID",
	[BOX_3rd_ID]  = "gpio_3rdboxID",
	[BOX_4th_ID]  = "gpio_4thboxID",
};

static const char * const box_map_table[BOX_NUM_MAX] = {
	[SPEAKER_ID]  = "speaker_map",
	[RECEIVER_ID] = "receiver_map",
	[BOX_3rd_ID]  = "box3rd_map",
	[BOX_4th_ID]  = "box4th_map",
};

struct box_info_st{
	int box_enable;
	int gpio_num;
	int gpio_id[SUPPORT_GPIO_NUM_MAX];
	int gpio_request_flag[SUPPORT_GPIO_NUM_MAX];
	int gpio_status[SUPPORT_GPIO_NUM_MAX];
	int box_map[VENDOR][VENDOR];
};

struct out_audio_device_id {
	int adc_channel;
	int deviceboxID_limit;
	int check_mode;
	int box_num;
	bool gpio_extra_pull_up_enable;
	struct regulator *pull_up_vdd;
	struct box_info_st box_info[BOX_NUM_MAX];
};

struct out_audio_device_id deviceboxID = {-1};

int deviceboxID_read(int out_id)
{
	int id = out_id;
	int vendor = VENDOR_DEFAULT;

	if (id > BOX_4th_ID) {
		pr_err("%s: out_id is invalid\n", __func__);
		return -EINVAL;
	}

	if (DEVICEBOX_ID_MODE_USE_GPIO == deviceboxID.check_mode) {
		if (deviceboxID.box_info[id].gpio_num == USE_ONE_GPIO) {
			vendor = deviceboxID.box_info[id].box_map[0][deviceboxID.box_info[id].gpio_status[0]];
		} else if (deviceboxID.box_info[id].gpio_num == USE_TWO_GPIO) {
			vendor = deviceboxID.box_info[id].box_map[deviceboxID.box_info[id].gpio_status[0]][deviceboxID.box_info[id].gpio_status[1]];
		}
	}

	pr_info("deviceboxID_read box_id:%d  vendor:%d\n", id, vendor);
	return vendor;
}
EXPORT_SYMBOL(deviceboxID_read);

/*lint -save -e1564 -e120 -e101 -e1058 -e438 -e550 -e529 -e78 -e530 -e529 -e160 -e64 -e40*/
static long deviceboxID_do_ioctl(struct file *file, unsigned int cmd,
				 void __user *p, int compat_mode)
{
	int ret = 0;
	unsigned int value = VENDOR_DEFAULT;
	unsigned int __user *pUser = (unsigned int __user *) p;

	if (NULL == pUser) {
		pr_err("%s: pUser is null\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case DEVICEBOX_ID_GET_SPEAKER:
		if (deviceboxID.box_info[SPEAKER_ID].box_enable) {
			value = deviceboxID_read(SPEAKER_ID);
		}
		ret |= put_user(value, pUser);
		break;
	case DEVICEBOX_ID_GET_RECEIVER:
		if (deviceboxID.box_info[RECEIVER_ID].box_enable) {
			value = deviceboxID_read(RECEIVER_ID);
		}
		ret |= put_user(value, pUser);
		break;
	case DEVICEBOX_ID_GET_3rdBOX:
		if (deviceboxID.box_info[BOX_3rd_ID].box_enable) {
			value = deviceboxID_read(BOX_3rd_ID);
		}
		ret |= put_user(value, pUser);
		break;
	case DEVICEBOX_ID_GET_4thBOX:
		if (deviceboxID.box_info[BOX_4th_ID].box_enable) {
			value = deviceboxID_read(BOX_4th_ID);
		}
		ret |= put_user(value, pUser);
		break;
	case DEVICEBOX_ID_GET_BOX_NUM:
		value = deviceboxID.box_num;
		ret |= put_user(value, pUser);
		break;
	default:
		pr_err("unsupport deviceboxID cmd\n");
		ret = -EINVAL;
		break;
	}
	return (long)ret;
}
/*lint -restore*/
static long deviceboxID_ioctl(struct file *file, unsigned int command,
						  unsigned long arg)
{
 /**
  * The use of parameters "0" is to meet format of linux driver,
  * it has no practical significance.
  */
	return deviceboxID_do_ioctl(file, command, (void __user *)arg, 0);
}
/*lint -restore*/
static long deviceboxID_compat_ioctl(struct file *file, unsigned int command,
						  unsigned long arg)
{
 /**
  * The use of parameters "0" is to meet format of linux driver,
  * it has no practical significance.
  */
	return deviceboxID_do_ioctl(file, command, (void __user *)compat_ptr(arg), 0);
}

/*lint -save -e528 */
static const struct of_device_id deviceboxID_match[] = {
	{ .compatible = "huawei,deviceboxID", },
	{},
};
/*lint -restore*/
/*lint -save -e814 */
MODULE_DEVICE_TABLE(of, deviceboxID_match);
/*lint -restore*/

static const struct file_operations deviceboxID_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = deviceboxID_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = deviceboxID_compat_ioctl,
#endif
};

static struct miscdevice deviceboxID_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = DEVICEBOX_ID_NAME,
	.fops   = &deviceboxID_fops,
};

char *get_box_name(int map_id)
{
	if((map_id > BOX_NAME_DEFAULT)&&(map_id < BOX_VENDOR_MAX)) {
		return boxtable[map_id];
	}else{
		return "NA ";
	}
}

#ifdef DEVICEBOXID_DEBUG
static ssize_t deviceboxID_info_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int box_index = BOX_INDEX_DEFAULT;
	int gpio_index = GPIO_INDEX_DEFAULT;
	int map_id = BOX_NAME_DEFAULT;

	if(NULL == buf) {
		pr_err("%s: buf is null\n", __func__);
		return 0;
	}

	snprintf(buf, PAGE_SIZE_MAX, "---deviceboxID info begin---\n");

	snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "GPIO_STATUS:\n");
	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			for (gpio_index = 0; gpio_index < deviceboxID.box_info[box_index].gpio_num; gpio_index++) {
				snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    box[%d].gpio[%d].status:%d\n",
					box_index, gpio_index, deviceboxID.box_info[box_index].gpio_status[gpio_index]);
			}
		}
	}

	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "Box[%d]_MAP:\n", box_index);
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    -----------------\n");
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    |  %s  |  %s  |\n",
					get_box_name(deviceboxID.box_info[box_index].box_map[0][0]),
					get_box_name(deviceboxID.box_info[box_index].box_map[0][1]));
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    -----------------\n");
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    |  %s  |  %s  |\n",
					get_box_name(deviceboxID.box_info[box_index].box_map[1][0]),
					get_box_name(deviceboxID.box_info[box_index].box_map[1][1]));
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    -----------------\n");
		}
	}

	snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "Box_NAME:\n");
	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			map_id = deviceboxID_read(box_index);
			snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "    box[%d]  :  %s\n", box_index, get_box_name(map_id));
		}
	}

	snprintf(buf+strlen(buf), PAGE_SIZE_MAX-strlen(buf), "---deviceboxID info end---\n");

	return strlen(buf);

}
/*lint -save -e84 -e514 */
static DEVICE_ATTR(deviceboxID_info,  S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP, deviceboxID_info_show, NULL);
/*lint -restore*/
static struct attribute *deviceboxID_attributes[] = {
	&dev_attr_deviceboxID_info.attr,
	NULL,
};

static const struct attribute_group deviceboxID_attr_group = {
	.attrs = deviceboxID_attributes,
};
#endif

static int get_check_mode(struct device_node *dev_node)
{
	const char *mode = NULL;
	int rc = 0;

	if (NULL == dev_node) {
		pr_err("%s: dev_node is null\n", __func__);
		/*if dev_node is NULL, select GPIO mode*/
		return DEVICEBOX_ID_MODE_USE_GPIO;
	}
	/*get check mode*/
	rc = of_property_read_string(dev_node, "check_mode", &mode);
	if (rc) {
		pr_info("%s: not find dev_node ,rc=%d\n", __func__, rc);
		return DEVICEBOX_ID_MODE_USE_ADC;
	} else {
		pr_info("%s: mode: %s\n", __func__, mode);
	}

	if (!strncmp(mode, DEVICEBOX_ID_MODE_OF_GPIO, sizeof(DEVICEBOX_ID_MODE_OF_GPIO))) {
		return DEVICEBOX_ID_MODE_USE_GPIO;
	} else {
		return DEVICEBOX_ID_MODE_USE_ADC;
	}
}

static int get_box_num(struct device_node *dev_node)
{
	int box_num = BOX_NUM_DEFAULT;
	int rc = 0;

	if (NULL == dev_node) {
		pr_err("%s: dev_node is null\n", __func__);
		return BOX_NUM_DEFAULT;
	}
	/*get box number*/
	/*lint -save -e64 */
	rc = of_property_read_u32(dev_node, "box_num", &box_num);
	/*lint -restore*/
	if (rc < 0) {
		pr_info("%s: not find dev_node ,rc=%d\n", __func__, rc);
	} else {
		pr_info("%s: box_num: %d\n", __func__, box_num);
	}

	return box_num;
}

/*lint -save -e40 */
static int get_gpio_status(struct device_node *node, char *propname, int box_index, int gpio_index)
{
	int ret = 0;

	pr_debug("%s, propname=%s, box_index=%d, gpio_index=%d\n", __func__, propname, box_index, gpio_index);
	if (NULL == node || NULL == propname) {
		pr_err("%s: node or propname is null\n", __func__);
		return -ENOENT;
	}
	if (box_index >= deviceboxID.box_num || gpio_index >= deviceboxID.box_info[box_index].gpio_num) {
		pr_err("%s: box_index or gpio_index is invalid\n", __func__);
		return -ENOENT;
	}
	deviceboxID.box_info[box_index].gpio_id[gpio_index]= of_get_named_gpio(node, propname, gpio_index);
	if (deviceboxID.box_info[box_index].gpio_id[gpio_index]  < 0) {
		pr_err("%s: box_info[%d].gpio_id[%d] is unvalid!\n", __func__, box_index, gpio_index);
		return -ENOENT;
	}

	if (!gpio_is_valid(deviceboxID.box_info[box_index].gpio_id[gpio_index])) {
		pr_err("%s:box_info[%d].gpio_id[%d] is unvalid!\n", __func__, box_index, gpio_index);
		return -ENOENT;
	}

	ret = gpio_request(deviceboxID.box_info[box_index].gpio_id[gpio_index], "gpio_id_check");
	if (ret) {
		pr_err("%s:error request, box_info[%d].gpio_id[%d] for gpio_id_check fail %d\n", __func__, box_index, gpio_index, ret);
		return -ENOENT;
	} else {
		deviceboxID.box_info[box_index].gpio_request_flag[gpio_index] = ret;
	}

	/* set gpio to input status */
	ret = gpio_direction_input(deviceboxID.box_info[box_index].gpio_id[gpio_index]);
	if (ret) {
		pr_err("%s:set gpio to input status error! ret is %d\n", __func__, ret);
	}

	deviceboxID.box_info[box_index].gpio_status[gpio_index] = gpio_get_value_cansleep(deviceboxID.box_info[box_index].gpio_id[gpio_index]);

	if (deviceboxID.box_info[box_index].gpio_status[gpio_index] < ELECTRIC_LEVEL_LOW ||
		deviceboxID.box_info[box_index].gpio_status[gpio_index] > ELECTRIC_LEVEL_HIGH ) {
		/*if get status failed , we should set a default value to avoid overrange*/
		deviceboxID.box_info[box_index].gpio_status[gpio_index] = ELECTRIC_LEVEL_LOW;
		pr_err("%s:box_info[%d].gpio_status[%d] is invalid\n", __func__, box_index, gpio_index);
		return -ENOENT;

	}
	pr_info("%s:read box_info[%d].gpio_status[%d] is %d\n", __func__, box_index, gpio_index, deviceboxID.box_info[box_index].gpio_status[gpio_index]);

	return 0;
}
/*lint -restore*/
static bool get_boxID_extra_pull_up_enable(struct device_node *node)
{
	if (NULL == node) {
		pr_err("%s: node is null\n", __func__);
		return false;
	}
	if (of_property_read_bool(node, "boxID_extra_pull_up_enable")) {
		pr_info("%s: boxID extra pull up regulator enabled\n", __func__);
		return true;
	} else {
		pr_info("%s: boxID extra pull up regulator not set\n", __func__);
		return false;
	}
}

static struct regulator *get_boxID_gpio_vdd(struct device *dev)
{
	struct regulator *vdd = NULL;

	if (NULL == dev) {
		pr_err("%s: dev is null\n", __func__);
		return NULL;
	}

	vdd = regulator_get(dev, "boxID_pull_up_vdd");
	if (IS_ERR(vdd)) {
		pr_err("%s: boxID regulator get failed\n", __func__);
		return NULL;
	}
	return vdd;
}

static int get_deviceboxID_info(struct device_node *node)
{
	int i = 0;
	int temp = BOX_ID_DISABLED;

	if (NULL == node) {
		pr_err("%s: node is null\n", __func__);
		return -ENOENT;
	}

	pr_debug("%s\n", __func__);

	deviceboxID.check_mode = get_check_mode(node);

	deviceboxID.box_num = get_box_num(node);
	if (deviceboxID.box_num > BOX_NUM_MAX) {
		pr_err("%s: box_num is out of range\n", __func__);
		return -ENOENT;
	}

	deviceboxID.gpio_extra_pull_up_enable = get_boxID_extra_pull_up_enable(node);

	for(i=0; i<deviceboxID.box_num; i++) {
		/*lint -save -e64 */
		if(!of_property_read_u32_index(node, "enable_boxID", i, &temp)) {
		/*lint -restore*/
			deviceboxID.box_info[i].box_enable = temp;
		} else {
			deviceboxID.box_info[i].box_enable = BOX_ID_DISABLED;
		}
		pr_debug("%s, box_info[%d].box_enable=%d\n", __func__, i, deviceboxID.box_info[i].box_enable);
	}

	for(i=0; i<deviceboxID.box_num; i++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[i].box_enable) {
			/*lint -save -e64 */
			if(!of_property_read_u32_index(node, "gpio_num", i, &temp)) {
			/*lint -restore*/
				deviceboxID.box_info[i].gpio_num = temp;
			} else {
				deviceboxID.box_info[i].gpio_num = USE_ONE_GPIO;
			}
		}
		pr_debug("%s, box_info[%d].gpio_num=%d\n", __func__, i, deviceboxID.box_info[i].gpio_num);
	}

	return 0;
}
/*lint -save -e64 -e40*/
static int get_deviceboxID_map_priv(struct device_node *node, char *propname, int box_index)
{
	int ret = 0;

	pr_debug("%s, propname=%s, box_index=%d, gpio_num=%d\n", __func__, propname, box_index, deviceboxID.box_info[box_index].gpio_num);
	if (NULL == node || NULL == propname) {
		pr_err("%s: node or propname is null\n", __func__);
		return -ENOENT;
	}
	if (box_index >= deviceboxID.box_num){
		pr_err("box_index is invalid\n");
		return -ENOENT;
	}
	if (deviceboxID.box_info[box_index].gpio_num == USE_ONE_GPIO) {
		if (of_property_read_u32_array(node, propname, deviceboxID.box_info[box_index].box_map[0],
								sizeof(deviceboxID.box_info[box_index].box_map) / sizeof(int) / COEFF_OF_READ_ARRAY_NUM)) {
			pr_err("of_property_read_u32_array by one gpio box_id_map err,box_index:%d\n",box_index);
			ret = -ENOENT;
		} else
			pr_debug("%s, box_index=%d\n", __func__, box_index);
	} else if (deviceboxID.box_info[box_index].gpio_num == USE_TWO_GPIO) {
		if (of_property_read_u32_array(node, propname, deviceboxID.box_info[box_index].box_map[0],
								sizeof(deviceboxID.box_info[box_index].box_map) / sizeof(int))) {
			pr_err("of_property_read_u32_array by two gpio box_id_map err,box_index:%d\n",box_index);
			ret = -ENOENT;
		} else
			pr_debug("%s, box_index=%d\n", __func__, box_index);
	} else {
		pr_err("gpio_num out of range,box[%d].gpio_num:%d\n", box_index, deviceboxID.box_info[box_index].gpio_num);
		ret = -ENOENT;
	}

	return ret;
}
/*lint -restore*/
static int get_deviceboxID_map(struct device_node *node)
{
	int ret = 0;
	int box_index = BOX_INDEX_DEFAULT;
	char map_name[NAME_LEN_MAX]={0};

	pr_debug("%s\n", __func__);

	if (NULL == node) {
		pr_err("%s: node is null\n", __func__);
		return -ENOENT;
	}

	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			strncpy(map_name, box_map_table[box_index], NAME_LEN_MAX-1);
			ret = get_deviceboxID_map_priv(node, map_name, box_index);
			if(ret) {
				return ret;
			}
			memset(map_name, 0, NAME_LEN_MAX);
		}
	}

	return ret;
}

static int get_deviceboxID_gpio_status(struct device_node *node)
{
	int ret = 0;
	int box_index = BOX_INDEX_DEFAULT;
	int	gpio_index = GPIO_INDEX_DEFAULT;
	char gpio_name[NAME_LEN_MAX]={0};

	if (NULL == node) {
		pr_err("%s: node is null\n", __func__);
		return -ENOENT;
	}

	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		strncpy(gpio_name, gpio_name_table[box_index], NAME_LEN_MAX-1);
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			for (gpio_index = 0; gpio_index < deviceboxID.box_info[box_index].gpio_num; gpio_index++) {
				ret = get_gpio_status(node, gpio_name, box_index, gpio_index);
				if(ret != 0) {
					return ret;
				}
			}
		}
		memset(gpio_name, 0, NAME_LEN_MAX);
	}

	return 0;
}

static int boxID_regulator_config(struct regulator *vdd, bool on)
{
	int rc = 0;

	if (NULL == vdd) {
		pr_err("%s: vdd is null\n", __func__);
		return -EINVAL;
	}

	if (on) {
		if (regulator_count_voltages(vdd) > 0) {
			rc = regulator_set_voltage(vdd, BOXID_VDD_MIN_UV, BOXID_VDD_MAX_UV);
			if (rc) {
				pr_err("%s: regulator boxID_pull_up_vdd set_vtg on failed rc=%d\n", __func__, rc);
				regulator_put(vdd);
			}
		}
	} else {
		if (regulator_count_voltages(vdd) > 0) {
			rc = regulator_set_voltage(vdd, 0, BOXID_VDD_MAX_UV);
			if (rc)
				pr_err("%s: regulator boxID_pull_up_vdd set_vtg off failed rc=%d\n", __func__, rc);
		}
		regulator_put(vdd);
	}
	return rc;
}

static int boxID_regulator_set(struct regulator *vdd, bool on)
{
	int rc = 0;

	if (NULL == vdd) {
		pr_err("%s: vdd is null\n", __func__);
		return -EINVAL;
	}

	if (on) {
		if (regulator_count_voltages(vdd) > 0) {
			rc = regulator_enable(vdd);
			if (rc)
				pr_err("%s: regulator boxID_pull_up_vdd enable failed rc=%d\n", __func__, rc);
		}
	} else {
		if (regulator_count_voltages(vdd) > 0) {
			rc = regulator_disable(vdd);
			if (rc)
				pr_err("%s: regulator boxID_pull_up_vdd disable failed rc=%d\n", __func__, rc);
		}
	}
	return rc;
}
/*lint -save -e40 */
static int deviceboxID_probe(struct platform_device *pdev)
{
	int ret = -ENODEV;
	const struct of_device_id *match = NULL;
	struct device_node *node = NULL;
	int box_index = BOX_INDEX_DEFAULT;
	int	gpio_index = GPIO_INDEX_DEFAULT;
	struct pinctrl *p = NULL;
	struct pinctrl_state *pinctrl_def = NULL;

	pr_info("deviceboxID_probe++\n");
	if (NULL == pdev) {
		pr_err("%s: pdev is null\n", __func__);
		return -ENOENT;
	}
	match = of_match_device(deviceboxID_match, &pdev->dev);
	if (!match) {
		pr_err("%s:get deviceboxID device info err\n", __func__);
		return -ENOENT;
	}
	node = pdev->dev.of_node;
	if (NULL == node) {
		pr_err("%s: node is null\n", __func__);
		return -ENOENT;
	}
	/* get deviceboxID info from dts.such as check_mode,gpio_num,box_enable etc */
	ret = get_deviceboxID_info(node);
	if(ret) {
		pr_err("%s:get deviceboxID_info err\n", __func__);
		return -ENOENT;
	}

	deviceboxID.pull_up_vdd = deviceboxID.gpio_extra_pull_up_enable ? get_boxID_gpio_vdd(&pdev->dev) : NULL;

	/* read boxID map info from dts */
	ret = get_deviceboxID_map(node);
	if(ret) {
		pr_err("%s:get boxID_map info err\n", __func__);
		return -ENOENT;
	}

	if (DEVICEBOX_ID_MODE_USE_GPIO == deviceboxID.check_mode) {
		p = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR(p)) {
			pr_err("%s:could not get pinctrl.\n", __func__);
			return -ENOENT;
		}

		pinctrl_def = pinctrl_lookup_state(p, "default");
		if (IS_ERR(pinctrl_def)) {
			pr_err("%s:could not get defstate.\n", __func__);
			return -ENOENT;
		}

		ret = pinctrl_select_state(p, pinctrl_def);
		if (ret) {
			pr_err("%s:could not set pins to default state.\n", __func__);
			return -ENOENT;
		}
#ifdef DEVICEBOXID_DEBUG
		/* create sysfs for debug function */
		if ((sysfs_create_group(&pdev->dev.kobj, &deviceboxID_attr_group)) < 0) {
			pr_err("%s:failed to register sysfs\n", __func__);
			return -ENOENT;
		}
#endif
		if (NULL != deviceboxID.pull_up_vdd) {
			if (boxID_regulator_config(deviceboxID.pull_up_vdd, true) == 0) {
				if (boxID_regulator_set(deviceboxID.pull_up_vdd, true)) {
					pr_err("%s:failed to enable pull_up_vdd regulator\n", __func__);
					boxID_regulator_config(deviceboxID.pull_up_vdd, false);
					deviceboxID.pull_up_vdd = NULL;
				}
			} else {
				deviceboxID.pull_up_vdd = NULL;
			}
		}

		ret = get_deviceboxID_gpio_status(node);
		if(ret) {
			pr_err("%s:read gpio_status fail.\n", __func__);
			if (NULL != deviceboxID.pull_up_vdd) {
				boxID_regulator_set(deviceboxID.pull_up_vdd, false);
				boxID_regulator_config(deviceboxID.pull_up_vdd, false);
			}
			goto err_get_gpio_status;
		}

		if (NULL != deviceboxID.pull_up_vdd) {
			boxID_regulator_set(deviceboxID.pull_up_vdd, false);
			boxID_regulator_config(deviceboxID.pull_up_vdd, false);
		}

		/* reset gpio to NP status for saving power */
		pinctrl_def = pinctrl_lookup_state(p, "idle");
		if (IS_ERR(pinctrl_def)) {
			pr_err("%s:could not get idle defstate.\n", __func__);
			goto err_get_gpio_status;
		}

		ret = pinctrl_select_state(p, pinctrl_def);
		if (ret) {
			pr_err("%s:could not set pins to idle state.\n", __func__);
			goto err_get_gpio_status;
		}

		ret = misc_register(&deviceboxID_device);
		if (ret) {
			pr_err("%s: deviceboxID_device register failed", __func__);
			goto err_get_gpio_status;
		}
	}

	pr_info("deviceboxID_probe--\n");
	return 0;

err_get_gpio_status:
	for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
		if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable) {
			for (gpio_index = 0; gpio_index < deviceboxID.box_info[box_index].gpio_num; gpio_index++) {
				if(GPIO_REQ_SUCCESS == deviceboxID.box_info[box_index].gpio_request_flag[gpio_index]) {
					gpio_free(deviceboxID.box_info[box_index].gpio_id[gpio_index]);
					deviceboxID.box_info[box_index].gpio_id[gpio_index] = GPIO_ID_FAIL;
					deviceboxID.box_info[box_index].gpio_request_flag[gpio_index] = GPIO_REQ_FAIL;
				}
			}
		}
	}
#ifdef DEVICEBOXID_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &deviceboxID_attr_group);
#endif

	return ret;
}
/*lint -restore*/
static int deviceboxID_remove(struct platform_device *pdev)
{
	int box_index = BOX_INDEX_DEFAULT;
	int gpio_index = GPIO_INDEX_DEFAULT;

	if (NULL == pdev) {
		pr_err("%s: pdev is null\n", __func__);
		return 0;
	}
	if (DEVICEBOX_ID_MODE_USE_GPIO == deviceboxID.check_mode) {
		for(box_index=0; box_index<deviceboxID.box_num; box_index++) {
			if(BOX_ID_ENABLED == deviceboxID.box_info[box_index].box_enable){
				for (gpio_index = 0; gpio_index < deviceboxID.box_info[box_index].gpio_num; gpio_index++) {
					if(GPIO_REQ_SUCCESS == deviceboxID.box_info[box_index].gpio_request_flag[gpio_index]) {
						gpio_free(deviceboxID.box_info[box_index].gpio_id[gpio_index]);
						deviceboxID.box_info[box_index].gpio_id[gpio_index] = GPIO_ID_FAIL;
						deviceboxID.box_info[box_index].gpio_request_flag[gpio_index] = GPIO_REQ_FAIL;
					}
				}
			}
		}
#ifdef DEVICEBOXID_DEBUG
		sysfs_remove_group(&pdev->dev.kobj, &deviceboxID_attr_group);
#endif
		misc_deregister(&deviceboxID_device);
	}

	return 0;
}


static struct platform_driver deviceboxID_driver = {
	.driver = {
		.name  = DEVICEBOX_ID_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(deviceboxID_match),
	},
	.probe  = deviceboxID_probe,
	.remove = deviceboxID_remove,
};

static int __init deviceboxID_init(void)
{
	return platform_driver_register(&deviceboxID_driver);
}

static void __exit deviceboxID_exit(void)
{
	platform_driver_unregister(&deviceboxID_driver);
}
/*lint -save -e528 -e814 */
module_init(deviceboxID_init);
/*lint -restore*/
/*lint -save -e528 -e814 */
module_exit(deviceboxID_exit);
/*lint -restore*/
MODULE_DESCRIPTION("deviceboxID driver");
MODULE_LICENSE("GPL");
