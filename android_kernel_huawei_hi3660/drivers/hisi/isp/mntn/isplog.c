/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*lint -e715 -e838 -e529 -e438 -e30 -e142 -e528 -e750 -e753 -e754 -e785 -e655 -e749 -e732 -e747 -e708 -e712 -e64 -e845 -e713 -e40 -e578 -e774
 -esym(715,*) -esym(838,*) -esym(529,*) -esym(438,*) -esym(30,*) -esym(142,*) -esym(528,*) -esym(750,*) -esym(753,*) -esym(754,*) -esym(785,*) -esym(655,*) -esym(749,*) -esym(732,*) -esym(747,*) -esym(708,*) -esym(712,*) -esym(64,*) -esym(845,*) -esym(713,*) -esym(40,*) -esym(578,*) -esym(774,*)*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/rproc_share.h>
#include <linux/remoteproc.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/dma-buf.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include "isprdr.h"
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/version.h>

#define MEM_MAP_MAX_SIZE    (0x40000)
#define MEM_SHARED_SIZE     (0x1000)
#define ISP_IOCTL_MAGIC     (0x70)
#define ISP_IOCTL_MAX_NR    (0x10)
#define POLLING_TIME_NS     (10)
#define POLLING_TIMEOUT_NS  (400)

struct log_user_para {
    unsigned int log_write;
    unsigned int log_head_size;
    unsigned int exc_flag;/*bit 0:exc cur ;bit 1:ion flag ; bit 2:exc rtos save;bit 3:exc handler over*/
    unsigned int boot_flag;/*firmware boot flag */
};

struct coredump_user_para
{
    unsigned int coredump_addr;
/* for sec scene */
    unsigned int type;
    int sharefd;
    int size;
    unsigned long prot;
};


#define LOG_WR_OFFSET       _IOWR(ISP_IOCTL_MAGIC, 0x00, struct log_user_para)
#define LOG_COREDUMP_START       _IOWR(ISP_IOCTL_MAGIC, 0x01, struct coredump_user_para) //save ion address and set ion flag
#define LOG_COREDUMP_WRITE       _IOWR(ISP_IOCTL_MAGIC, 0x02, struct coredump_user_para)  //set all finised flag



struct isplog_device_s {
    struct device *ispdev;
    void __iomem *share_mem;
    struct rproc_shared_para *share_para;
    wait_queue_head_t wait_ctl;
    struct timer_list sync_timer;
    atomic_t open_cnt;
    int use_cacheable_rdr;
    int initialized;
    atomic_t timer_cnt;
    unsigned int local_loglevel;
    unsigned int coredump_addr;
    unsigned int exc_flag;/*bit 0:exc cur ;bit 1:ion flag ; bit 2:exc rtos save;bit 3:exc handler over*/
} isplog_dev;

int sync_isplogcat(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

    if (use_sec_isp())
        dev->share_mem = (void *)getsec_a7sharedmem_addr();
    else if (use_nonsec_isp())
        dev->share_mem = get_a7sharedmem_va();

    if (!dev->share_mem) {
        pr_err("[%s] Failed: share_mem.%pK\n", __func__, dev->share_mem);
        return -ENOMEM;
    }
    dev->share_para = (struct rproc_shared_para *)dev->share_mem;

    if(dev->share_para->log_flush_flag)
        wake_up(&dev->wait_ctl);

    return 0;
}

static void sync_timer_fn(unsigned long data)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_debug("sync_timer_fn enter.%pK\n", dev->share_para);

    if (sync_isplogcat() < 0)
        pr_err("[%s] Failed: sync_isplogcat.%pK\n", __func__, dev->share_para);

	mod_timer(&dev->sync_timer, jiffies + msecs_to_jiffies(POLLING_TIME_NS));
}

