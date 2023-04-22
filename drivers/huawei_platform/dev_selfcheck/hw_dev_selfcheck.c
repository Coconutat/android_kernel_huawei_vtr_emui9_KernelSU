#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/crc-ccitt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/dev_detect/hw_dev_detect.h>

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define HWLOG_TAG dev_detect
HWLOG_REGIST();

enum {
	NO_DETECT = 0,    /*no detect*/
	DBC_DETECT = 1,   /*dbc detect*/
	RT_DETECT = 2,    /*running detect*/
	MMI1_DETECT = 4,  /*mmi1 factory detect*/
	MMI2_DETECT = 8,  /*mmi2 normal detect*/
};

#define FLAG_NOT_SET  '0'
#define FLAG_SET  '1'
/* 64 devices need to detect */
#define DEV_DETECT_NUMS  64
#define NEW_LINE_SIZE 1
#define END_TERMINATOR_SIZE 1
#define DEV_DETECT_DEFAULT \
"0000000000000000000000000000000000000000000000000000000000000000"
/* save the detect result, if device's probe function is completed,
set dev_detect_result[dev_id] = '1' */
static char dev_detect_result[DEV_DETECT_NUMS + 1] = DEV_DETECT_DEFAULT;
/* save the config message of device, which need to detect
at board check */
static char dbc_detect_flag[DEV_DETECT_NUMS + 1] = DEV_DETECT_DEFAULT;
static char rt_detect_flag[DEV_DETECT_NUMS + 1] = DEV_DETECT_DEFAULT;
static char mmi1_detect_flag[DEV_DETECT_NUMS + 1] = DEV_DETECT_DEFAULT;
static char mmi2_detect_flag[DEV_DETECT_NUMS + 1] = DEV_DETECT_DEFAULT;
/* the device detect's flag is shown in sysfs node property,
with ending terminator and new line */
#define PRINT_CHECK_MAX_LENGTH (DEV_DETECT_NUMS + NEW_LINE_SIZE +\
END_TERMINATOR_SIZE)
#define RETURN_FAIL (-1)
#define DEV_DETECT_PROBE_SUCCESS (1)

static DEFINE_MUTEX(devdetect_lock);

/* the compatible name in the dts */
#define DTS_COMP_DEVICE_DETECT_NAME "huawei,dev_detect"

static struct detect_flag_device {
	const char *name;
	struct device *dev;
	const int index;
	const struct device_attribute attr;
};

static int dev_detect_probe_flag;
/**
 * function: set the device detect flag corresponding to dev_id in
 * dev_detect_result[].
 * It is used in the device's probe function, which device need to detect.
 * input:
 * dev_id: choose the place in dev_detect_result[], which define in the
 * hw_dev_detect.h.
 */
int set_hw_dev_detect_result(const size_t dev_id)
{
	size_t array_size = ARRAY_SIZE(hw_detect_device_array);

	if ((array_size <= DEV_DETECT_NUMS) && (dev_id < array_size)) {
		mutex_lock(&devdetect_lock);
		dev_detect_result[dev_id] = FLAG_SET;
		mutex_unlock(&devdetect_lock);
		hwlog_info("set %zu in successfully\n", dev_id);
		return true;
	}

	hwlog_err("set the dev_detect_result fail, and dev_id is:%zu!\n",
	dev_id);
	return false;
}
/**
 *   function: get the device detect's attribute value.
 *   input:
 *       np: the device node .
 *       dev_name: the attribute name.
 *   output:
 *       type: the dev_name's attribute value.
 *   return:
 *       return 0 if get the value successfully, others indicates fail.
 */
static int get_hw_dts_devdetect_value(struct device_node *np,
const char *dev_name, unsigned int *type)
{
	if ((NULL == np) || (NULL == dev_name) || (NULL == type)) {
		hwlog_err("[%s] parameter is NULL!\n", __func__);
		return RETURN_FAIL;
	}

	int result = of_property_read_u32(np, dev_name, type);

	if (result)
		hwlog_warn("Get the node's %s value fail and the reason is %d\n",
			dev_name, result);

	return result;
}

