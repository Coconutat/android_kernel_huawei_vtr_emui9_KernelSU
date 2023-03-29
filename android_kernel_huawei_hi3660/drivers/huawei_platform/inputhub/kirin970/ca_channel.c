

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/inputhub/cahub.h>
#include <huawei_platform/inputhub/sensorhub.h>
#include "protocol.h"
#include "contexthub_route.h"
#include "contexthub_boot.h"
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif

extern bool really_do_enable_disable(int *ref_cnt, bool enable, int bit);
extern int send_app_config_cmd(int tag, void *app_config, bool use_lock);

static bool ca_status[CA_TYPE_END];
static int ca_ref_cnt;

extern int flag_for_sensor_test;

struct cas_cmd_map {
    int         chb_ioctl_app_cmd;
    int         ca_type;
    int         tag;
    obj_cmd_t cmd;
    obj_sub_cmd_t subcmd;
};

static const struct cas_cmd_map cas_cmd_map_tab[] = {
    {CHB_IOCTL_CA_START, -1,  TAG_CA, CMD_CMN_OPEN_REQ, SUB_CMD_NULL_REQ},
    {CHB_IOCTL_CA_STOP,   -1,  TAG_CA, CMD_CMN_CLOSE_REQ, SUB_CMD_NULL_REQ},
    {CHB_IOCTL_CA_ATTR_START,-1, TAG_CA, CMD_CMN_CONFIG_REQ, SUB_CMD_CA_ATTR_ENABLE_REQ},
    {CHB_IOCTL_CA_ATTR_STOP,  -1, TAG_CA, CMD_CMN_CONFIG_REQ, SUB_CMD_CA_ATTR_DISABLE_REQ},
    {CHB_IOCTL_CA_INTERVAL_SET, -1, TAG_CA, CMD_CMN_INTERVAL_REQ, SUB_CMD_NULL_REQ},
};

static char * ca_type_str[] = {
	[CA_TYPE_START] = "start",
	[CA_TYPE_PICKUP] = "pickup",
	[CA_TYPE_PUTDOWN] = "putdown",
	[CA_TYPE_ACTIVITY] = "activity",
	[CA_TYPE_HOLDING] = "holding",
	[CA_TYPE_MOTION] = "motion",
	[CA_TYPE_PLACEMENT] = "placement",
	[CA_TYPE_END] = "end",
};

static void update_ca_info(obj_cmd_t cmd, ca_type_t type)
{
	switch (cmd) {
	    case CMD_CMN_OPEN_REQ:
	        ca_status[type] = true;
	        break;

	    case CMD_CMN_CLOSE_REQ:
	        ca_status[type] = false;
	        break;

	    default:
	        hwlog_err("unknown cmd type in %s\n", __func__);
	        break;
	}
}

static int send_ca_cmd_internal(int tag, obj_cmd_t cmd, obj_sub_cmd_t subcmd, ca_type_t type, bool use_lock)
{
	uint8_t app_config[16] = {0,};
	interval_param_t interval_param;

	if (!(CA_TYPE_START <= type && type < CA_TYPE_END))
	    return -EINVAL;

	app_config[0] = type;
	app_config[1] = cmd;

	memset(&interval_param, 0, sizeof(interval_param));
	update_ca_info(cmd, type);

	if (CMD_CMN_OPEN_REQ == cmd) {
		if (really_do_enable_disable(&ca_ref_cnt, true, type)) {
			if (use_lock) {
				inputhub_sensor_enable(tag, true);
				inputhub_sensor_setdelay(tag, &interval_param);
	    		} else {
				inputhub_sensor_enable_nolock(tag, true);
				inputhub_sensor_setdelay_nolock(tag, &interval_param);
	    		}
		}
		send_app_config_cmd(TAG_CA, app_config, use_lock);
	} else if ( CMD_CMN_CLOSE_REQ == cmd) {
		send_app_config_cmd(TAG_CA, app_config, use_lock);

		if (really_do_enable_disable(&ca_ref_cnt, false, type)) {
			if (use_lock) {
				inputhub_sensor_enable(tag, false);
			} else {
				inputhub_sensor_enable_nolock(tag, false);
			}
		}
	} else {
		hwlog_err("send_ca_cmd_internal unknown cmd!\n");
		return -EINVAL;
	}
	return 0;
}

static int send_ca_cmd(unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int argvalue = 0;
	int i;

	if (flag_for_sensor_test)
	return 0;

	hwlog_info("send_ca_cmd enter!\n");
	for (i = 0; i < sizeof(cas_cmd_map_tab) / sizeof(cas_cmd_map_tab[0]); ++i) {
	    if (cas_cmd_map_tab[i].chb_ioctl_app_cmd == cmd) {
	        break;
	    }
	}

	if (sizeof(cas_cmd_map_tab) / sizeof(cas_cmd_map_tab[0]) == i) {
	    hwlog_err("send_ca_cmd unknown cmd %d in parse_ca_cmd!\n", cmd);
	    return -EFAULT;
	}

	if (copy_from_user(&argvalue, argp, sizeof(argvalue))) {
	    hwlog_err("send_ca_cmd copy_from_user failed!\n");
	    return -EFAULT;
	}

	hwlog_info("send_ca_cmd leave before send_ca_cmd_internal!\n");
	return send_ca_cmd_internal(cas_cmd_map_tab[i].tag, cas_cmd_map_tab[i].cmd, cas_cmd_map_tab[i].subcmd, argvalue, true);
}