void wait_firmware_coredump(void)
{

    struct rproc_shared_para *param = NULL;
    int  timeout = 10;
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);
    hisp_lock_sharedbuf();
    if ((param = rproc_get_share_para()) == NULL) {
        hisp_unlock_sharedbuf();
        return;
    }

    pr_info("[%s] exc_flag.0x%x\n", __func__, param->exc_flag);
    if ((param->exc_flag & CORE_DUMP_EXC_CUR) != CORE_DUMP_EXC_CUR) {
        hisp_unlock_sharedbuf();
        return;
    }

    do {
        pr_err("[%s] exc_flag.0x%x, timeout.%d, wait ......\n", __func__, param->exc_flag, timeout);
        hisp_unlock_sharedbuf();
        mdelay(POLLING_TIMEOUT_NS);

        hisp_lock_sharedbuf();
        if ((param = rproc_get_share_para()) == NULL) {
            hisp_unlock_sharedbuf();
            return;
        }

        pr_info("[%s] exc_flag.0x%x\n", __func__, param->exc_flag);
        if ((param->exc_flag & CORE_DUMP_ALL_FINISH) == CORE_DUMP_ALL_FINISH)
            break;
    } while (timeout -- > 0);
    hisp_unlock_sharedbuf();

    /*coredump interrupted exception*/
    hisp_lock_sharedbuf();
    if ((param = rproc_get_share_para()) == NULL) {
        hisp_unlock_sharedbuf();
        return;
    }

     if ((param->exc_flag & CORE_DUMP_ION) != CORE_DUMP_ION) {
        pr_err("[%s] exc_flag.0x%x\n", __func__, param->exc_flag);
        hisp_unlock_sharedbuf();
        return;
    }

    if ((param->exc_flag & CORE_DUMP_ALL_FINISH) != CORE_DUMP_ALL_FINISH)
    {
        param->exc_flag = CORE_DUMP_EXCEPTION;
        param->log_flush_flag = 1;/*wake up isplogcat free memory*/
        wake_up(&dev->wait_ctl);
        pr_err("[coredump] %s coredump unfinished \n",__func__);
    }
    hisp_unlock_sharedbuf();
    pr_info("[%s] -\n", __func__);
}
/*lint -save -e529 -e438*/
void stop_isplogcat(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);
    if (!dev->initialized) {
        pr_err("[%s] ISP RDR not ready\n", __func__);
        return;
    }

	if (0 == atomic_read(&dev->open_cnt)) {
		pr_err("[%s] Failed : device not ready open_cnt.%d\n", __func__, atomic_read(&dev->open_cnt));
		return;
	}

    if (0 == atomic_read(&dev->timer_cnt)) {
        pr_err("[%s] Failed : timer_cnt.%d...Nothing todo\n", __func__, atomic_read(&dev->timer_cnt));
        return;
    }

    del_timer_sync(&dev->sync_timer);
    atomic_set(&dev->timer_cnt, 0);
    if (sync_isplogcat() < 0)
        pr_err("[%s] Failed: sync_isplogcat\n", __func__);
    pr_info("[%s] -\n", __func__);
}

int start_isplogcat(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);
    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

	if (0 == atomic_read(&dev->open_cnt)) {
		pr_err("[%s] Failed : device not ready open_cnt.%d\n", __func__, atomic_read(&dev->open_cnt));
		return -ENODEV;
	}

    if (0 != atomic_read(&dev->timer_cnt)) {
        pr_err("[%s] Failed : timer_cnt.%d...stop isplogcat\n", __func__, atomic_read(&dev->timer_cnt));
        stop_isplogcat();
    }

    mod_timer(&dev->sync_timer, jiffies + msecs_to_jiffies(POLLING_TIME_NS));
    atomic_set(&dev->timer_cnt, 1);
    pr_info("[%s] -\n", __func__);

    return 0;
}

