#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/soh/hisi_soh_interface.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/batt_acr_detect.h>

#define HWLOG_TAG batt_acr_detect
HWLOG_REGIST();

static int acr_rt_detect_run = ACR_FALSE;

static int acr_runningtest_detect(struct acr_device_info *di)
{
	int ret = -1;

	soh_acr_low_precision_cal_start();
	msleep(1000);//sleep 1000ms, wait for acr cal finish
	memset(&di->acr_data, 0, sizeof(di->acr_data));
	ret = soh_get_acr_resistance(&di->acr_data, ACR_L_PRECISION);
	if (0 == ret) {
		hwlog_info("acr rt detect finish, acr_rt = %d\n", di->acr_data.batt_acr);
		return ret;
	}

	hwlog_info("acr rt detect fail\n");
	return ret;
}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static void acr_fmd_init(struct acr_device_info *di)
{
	di->acr_msg.version = ACR_RT_DEFAULT_VERSION;
	di->acr_msg.data_source = DATA_FROM_KERNEL;
	di->acr_msg.num_events = 1;

	di->acr_msg.events[0].error_code = 0;
	di->acr_msg.events[0].item_id = ACR_RT_ITEM_ID;
	di->acr_msg.events[0].cycle = 0;
	memcpy(di->acr_msg.events[0].station, ACR_RT_NA, sizeof(ACR_RT_NA));
	memcpy(di->acr_msg.events[0].bsn, ACR_RT_NA, sizeof(ACR_RT_NA));
	memcpy(di->acr_msg.events[0].time, ACR_RT_NA, sizeof(ACR_RT_NA));
	memcpy(di->acr_msg.events[0].device_name, ACR_RT_DEVICE_NAME, sizeof(ACR_RT_DEVICE_NAME));
	memcpy(di->acr_msg.events[0].test_name, ACR_RT_TEST_NAME, sizeof(ACR_RT_TEST_NAME));
	snprintf(di->acr_msg.events[0].min_threshold, MAX_VAL_LEN, "%d", di->acr_rt_fmd_min);
	snprintf(di->acr_msg.events[0].max_threshold, MAX_VAL_LEN, "%d", di->acr_rt_fmd_max);
}
#endif

static int acr_fmd_report(struct acr_device_info *di, int acr_rt)
{
	int ret = -1;
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	char* batt_brand = hisi_battery_brand();
	int batt_volt = hisi_battery_voltage();
	int batt_temp = hisi_battery_temperature();

	acr_fmd_init(di);
	if (acr_rt > di->acr_rt_threshold) {
		memcpy(di->acr_msg.events[0].result, ACR_RT_FMD_FAIL, sizeof(ACR_RT_FMD_FAIL));
	} else {
		memcpy(di->acr_msg.events[0].result, ACR_RT_FMD_PASS, sizeof(ACR_RT_FMD_PASS));
	}
	snprintf(di->acr_msg.events[0].value, MAX_VAL_LEN, "%d", acr_rt);
	memcpy(di->acr_msg.events[0].firmware, batt_brand, MAX_FIRMWARE_LEN);
	snprintf(di->acr_msg.events[0].description, MAX_DESCRIPTION_LEN, "batt_volt:%dmV, batt_temp:%d", batt_volt, batt_temp);

	ret = power_dsm_bigdata_report(POWER_DSM_BATTERY, DA_BATTERY_ACR_ERROR_NO, &di->acr_msg);
	return ret;
#endif
	return ret;
}

static void acr_dmd_report(int acr_rt)
{
	char buf[CHARGE_DMDLOG_SIZE] = {0};

	char* batt_brand = hisi_battery_brand();
	int batt_volt = hisi_battery_voltage();
	int batt_curr = -hisi_battery_current();
	int batt_soc = hisi_battery_capacity();
	int batt_temp = hisi_battery_temperature();

	snprintf(buf, sizeof(buf) - 1, "batt_brand:%s, batt_volt:%dmV, batt_curr:%dmA, batt_soc:%d, batt_temp:%d, batt_acr:%d\n",
		batt_brand, batt_volt, batt_curr, batt_soc, batt_temp, acr_rt);
	power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_BATT_ACR_OVER_THRESHOLD, buf);
}

#ifdef CONFIG_SYSFS
#define ACR_SYSFS_FIELD(_name, n, m, store)          \
{                                                    \
    .attr = __ATTR(_name, m, acr_sysfs_show, store),     \
    .name = ACR_SYSFS_##n,          \
}

