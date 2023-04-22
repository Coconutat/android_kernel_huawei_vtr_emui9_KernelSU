

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/inputhub/motionhub.h>
#include "protocol.h"
#include "contexthub_route.h"
#include "contexthub_boot.h"
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include "contexthub_ext_log.h"

static bool motion_status[MOTION_TYPE_END];
static int motion_ref_cnt;

extern int stop_auto_motion;
extern int step_ref_cnt;
extern int flag_for_sensor_test;

extern bool really_do_enable_disable(int *ref_cnt, bool enable, int bit);
extern int send_app_config_cmd(int tag, void *app_config, bool use_lock);
extern void save_step_count(void);

struct motions_cmd_map {
	int mhb_ioctl_app_cmd;
	int motion_type;
	int tag;
	obj_cmd_t cmd;
	obj_sub_cmd_t subcmd;
};
static const struct motions_cmd_map motions_cmd_map_tab[] = {
	{MHB_IOCTL_MOTION_START, -1, TAG_MOTION, CMD_CMN_OPEN_REQ, SUB_CMD_NULL_REQ},
	{MHB_IOCTL_MOTION_STOP, -1, TAG_MOTION, CMD_CMN_CLOSE_REQ, SUB_CMD_NULL_REQ},
	{MHB_IOCTL_MOTION_ATTR_START, -1, TAG_MOTION, CMD_CMN_CONFIG_REQ, SUB_CMD_MOTION_ATTR_ENABLE_REQ},
	{MHB_IOCTL_MOTION_ATTR_STOP, -1, TAG_MOTION, CMD_CMN_CONFIG_REQ, SUB_CMD_MOTION_ATTR_DISABLE_REQ},
	{MHB_IOCTL_MOTION_INTERVAL_SET, -1, TAG_MOTION, CMD_CMN_INTERVAL_REQ, SUB_CMD_NULL_REQ},
};

static char *motion_type_str[] = {
	[MOTION_TYPE_START] = "start",
	[MOTION_TYPE_PICKUP] = "pickup",
	[MOTION_TYPE_FLIP] = "flip",
	[MOTION_TYPE_PROXIMITY] = "proximity",
	[MOTION_TYPE_SHAKE] = "shake",
	[MOTION_TYPE_TAP] = "tap",
	[MOTION_TYPE_TILT_LR] = "tilt_lr",
	[MOTION_TYPE_ROTATION] = "rotation",
	[MOTION_TYPE_POCKET] = "pocket",
	[MOTION_TYPE_ACTIVITY] = "activity",
	[MOTION_TYPE_TAKE_OFF] = "take_off",
	[MOTION_TYPE_EXTEND_STEP_COUNTER] = "ext_step_counter",
	[MOTION_TYPE_EXT_LOG] = "ext_log",
	[MOTION_TYPE_HEAD_DOWN] = "head_down",
	[MOTION_TYPE_PUT_DOWN] = "put_down",
	[MOTION_TYPE_SIDEGRIP] = "sidegrip",
	[MOTION_TYPE_END] = "end",
};

static int motion_ext_log_hanlder (const pkt_header_t *head)
{
	int offset, total_len;
	size_t payload_len;
	ext_logger_req_t *pkt_ext = (ext_logger_req_t *)head;
	pedo_ext_logger_req_t *pkt_pedo = (pedo_ext_logger_req_t *)pkt_ext->data;
    //split packet and write to route
	hwlog_debug("%s in head tag %d cmd %d len %d handler tag %d\n", __func__, head->tag, head->cmd, head->length, pkt_ext->tag);
	//extract every payload
	offset = 0;
	total_len = pkt_ext->hd.length - (offsetof(ext_logger_req_t, data) - sizeof(pkt_header_t));
	for(;offset < total_len;)
	{
		payload_len = pkt_pedo->len + offsetof(pedo_ext_logger_req_t, data);
		hwlog_debug("motion_ext_log_hanlder offset %d len %d, pointer %pK\n", offset, payload_len, pkt_pedo);
		if(payload_len + offset > total_len)
		{
			hwlog_err("%s overstacked payload_len %d offset %d total_len %d\n", payload_len, offset, total_len);
			break;
		}
		inputhub_route_write(ROUTE_MOTION_PORT, (char *)pkt_pedo, payload_len);
		offset += payload_len;
		pkt_pedo = (pedo_ext_logger_req_t *)(pkt_ext->data + offset);
	}
    return 0;
}