static int isplog_open(struct inode *inode, struct file *filp)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);

    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

    if (0 != atomic_read(&dev->open_cnt)) {
        pr_err("%s: Failed: has been opened\n", __func__);
        return -EBUSY;
    }

    if (!use_sec_isp() && !use_nonsec_isp()) {
        dev->share_mem = ioremap_nocache(get_a7sharedmem_addr(), MEM_SHARED_SIZE);
        if (!dev->share_mem) {
            pr_err("[%s] Failed: share_mem.%pK\n", __func__, dev->share_mem);
            return -ENOMEM;
        }
        dev->share_para = (struct rproc_shared_para *)dev->share_mem;
        pr_info("[%s] use_sec_isp.%d, share_para.%pK = %pK\n", __func__,
            use_sec_isp(), dev->share_para, dev->share_mem);
    }

	atomic_inc(&dev->open_cnt);
	if (is_ispcpu_powerup())
	    start_isplogcat();
	pr_info("[%s] -\n", __func__);

    return 0;
}
static long isplog_ioctl(struct file *filp, unsigned int cmd,
              unsigned long args)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    struct log_user_para tmp;
    int ret;

    pr_debug("[%s] cmd.0x%x +\n", __func__,cmd);
    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

    if (_IOC_TYPE(cmd) != ISP_IOCTL_MAGIC) {
        pr_err("[%s] type is wrong.\n", __func__);
        return -EINVAL;
    }

    if (_IOC_NR(cmd) >= ISP_IOCTL_MAX_NR) {
        pr_err("[%s] number is wrong.\n", __func__);
        return -EINVAL;
    }


    if (!dev->share_para) {
        if (sync_isplogcat() < 0)
            pr_err("[%s] Failed: sync_isplogcat.%pK\n", __func__, dev->share_para);
        pr_err("[%s] Failed : share_para.%pK\n", __func__, dev->share_para);
        return -EAGAIN;
    }

    if(0 == args)
    {
        pr_err("[%s] cmd[%d] args NULL",__func__,cmd);

        return -EFAULT;
    }

    switch (cmd) {
    case LOG_WR_OFFSET:
        ret = wait_event_timeout(dev->wait_ctl,
                   dev->share_para->log_flush_flag,
			       msecs_to_jiffies(POLLING_TIMEOUT_NS));//lint !e666
        if (0 == ret) {
            pr_debug("[%s] wait timeout, ret = %d\n", __func__, ret);
            return -ETIMEDOUT;
        }
        if(((last_boot_state==0)&& is_ispcpu_powerup())==1)
        {
            tmp.boot_flag = 1;//first boot
        }else{
            tmp.boot_flag = 0;
        }
        last_boot_state = is_ispcpu_powerup();

        if (dev->use_cacheable_rdr)
            dev->share_para->log_flush_flag = 0;
        tmp.log_write = dev->share_para->log_cache_write;
        tmp.log_head_size = dev->share_para->log_head_size;
        /*two case :r8 [0001](rtos requset ion malloc)  [0111](rtos requset ap save data to file)
        bit 0:exc cur ;bit 1:ion flag ; bit 2:rtos dump over  bit3:handle over bit5:coredump too busy to handle
        CORE_DUMP_EXC_CUR
        (CORE_DUMP_EXC_CUR|CORE_DUMP_ION|CORE_DUMP_RTOS_FINISH)
        CORE_DUMP_EXCEPTION
        */
        tmp.exc_flag = dev->share_para->exc_flag;
        pr_debug("[%s] write = %u, size = %d.\n", __func__,
             tmp.log_write, tmp.log_head_size);

        if (copy_to_user((void __user *)args, &tmp, sizeof(tmp))) {
            pr_err("[%s] copy_to_user failed.\n", __func__);
            return -EFAULT;
        }
        break;
    default:
        pr_err("[%s] don't support cmd.\n", __func__);
        break;
    };

    pr_debug("[%s] -\n", __func__);

    return 0;
}

static int isplog_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    u64 isprdr_addr;
    unsigned long size;
    int ret;

    pr_info("[%s] +\n", __func__);
    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

    if ((isprdr_addr = get_isprdr_addr()) == 0) {
        pr_err("[%s] Failed : isprdr_addr.0\n", __func__);
        return -ENOMEM;
    }

    if (vma->vm_start == 0) {
        pr_err("[%s] Failed : vm_start.0x%lx\n", __func__, vma->vm_start);
        return -EINVAL;
    }

    size = vma->vm_end - vma->vm_start;
    if (MEM_MAP_MAX_SIZE < size) {
        pr_err("%s: size.0x%lx.\n", __func__, size);
        return -EINVAL;
    }
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start,
                        (isprdr_addr >> PAGE_SHIFT),
                        size, vma->vm_page_prot);
    if (ret) {
        pr_err("[%s] remap_pfn_range failed, ret.%d\n", __func__, ret);
        return ret;
    }

    pr_info("[%s] -\n", __func__);

    return 0;
}

static int isplog_release(struct inode *inode, struct file *filp)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);
    if (!dev->initialized) {
        pr_err("[%s] Failed : ISP RDR not ready\n", __func__);
        return -ENXIO;
    }

    if (0 >= atomic_read(&dev->open_cnt)) {
        pr_err("%s: Failed: has been closed\n", __func__);
        return -EBUSY;
    }
	stop_isplogcat();
	dev->share_para = NULL;
	if (!use_sec_isp() && !use_nonsec_isp())
		iounmap(dev->share_mem);
	atomic_dec(&dev->open_cnt);
	pr_info("[%s] -\n", __func__);

    return 0;
}