#define ACR_SYSFS_FIELD_RW(_name, n)          \
	ACR_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, acr_sysfs_store)

#define ACR_SYSFS_FIELD_RO(_name, n)          \
	ACR_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t acr_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t acr_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

struct acr_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct acr_sysfs_field_info acr_sysfs_field_tbl[] = {
	ACR_SYSFS_FIELD_RO(acr_rt_support, ACR_RT_SUPPORT),
	ACR_SYSFS_FIELD_RW(acr_rt_detect, ACR_RT_DETECT),
};

static struct attribute *acr_sysfs_attrs[ARRAY_SIZE(acr_sysfs_field_tbl) + 1];

static const struct attribute_group acr_sysfs_attr_group = {
	.attrs = acr_sysfs_attrs,
};

static void acr_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(acr_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		acr_sysfs_attrs[i] = &acr_sysfs_field_tbl[i].attr.attr;
	}
	acr_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

static struct acr_sysfs_field_info *acr_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(acr_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp(name, acr_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &acr_sysfs_field_tbl[i];
}

static ssize_t acr_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct acr_sysfs_field_info *info = NULL;
	struct acr_device_info *di = dev_get_drvdata(dev);

	info = acr_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case ACR_SYSFS_ACR_RT_SUPPORT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->acr_rt_support);
	case ACR_SYSFS_ACR_RT_DETECT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->acr_rt_status);
	default:
		hwlog_err("sysfs show fail, not have node:%d\n", info->name);
		break;
	}
	return 0;
}

static ssize_t acr_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct acr_sysfs_field_info *info = NULL;
	struct acr_device_info *di = dev_get_drvdata(dev);
	long val = 0;
	int ret;

	info = acr_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case ACR_SYSFS_ACR_RT_DETECT:
		if ((strict_strtol(buf, 10, &val) < 0) || (val != 1))
			return -EINVAL;
		if (di->acr_rt_support
			&& strstr(saved_command_line, "androidboot.swtype=factory")
			&& ACR_FALSE == acr_rt_detect_run) {
			di->acr_rt_status = ACR_RT_STATUS_INIT;
			hwlog_info("RUNNINGTEST batt acr detect start\n");
			schedule_delayed_work(&di->acr_rt_work, msecs_to_jiffies(0));
		}
		break;
	default:
		hwlog_err("sysfs store fail, not have node:%d\n", info->name);
		break;
	}
	return count;
}

static int acr_sysfs_create_group(struct acr_device_info *di)
{
	acr_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &acr_sysfs_attr_group);
}

static inline void acr_sysfs_remove_group(struct acr_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &acr_sysfs_attr_group);
}

#else
static int acr_sysfs_create_group(struct acr_device_info *di)
{
	return 0;
}

static inline void acr_sysfs_remove_group(struct acr_device_info *di)
{
}
#endif

static void batt_acr_detect_parse_dts(struct device_node *np, struct acr_device_info *di)
{
	int ret = 0;

	ret = of_property_read_u32(np, "acr_rt_support", &(di->acr_rt_support));
	if (ret) {
		hwlog_err("get acr_rt_support fail, use default value\n");
		di->acr_rt_support = ACR_RT_NOT_SUPPORT;
	}
	hwlog_info("acr_rt_support = %d\n", di->acr_rt_support);

	ret = of_property_read_u32(np, "acr_rt_threshold", &(di->acr_rt_threshold));
	if (ret) {
		hwlog_err("get acr_rt_threshold fail, use default value\n");
		di->acr_rt_threshold = ACR_RT_THRESHOLD_DEFAULT;
	}
	di->acr_rt_threshold *= 1000;//mA->uA
	hwlog_info("acr_rt_threshold = %d\n", di->acr_rt_threshold);

	ret = of_property_read_u32(np, "acr_rt_fmd_min", &(di->acr_rt_fmd_min));
	if (ret) {
		hwlog_err("get acr_rt_fmd_min fail, use default value\n");
		di->acr_rt_fmd_min = ACR_RT_FMD_MIN_DEFAULT;
	}
	hwlog_info("acr_rt_fmd_min = %d\n", di->acr_rt_fmd_min);

	ret = of_property_read_u32(np, "acr_rt_fmd_max", &(di->acr_rt_fmd_max));
	if (ret) {
		hwlog_err("get acr_rt_fmd_max fail, use default value\n");
		di->acr_rt_fmd_max = ACR_RT_FMD_MAX_DEFAULT;
	}
	hwlog_info("acr_rt_fmd_max = %d\n", di->acr_rt_fmd_max);

}

