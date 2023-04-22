/*
 *  drivers/misc/inputhub/fingerprinthub_channel.c
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

#include "inputhub_route.h"
#include "inputhub_bridge.h"
#include "protocol.h"
#include <linux/wakelock.h>
/*******************************************************************************************
Function:       fhb_read
Description:    read /dev/fingerprinthub
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, char __user *buf, size_t count, loff_t *pos
Output:         no
Return:         length of read data
*******************************************************************************************/
static ssize_t fhb_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	hwlog_info("fingerprint: enter %s \n", __func__);
	return inputhub_route_read(ROUTE_FHB_PORT, buf, count);
}

/*******************************************************************************************
Function:       chb_write
Description:    write to /dev/fingerprinthub	, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, const char __user *data, size_t len, loff_t *ppos
Output:         no
Return:         length of write data
*******************************************************************************************/

static ssize_t fhb_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{

    hwlog_info("fingerprint: enter %s \n", __func__);
    return len;
}

/*******************************************************************************************
Function:       chb_ioctl
Description:    ioctrl function to /dev/fingerprinthub, do open, close ca, or set interval and attribute to fingerprinthub
Data Accessed:  no
Data Updated:   no
Input:          struct file *file, unsigned int cmd, unsigned long arg
                   cmd indicates command, arg indicates parameter
Output:         no
Return:         result of ioctrl command, 0 successed, -ENOTTY failed
*******************************************************************************************/
static long fhb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
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

    inputhub_route_cmd(ROUTE_FHB_PORT,cmd,arg);

    return 0;
}

/*******************************************************************************************
Function:       fhb_open
Description:    open to /dev/fingerprinthub, do nothing now
Data Accessed:  no
Data Updated:   no
Input:          struct inode *inode, struct file *file
Output:         no
Return:         result of open
*******************************************************************************************/
static int fhb_open(struct inode *inode, struct file *file)
{
    hwlog_info("fingerprint: enter %s \n", __func__);
    return 0;
}
/****
*****/
extern int inputhub_mcu_write_cmd_adapter(const void *buf, unsigned int length,
				   struct read_info *rd);
extern bool really_do_enable_disable(int *ref_cnt, bool enable, int bit);
struct fingerprint_cmd_map
{
    int         fhb_ioctl_app_cmd;
    int         ca_type;
    int         tag;
    obj_cmd_t cmd;
};
static int fp_ref_cnt;
static const struct fingerprint_cmd_map fingerprint_cmd_map_tab[] =
{
    {FHB_IOCTL_FP_START, -1,  TAG_FP, CMD_FINGERPRINT_OPEN_REQ},
    {FHB_IOCTL_FP_STOP,   -1,  TAG_FP, CMD_FINGERPRINT_CLOSE_REQ},
    {FHB_IOCTL_FP_DISABLE_SET, -1, TAG_FP, FHB_IOCTL_FP_DISABLE_SET_CMD},
};
static bool fingerprint_status[FINGERPRINT_TYPE_END];
bool is_fingerprint_data_report(const pkt_header_t* head)
{
    return (head->tag == TAG_FP)
           && (CMD_FINGERPRINT_REPORT_REQ == head->cmd);
}

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
#define FINGERPRINT_DATA 2
#define FINGERPRINT_LENGTH 12
void fingerprint_report(void)
{
    fingerprint_upload_pkt_t fingerprint_upload;
    memset(&fingerprint_upload, 0, sizeof(fingerprint_upload_pkt_t));
    fingerprint_upload.hd.tag = TAG_FP;
    fingerprint_upload.hd.cmd = CMD_FINGERPRINT_REPORT_REQ;
    fingerprint_upload.data = FINGERPRINT_DATA;
    fingerprint_upload.hd.length = FINGERPRINT_LENGTH;
    inputhub_route_write(ROUTE_FHB_PORT, &fingerprint_upload, fingerprint_upload.hd.length);
}