/**
 * show the device detect result
 *
 * Parameters do not need to be checked, because @dev in this driver
 * the probe function has been created the device, @attr: In the device
 * structure device_attribute has been defined by __ATTR(), @buf: in the
 * sysfs system show function call count = seq_get_buf(sf, & buf)
 * application to acquire buf memory.
 */
static ssize_t dev_detect_result_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	hwlog_info("now checking the device, and dev_detect_result is:%s!\n",
	dev_detect_result);
	return snprintf((char *)buf, PRINT_CHECK_MAX_LENGTH, "%s\n",
	dev_detect_result);
}

/**
 * show the config result which set in the dts, when we need to
 * detect at the board detect.
 */
static ssize_t dbc_detect_flag_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	hwlog_info("now checking the device, and dbc_detect_flag is:%s !\n",
	dbc_detect_flag);
	return snprintf((char *)buf, PRINT_CHECK_MAX_LENGTH, "%s\n",
	dbc_detect_flag);
}

/**
 * show the config result which set in the dts, when we need to
 * detect at the running detect.
 */
static ssize_t rt_detect_flag_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	hwlog_info("now checking the device, and rt_detect_flag is:%s !\n",
	rt_detect_flag);
	return snprintf((char *)buf, PRINT_CHECK_MAX_LENGTH, "%s\n",
	rt_detect_flag);
}

/**
 * show the config result which set in the dts, when we need to
 * detect at the mmi1 detect.
 */
static ssize_t mmi1_detect_flag_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	hwlog_info("now checking the device, and mmi1_detect_flag is:%s !\n",
	mmi1_detect_flag);
	return snprintf((char *)buf, PRINT_CHECK_MAX_LENGTH, "%s\n",
	mmi1_detect_flag);
}

/**
 * show the config result which set in the dts, when we need to
 * detect at the mmi2 detect.
 */
static ssize_t mmi2_detect_flag_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	hwlog_info("now checking the device, and mmi2_detect_flag is:%s !\n",
	mmi2_detect_flag);
	return snprintf((char *)buf, PRINT_CHECK_MAX_LENGTH, "%s\n",
	mmi2_detect_flag);
}

static struct detect_flag_device device_array[] = {
	{.name = "dev_detect_result_flag", .index = 0,
	.attr = __ATTR(dev_detect_result_flag, S_IRUGO,
				dev_detect_result_show, NULL)},
	{.name = "dev_detect_dbc_flag", .index = 0,
	.attr = __ATTR(dev_detect_dbc_flag, S_IRUGO,
				dbc_detect_flag_show, NULL)},
	{.name = "dev_detect_rt_flag", .index = 0,
	.attr = __ATTR(dev_detect_rt_flag, S_IRUGO,
				rt_detect_flag_show, NULL)},
	{.name = "dev_detect_mmi1_flag", .index = 0,
	.attr = __ATTR(dev_detect_mmi1_flag, S_IRUGO,
				mmi1_detect_flag_show, NULL)},
	{.name = "dev_detect_mmi2_flag", .index = 0,
	.attr = __ATTR(dev_detect_mmi2_flag, S_IRUGO,
	mmi2_detect_flag_show, NULL)},
};

