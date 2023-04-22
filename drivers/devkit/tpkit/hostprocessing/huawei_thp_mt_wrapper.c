/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>
#include <linux/poll.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#include "huawei_thp_mt_wrapper.h"
#include "huawei_thp.h"

#ifdef CONFIG_INPUTHUB_20
#include "contexthub_recovery.h"
#endif

#if defined(CONFIG_HUAWEI_TS_KIT_3_0)
#include "../3_0/trace-events-touch.h"
#else
#define trace_touch(x...)
#endif

#define DEVICE_NAME	"input_mt_wrapper"

static struct thp_mt_wrapper_data *g_thp_mt_wrapper = 0;

int thp_mt_wrapper_ioctl_get_events(unsigned long event)
{
	int t = 0;
	int __user *events = (int *)event;
	struct thp_core_data *cd = thp_get_core_data();

	if (!cd || !events) {
		THP_LOG_INFO("%s: input null\n", __func__);
		return -ENODEV;
	}

	THP_LOG_INFO("%d: cd->event_flag \n",cd->event_flag);
	if (cd->event_flag) {
		if(copy_to_user(events, &cd->event, sizeof(cd->event))) {
			THP_LOG_ERR("%s:copy events failed\n", __func__);
			return -EFAULT;
		}

		cd->event_flag = false;
	} else {
		cd->thp_event_waitq_flag = WAITQ_WAIT;
		t = wait_event_interruptible(cd->thp_event_waitq,
				(cd->thp_event_waitq_flag == WAITQ_WAKEUP));
		THP_LOG_INFO("%s:  set wait finish \n",__func__);
	}

	return 0;
}

static long thp_mt_wrapper_ioctl_set_coordinate(unsigned long arg)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;
	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;
	struct thp_mt_wrapper_ioctl_touch_data data;
	u8 i;

	trace_touch(TOUCH_TRACE_ALGO_SET_EVENT, TOUCH_TRACE_FUNC_IN, "thp");
	if (arg == 0) {
		THP_LOG_ERR("%s:arg is null.\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&data, argp,
			sizeof(struct thp_mt_wrapper_ioctl_touch_data))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}
	trace_touch(TOUCH_TRACE_ALGO_SET_EVENT, TOUCH_TRACE_FUNC_OUT, "thp");

	trace_touch(TOUCH_TRACE_INPUT, TOUCH_TRACE_FUNC_IN, "thp");
	for (i = 0; i < INPUT_MT_WRAPPER_MAX_FINGERS; i++) {
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(input_dev, i);
		input_mt_report_slot_state(input_dev,
			data.touch[i].tool_type, data.touch[i].valid != 0);
#endif

		if (data.touch[i].valid != 0) {
			input_report_abs(input_dev, ABS_MT_POSITION_X,
						data.touch[i].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
						data.touch[i].y);
			input_report_abs(input_dev, ABS_MT_PRESSURE,
						data.touch[i].pressure);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID,
						data.touch[i].tracking_id);
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
						data.touch[i].major);
			input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
						data.touch[i].minor);
			input_report_abs(input_dev, ABS_MT_ORIENTATION,
						data.touch[i].orientation);
			input_report_abs(input_dev, ABS_MT_TOOL_TYPE,
						data.touch[i].tool_type);
			input_report_abs(input_dev, ABS_MT_BLOB_ID,
						data.touch[i].hand_side);
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(input_dev);
#endif
		}
	}

	/* BTN_TOUCH DOWN */
	if (data.t_num > 0)
		input_report_key(input_dev, BTN_TOUCH, 1);

	/* BTN_TOUCH UP */
	if (data.t_num == 0) {
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(input_dev);
#endif
		input_report_key(input_dev, BTN_TOUCH, 0);
	}

	input_sync(input_dev);

	trace_touch(TOUCH_TRACE_INPUT, TOUCH_TRACE_FUNC_OUT, "thp");
	return ret;
}