extern  struct wake_lock wlock;
int send_fingerprint_cmd_internal(int tag, obj_cmd_t cmd, fingerprint_type_t type, bool use_lock)
{
    pkt_header_t hpkt;
    pkt_cmn_interval_req_t i_pkt;
    pkt_cmn_close_req_t c_pkt;
    struct read_info rd;
    memset(&rd, 0, sizeof(rd));
    memset(&i_pkt, 0, sizeof(i_pkt));
    memset(&c_pkt, 0, sizeof(c_pkt));
    update_fingerprint_info(cmd, type);
    if (CMD_CMN_OPEN_REQ == cmd)
    {
        if (really_do_enable_disable(&fp_ref_cnt, true, type))
        {
            hpkt.tag = TAG_FP;
            hpkt.cmd = CMD_CMN_OPEN_REQ;
            hpkt.resp = RESP;//NO_RESP
            hpkt.length = 0;
            if (use_lock)
            {
                inputhub_mcu_write_cmd_adapter(&hpkt, sizeof(hpkt), &rd);
            }
            else
            {
                inputhub_mcu_write_cmd_nolock(&hpkt, sizeof(hpkt));
            }
            i_pkt.hd.tag = tag;
            i_pkt.hd.cmd = CMD_CMN_INTERVAL_REQ;
            i_pkt.hd.resp = RESP;
            i_pkt.hd.length = sizeof(i_pkt.param);
            if (use_lock)
            {
                inputhub_mcu_write_cmd_adapter(&i_pkt, sizeof(i_pkt), &rd);
            }
            else
            {
                inputhub_mcu_write_cmd_nolock(&i_pkt, sizeof(i_pkt));
            }
            hwlog_info("fingerprint: %s:CMD_CMN_OPEN_REQ cmd:%d!\n",__func__, cmd);
        }
    }
    else if ( CMD_CMN_CLOSE_REQ == cmd)
    {
        if (really_do_enable_disable(&fp_ref_cnt, false, type))
        {
            c_pkt.hd.tag = TAG_FP;
            c_pkt.hd.cmd = CMD_CMN_CLOSE_REQ;
            c_pkt.hd.resp = RESP;
            c_pkt.hd.length = sizeof(c_pkt.close_param);
            if (use_lock)
            {
                inputhub_mcu_write_cmd_adapter(&c_pkt, sizeof(c_pkt), &rd);
            }
            else
            {
                inputhub_mcu_write_cmd_nolock(&c_pkt, sizeof(c_pkt));
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

int send_fingerprint_cmd(unsigned int cmd, unsigned long arg)
{
    void __user* argp = (void __user*)arg;
    int argvalue = 0;
    int i;
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

    if (!
        (FINGERPRINT_TYPE_START <= argvalue
            && argvalue < FINGERPRINT_TYPE_END)) {
        hwlog_err("error fingerprint type %d in %s\n", argvalue, __func__);
        return -EINVAL;
    }
    return send_fingerprint_cmd_internal(fingerprint_cmd_map_tab[i].tag, fingerprint_cmd_map_tab[i].cmd, argvalue, true);//true
}

void enable_fingerprint_when_recovery_iom3(void)
{
    fp_ref_cnt = 0;
    fingerprint_type_t type;
    for (type = FINGERPRINT_TYPE_START; type < FINGERPRINT_TYPE_END; ++type)
    {
        if (fingerprint_status[type])
        {
            hwlog_info("fingerprint: finger state %d in %s\n", type, __func__);
            send_fingerprint_cmd_internal(TAG_FP, CMD_CMN_OPEN_REQ, type, false);
        }
    }
}
void disable_fingerprint_when_sysreboot(void)
{
    fingerprint_type_t type;
    for (type = FINGERPRINT_TYPE_START; type < FINGERPRINT_TYPE_END; ++type)
    {
        if (fingerprint_status[type])
        {
            hwlog_info("fingerprint: finger state %d in %s\n", type, __func__);
            send_fingerprint_cmd_internal(TAG_FP, CMD_CMN_CLOSE_REQ, type, false);
        }
    }
    }

/*******************************************************************************************
Description:   file_operations to fingerprinthub
*******************************************************************************************/
static const struct file_operations fhb_fops =
{
    .owner             = THIS_MODULE,
    .read              = fhb_read,
    .write             = fhb_write,
    .unlocked_ioctl    = fhb_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl      = fhb_ioctl,
#endif
    .open              = fhb_open,
};

/*******************************************************************************************
Description:   miscdevice to fingerprinthub
*******************************************************************************************/
static struct miscdevice fingerprinthub_miscdev =
{
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "fingerprinthub",
    .fops =     &fhb_fops,
};

/*******************************************************************************************
Function:       fingerprinthub_init
Description:    apply kernel buffer, register fingerprinthub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         result of function, 0 successed, else false
*******************************************************************************************/
static int __init fingerprinthub_init(void)
{
    int ret;

    ret = inputhub_route_open(ROUTE_FHB_PORT);
    if(ret != 0)
    {
        hwlog_err("fingerprint: %s cannot open inputhub route err=%d\n", __func__, ret);
        return ret;
    }

    ret = misc_register(&fingerprinthub_miscdev);
    if(ret != 0)
    {
        hwlog_err("%s cannot register miscdev err=%d\n", __func__, ret);
        inputhub_route_close(ROUTE_FHB_PORT);
        return ret;
    }
    hwlog_info( "%s  ok \n", __func__);

    return ret;
}

/*******************************************************************************************
Function:       fingerprinthub_exit
Description:    release kernel buffer, deregister fingerprinthub_miscdev
Data Accessed:  no
Data Updated:   no
Input:          void
Output:         void
Return:         void
*******************************************************************************************/
static void __exit fingerprinthub_exit(void)
{
    inputhub_route_close(ROUTE_FHB_PORT);
    misc_deregister(&fingerprinthub_miscdev);
    hwlog_info("exit %s\n", __func__);
}

module_init(fingerprinthub_init);
module_exit(fingerprinthub_exit);

MODULE_AUTHOR("FPHub <weixiangzhong@huawei.com>");
MODULE_DESCRIPTION("FPHub driver");
MODULE_LICENSE("GPL");
