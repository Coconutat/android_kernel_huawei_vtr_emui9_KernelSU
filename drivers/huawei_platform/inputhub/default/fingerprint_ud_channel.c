/*
 *  drivers/misc/inputhub/fingerprinthub_ud_channel.c
 *  FHB Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: @
 *  Date:   2016.3.10
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/inputhub/fingerprinthub.h>

#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "protocol.h"

static int fp_ref_cnt;
static bool fingerprint_status[FINGERPRINT_TYPE_END];

#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#define FHB_CONFIG_UD_OPTICAL_BRIGHTNESS 14
static uint8_t g_optical_brightness[5] = {0};
static uint8_t g_optical_brightness_len = 0;

extern  struct wake_lock wlock;
extern int flag_for_sensor_test;

extern bool really_do_enable_disable(int *ref_cnt, bool enable, int bit);
extern int send_app_config_cmd_with_resp(int tag, void *app_config, bool use_lock);

struct fingerprint_cmd_map
{
    int         fhb_ioctl_app_cmd;
    int         ca_type;
    int         tag;
    obj_cmd_t cmd;
};

static const struct fingerprint_cmd_map fingerprint_cmd_map_tab[] =
{
    {FHB_IOCTL_FP_START, -1,  TAG_FP_UD, CMD_CMN_OPEN_REQ},
    {FHB_IOCTL_FP_STOP,   -1,  TAG_FP_UD, CMD_CMN_CLOSE_REQ},
    {FHB_IOCTL_FP_DISABLE_SET, -1, TAG_FP_UD, FHB_IOCTL_FP_DISABLE_SET_CMD},
};

static void update_fingerprint_info(obj_cmd_t cmd, fingerprint_type_t type)
{
    switch (cmd)
    {
        case CMD_CMN_OPEN_REQ:
            fingerprint_status[type] = true;
            hwlog_err("fingerprint: CMD_CMN_OPEN_REQ in %s, type:%d, %d\n", __func__,type,fingerprint_status[type]);
            break;
        case CMD_CMN_CLOSE_REQ:
            fingerprint_status[type] = false;
            hwlog_err("fingerprint: CMD_CMN_CLOSE_REQ in %s, type:%d, %d\n", __func__,type,fingerprint_status[type]);
            break;
        default:
            hwlog_err("fingerprint: unknown cmd type in %s, type:%d\n", __func__,type);
            break;
    }
}

static void fingerprint_report(void)
{
    char* fingerprint_data = NULL;
    fingerprint_upload_pkt_t fingerprint_upload;
    memset(&fingerprint_upload, 0, sizeof(fingerprint_upload_pkt_t));
    fingerprint_upload.fhd.hd.tag = TAG_FP_UD;
    fingerprint_upload.fhd.hd.cmd = CMD_DATA_REQ;
    fingerprint_upload.data = 0; //0: cancel wait sensorhub msg
    fingerprint_data = (char*)(&fingerprint_upload) + sizeof(pkt_common_data_t);
    inputhub_route_write(ROUTE_FHB_UD_PORT, fingerprint_data, sizeof(fingerprint_upload.data));
}

static int send_fingerprint_cmd_internal(int tag, obj_cmd_t cmd, fingerprint_type_t type, bool use_lock)
{
    interval_param_t interval_param;
    uint8_t app_config[16] = {0,};

    memset(&interval_param, 0, sizeof(interval_param));
    update_fingerprint_info(cmd, type);
    if (CMD_CMN_OPEN_REQ == cmd)
    {
        if (really_do_enable_disable(&fp_ref_cnt, true, type))
        {
            app_config[0] = SUB_CMD_FINGERPRINT_OPEN_REQ;
            if (use_lock)
            {
                inputhub_sensor_enable(tag, true);
                send_app_config_cmd_with_resp(tag, app_config, true);
            }
            else
            {
                inputhub_sensor_enable_nolock(tag, true);
                send_app_config_cmd_with_resp(tag, app_config, false);
            }
            hwlog_info("fingerprint: %s:CMD_CMN_OPEN_REQ cmd:%d!\n",__func__, cmd);
        }
    }
    else if (CMD_CMN_CLOSE_REQ == cmd)
    {
        if (really_do_enable_disable(&fp_ref_cnt, false, type))
        {
            app_config[0] = SUB_CMD_FINGERPRINT_CLOSE_REQ;
            if (use_lock)
            {
                send_app_config_cmd_with_resp(tag, app_config, true);
                inputhub_sensor_enable(tag, false);
            }
            else
            {
                send_app_config_cmd_with_resp(tag, app_config, false);
                inputhub_sensor_enable_nolock(tag, false);
            }
            hwlog_info("fingerprint: %s: CMD_CMN_CLOSE_REQ cmd:%d !\n", __func__, cmd);
        }
    }
    else if (FHB_IOCTL_FP_DISABLE_SET_CMD == cmd)
    {
        fingerprint_report();
        hwlog_info("fingerprint: %s: CMD_FINGERPRINT_DISABLE_SET cmd:%d !\n", __func__, cmd);
    }
    else
    {
        hwlog_err("fingerprint: %s: unknown cmd!\n", __func__);
        return -EINVAL;
    }

    return 0;
}

static int send_fingerprint_cmd(unsigned int cmd, unsigned long arg)
{
    void __user* argp = (void __user*)arg;
    int argvalue = 0;
    int i;

    if (flag_for_sensor_test)
    return 0;

    hwlog_info("fingerprint: %s enter!\n", __func__);
    for (i = 0; i < sizeof(fingerprint_cmd_map_tab) / sizeof(fingerprint_cmd_map_tab[0]); i++)
    {
        if (fingerprint_cmd_map_tab[i].fhb_ioctl_app_cmd == cmd)
        {
            break;
        }
    }
    if (sizeof(fingerprint_cmd_map_tab) / sizeof(fingerprint_cmd_map_tab[0]) == i)
    {
        hwlog_err("fingerprint: %s unknown cmd %d in parse_ca_cmd!\n", __func__,cmd);
        return -EFAULT;
    }
    if (copy_from_user(&argvalue, argp, sizeof(argvalue)))
    {
        hwlog_err("fingerprint: %s copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    if (!(FINGERPRINT_TYPE_START <= argvalue && argvalue < FINGERPRINT_TYPE_END)) {
        hwlog_err("error fingerprint type %d in %s\n", argvalue, __func__);
        return -EINVAL;
    }

    return send_fingerprint_cmd_internal(fingerprint_cmd_map_tab[i].tag, fingerprint_cmd_map_tab[i].cmd, argvalue, true);//true
}

static int fingerprint_recovery_config_para(void* data, int len)
{
    fingerprint_req_t fp_pkt;
    int ret = 0;
    write_info_t pkg_ap;
    int i = 0;

    memset(&fp_pkt, 0, sizeof(fp_pkt));
    memset(&pkg_ap, 0, sizeof(pkg_ap));

    if (len > sizeof(fp_pkt.buf))
    {
        hwlog_warn("fingerprint: fingerprint_recovery_config_para len is out of size, len=%d\n", len);
        return -1;
    }

    memcpy(&fp_pkt.buf[0], data, len);

    fp_pkt.len = len;
    fp_pkt.sub_cmd = SUB_CMD_FINGERPRINT_CONFIG_SENSOR_DATA_REQ;

    hwlog_info("fingerprint: fingerprint_recovery_config_para data=%d, len=%d\n", fp_pkt.buf[0], len);

    pkg_ap.tag = TAG_FP_UD;
    pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
    pkg_ap.wr_buf = &fp_pkt;
    pkg_ap.wr_len = sizeof(fp_pkt);
    ret = write_customize_cmd(&pkg_ap, NULL, false);
    if (ret) {
        hwlog_err("fhb_ud_write fail,ret=%d\n", ret);
    }

    return ret;
}

static void enable_fingerprint_when_recovery_iom3(void)
{
    fingerprint_type_t type;

    fp_ref_cnt = 0;
    fingerprint_recovery_config_para(g_optical_brightness, g_optical_brightness_len);
    for (type = FINGERPRINT_TYPE_START; type < FINGERPRINT_TYPE_END; ++type)
    {
        if (fingerprint_status[type])
        {
            hwlog_info("fingerprint: finger state %d in %s\n", type, __func__);
            send_fingerprint_cmd_internal(TAG_FP_UD, CMD_CMN_OPEN_REQ, type, false);
        }
    }
}

void disable_fingerprint_ud_when_sysreboot(void)
{
    fingerprint_type_t type;
    for (type = FINGERPRINT_TYPE_START; type < FINGERPRINT_TYPE_END; ++type)
    {
        if (fingerprint_status[type])
        {
            hwlog_info("fingerprint: finger state %d in %s\n", type, __func__);
            send_fingerprint_cmd_internal(TAG_FP_UD, CMD_CMN_CLOSE_REQ, type, false);
        }
    }
}

/*******************************************************************************************
Function:       fhb_read
Description:    read /dev/fingerprinthub_ud
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         no
Return:         length of read data
*******************************************************************************************/
static ssize_t fhb_ud_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    return inputhub_route_read(ROUTE_FHB_UD_PORT, buf, count);
}

