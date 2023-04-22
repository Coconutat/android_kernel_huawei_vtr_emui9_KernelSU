#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/hisi/hw_cmdline_parse.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <huawei_platform/log/hw_log.h>
#include "led_alwayson.h"
#include <linux/platform_device.h>


#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG  led_alwayson
HWLOG_REGIST();


#define LED_STATUS_NV_NUM	365
#define LED_STATUS_NV_SIZE	(4)
#define LED_ALWAYSON_EXIST    "led_alwayson_support"

static int led_alwayson_flag;
extern unsigned int runmode_is_factory(void);
static char led_alwayson_nv[LED_STATUS_NV_SIZE+1] = {0};


/*******************************************************************************
Function:	read_led_status_from_nv
Description:   read led status
Data Accessed:   null
Data Updated:   null
Input:   null
Output:  null
Return:  0->success, -1->fail
*******************************************************************************/
int read_led_status_from_nv(void)
{
	int ret = 0;

	struct hisi_nve_info_user user_info;

	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = NV_READ;
	user_info.nv_number = LED_STATUS_NV_NUM;
	user_info.valid_size = LED_STATUS_NV_SIZE;
	strncpy(user_info.nv_name, "LEDST", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		hwlog_err("%s:nve_direct_access read error(%d)\n", __func__,ret);
		return -1;
	}

	memcpy(led_alwayson_nv, (char *)user_info.nv_data, sizeof(led_alwayson_nv));
	hwlog_info("%s:read led color:%d, status:%d\n",
			 __func__, led_alwayson_nv[0], led_alwayson_nv[1]);

	if(led_alwayson_nv[1] == 1){
		led_alwayson_flag = 1;
		hwlog_info("%s:led alwayson is open\n",__func__);
	}else{
		led_alwayson_flag = 0;
		hwlog_info("%s:led alwayson is close\n",__func__);
	}

	return 0;
}

/*******************************************************************************
Function:	write_led_status_to_nv
Description:   write led status
Data Accessed:   null
Data Updated:   null
Input:   led_alwayson status
Output:  null
Return:  0->success, -1->fail
*******************************************************************************/
int write_led_status_to_nv(char *temp)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	if (temp == NULL) {
		hwlog_err("write_led_status_to_nv fail, invalid temp!\n");
		return -1;
	}
	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = NV_WRITE;
	user_info.nv_number = LED_STATUS_NV_NUM;
	user_info.valid_size = LED_STATUS_NV_SIZE;
	strncpy(user_info.nv_name, "LEDST", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';

	user_info.nv_data[0] = temp[0];
	user_info.nv_data[1] = temp[1];
	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		hwlog_err("%s:nve_direct_access write error(%d)\n", __func__,ret);
		return -1;
	}
	hwlog_info("write_led_status_to_nv succ(nv data:%d,%d)!\n",
		user_info.nv_data[0],user_info.nv_data[1]);

	return ret;
}


static ssize_t led_status_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	int ret = 0;

	if (!runmode_is_factory()){
		led_alwayson_flag = 0;
	} else {
		ret = read_led_status_from_nv();
		if(ret){
			hwlog_err("%s:read led_status fail\n", __func__);
			led_alwayson_flag = 0;
		}
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", led_alwayson_flag);
}

static ssize_t led_status_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	int ret = 0;
	uint64_t led_status = 0;

	if (strict_strtoull(buf, 10, &led_status)) {
		hwlog_err("%s:read led color fail\n", __func__);
		return size;
	}

	hwlog_info("%s:led status:%d\n", __func__,led_status);

	if (!runmode_is_factory())
		return size;

	ret = read_led_status_from_nv();
	if(ret){
		hwlog_err("%s:read led_status fail\n", __func__);
		return size;
	}

	/*update led status*/
	led_alwayson_nv[1] = (char)led_status;

	ret = write_led_status_to_nv(led_alwayson_nv);
	if(ret){
		hwlog_err("%s:write led status fail\n", __func__);
	}

	return size;
}

static ssize_t version_mode_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	int version_mode = 0;

	if (runmode_is_factory()){
		hwlog_info("%s:version_mode is factory\n", __func__);
		version_mode = 1;
	} else {
		hwlog_info("%s:version_mode is normal\n", __func__);
		version_mode = 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", version_mode);
}