struct level_switch_s loglevel[] = {
    {
    ISPCPU_LOG_USE_APCTRL, "yes", "no", "LOG Controlled by AP"}, {
    ISPCPU_LOG_TIMESTAMP_FPGAMOD, "yes", "no",
            "LOG Timestamp syscounter Mode"}, {
    ISPCPU_LOG_FORCE_UART, "enable", "disable", "uart"}, {
    ISPCPU_LOG_LEVEL_WATCHDOG, "enable", "disable", "watchdog"}, {
    ISPCPU_LOG_RESERVE_27, "enable", "disable", "reserved 27"}, {
    ISPCPU_LOG_RESERVE_26, "enable", "disable", "reserved 26"}, {
    ISPCPU_LOG_RESERVE_25, "enable", "disable", "reserved 25"}, {
    ISPCPU_LOG_RESERVE_24, "enable", "disable", "reserved 24"}, {
    ISPCPU_LOG_RESERVE_23, "enable", "disable", "reserved 23"}, {
    ISPCPU_LOG_RESERVE_22, "enable", "disable", "reserved 22"}, {
    ISPCPU_LOG_RESERVE_21, "enable", "disable", "reserved 21"}, {
    ISPCPU_LOG_RESERVE_20, "enable", "disable", "reserved 20"}, {
    ISPCPU_LOG_RESERVE_19, "enable", "disable", "reserved 19"}, {
    ISPCPU_LOG_RESERVE_18, "enable", "disable", "reserved 18"}, {
    ISPCPU_LOG_RESERVE_17, "enable", "disable", "reserved 17"}, {
    ISPCPU_LOG_RESERVE_16, "enable", "disable", "reserved 16"}, {
    ISPCPU_LOG_RESERVE_15, "enable", "disable", "reserved 15"}, {
    ISPCPU_LOG_RESERVE_14, "enable", "disable", "reserved 14"}, {
    ISPCPU_LOG_RESERVE_13, "enable", "disable", "reserved 13"}, {
    ISPCPU_LOG_RESERVE_12, "enable", "disable", "reserved 12"}, {
    ISPCPU_LOG_RESERVE_11, "enable", "disable", "reserved 11"}, {
    ISPCPU_LOG_RESERVE_10, "enable", "disable", "reserved 10"}, {
    ISPCPU_LOG_RESERVE_09, "enable", "disable", "reserved 9"}, {
    ISPCPU_LOG_RESERVE_08, "enable", "disable", "reserved 8"}, {
    ISPCPU_LOG_LEVEL_DEBUG_ALGO, "enable", "disable", "algodebug"}, {
    ISPCPU_LOG_LEVEL_ERR_ALGO, "enable", "disable", "algoerr"}, {
    ISPCPU_LOG_LEVEL_TRACE, "enable", "disable", "trace"}, {
    ISPCPU_LOG_LEVEL_DUMP, "enable", "disable", "dump"}, {
    ISPCPU_LOG_LEVEL_DBG, "enable", "disable", "dbg"}, {
    ISPCPU_LOG_LEVEL_INFO, "enable", "disable", "info"}, {
    ISPCPU_LOG_LEVEL_WARN, "enable", "disable", "warn"}, {
    ISPCPU_LOG_LEVEL_ERR, "enable", "disable", "err"},};

void isploglevel_update(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    struct rproc_shared_para *param = NULL;

    hisp_lock_sharedbuf();
    if ((param = rproc_get_share_para()) == NULL) {
        hisp_unlock_sharedbuf();
        return;
    }

    param->logx_switch = (param->logx_switch & ~ISPCPU_LOG_LEVEL_MASK) |
                        (dev->local_loglevel & ISPCPU_LOG_LEVEL_MASK);
    pr_info("[%s] 0x%x = 0x%x\n", __func__, param->logx_switch,
        dev->local_loglevel);
    hisp_unlock_sharedbuf();
}

static ssize_t isplogctrl_show(struct device *pdev,
                   struct device_attribute *attr, char *buf)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    struct rproc_shared_para *param = NULL;
    unsigned int logx_switch = 0;
    char *s = buf;
    ssize_t size;
    int i;

    if (buf == NULL) {
        pr_err("[%s] Failed : buf.%pK\n", __func__, buf);
        return 0;
    }

    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if (param != NULL)
        logx_switch = param->logx_switch;

    for (i = 0; i < (int)((int)sizeof(loglevel) / (int)sizeof(struct level_switch_s)); i ++) {
        // cppcheck-suppress *
        s += sprintf(s, "[%s.%s] : %s\n",
                 (param ? ((logx_switch & loglevel[i].level)
                 ? loglevel[i].enable_cmd : loglevel[i].disable_cmd) : "ispoffline"),
                 ((dev->local_loglevel & loglevel[i].
                  level) ? loglevel[i].enable_cmd : loglevel[i].disable_cmd),
                 loglevel[i].info);/*lint !e421 */
    }
    hisp_unlock_sharedbuf();
    size = s - buf;
    return size;
}