void thp_clean_fingers(void)
{

	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;
	struct thp_mt_wrapper_ioctl_touch_data data;

	memset(&data, 0, sizeof(data));

	input_mt_sync(input_dev);
	input_sync(input_dev);

	input_report_key(input_dev, BTN_TOUCH, 0);
	input_sync(input_dev);
}



static int thp_mt_wrapper_open(struct inode *inode, struct file *filp)
{
	THP_LOG_INFO("%s:called\n", __func__);
	return 0;
}

static int thp_mt_wrapper_release(struct inode *inode,
						struct file *filp)
{
	return 0;
}

static int thp_mt_wrapper_ioctl_read_status(unsigned long arg)
{
	int __user *status = (int *)arg;
	u32 thp_status = thp_get_status_all();

	THP_LOG_INFO("%s:status=%d\n", __func__, thp_status);

	if(!status) {
		THP_LOG_ERR("%s:input null\n", __func__);
		return -EINVAL;
	}

	if(copy_to_user(status, &thp_status, sizeof(u32))) {
		THP_LOG_ERR("%s:copy status failed\n", __func__);
		return -EFAULT;
	}

	if (atomic_read(&g_thp_mt_wrapper->status_updated) != 0)
		atomic_dec(&g_thp_mt_wrapper->status_updated);

	return 0;
}