/*******************************************************************************************
Function:       chb_write
Description:    write to /dev/fingerprinthub_ud, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, const char __user *data, size_t len, loff_t *ppos
Output:         no
Return:         length of write data
*******************************************************************************************/

static ssize_t fhb_ud_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
    fingerprint_req_t fp_pkt;
    int ret = 0;
    write_info_t pkg_ap;

    memset(&fp_pkt, 0, sizeof(fp_pkt));
    memset(&pkg_ap, 0, sizeof(pkg_ap));

    if (len > sizeof(fp_pkt.buf))
    {
        hwlog_warn("fingerprint: fhb_ud_write len is out of size, len=%d\n", len);
        return -1;
    }
    if (copy_from_user(fp_pkt.buf, data, len))
    {
        hwlog_warn("fingerprint: %s copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    if (FHB_CONFIG_UD_OPTICAL_BRIGHTNESS == fp_pkt.buf[0])
    {
        g_optical_brightness_len = MIN(len, sizeof(g_optical_brightness));
        memcpy(g_optical_brightness, fp_pkt.buf, g_optical_brightness_len);

    }

    fp_pkt.len = len;
    fp_pkt.sub_cmd = SUB_CMD_FINGERPRINT_CONFIG_SENSOR_DATA_REQ;

    hwlog_info("fingerprint: fhb_ud_write data=%d, len=%d\n", fp_pkt.buf[0], len);

    pkg_ap.tag = TAG_FP_UD;
    pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
    pkg_ap.wr_buf = &fp_pkt;
    pkg_ap.wr_len = sizeof(fp_pkt);
    ret = write_customize_cmd(&pkg_ap, NULL, true);
    if (ret) {
        hwlog_err("fhb_ud_write fail,ret=%d\n", ret);
    }

    return len;
}

/*******************************************************************************************
Function:       chb_ioctl
Description:    ioctrl function to /dev/fingerprinthub_ud, do open, close ca, or set interval and attribute to fingerprinthub_ud
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, unsigned int cmd, unsigned long arg
                   cmd indicates command, arg indicates parameter
Output:         no
Return:         result of ioctrl command, 0 successed, -ENOTTY failed
*******************************************************************************************/
static long fhb_ud_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case FHB_IOCTL_FP_START:
            hwlog_err("fingerprint: %s  cmd: FHB_IOCTL_FP_START\n", __func__);
            break;
        case FHB_IOCTL_FP_STOP:
            hwlog_err("fingerprint: %s cmd: FHB_IOCTL_FP_STOP\n", __func__);
            break;
        case FHB_IOCTL_FP_DISABLE_SET:
            hwlog_err("fingerprint: %s set cmd : FHB_IOCTL_FP_DISABLE_SET\n", __func__);
            break;
        default:
            hwlog_err("fingerprint: %s unknown cmd : %d\n", __func__, cmd);
            return -ENOTTY;
    }

    return send_fingerprint_cmd(cmd, arg);
}