static void update_motion_info(obj_cmd_t cmd, motion_type_t type)
{
	if (!(MOTION_TYPE_START <= type && type < MOTION_TYPE_END))
		return;

	switch (cmd) {
	case CMD_CMN_OPEN_REQ:
		motion_status[type] = true;
		break;

	case CMD_CMN_CLOSE_REQ:
		motion_status[type] = false;
		break;

	default:
		hwlog_err("unknown cmd type in %s\n", __func__);
		break;
	}
}

static int extend_step_counter_process(bool enable)
{
	uint8_t app_config[16] = { 0, };
	interval_param_t extend_open_param = {
		.period = 20,	/*default delay_ms*/
		.batch_count = 1,
		.mode = AUTO_MODE,
		.reserved[0] = TYPE_EXTEND
	};

	inputhub_sensor_enable_stepcounter(enable, TYPE_EXTEND);
	if (enable) {
		app_config[0] = enable;
		app_config[1] = TYPE_EXTEND;

		inputhub_sensor_setdelay(TAG_STEP_COUNTER, &extend_open_param);
		return send_app_config_cmd(TAG_STEP_COUNTER, app_config, true);
	}
	return 0;
}

static int extend_step_counter_process_nolock(bool enable)
{
	uint8_t app_config[16] = { 0, };

	app_config[0] = enable;
	app_config[1] = TYPE_EXTEND;
	/*close step counter*/
	if (!enable) {
		send_app_config_cmd(TAG_STEP_COUNTER, app_config, false);
	}
	hwlog_info("%s extend_step_counter!\n", enable ? "open" : "close");
	if (really_do_enable_disable(&step_ref_cnt, enable, TYPE_EXTEND)) {
		inputhub_sensor_enable_nolock(TAG_STEP_COUNTER, enable);
	}
	if (enable) {
		interval_param_t extend_open_param = {
			.period = 20,	/*default delay_ms*/
			.batch_count = 1,
			.mode = AUTO_MODE,
			.reserved[0] = TYPE_EXTEND
		};
		inputhub_sensor_setdelay_nolock(TAG_STEP_COUNTER, &extend_open_param);
		return send_app_config_cmd(TAG_STEP_COUNTER, app_config, false);
	}
	return 0;
}

static int send_motion_cmd_internal(int tag, obj_cmd_t cmd, obj_sub_cmd_t subcmd, motion_type_t type, bool use_lock)
{
	uint8_t app_config[16] = { 0, };
	interval_param_t interval_param;

	if(stop_auto_motion == 1) {
		hwlog_info("%s stop_auto_motion: %d !", __func__, stop_auto_motion);
		return 0;
	}

	app_config[0] = type;
	app_config[1] = cmd;
	memset(&interval_param, 0, sizeof(interval_param));

	if ((MOTIONHUB_TYPE_HW_STEP_COUNTER == type) && (CMD_CMN_OPEN_REQ == cmd || CMD_CMN_CLOSE_REQ == cmd)) {
		if (use_lock) {
			return extend_step_counter_process(CMD_CMN_OPEN_REQ == cmd);
		} else {
			return extend_step_counter_process_nolock(CMD_CMN_OPEN_REQ == cmd);
		}
	}

	if (CMD_CMN_OPEN_REQ == cmd) {
		/*send open motion cmd when open first sub type*/
		if (really_do_enable_disable(&motion_ref_cnt, true, type)) {
			if (use_lock) {
                		inputhub_sensor_enable(tag, true);
				inputhub_sensor_setdelay(tag, &interval_param);
        		} else {
               			inputhub_sensor_enable_nolock(tag, true);
				inputhub_sensor_setdelay_nolock(tag, &interval_param);
        		}
			hwlog_info("send_motion_cmd open cmd:%d motion: %s !", cmd, motion_type_str[type]);
		}
		/*send config cmd to open motion type*/
		send_app_config_cmd(TAG_MOTION, app_config, use_lock);
		hwlog_info("send_motion_cmd config cmd:%d motion: %s !motion_ref_cnt= %x", cmd, motion_type_str[type], motion_ref_cnt);
	} else if (CMD_CMN_CLOSE_REQ == cmd) {
		/*send config cmd to close motion type*/
		send_app_config_cmd(TAG_MOTION, app_config, use_lock);
		hwlog_info("send_motion_cmd config cmd:%d motion: %s !motion_ref_cnt= %x", cmd, motion_type_str[type], motion_ref_cnt);

		/*send close motion cmd when all sub type closed*/
		if (really_do_enable_disable(&motion_ref_cnt, false, type)) {
			if (use_lock) {
                		inputhub_sensor_enable(tag, false);
            		} else {
               			inputhub_sensor_enable_nolock(tag, false);
            		}
			hwlog_info("send_motion_cmd close cmd:%d motion: %s !", cmd, motion_type_str[type]);
		}
	} else {
		hwlog_err("send_motion_cmd unknown cmd!\n");
		return -EINVAL;
	}

	return 0;
}