static int thp_mt_ioctl_read_input_config(unsigned long arg)
{
	struct thp_input_dev_config __user *config = (struct thp_input_dev_config *)arg;
	struct thp_input_dev_config *input_config = &g_thp_mt_wrapper->input_dev_config;
	if (!config) {
		THP_LOG_ERR("%s:input null\n", __func__);
		return -EINVAL;
	}

	if(copy_to_user(config, input_config, sizeof(struct thp_input_dev_config))) {
		THP_LOG_ERR("%s:copy input config failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int thp_mt_wrapper_ioctl_read_scene_info(unsigned long arg)
{
	struct thp_scene_info __user *config = (struct thp_scene_info *)arg;
	struct thp_core_data *cd = thp_get_core_data();
	struct thp_scene_info* scene_info;

	if (!cd) {
		THP_LOG_ERR("%s:thp_core_data is NULL.\n", __func__);
		return -EINVAL;
	}
	scene_info= &(cd->scene_info);

	THP_LOG_INFO("%s:%d,%d,%d\n", __func__,
		scene_info->type, scene_info->status, scene_info->parameter);

	if(copy_to_user(config, scene_info, sizeof(struct thp_scene_info))) {
		THP_LOG_ERR("%s:copy scene_info failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int thp_mt_wrapper_ioctl_get_window_info(unsigned long arg)
{
	struct thp_window_info __user *window_info = (struct thp_window_info *)arg;
	struct thp_core_data *cd = thp_get_core_data();

	if (!cd || !window_info) {
		THP_LOG_ERR("%s:args error\n", __func__);
		return -EINVAL;
	}

	THP_LOG_INFO("%s:x0=%d,y0=%d,x1=%d,y1=%d\n", __func__,
		cd->window.x0, cd->window.y0, cd->window.x1, cd->window.y1);

	if(copy_to_user(window_info, &cd->window, sizeof(struct thp_window_info))) {
		THP_LOG_ERR("%s:copy window_info failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int thp_mt_wrapper_ioctl_get_projectid(unsigned long arg)
{
	char __user *project_id = (char __user *)arg;
	struct thp_core_data *cd = thp_get_core_data();

	if (!cd || !project_id) {
		THP_LOG_ERR("%s:args error\n", __func__);
		return -EINVAL;
	}

	THP_LOG_INFO("%s:project id:%s\n", __func__, cd->project_id);

	if(copy_to_user(project_id, cd->project_id, sizeof(cd->project_id))) {
		THP_LOG_ERR("%s:copy project_id failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int thp_mt_wrapper_ioctl_set_roi_data(unsigned long arg)
{
	short __user *roi_data = (short __user *)arg;
	struct thp_core_data *cd = thp_get_core_data();

	if (!cd || !roi_data) {
		THP_LOG_ERR("%s:args error\n", __func__);
		return -EINVAL;
	}

	if(copy_from_user(cd->roi_data, roi_data, sizeof(cd->roi_data))) {
		THP_LOG_ERR("%s:copy roi data failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static long thp_mt_wrapper_ioctl_set_events(unsigned long arg)
{
	long ret = 0;
	struct thp_core_data *cd = thp_get_core_data();
	void __user *argp = (void __user *)arg;
	int val;

	if (arg == 0) {
		THP_LOG_ERR("%s:arg is null.\n", __func__);
		return -EINVAL;
	}
	if (copy_from_user(&val, argp,
			sizeof(int))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}
	THP_LOG_INFO("thp_send, write: %d\n", val);
	cd->event_flag = true;
	cd->event = val;
	if (cd->event_flag) {
		cd->thp_event_waitq_flag = WAITQ_WAKEUP;
		wake_up_interruptible(&cd->thp_event_waitq);
		THP_LOG_INFO("%d: wake_up\n",cd->event);
	}

	return ret;
}

static int thp_mt_ioctl_report_keyevent(unsigned long arg)
{
	int report_value[PROX_VALUE_LEN] = {0};
	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;
	void __user *argp = (void __user *)arg;
	enum input_mt_wrapper_keyevent keyevent;

	if (arg == 0) {
		THP_LOG_ERR("%s:arg is null.\n", __func__);
		return -EINVAL;
	}
	if (copy_from_user(&keyevent, argp,
			sizeof(enum input_mt_wrapper_keyevent))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}

	if (keyevent == INPUT_MT_WRAPPER_KEYEVENT_ESD) {
		input_report_key(input_dev,KEY_F26,1);
		input_sync(input_dev);
		input_report_key(input_dev,KEY_F26,0);
		input_sync(input_dev);
	} else if (keyevent == INPUT_MT_WRAPPER_KEYEVENT_APPROACH) {
		THP_LOG_INFO("[Proximity_feature] %s: Here report [near] event !\n",
			__func__);
		report_value[0] = APPROCH_EVENT_VALUE;
#ifdef CONFIG_INPUTHUB_20
		thp_prox_event_report(report_value, PROX_EVENT_LEN);
#endif
	} else if (keyevent == INPUT_MT_WRAPPER_KEYEVENT_AWAY) {
		THP_LOG_INFO("[Proximity_feature] %s: Here report [far] event !\n",
			__func__);
		report_value[0] = AWAY_EVENT_VALUE;
#ifdef CONFIG_INPUTHUB_20
		thp_prox_event_report(report_value, PROX_EVENT_LEN);
#endif
	}

	return 0;
}

static long thp_mt_wrapper_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	long ret;

	switch (cmd) {
	case INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES:
		ret = thp_mt_wrapper_ioctl_set_coordinate(arg);
		break;

	case INPUT_MT_WRAPPER_IOCTL_READ_STATUS:
		ret = thp_mt_wrapper_ioctl_read_status(arg);
		break;

	case INPUT_MT_WRAPPER_IOCTL_READ_INPUT_CONFIG:
		ret = thp_mt_ioctl_read_input_config(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_READ_SCENE_INFO:
		ret = thp_mt_wrapper_ioctl_read_scene_info(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_GET_WINDOW_INFO:
		ret = thp_mt_wrapper_ioctl_get_window_info(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_GET_PROJECT_ID:
		ret = thp_mt_wrapper_ioctl_get_projectid(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_CMD_SET_EVENTS:
		ret = thp_mt_wrapper_ioctl_set_events(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_CMD_GET_EVENTS:
		ret = thp_mt_wrapper_ioctl_get_events(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_SET_ROI_DATA:
		ret = thp_mt_wrapper_ioctl_set_roi_data(arg);
		break;
	case INPUT_MT_WRAPPER_IOCTL_CMD_REPORT_KEYEVENT:
		ret = thp_mt_ioctl_report_keyevent(arg);
		break;
	default:
		THP_LOG_ERR("cmd unkown.\n");
		ret = -EINVAL;
	}

	return ret;
}

int thp_mt_wrapper_wakeup_poll(void)
{
	if(!g_thp_mt_wrapper) {
		THP_LOG_ERR("%s: wrapper not init\n", __func__);
		return -ENODEV;
	}
	atomic_inc(&g_thp_mt_wrapper->status_updated);
	wake_up_interruptible(&g_thp_mt_wrapper->wait);
	return 0;
}

static unsigned int thp_mt_wrapper_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	THP_LOG_DEBUG("%s:poll call in\n", __func__);

	poll_wait(file, &g_thp_mt_wrapper->wait, wait);
	if (atomic_read(&g_thp_mt_wrapper->status_updated) > 0)
		mask |= POLLIN | POLLRDNORM;

	THP_LOG_DEBUG("%s:poll call out, mask = 0x%x\n", __func__, mask);
	return mask;
}

static const struct file_operations g_thp_mt_wrapper_fops = {
	.owner = THIS_MODULE,
	.open = thp_mt_wrapper_open,
	.release = thp_mt_wrapper_release,
	.unlocked_ioctl = thp_mt_wrapper_ioctl,
	.poll = thp_mt_wrapper_poll,
};

static struct miscdevice g_thp_mt_wrapper_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &g_thp_mt_wrapper_fops,
};

static void set_default_input_config(struct thp_input_dev_config *input_config)
{
	input_config->abs_max_x = THP_MT_WRAPPER_MAX_X;
	input_config->abs_max_y = THP_MT_WRAPPER_MAX_Y;
	input_config->abs_max_z = THP_MT_WRAPPER_MAX_Z;
	input_config->tracking_id_max = THP_MT_WRAPPER_MAX_FINGERS;
	input_config->major_max = THP_MT_WRAPPER_MAX_MAJOR;
	input_config->minor_max = THP_MT_WRAPPER_MAX_MINOR;
	input_config->orientation_min = THP_MT_WRAPPER_MIN_ORIENTATION;
	input_config->orientation_max = THP_MT_WRAPPER_MAX_ORIENTATION;
	input_config->tool_type_max = THP_MT_WRAPPER_TOOL_TYPE_MAX;
}
static int thp_parse_input_config(struct thp_input_dev_config *config)
{
	int rc = 0;
	struct device_node *thp_dev_node = NULL;

	thp_dev_node = of_find_compatible_node(NULL, NULL,
					THP_INPUT_DEV_COMPATIBLE);
	if (!thp_dev_node) {
		THP_LOG_INFO("%s:not found node, use defatle config\n",
					__func__);
		goto use_defaule;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_x",
						&config->abs_max_x);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_x not config, use deault\n", __func__);
		config->abs_max_x = THP_MT_WRAPPER_MAX_X;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_y",
						&config->abs_max_y);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_y not config, use deault\n", __func__);
		config->abs_max_y = THP_MT_WRAPPER_MAX_Y;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_z",
						&config->abs_max_z);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_z not config, use deault\n", __func__);
		config->abs_max_z = THP_MT_WRAPPER_MAX_Z;
	}

	rc = of_property_read_u32(thp_dev_node, "tracking_id_max",
						&config->tracking_id_max);
	if (rc) {
		THP_LOG_ERR("%s:tracking_id_max not config, use deault\n",
				__func__);
		config->tracking_id_max = THP_MT_WRAPPER_MAX_FINGERS;
	}

	rc = of_property_read_u32(thp_dev_node, "major_max",
						&config->major_max);
	if (rc) {
		THP_LOG_ERR("%s:major_max not config, use deault\n", __func__);
		config->major_max = THP_MT_WRAPPER_MAX_MAJOR;
	}

	rc = of_property_read_u32(thp_dev_node, "minor_max",
						&config->minor_max);
	if (rc) {
		THP_LOG_ERR("%s:minor_max not config, use deault\n", __func__);
		config->minor_max = THP_MT_WRAPPER_MAX_MINOR;
	}

	rc = of_property_read_u32(thp_dev_node, "orientation_min",
						&config->orientation_min);
	if (rc) {
		THP_LOG_ERR("%s:orientation_min not config, use deault\n",
				__func__);
		config->orientation_min = THP_MT_WRAPPER_MIN_ORIENTATION;
	}

	rc = of_property_read_u32(thp_dev_node, "orientation_max",
					&config->orientation_max);
	if (rc) {
		THP_LOG_ERR("%s:orientation_max not config, use deault\n",
				__func__);
		config->orientation_max = THP_MT_WRAPPER_MAX_ORIENTATION;
	}

	rc = of_property_read_u32(thp_dev_node, "tool_type_max",
					&config->tool_type_max);
	if (rc) {
		THP_LOG_ERR("%s:tool_type_max not config, use deault\n",
				__func__);
		config->tool_type_max = THP_MT_WRAPPER_TOOL_TYPE_MAX;
	}

	return 0;

use_defaule:
	set_default_input_config(config);
	return 0;
}

int thp_mt_wrapper_init(void)
{
	struct input_dev *input_dev;
	static struct thp_mt_wrapper_data *mt_wrapper;
	int rc;

	if (g_thp_mt_wrapper) {
		THP_LOG_ERR("%s:thp_mt_wrapper have inited, exit\n", __func__);
		return 0;
	}

	mt_wrapper = kzalloc(sizeof(struct thp_mt_wrapper_data), GFP_KERNEL);
	if (!mt_wrapper) {
		THP_LOG_ERR("%s:out of memory\n", __func__);
		return -ENOMEM;
	}
	init_waitqueue_head(&mt_wrapper->wait);

	input_dev = input_allocate_device();
	if (!input_dev) {
		THP_LOG_ERR("%s:Unable to allocated input device\n", __func__);
		kfree(mt_wrapper);
		return	-ENODEV;
	}

	input_dev->name = THP_INPUT_DEVICE_NAME;

	rc = thp_parse_input_config(&mt_wrapper->input_dev_config);
	if (rc)
		THP_LOG_ERR("%s: parse config fail\n", __func__);

	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	__set_bit(KEY_F26, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X,
			     0, mt_wrapper->input_dev_config.abs_max_x - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, mt_wrapper->input_dev_config.abs_max_y - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE,
			0, mt_wrapper->input_dev_config.abs_max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			0, mt_wrapper->input_dev_config.abs_max_x - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			0, mt_wrapper->input_dev_config.abs_max_y - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			0, mt_wrapper->input_dev_config.abs_max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,
		0, mt_wrapper->input_dev_config.tracking_id_max - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			0, mt_wrapper->input_dev_config.major_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
			0, mt_wrapper->input_dev_config.minor_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
			mt_wrapper->input_dev_config.orientation_min,
			mt_wrapper->input_dev_config.orientation_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_BLOB_ID,
			0,
			10, 0, 0);
#ifdef TYPE_B_PROTOCOL
	input_mt_init_slots(input_dev, THP_MT_WRAPPER_MAX_FINGERS);
#endif

	rc = input_register_device(input_dev);
	if (rc) {
		THP_LOG_ERR("%s:failed to register input device\n", __func__);
		goto input_dev_reg_err;
	}

	rc = misc_register(&g_thp_mt_wrapper_misc_device);
	if (rc) {
		THP_LOG_ERR("%s:failed to register misc device\n", __func__);
		goto misc_dev_reg_err;
	}

	mt_wrapper->input_dev = input_dev;
	g_thp_mt_wrapper = mt_wrapper;
	atomic_set(&g_thp_mt_wrapper->status_updated, 0);
	return 0;

misc_dev_reg_err:
	input_unregister_device(input_dev);
input_dev_reg_err:
	kfree(mt_wrapper);

	return rc;
}
EXPORT_SYMBOL(thp_mt_wrapper_init);

void thp_mt_wrapper_exit(void)
{
	if (!g_thp_mt_wrapper)
		return;

	input_unregister_device(g_thp_mt_wrapper->input_dev);
	misc_deregister(&g_thp_mt_wrapper_misc_device);
}
EXPORT_SYMBOL(thp_mt_wrapper_exit);

