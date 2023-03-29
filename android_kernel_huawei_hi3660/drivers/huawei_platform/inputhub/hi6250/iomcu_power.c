/*
 * Copyright (C) huawei company
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 
  * Filename:  iomcu_power.c
 *
 * Discription: some functions of sensorhub power
 *
 * Owner:DIVS_SENSORHUB
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include "inputhub_route.h"
#include "inputhub_bridge.h"
#include "rdr_sensorhub.h"
#include "protocol.h"
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/fs.h>
#include "sensor_info.h"
#include "sensor_sys_info.h"
#include "sensor_debug.h"
#include "iomcu_power.h"
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/rtc.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/inputhub/sensorhub.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif

#define SINGLE_STR_LENGTH_MAX 30

static iomcu_power_status i_power_status;
static DEFINE_MUTEX(mutex_pstatus);
static char show_str[MAX_STR_SIZE] = {0};

static struct class *iomcu_power;

char *iomcu_app_id_str[] = {
	[APP_PEDOMETER] = "TAG_STEP_COUNTER",
	[APP_SIGNIFICANT_MOTION] = "TAG_SIGNIFICANT_MOTION",
	[APP_STEP_DETECTOR] = "TAG_STEP_DETECTOR",
	[APP_ACTIVITY] = "TAG_ACTIVITY",
	[APP_ORIENTATION] = "TAG_ORIENTATION",
	[APP_LINEAR_ACCEL] = "TAG_LINEAR_ACCEL",
	[APP_GRAVITY] = "TAG_GRAVITY",
	[APP_ROTATION_VECTOR] = "TAG_ROTATION_VECTORS",
	[APP_GEOMAGNETIC_ROTATION_VECTOR] = "TAG_GEOMAGNETIC_RV",
	[APP_MOTION] = "TAG_MOTION",
	[APP_ACCEL] = "TAG_ACCEL",
	[APP_GYRO] = "TAG_GYRO",
	[APP_MAG] = "TAG_MAG",
	[APP_ALS] = "TAG_ALS",
	[APP_PS] = "TAG_PS",
	[APP_AIRPRESS] = "TAG_PRESSURE",
	[APP_PDR] = "TAG_PDR",
	[APP_AR] = "TAG_AR",
	[APP_FINGERSENSE] = "TAG_FINGERSENSE",
	[APP_PHONECALL] = "TAG_PHONECALL",
	[APP_GSENSOR_GATHER] = "TAG_GPS_4774_I2C",
	[APP_UNCALIBRATE_MAG] = "TAG_MAG_UNCALIBRATED",
	[APP_UNCALIBRATE_GYRO] = "TAG_GYRO_UNCALIBRATED",
	[APP_HANDPRESS] = "TAG_HANDPRESS",
	[APP_CA] = "TAG_CA",
	[APP_OIS] = "TAG_OIS",
	[APP_FINGERPRINT] = "TAG_FP",
	[APP_CAP_PROX]="TAG_CAP_PROX",
	[APP_KEY] = "TAG_KEY",
	[APP_AOD] = "TAG_AOD",
	[APP_CHARGING] = "TAG_CHARGER",
	[APP_SWITCH] = "TAG_SWITCH",
	[APP_MAGN_BRACKET] = "TAG_MAGN_BRACKET",
	[APP_GPS] = "TAG_GPS",
	[APP_FLP] = "TAG_FLP",
	[APP_TILT_DETECTOR] = "TAG_TILT_DETECTOR",
        [APP_RPC] = "TAG_RPC",
	[APP_MAX] = "TAG_MAX",
};

static const char * get_iomcu_power_status(void)
{
	int status = 0;

	memset(show_str,0,MAX_STR_SIZE);

	mutex_lock(&mutex_pstatus);
	status = i_power_status.power_status;
	mutex_unlock(&mutex_pstatus);

	switch(status){
		case SUB_POWER_ON:
			snprintf(show_str, MAX_STR_SIZE, "%s", "SUB_POWER_ON");
			break;
		case SUB_POWER_OFF:
			snprintf(show_str, MAX_STR_SIZE, "%s", "SUB_POWER_OFF");
			break;
		default:
			snprintf(show_str, MAX_STR_SIZE, "%s", "unknown status");
			break;
	}
	//hwlog_info("get_iomcu_power_status %s\n",show_str);
	return show_str;
}

static const char *get_iomcu_current_opened_app(void)
{
	int i = 0;
	char buf[SINGLE_STR_LENGTH_MAX] = {0};
	int index = 0;
	int copy_length = 0;

	memset(show_str,0,MAX_STR_SIZE);

	mutex_lock(&mutex_pstatus);
	for(i=0;i< TAG_END;i++)
	{
		memset(buf,0,SINGLE_STR_LENGTH_MAX);
		if(i_power_status.app_status[i]){
			//hwlog_info("tag %d, opend %d\n",i,i_power_status.app_status[i]);
			if(obj_tag_str[i] != NULL){
				copy_length = (strlen(obj_tag_str[i]) > (SINGLE_STR_LENGTH_MAX - 1) ) ? (SINGLE_STR_LENGTH_MAX - 1) : strlen(obj_tag_str[i]);
				strncpy(buf,obj_tag_str[i],copy_length);
			}else{
				copy_length = 2;
				snprintf(buf, 3, "%3d", i);
			}
			buf[copy_length] = '\n';
			index += (copy_length + 1);
			if(index < MAX_STR_SIZE){
				strcat(show_str,buf);
			}else{
				show_str[MAX_STR_SIZE - 1] = 'X';
				hwlog_err("show_str too long\n");
				break;
			}

		}
	}
	mutex_unlock(&mutex_pstatus);
	//hwlog_info("get_iomcu_current_opened_app %s\n",show_str);
	return show_str;
}


static int get_iomcu_idle_time(void)
{
	return i_power_status.idle_time;
}

static const char *get_iomcu_active_app_during_suspend(void)
{
	int i = 0;
	char buf[SINGLE_STR_LENGTH_MAX] = {0};
	int index = 0;
	int tf = 0;
	uint64_t bit_map = 0;
	int copy_length = 0;

	memset(show_str,0,MAX_STR_SIZE);

	mutex_lock(&mutex_pstatus);
	bit_map = i_power_status.active_app_during_suspend;
	mutex_unlock(&mutex_pstatus);

	for(i=0;i< APP_MAX;i++)
	{
		memset(buf,0,SINGLE_STR_LENGTH_MAX);
		tf = (bit_map >> i) & 0x01;
		if(tf){
			if(iomcu_app_id_str[i] != NULL){
				copy_length = (strlen(iomcu_app_id_str[i]) > (SINGLE_STR_LENGTH_MAX - 1) ) ? (SINGLE_STR_LENGTH_MAX - 1) : strlen(iomcu_app_id_str[i]);
				strncpy(buf,iomcu_app_id_str[i],copy_length);
			}else{
				copy_length = 2;
				snprintf(buf, 3, "%3d", i);
			}
			buf[copy_length] = '\n';
			index += (copy_length + 1);
			if(index < MAX_STR_SIZE){
				strcat(show_str,buf);
			}else{
				show_str[MAX_STR_SIZE - 1] = 'X';
				hwlog_err("show_str too long\n");
				break;
			}

		}
	}
	//hwlog_info("get_iomcu_active_app_during_suspend %s\n",show_str);
	return show_str;
}

void update_current_app_status(uint8_t tag,uint8_t cmd)
{
	if ((TAG_SENSOR_BEGIN <= tag) && (tag < TAG_END)) {
		//hwlog_info("update_current_app_status:tag %d, cmd:%d\n", tag ,cmd);
		mutex_lock(&mutex_pstatus);
		if ((CMD_CMN_OPEN_REQ == cmd) || (CMD_CMN_INTERVAL_REQ == cmd)) {
			i_power_status.app_status[tag] = 1;
		} else if (CMD_CMN_CLOSE_REQ == cmd) {
			i_power_status.app_status[tag] = 0;
		}
		mutex_unlock(&mutex_pstatus);
	}else{
		hwlog_err("update_current_app_status: error tag %d\n", tag);
	}
}

void check_current_app(void)
{
	int i = 0;
	int flag = 0;
	mutex_lock(&mutex_pstatus);
	for(i=0;i< TAG_END;i++)
	{
		if(i_power_status.app_status[i]){
			//hwlog_info("check_current_app-tag %d, opend %d\n",i,i_power_status.app_status[i]);
			flag ++;
		}
	}
	if(flag > 0){
		hwlog_info("total %d app running after ap suspend\n",flag);
		i_power_status.power_status = SUB_POWER_ON;
		flag = 0;
	}else{
		hwlog_info("iomcu will power off after ap suspend\n");
		i_power_status.power_status = SUB_POWER_OFF;
	}
	mutex_unlock(&mutex_pstatus);
	return;
}

static int mcu_power_log_process(const pkt_header_t *head)
{
	hwlog_info("mcu_power_log_process in\n");

	mutex_lock(&mutex_pstatus);
	i_power_status.idle_time= ((pkt_power_log_report_req_t*)head)->idle_time;
	i_power_status.active_app_during_suspend=  ((pkt_power_log_report_req_t*)head)->current_app_mask;
	mutex_unlock(&mutex_pstatus);

	hwlog_info("last suspend iomcu idle time is %d , active apps high is  0x%x, low is  0x%x\n",i_power_status.idle_time,
		(i_power_status.active_app_during_suspend>>32)&0xffffffff, i_power_status.active_app_during_suspend&0xffffffff);
	return 0;
}

static ssize_t show_power_status(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	return snprintf(buf, MAX_STR_SIZE, "%s\n", get_iomcu_power_status());
}

static ssize_t show_app_status(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	return snprintf(buf, MAX_STR_SIZE, "%s\n", get_iomcu_current_opened_app());
}
static ssize_t show_idle_time(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	return snprintf(buf, MAX_STR_SIZE, "%d\n", get_iomcu_idle_time());
}

static ssize_t show_active_app_during_suspend(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t size)
{
	return snprintf(buf, MAX_STR_SIZE, "%s\n", get_iomcu_active_app_during_suspend());
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
static DEVICE_ATTR(power_status, 0440, show_power_status, NULL);
static DEVICE_ATTR(current_app, 0440, show_app_status, NULL);
static DEVICE_ATTR(idle_time, 0440, show_idle_time, NULL);
static DEVICE_ATTR(active_app_during_suspend, 0440, show_active_app_during_suspend, NULL);

static struct attribute *power_info_attrs[] = {
	&dev_attr_power_status.attr,
	&dev_attr_current_app.attr,
	&dev_attr_idle_time.attr,
	&dev_attr_active_app_during_suspend.attr,
	NULL,
};

static const struct attribute_group power_info_attrs_grp = {
	.attrs = power_info_attrs,
};

#else
static struct device_attribute power_info_class_attrs[] = {
	__ATTR(power_status, 0440, show_power_status, NULL),
	__ATTR(current_app, 0440, show_app_status, NULL),
	__ATTR(idle_time, 0440, show_idle_time, NULL),
	__ATTR(active_app_during_suspend, 0440, show_active_app_during_suspend, NULL),
	__ATTR_NULL,
};
#endif

static struct power_dbg power_info = {
	 .name = "power_info",
	 .attrs_group = &power_info_attrs_grp,
};

static void iomcu_power_info_init(void)
{
    memset(&i_power_status, 0, sizeof(iomcu_power_status));

    register_mcu_event_notifier(TAG_LOG, CMD_LOG_POWER_REQ,
                                mcu_power_log_process);

    iomcu_power = class_create(THIS_MODULE, "iomcu_power");

    if (IS_ERR(iomcu_power))
    { return PTR_ERR(iomcu_power); }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
    iomcu_power->dev_groups = &power_info_attrs_grp;
#else
    iomcu_power->dev_attrs = power_info_class_attrs;
#endif

    power_info.dev = device_create(iomcu_power, NULL, 0, &power_info, power_info.name);

    if (power_info.dev == NULL)
    {
        hwlog_err(" %s creat dev fail\n", __func__);
        class_destroy(iomcu_power);
        return;
    }

    if (power_info.attrs_group != NULL)
    {
        if (sysfs_create_group(&power_info.dev->kobj, power_info.attrs_group))
        {
            hwlog_err("create files failed in %s\n", __func__);
        }
        else
        {
            hwlog_info("%s ok\n", __func__);
            return;
        }
    }
    else
    {
        hwlog_err("power_info.attrs_group is null\n");
    }

    device_destroy(iomcu_power, 0);
    class_destroy(iomcu_power);
}

static void iomcu_power_info_exit(void)
{
    device_destroy(iomcu_power, 0);
    class_destroy(iomcu_power);
}

late_initcall_sync(iomcu_power_info_init);
module_exit(iomcu_power_info_exit);

MODULE_AUTHOR("DIVS SensorHub");
MODULE_DESCRIPTION("SensorHub power driver");
MODULE_LICENSE("GPL");