/*files create*/
static DEVICE_ATTR(led_status, 0660, led_status_show,
		led_status_store);
static DEVICE_ATTR(version_mode, 0660, version_mode_show,
		NULL);


static struct attribute *led_alwayson_attrs[] = {
	&dev_attr_led_status.attr,
	&dev_attr_version_mode.attr,
	NULL
};

static const struct attribute_group led_alwayson_attrs_group = {
	.attrs = led_alwayson_attrs,
};


static const struct of_device_id led_alwayson_match_table[] = {
	{.compatible = "huawei,led_alwayson",},
	{},
};

MODULE_DEVICE_TABLE(of, led_alwayson_match_table);


static int led_alwayson_probe(struct platform_device *pdev)
{
	int ret = 0;
	int default_state = 0;
	const char *state = NULL;
	struct led_alwayson_data* data = NULL;
	struct device_node *led_node = NULL;

	data = devm_kzalloc(&pdev->dev,sizeof(struct led_alwayson_data),
					       GFP_KERNEL);
	if (!data) {
		hwlog_err("Failed to allocate memory for data\n");
		return -ENOMEM;
	}

	data->pdev = pdev;

	led_node = pdev->dev.of_node;
	if(!led_node){
		hwlog_err("%s failed to find dts node led_alwayson\n", __func__);
		ret = -ENODEV;
		goto free_devm;
	}

	ret = of_property_read_string(led_node, LED_ALWAYSON_EXIST, &state);
	if (ret) {
		hwlog_err("%s failed to find LED_ALWAYSON_EXIST\n", __func__);
		ret = -ENODEV;
		goto free_devm;
	}
	if (strncmp(state, "yes", sizeof("yes")) != 0){
		hwlog_err("%s:led alwayson is not support\n", __func__);
		ret = 0;
		goto free_devm;
	}

	data->led_alwayson_class = class_create(THIS_MODULE, "led_alwayson");
	if (IS_ERR(data->led_alwayson_class)){
		ret = PTR_ERR(data->led_alwayson_class);
		goto free_devm;
	}

	data->dev = device_create(data->led_alwayson_class, NULL,
			0, NULL, "led_alwayson");
	if (NULL == data->dev) {
		hwlog_err("[%s] device_create Failed", __func__);
		ret = -1;
		goto free_class;
	}

	ret = sysfs_create_group(&data->dev->kobj, &led_alwayson_attrs_group);
	if (ret){
		hwlog_err("%s sysfs_create_group failed ret=%d.\n", __func__, ret);
		ret = -1;
		goto free_device;
	}
	hwlog_info("%s succ.\n", __func__);
	return 0;

free_device:
	device_destroy(data->led_alwayson_class, 0);
free_class:
	class_destroy(data->led_alwayson_class);
free_devm:
	devm_kfree(&pdev->dev, data);
	return ret;
}

static int led_alwayson_remove(struct platform_device *pdev)
{
	struct  led_alwayson_data* data = dev_get_drvdata(&pdev->dev);
	if (NULL == data)
		return -EINVAL;

	sysfs_remove_group(&data->dev->kobj, &led_alwayson_attrs_group);
	device_destroy(data->led_alwayson_class, 0);
	class_destroy(data->led_alwayson_class);
	devm_kfree(&pdev->dev, data);
	hwlog_info("%s\n", __func__);
	return 0;
}


struct platform_driver led_alwayson_driver = {
	.probe = led_alwayson_probe,
	.remove = led_alwayson_remove,
	.driver = {
		   .name = LED_ALWAYSON_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(led_alwayson_match_table),
		   },
};

static int __init led_alwayson_init(void)
{
	hwlog_info("init!\n");
	return platform_driver_register(&led_alwayson_driver);
}

static void __exit led_alwayson_exit(void)
{
	hwlog_info("exit!\n");
	platform_driver_unregister(&led_alwayson_driver);
}

module_init(led_alwayson_init);
module_exit(led_alwayson_exit);

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("Led alwayson driver");
MODULE_LICENSE("GPL");