static int send_motion_cmd(unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int argvalue = 0;
	int i;

	if (flag_for_sensor_test)
		return 0;

	for (i = 0; i < sizeof(motions_cmd_map_tab) / sizeof(motions_cmd_map_tab[0]); ++i) {
		if (motions_cmd_map_tab[i].mhb_ioctl_app_cmd == cmd) {
			break;
		}
	}

	if (sizeof(motions_cmd_map_tab) / sizeof(motions_cmd_map_tab[0]) == i) {
		hwlog_err("send_motion_cmd unknown cmd %d in parse_motion_cmd!\n", cmd);
		return -EFAULT;
	}

	if (copy_from_user(&argvalue, argp, sizeof(argvalue)))
		return -EFAULT;

	if (!(MOTION_TYPE_START <= argvalue && argvalue < MOTION_TYPE_END)) {
		hwlog_err("error motion type %d in %s\n", argvalue, __func__);
		return -EINVAL;
	}
	update_motion_info(motions_cmd_map_tab[i].cmd, argvalue);

	return send_motion_cmd_internal(motions_cmd_map_tab[i].tag, motions_cmd_map_tab[i].cmd,
		motions_cmd_map_tab[i].subcmd, argvalue, true);
}

void enable_motions_when_recovery_iom3(void)
{
	motion_type_t type;
	motion_ref_cnt = 0;	/*to send open motion cmd when open first type*/
	for (type = MOTION_TYPE_START; type < MOTION_TYPE_END; ++type) {
		if (motion_status[type]) {
			hwlog_info("motion state %d in %s\n", type, __func__);
			send_motion_cmd_internal(TAG_MOTION, CMD_CMN_OPEN_REQ, SUB_CMD_NULL_REQ, type, false);
		}
	}
}

void disable_motions_when_sysreboot(void)
{
	motion_type_t type;
	for (type = MOTION_TYPE_START; type < MOTION_TYPE_END; ++type) {
		if (motion_status[type]) {
			hwlog_info("motion state %d in %s\n", type, __func__);
			send_motion_cmd_internal(TAG_MOTION, CMD_CMN_CLOSE_REQ, SUB_CMD_NULL_REQ, type, false);
		}
	}
}

/*******************************************************************************************
Function:       mhb_read
Description:   read /dev/motionhub
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         no
Return:         length of read data
*******************************************************************************************/
static ssize_t mhb_read(struct file *file, char __user *buf, size_t count,
			loff_t *pos)
{
	return inputhub_route_read(ROUTE_MOTION_PORT, buf, count);
}

/*******************************************************************************************
Function:       mhb_write
Description:   write to /dev/motionhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, const char __user *data, size_t len, loff_t *ppos
Output:         no
Return:         length of write data
*******************************************************************************************/
static ssize_t mhb_write(struct file *file, const char __user *data,
			 size_t len, loff_t *ppos)
{
	hwlog_info("%s need to do...\n", __func__);

	return len;
}