static int dev_detect_probe(struct platform_device *pdev)
{
	if (dev_detect_probe_flag) {
		hwlog_err("[%s] dev_detect_probe is done. Can't enter again!\n",
		__func__);
		return 0;
	}

	hwlog_info("[%s] function begin!\n", __func__);

	if (NULL == pdev) {
		hwlog_err("[%s] none device!\n", __func__);
		return -ENODEV;
	}

	struct device_node *np = pdev->dev.of_node;

	if (NULL == np) {
		hwlog_err("[%s] Unable to find %s device node!\n", __func__,
		DTS_COMP_DEVICE_DETECT_NAME);
		return -ENODEV;
	}

	struct class *myclass;

	myclass = class_create(THIS_MODULE, "dev_detect");
	if (IS_ERR(myclass)) {
		hwlog_err("[%s] Failed to create dev_detect class\n", __func__);
		return RETURN_FAIL;
	}

	size_t device_array_size = ARRAY_SIZE(device_array);
	unsigned long dev_create_flag = 0;
	unsigned long dev_file_create_flag = 0;
	size_t index = 0;
	size_t j = 0;
	int ret = 0;

	for (index = 0; index < device_array_size; index++) {
		device_array[index].dev = device_create(myclass, NULL,
			MKDEV(0, device_array[index].index),
			NULL, device_array[index].name);
		if (NULL == device_array[index].dev) {
			hwlog_err("[%s] Failed to create %s device\n", __func__,
			device_array[index].name);
			ret = RETURN_FAIL;
			goto free_dev_and_file;
		}
		set_bit(index, &dev_create_flag);

		ret = device_create_file(device_array[index].dev,
				&device_array[index].attr);
		if (ret < 0) {
			hwlog_err("[%s] Failed to create %s file\n", __func__,
				device_array[index].name);
			goto free_dev_and_file;
		}
		set_bit(index, &dev_file_create_flag);
	}

	unsigned int type = 0;
	int result = 0;
	size_t array_size = ARRAY_SIZE(hw_detect_device_array);

	if (array_size > DEV_DETECT_NUMS) {
		hwlog_err("[%s] hw_detect_device_array over\n", __func__);
		ret = RETURN_FAIL;
		goto free_dev_and_file;
	}

	size_t i = 0;

	for (i = 0; i < array_size; i++) {
		result = get_hw_dts_devdetect_value(np,
			hw_detect_device_array[i].device_name, &type);
		if (!result) {
			dbc_detect_flag[i] =
				((NO_DETECT != type) && (type & DBC_DETECT))
				? FLAG_SET : FLAG_NOT_SET;
			rt_detect_flag[i] =
				((NO_DETECT != type) && (type & RT_DETECT))
				? FLAG_SET : FLAG_NOT_SET;
			mmi1_detect_flag[i] =
				((NO_DETECT != type) && (type & MMI1_DETECT))
				? FLAG_SET : FLAG_NOT_SET;
			mmi2_detect_flag[i] =
				((NO_DETECT != type) && (type & MMI2_DETECT))
				? FLAG_SET : FLAG_NOT_SET;
		} else {
			hwlog_err("get devid:%zu device name:%s 's value fail\n",
				i, hw_detect_device_array[i].device_name);
			dbc_detect_flag[i] = FLAG_NOT_SET;
			rt_detect_flag[i] = FLAG_NOT_SET;
			mmi1_detect_flag[i] = FLAG_NOT_SET;
			mmi2_detect_flag[i] = FLAG_NOT_SET;
		}
	}

	dev_detect_probe_flag = DEV_DETECT_PROBE_SUCCESS;
	hwlog_info("[%s] function end!\n", __func__);
	return 0;

free_dev_and_file:
	for (j = 0; j < index; j++) {
		if (test_and_clear_bit(j, &dev_file_create_flag))
			device_remove_file(device_array[j].dev,
						&device_array[j].attr);

		if (test_and_clear_bit(j, &dev_create_flag))
			device_destroy(myclass, device_array[j].dev->devt);
	}

	class_destroy(myclass);
	return ret;
}

static const struct of_device_id device_detect_match_table[] = {
	{.compatible = DTS_COMP_DEVICE_DETECT_NAME,
	.data = NULL,
	},
	{
	},
};
MODULE_DEVICE_TABLE(of, device_detect_match_table);


static struct platform_driver dev_detect_driver = {
	.driver = {
		.name = DTS_COMP_DEVICE_DETECT_NAME,
		.of_match_table = of_match_ptr(device_detect_match_table),
	},
	.probe  = dev_detect_probe,
	.remove = NULL,
};

static int __init hw_dev_detect_init(void)
{
	return platform_driver_register(&dev_detect_driver);
}

static void __exit hw_dev_detect_exit(void)
{
	platform_driver_unregister(&dev_detect_driver);
}

/* priority is 7s */
late_initcall_sync(hw_dev_detect_init);

module_exit(hw_dev_detect_exit);

MODULE_AUTHOR("fac");
MODULE_DESCRIPTION("Device Detect Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dev_detect");