static void acr_runningtest_work(struct work_struct *work)
{
	struct acr_device_info *di = container_of(work, struct acr_device_info, acr_rt_work);
	int i = 0, ichg_coul = 0, acr_rt = 0;

	acr_rt_detect_run = ACR_TRUE;
	if (0 == charge_set_input_current_max()) {
		for (i = 0; i < ACR_RT_RETRY_TIMES; i++) {
			ichg_coul = -hisi_battery_current();
			if (abs(ichg_coul) < ACR_RT_CURRENT_MIN) {
				hwlog_info("charging disabled, i:%d, ichg_coul:%d\n", i, ichg_coul);
				goto acr_detect;
			}
			msleep(100);//sleep 100ms for next check
		}
	}
	di->acr_rt_status = ACR_RT_STATUS_FAIL;
	acr_rt_detect_run = ACR_FALSE;
	hwlog_err("RUNNINGTEST batt acr detect fail1\n");
	return;

acr_detect:
	if (0 == acr_runningtest_detect(di)) {
		acr_rt = di->acr_data.batt_acr;
		di->acr_rt_status = ACR_RT_STATUS_SUCC;
		if (0 > acr_fmd_report(di, acr_rt)) {
			hwlog_err("RUNNINGTEST batt acr detect fmd data acquisition fail\n");
		}
		msleep(50);//sleep 50ms, wait for dsm_battery client free
		if (acr_rt > di->acr_rt_threshold) {
			acr_dmd_report(acr_rt);
			hwlog_err("RUNNINGTEST batt acr:%d over threshold:%d\n", acr_rt, di->acr_rt_threshold);
		}
		acr_rt_detect_run = ACR_FALSE;
		hwlog_info("RUNNINGTEST batt acr detect succ\n");
		return;
	}

	di->acr_rt_status = ACR_RT_STATUS_FAIL;
	acr_rt_detect_run = ACR_FALSE;
	hwlog_err("RUNNINGTEST batt acr detect fail2\n");
	return;
}

static int batt_acr_detect_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct acr_device_info *di;
	struct device_node *np;
	struct class *power_class = NULL;

	di = kzalloc(sizeof(struct acr_device_info), GFP_KERNEL);
	if (NULL == di) {
		hwlog_err("alloc di fail\n");
		return -ENOMEM;
	}

	di->dev = &pdev->dev;
	np = di->dev->of_node;
	platform_set_drvdata(pdev, di);
	batt_acr_detect_parse_dts(np, di);

	INIT_DELAYED_WORK(&di->acr_rt_work, acr_runningtest_work);

	ret = acr_sysfs_create_group(di);
	if (ret)
		hwlog_err("can't create	acr sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "acr");
		if (ret) {
			hwlog_err("create link to acr fail\n");
			goto acr_fail;
		}
	}

	hwlog_info("probe succ\n");
	return 0;

acr_fail:
	acr_sysfs_remove_group(di);
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	return ret;
}

static int batt_acr_detect_remove(struct platform_device *pdev)
{
	struct acr_device_info *di = platform_get_drvdata(pdev);
	if (NULL == di) {
		hwlog_err("remove fail, di is null\n");
		return -ENODEV;
	}

	acr_sysfs_remove_group(di);
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	return 0;
}

static struct of_device_id batt_acr_detect_match_table[] = {
	{
		.compatible = "huawei,batt_acr_detect",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver batt_acr_detect_driver = {
	.probe = batt_acr_detect_probe,
	.remove = batt_acr_detect_remove,
	.driver = {
		.name = "huawei,batt_acr_detect",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(batt_acr_detect_match_table),
	},
};

static int __init batt_acr_detect_init(void)
{
	return platform_driver_register(&batt_acr_detect_driver);
}

late_initcall(batt_acr_detect_init);

static void __exit batt_acr_detect_exit(void)
{
	platform_driver_unregister(&batt_acr_detect_driver);
}

module_exit(batt_acr_detect_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("batt acr detect module driver");
MODULE_AUTHOR("HUAWEI Inc");