static void usage_isplogctrl(void)
{
    int i = 0;

    pr_info("<Usage: >\n");
    for (i = 0; i < (int)((int)sizeof(loglevel) / (int)sizeof(struct level_switch_s)); i ++) {
        if (loglevel[i].level == (unsigned int)ISPCPU_LOG_USE_APCTRL
            || loglevel[i].level == (unsigned int)ISPCPU_LOG_TIMESTAMP_FPGAMOD)
            continue;
        pr_info("echo <%s>:<%s/%s> > isplogctrl\n", loglevel[i].info,
               loglevel[i].enable_cmd, loglevel[i].disable_cmd);
    }
}

static ssize_t isplogctrl_store(struct device *pdev,
                struct device_attribute *attr, const char *buf,
                size_t count)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    int i = 0, len = 0, flag = 0;
    char *p = NULL;

    if ((buf == NULL) || (count == 0)) {
       pr_err("[%s] Failed : buf.%pK, count.0x%lx\n", __func__, buf, count);
       return 0;
    }

    p = memchr(buf, ':', count);
    if (!p)
        return (ssize_t)count;

    len = (int)(p - buf);
    for (i = 0; i < (int)((int)sizeof(loglevel) / (int)sizeof(struct level_switch_s)); i ++) {
        if (loglevel[i].level == (unsigned int)ISPCPU_LOG_USE_APCTRL
            || loglevel[i].level == (unsigned int)ISPCPU_LOG_TIMESTAMP_FPGAMOD)
            continue;
        if (!strncmp(buf, loglevel[i].info, len)) {
            flag = 1;
            p += 1;
            if (!strncmp(p, loglevel[i].enable_cmd,
                (int)strlen(loglevel[i].enable_cmd)))
                dev->local_loglevel |= loglevel[i].level;
            else if (!strncmp(p, loglevel[i].disable_cmd,
                  (int)strlen(loglevel[i].disable_cmd)))
                dev->local_loglevel &= ~(loglevel[i].level);
            else
                flag = 0;
            break;
        }
    }

    if (!flag)
        usage_isplogctrl();

    isploglevel_update();

    return (ssize_t)count;
}

/*lint -e846 -e514 -e778 -e866 -e84*/
static DEVICE_ATTR(isplogctrl, (S_IRUGO | S_IWUSR | S_IWGRP), isplogctrl_show,
           isplogctrl_store);
/*lint +e846 +e514 +e778 +e866 +e84*/
static const struct file_operations isplog_ops = {
    .open           = isplog_open,
    .release        = isplog_release,
    .unlocked_ioctl = isplog_ioctl,
    .compat_ioctl   = isplog_ioctl,
    .mmap           = isplog_mmap,
    .owner          = THIS_MODULE,
};

static struct miscdevice isplog_miscdev = {
    .minor  = 255,
    .name   = "isp_log",
    .fops   = &isplog_ops,
};

static int __init isplog_init(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;
    int ret = 0;

    pr_info("[%s] +\n", __func__);

    dev->initialized = 0;
    init_waitqueue_head(&dev->wait_ctl);

    if ((ret = misc_register((struct miscdevice *)&isplog_miscdev)) != 0) {
        pr_err("[%s] Failed : misc_register.%d\n", __func__, ret);
        return ret;
    }

    ret = device_create_file(isplog_miscdev.this_device, &dev_attr_isplogctrl);
    if (0 != ret)
        pr_err("[%s] Faield : isplog device_create_file.%d\n", __func__, ret);

    dev->local_loglevel = ISPCPU_DEFAULT_LOG_LEVEL;
    atomic_set(&dev->open_cnt, 0);
	atomic_set(&dev->timer_cnt, 0);
	setup_timer(&dev->sync_timer, sync_timer_fn, 0);
    dev->use_cacheable_rdr = 1;
    dev->ispdev = isplog_miscdev.this_device;
    dev->initialized = 1;
    pr_info("[%s] -\n", __func__);

    return 0;
}

static void __exit isplog_exit(void)
{
    struct isplog_device_s *dev = (struct isplog_device_s *)&isplog_dev;

    pr_info("[%s] +\n", __func__);
    misc_deregister((struct miscdevice *)&isplog_miscdev);
    dev->initialized = 0;
    pr_info("[%s] -\n", __func__);
}

module_init(isplog_init);
module_exit(isplog_exit);
MODULE_LICENSE("GPL v2");
