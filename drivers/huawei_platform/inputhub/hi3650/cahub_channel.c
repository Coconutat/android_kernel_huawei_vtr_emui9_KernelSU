

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

#include "inputhub_route.h"
#include "inputhub_bridge.h"

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
/*
    char cmdbuf[DEV_CAHUB_MAX_WRITE_SIZE];

    if(len <= DEV_CAHUB_MAX_WRITE_SIZE)
    {
        if(copy_from_user(cmdbuf, data, len))
        {
            return -EFAULT;
        }
        //parse_write_cmds_from_devcahub(cmdbuf, len);
    }
    else
    {
        printk(KERN_INFO "hhb_write invalid len:%d, the max length is %d bytes\n", len, DEV_CAHUB_MAX_WRITE_SIZE);
    }
*/
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
            break;
        case CHB_IOCTL_CA_STOP:
            break;
        case CHB_IOCTL_CA_ATTR_START:
            break;
        case CHB_IOCTL_CA_ATTR_STOP:
            break;
        case CHB_IOCTL_CA_INTERVAL_SET:
            break;
        default:
            hwlog_err("%s unknown cmd : %d\n", __func__, cmd);
            return -ENOTTY;
    }

    inputhub_route_cmd(ROUTE_CA_PORT,cmd,arg);

    return 0;
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
	hwlog_info("enter %s \n", __func__);
    int ret;

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