/*******************************************************************************************
Function:       mhb_ioctl
Description:   ioctrl function to /dev/motionhub, do open, close motion, or set interval and attribute to motion
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, unsigned int cmd, unsigned long arg
			cmd indicates command, arg indicates parameter
Output:         no
Return:         result of ioctrl command, 0 successed, -ENOTTY failed
*******************************************************************************************/
static long mhb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MHB_IOCTL_MOTION_START:
	case MHB_IOCTL_MOTION_STOP:
	case MHB_IOCTL_MOTION_ATTR_START:
	case MHB_IOCTL_MOTION_ATTR_STOP:
		break;
	default:
		hwlog_err("%s unknown cmd : %d\n", __func__, cmd);
		return -ENOTTY;
	}
	return send_motion_cmd(cmd, arg);
}

/*******************************************************************************************
Function:       mhb_open
Description:   open to /dev/motionhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of open
*******************************************************************************************/
static int mhb_open(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok!\n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       mhb_release
Description:   releaseto /dev/motionhub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of release
*******************************************************************************************/
static int mhb_release(struct inode *inode, struct file *file)
{
	hwlog_info("%s ok!\n", __func__);
	return 0;
}

static int motion_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
{
	/* prevent access the emmc now: */
	hwlog_info("%s (%lu) +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		save_step_count();
		enable_motions_when_recovery_iom3();
		break;
	default:
		hwlog_err("%s -unknow state %ld\n", __func__, foo);
		break;
	}
	hwlog_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block motion_recovery_notify = {
	.notifier_call = motion_recovery_notifier,
	.priority = -1,
};

/*******************************************************************************************
Description:   file_operations to motion
*******************************************************************************************/
static const struct file_operations mhb_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = mhb_read,
	.write = mhb_write,
	.unlocked_ioctl = mhb_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mhb_ioctl,
#endif
	.open = mhb_open,
	.release = mhb_release,
};

/*******************************************************************************************
Description:   miscdevice to motion
*******************************************************************************************/
static struct miscdevice motionhub_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "motionhub",
	.fops = &mhb_fops,
};

/*******************************************************************************************
Function:       motionhub_init
Description:   apply kernel buffer, register motionhub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        result of function, 0 successed, else false
*******************************************************************************************/
static int __init motionhub_init(void)
{
	int ret;

	if (is_sensorhub_disabled())
		return -1;

	if (!getSensorMcuMode())
	{
		hwlog_err("mcu boot fail,motionhub_init exit\n");
		return -1;
	}

	hwlog_info("%s start \n", __func__);

	ret = inputhub_route_open(ROUTE_MOTION_PORT);
	if (ret != 0) {
		hwlog_err("cannot open inputhub route err=%d\n", ret);
		goto OUT;
	}

	ret = misc_register(&motionhub_miscdev);
	if (ret != 0) {
		hwlog_err("cannot register miscdev err=%d\n", ret);
		goto CLOSE;
	}
    ret = inputhub_ext_log_register_handler(TAG_MOTION, motion_ext_log_hanlder);
    if (ret != 0) {
        hwlog_err("cannot register ext_log err=%d\n", ret);
        goto CLOSE;
    }

	register_iom3_recovery_notifier(&motion_recovery_notify);
	hwlog_info("%s ok\n", __func__);
	goto OUT;
CLOSE:
    inputhub_route_close(ROUTE_MOTION_PORT);
OUT:
	return ret;
}

/*******************************************************************************************
Function:       motionhub_exit
Description:   release kernel buffer, deregister motionhub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        void
*******************************************************************************************/
static void __exit motionhub_exit(void)
{
	inputhub_route_close(ROUTE_MOTION_PORT);
	misc_deregister(&motionhub_miscdev);
	hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(motionhub_init);
module_exit(motionhub_exit);

MODULE_AUTHOR("MotionHub <smartphone@huawei.com>");
MODULE_DESCRIPTION("MotionHub driver");
MODULE_LICENSE("GPL");