/*******************************************************************************************
Function:       fhb_open
Description:    open to /dev/fingerprinthub_ud, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of open
*******************************************************************************************/
static int fhb_ud_open(struct inode *inode, struct file *file)
{
    hwlog_info("fingerprint: enter %s \n", __func__);
    return 0;
}

static int fingerprint_recovery_notifier(struct notifier_block *nb, unsigned long foo, void *bar)
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
        enable_fingerprint_when_recovery_iom3();
        break;
    default:
        hwlog_err("%s -unknow state %ld\n", __func__, foo);
        break;
    }
    hwlog_info("%s -\n", __func__);
    return 0;
}

static struct notifier_block fingerprint_recovery_notify = {
    .notifier_call = fingerprint_recovery_notifier,
    .priority = -1,
};

/*******************************************************************************************
Description:   file_operations to fingerprinthub_ud
*******************************************************************************************/
static const struct file_operations fhb_ud_fops =
{
    .owner             = THIS_MODULE,
    .read              = fhb_ud_read,
    .write             = fhb_ud_write,
    .unlocked_ioctl    = fhb_ud_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl      = fhb_ud_ioctl,
#endif
    .open              = fhb_ud_open,
};

/*******************************************************************************************
Description:   miscdevice to fingerprinthub_ud
*******************************************************************************************/
static struct miscdevice fingerprinthub_ud_miscdev =
{
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "fingerprinthub_ud",
    .fops =     &fhb_ud_fops,
};

/*******************************************************************************************
Function:       fingerprinthub_ud_init
Description:    apply kernel buffer, register fingerprinthub_ud_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         result of function, 0 successed, else false
*******************************************************************************************/
static int __init fingerprinthub_ud_init(void)
{
    int ret;

    if (is_sensorhub_disabled())
        return -1;

    ret = inputhub_route_open(ROUTE_FHB_UD_PORT);
    if(ret != 0)
    {
        hwlog_err("fingerprint: %s cannot open inputhub route err=%d\n", __func__, ret);
        return ret;
    }

    ret = misc_register(&fingerprinthub_ud_miscdev);
    if(ret != 0)
    {
        hwlog_err("%s cannot register miscdev err=%d\n", __func__, ret);
        inputhub_route_close(ROUTE_FHB_UD_PORT);
        return ret;
    }
    register_iom3_recovery_notifier(&fingerprint_recovery_notify);
    hwlog_info( "%s  ok \n", __func__);

    return ret;
}

/*******************************************************************************************
Function:       fingerprinthub_ud_exit
Description:    release kernel buffer, deregister fingerprinthub_ud_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         void
*******************************************************************************************/
static void __exit fingerprinthub_ud_exit(void)
{
    inputhub_route_close(ROUTE_FHB_UD_PORT);
    misc_deregister(&fingerprinthub_ud_miscdev);
    hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(fingerprinthub_ud_init);
module_exit(fingerprinthub_ud_exit);

MODULE_AUTHOR("FPHub <zhangli36@huawei.com>");
MODULE_DESCRIPTION("FPHub driver");
MODULE_LICENSE("GPL");