static void enable_cas_when_recovery_iom3(void)
{
	ca_type_t type;
	ca_ref_cnt = 0;//to send open motion cmd when open first type
	for (type = CA_TYPE_START; type < CA_TYPE_END; ++type) {
	    if (ca_status[type]) {
	        hwlog_info("ca state %d in %s\n", type, __func__);
	        send_ca_cmd_internal(TAG_CA, CMD_CMN_OPEN_REQ, SUB_CMD_NULL_REQ, type, false);
	    }
	}
}

void disable_cas_when_sysreboot(void)
{
	ca_type_t type;
	for (type = CA_TYPE_START; type < CA_TYPE_END; ++type) {
	    if (ca_status[type]) {
	        hwlog_info("ca state %d in %s\n", type, __func__);
	        send_ca_cmd_internal(TAG_CA, CMD_CMN_CLOSE_REQ, SUB_CMD_NULL_REQ, type, false);
	    }
	}
}

/*******************************************************************************************
Function:       chb_read
Description:    read /dev/cahub
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         no
Return:         length of read data
*******************************************************************************************/
static ssize_t chb_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	hwlog_info("enter %s \n", __func__);
	return inputhub_route_read(ROUTE_CA_PORT, buf, count);
}

/*******************************************************************************************
Function:       chb_write
Description:    write to /dev/cahub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, const char __user *data, size_t len, loff_t *ppos
Output:         no
Return:         length of write data
*******************************************************************************************/
static ssize_t chb_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	return len;
}

/*******************************************************************************************
Function:       chb_ioctl
Description:    ioctrl function to /dev/cahub, do open, close ca, or set interval and attribute to ca
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, unsigned int cmd, unsigned long arg
                   cmd indicates command, arg indicates parameter
Output:         no
Return:         result of ioctrl command, 0 successed, -ENOTTY failed
*******************************************************************************************/
static long chb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	hwlog_info("%s cmd: [%d]\n", __func__, cmd);

	switch(cmd)
	{
	    case CHB_IOCTL_CA_START:
	    case CHB_IOCTL_CA_STOP:
	    case CHB_IOCTL_CA_ATTR_START:
	    case CHB_IOCTL_CA_ATTR_STOP:
	    case CHB_IOCTL_CA_INTERVAL_SET:
	        break;
	    default:
	        hwlog_err("%s unknown cmd : %d\n", __func__, cmd);
	        return -ENOTTY;
	}

	return send_ca_cmd(cmd, arg);
}

/*******************************************************************************************
Function:       chb_open
Description:    open to /dev/cahub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of open
*******************************************************************************************/
static int chb_open(struct inode *inode, struct file *file)
{
	hwlog_info("enter %s \n", __func__);
	return 0;
}

/*******************************************************************************************
Function:       chb_release
Description:    release to /dev/cahub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of release
*******************************************************************************************/
static int chb_release(struct inode *inode, struct file *file)
{
	hwlog_info("enter %s \n", __func__);
	return 0;
}

static int ca_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
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
		enable_cas_when_recovery_iom3();
		break;
	default:
		hwlog_err("%s -unknow state %ld\n", __func__, foo);
		break;
	}
	hwlog_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block ca_recovery_notify = {
	.notifier_call = ca_recovery_notifier,
	.priority = -1,
};

/*******************************************************************************************
Description:   file_operations to ca
*******************************************************************************************/
static const struct file_operations chb_fops =
{
    .owner             = THIS_MODULE,
    .llseek            = no_llseek,
    .read              = chb_read,
    .write             = chb_write,
    .unlocked_ioctl    = chb_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl      = chb_ioctl,
#endif
    .open              = chb_open,
    .release           = chb_release,
};

/*******************************************************************************************
Description:   miscdevice to ca
*******************************************************************************************/
static struct miscdevice cahub_miscdev =
{
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "cahub",
    .fops =     &chb_fops,
};

/*******************************************************************************************
Function:       cahub_init
Description:    apply kernel buffer, register cahub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         result of function, 0 successed, else false
*******************************************************************************************/
static int __init cahub_init(void)
{
	int ret;

	if (is_sensorhub_disabled())
		return -1;

	hwlog_info("enter %s \n", __func__);
	ret = inputhub_route_open(ROUTE_CA_PORT);
	if(ret != 0)
	{
	    hwlog_err("%s cannot open inputhub route err=%d\n", __func__, ret);
	    return ret;
	}

	ret = misc_register(&cahub_miscdev);
	if(ret != 0)
	{
	    hwlog_err("%s cannot register miscdev err=%d\n", __func__, ret);
	    inputhub_route_close(ROUTE_CA_PORT);
	    return ret;
	}
	register_iom3_recovery_notifier(&ca_recovery_notify);
	hwlog_info( "%s ok \n", __func__);
	return ret;
}

/*******************************************************************************************
Function:       cahub_exit
Description:    release kernel buffer, deregister cahub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         void
*******************************************************************************************/
static void __exit cahub_exit(void)
{
	hwlog_info("enter %s \n", __func__);
	inputhub_route_close(ROUTE_CA_PORT);
	misc_deregister(&cahub_miscdev);
	hwlog_info("exit %s \n", __func__);
}

late_initcall_sync(cahub_init);
module_exit(cahub_exit);

MODULE_AUTHOR("CAHub <changxue.lu@huawei.com>");
MODULE_DESCRIPTION("CAHub driver");
MODULE_LICENSE("GPL");
