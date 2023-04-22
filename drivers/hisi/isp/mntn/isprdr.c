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
/*lint -e715 -e838 -e529 -e438 -e30 -e142 -e528 -e750 -e753 -e754 -e785 -e655 -e749 -e732 -e747 -e708 -e712 -e64 -e661 -e574 -e737 -e713 -e826 -e530 -e570
 -esym(715,*) -esym(838,*) -esym(529,*) -esym(438,*) -esym(30,*) -esym(142,*) -esym(528,*) -esym(750,*) -esym(753,*) -esym(754,*) -esym(785,*) -esym(655,*) -esym(749,*) -esym(732,*) -esym(747,*) -esym(708,*) -esym(712,*) -esym(64,*) -esym(661,*) -esym(574,*) -esym(737,*) -esym(713,*) -esym(826,*) -esym(530,*) -esym(570,*)*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/rproc_share.h>
#include <linux/version.h>
#include <linux/of_irq.h>
#include <linux/platform_data/remoteproc-hisi.h>

#include "isprdr.h"
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"

/* Watchdog Regs Offset */
#define ISP_WatchDog_WDG_LOCK_ADDR      (0x0C00)
#define ISP_WatchDog_WDG_CONTROL_ADDR   (0x0008)

/* SCtrl Regs Offset */
#define SCTRL_SCBAKDATA10_ADDR      (0x434)

/* PCtrl Regs Offset */
#define PCTRL_PERI_CTRL27_ADDR      (0x070)

enum RDR_ISP_SYSTEM_ERROR_TYPE {
    ISP_MODID_START = HISI_BB_MOD_ISP_START,
    ISP_WDT_TIMEOUT = 0x81fffdfe,
    ISP_SYSTEM_STATES,
    ISP_MODID_END = HISI_BB_MOD_ISP_END,
    ISP_SYSTEM_INVALID,
} rdr_isp_sys_err_t;

struct rdr_err_type {
    struct list_head node;
    enum RDR_ISP_SYSTEM_ERROR_TYPE type;
};

struct rdr_sys_err_dump_info {
    struct list_head node;
    u32 modid;
    u64 coreid;
    pfn_cb_dump_done cb;
};

struct rdr_isp_device {
    void __iomem *sctrl_addr;
    void __iomem *wdt_addr;
    void __iomem *pctrl_addr;
    void __iomem *rdr_addr;
    struct workqueue_struct *wq;
    struct work_struct err_work;
    struct work_struct dump_work;
    struct list_head err_list;
    struct list_head dump_list;
    spinlock_t err_list_lock;
    spinlock_t dump_list_lock;
    int wdt_irq;
    bool wdt_enable_flag;
    unsigned int offline_rdrlevel;
    unsigned char irq[IRQ_NUM];
    int isprdr_initialized;
    u64 isprdr_addr;
    u64 core_id;
    struct rdr_register_module_result current_info;
} rdr_isp_dev;

static struct rdr_exception_info_s exc_info[] = {
    [0] = {
           .e_modid = ISP_WDT_TIMEOUT,
           .e_modid_end = ISP_WDT_TIMEOUT,
           .e_process_priority = RDR_ERR,
           .e_reboot_priority = RDR_REBOOT_NO,
           .e_notify_core_mask = RDR_AP | RDR_ISP,
           .e_reset_core_mask = RDR_AP,
           .e_reentrant = RDR_REENTRANT_DISALLOW,
           .e_exce_type = ISP_S_ISPWD,
           .e_from_core = RDR_ISP,
           .e_from_module = MODULE_NAME,
           .e_desc = "RDR ISP WDT TIMEOUT",
           },
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
extern char *hisp_get_comp_name(void);
int hisp_get_wdt_irq(void)
{
    struct device_node *np = NULL;
    char *name = NULL;
    int irq = 0;

    name = hisp_get_comp_name();

    np = of_find_compatible_node(NULL, NULL, name);
    if (!np) {
        pr_err("%s: of_find_compatible_node failed, %s\n", __func__, name);
        return -ENXIO;
    }

    irq = irq_of_parse_and_map(np, 0);
    if (!irq) {
        pr_err("%s: irq_of_parse_and_map failed, irq.%d\n", __func__, irq);
        return -ENXIO;
    }

    pr_info("%s: comp.%s, wdt irq.%d.\n", __func__, name, irq);
    return irq;
}
#endif

u64 get_isprdr_addr(void)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;

    return dev->isprdr_addr;
}

static void rdr_system_err_dump_work(struct work_struct *work)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    struct rdr_sys_err_dump_info *entry, *tmp;
    unsigned int sync_word;
    int timeout = 20;

    list_for_each_entry_safe(entry, tmp, &dev->dump_list, node) {
        if (ISP_WDT_TIMEOUT == entry->modid) {
            /* check sync word */
            do {
                sync_word =
                    readl(dev->rdr_addr + RDR_SYNC_WORD_OFF);
                msleep(100);
            } while (RDR_ISP_SYNC != sync_word && timeout-- > 0);
            pr_info("%s: sync_word = 0x%x, timeout = %d.\n",
                __func__, sync_word, timeout);
        }
        entry->cb(entry->modid, entry->coreid);

        spin_lock(&dev->dump_list_lock);
        list_del(&entry->node);
        spin_unlock(&dev->dump_list_lock);

        kfree(entry);
    }
}
/*lint -save -e429*/
static void rdr_isp_dump(u32 modid, u32 etype, u64 coreid,
             char *pathname, pfn_cb_dump_done pfn_cb)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    struct rdr_sys_err_dump_info *dump_info;
    pr_info("%s: enter.\n", __func__);

    dump_info = kzalloc(sizeof(struct rdr_sys_err_dump_info), GFP_KERNEL);
    if (!dump_info) {
        pr_err("%s: kzalloc failed.\n", __func__);
        return;
    }

    dump_info->modid = modid;
    dump_info->coreid = dev->core_id;
    dump_info->cb = pfn_cb;

    spin_lock(&dev->dump_list_lock);
    list_add_tail(&dump_info->node, &dev->dump_list);
    spin_unlock(&dev->dump_list_lock);

    queue_work(dev->wq, &dev->dump_work);
    pr_info("%s: exit.\n", __func__);
    return;
}
/*lint -restore */
static void rdr_isp_reset(u32 modid, u32 etype, u64 coreid)
{
    pr_info("%s: enter.\n", __func__);
    return;
}
/*lint -save -e429*/
void ap_send_fiq2ispcpu(void)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    u32 exc_flag = 0x0;
    u32 value = 0x0;
    bool wdt_flag = false;
    exc_flag = get_share_exc_flag();
    if(exc_flag == 0xFFFFFFFF){
        pr_err("%s: get_share_exc_flag failed, exc_flag.0x%d\n", __func__, exc_flag);
        return;
    }
    pr_info("%s: exc_flag.0x%x\n", __func__, exc_flag);
    if (wait_share_excflag_timeout(ISP_CPU_POWER_DOWN,10) == 0x0) {
        if (dev->sctrl_addr == NULL){
            pr_err("%s: sctrl_addr NULL\n", __func__);
            return;
        }
        value = readl((volatile void __iomem *)(dev->sctrl_addr + SCTRL_SCBAKDATA10_ADDR));
        if (value & (1 << SC_ISP_WDT_BIT))
            wdt_flag = true;

        if ((wdt_flag) && (dev->sctrl_addr != NULL)) {
            /* send fiq to isp_a7 */
            pr_info("%s: send fiq to ispcpu!\n", __func__);
            if (dev->pctrl_addr == NULL){
                pr_err("%s: pctrl_addr NULL\n", __func__);
                return;
            }
            writel(0, (volatile void __iomem *)(dev->pctrl_addr + PCTRL_PERI_CTRL27_ADDR));
            writel(1, (volatile void __iomem *)(dev->pctrl_addr + PCTRL_PERI_CTRL27_ADDR));
            writel(0, (volatile void __iomem *)(dev->pctrl_addr + PCTRL_PERI_CTRL27_ADDR));
        }
        mdelay(50);
    }
}

static irqreturn_t isp_wdt_irq_handler(int irq, void *data)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    struct rdr_err_type *err_info;
    int ret = 0;
    pr_info("%s:enter.\n", __func__);

    /* disable wdt */
    if (dev->wdt_addr == NULL){
        pr_err("%s: wdt_addr NULL\n", __func__);
        return IRQ_NONE;
    }
    writel(WDT_UNLOCK, (volatile void __iomem *)(dev->wdt_addr + ISP_WatchDog_WDG_LOCK_ADDR));
    writel(0, (volatile void __iomem *)(dev->wdt_addr + ISP_WatchDog_WDG_CONTROL_ADDR));
    writel(WDT_LOCK, (volatile void __iomem *)(dev->wdt_addr + ISP_WatchDog_WDG_LOCK_ADDR));

    /* init sync work */
    writel(0, dev->rdr_addr + RDR_SYNC_WORD_OFF);

    if ((ret = get_ispcpu_cfg_info()) < 0)
        pr_err("%s: get_ispcpu_cfg_info failed, irq.0x%d\n", __func__, irq);

    err_info = kzalloc(sizeof(struct rdr_err_type), GFP_ATOMIC);
    if (!err_info) {
        pr_info("%s: kzalloc failed.\n", __func__);
        return IRQ_NONE;
    }

    err_info->type = ISP_WDT_TIMEOUT;

    spin_lock(&dev->err_list_lock);
    list_add_tail(&err_info->node, &dev->err_list);
    spin_unlock(&dev->err_list_lock);

    queue_work(dev->wq, &dev->err_work);
    pr_info("%s:exit.\n", __func__);
    return IRQ_HANDLED;
}
/*lint -restore */
static void rdr_system_err_work(struct work_struct *work)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    struct rdr_err_type *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &dev->err_list, node) {
        if (ISP_WDT_TIMEOUT == entry->type)
            rdr_system_error(entry->type, dev->wdt_irq, 0);
        else
            rdr_system_error(entry->type, 0, 0);

        spin_lock_irq(&dev->err_list_lock);
        list_del(&entry->node);
        spin_unlock_irq(&dev->err_list_lock);

        kfree(entry);
    }
}

static int rdr_isp_wdt_init(struct rdr_isp_device *dev)
{
    int ret = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
       int irq = 0;
#endif
    pr_info("%s: enter.\n", __func__);
    if (!dev->wdt_enable_flag) {
        pr_info("%s: isp wdt is disabled.\n", __func__);
        return 0;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
    irq = hisp_get_wdt_irq();
    if (irq <= 0) {
        pr_err("%s: hisp_get_wdt_irq failed, irq.0x%d\n", __func__, irq);
        return -EINVAL;
    }
    dev->wdt_irq = irq;
#else
    dev->wdt_irq = ISP_WDT_IRQ;
#endif

    ret =
        request_irq(dev->wdt_irq, isp_wdt_irq_handler, 0, "isp wtd hanler",
            NULL);
    if (0 != ret)
        pr_err("%s: request_irq failed, irq.%d, ret.%d.\n", __func__, dev->wdt_irq, ret);

    pr_info("%s: exit.\n", __func__);
    return 0;
}

static int rdr_isp_module_register(void)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    struct rdr_module_ops_pub module_ops;
    struct rdr_register_module_result ret_info;
    int ret = 0;

    pr_info("%s: enter.\n", __func__);
    module_ops.ops_dump = rdr_isp_dump;
    module_ops.ops_reset = rdr_isp_reset;

    dev->core_id = RDR_ISP;
    ret = rdr_register_module_ops(dev->core_id, &module_ops, &ret_info);
    if (ret != 0) {
        pr_err("%s: rdr_register_module_ops failed! return %d\n",
               __func__, ret);
        return ret;
    }

    dev->current_info.log_addr = ret_info.log_addr;
    dev->current_info.log_len = ret_info.log_len;
    dev->current_info.nve = ret_info.nve;
    dev->isprdr_addr = ret_info.log_addr;

    dev->rdr_addr = hisi_bbox_map((phys_addr_t)dev->isprdr_addr,
                                    dev->current_info.log_len);
    if (!dev->rdr_addr) {
        pr_err("%s: hisi_bbox_map rdr_addr failed.\n", __func__);
        return -ENOMEM;
    }

    dev->isprdr_initialized = 1;
    pr_info("%s: exit.\n", __func__);
    return 0;
}

static int rdr_isp_exception_register(struct rdr_isp_device *dev)
{
    int i, ret;

    pr_info("%s: enter.\n", __func__);
    for (i = 0; i < sizeof(exc_info) / sizeof(struct rdr_exception_info_s);
         i++) {
        pr_info("%s: register rdr exception, i = %d, type:%d", __func__,
            i, exc_info[i].e_exce_type);

        if (exc_info[i].e_modid == ISP_WDT_TIMEOUT)
            if (!dev->wdt_enable_flag)
                continue;

        ret = rdr_register_exception(&exc_info[i]);
        if (ret != exc_info[i].e_modid_end) {
            pr_info("%s: rdr_register_exception failed, ret.%d.\n",
                __func__, ret);
            return -EINVAL;
        }
    }

    pr_info("%s: exit.\n", __func__);
    return 0;
}

static int rdr_isp_dev_map(struct rdr_isp_device *dev)
{
    unsigned int value;
    bool wdt_flag = false;

    pr_info("%s: enter.\n", __func__);

    if (dev == NULL){
        pr_err("%s: rdr_isp_device is NULL.\n", __func__);
        return -EINVAL;
    }
    dev->wdt_enable_flag = wdt_flag;

    dev->sctrl_addr = get_regaddr_by_pa(SCTRL);
    if (!dev->sctrl_addr) {
        pr_err("%s: ioremp sctrl failed.\n", __func__);
        return -ENOMEM;
    }

    value = readl((volatile void __iomem *)(dev->sctrl_addr + SCTRL_SCBAKDATA10_ADDR));

    if (value & (1 << SC_ISP_WDT_BIT))
        wdt_flag = true;

    if (wdt_flag) {
        dev->wdt_addr = get_regaddr_by_pa(WDT);
        if (!dev->wdt_addr) {
            pr_err("%s: ioremp wdt failed.\n", __func__);
            goto err;
        }

        dev->pctrl_addr = get_regaddr_by_pa(PCTRL);
        if (!dev->pctrl_addr) {
            pr_err("%s: ioremp pctrl failed.\n", __func__);
            goto err;
        }
    }

    dev->wdt_enable_flag = wdt_flag;
    pr_info("%s: exit.\n", __func__);
    return 0;

err:
    pr_info("%s: error, exit.\n", __func__);
    return -ENOMEM;
}

int rdr_isp_init(void)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;
    int ret = 0;
    int i;

    pr_info("[%s] +\n", __func__);
    ret = rdr_isp_dev_map(dev);
    if (0 != ret) {
        pr_err("%s: rdr_isp_dev_map failed.\n", __func__);
        return ret;
    }

    ret = rdr_isp_wdt_init(dev);
    if (0 != ret) {
        pr_err("%s: rdr_isp_wdt_init failed.\n", __func__);
        return ret;
    }

    ret = rdr_isp_module_register();
    if (0 != ret) {
        pr_err("%s: rdr_isp_module_register failed.\n", __func__);
        return ret;
    }

    ret = rdr_isp_exception_register(dev);
    if (0 != ret) {
        pr_err("%s: rdr_isp_exception_register failed.\n", __func__);
        return ret;
    }

    dev->wq = create_singlethread_workqueue(MODULE_NAME);
    if (!dev->wq) {
        pr_err("%s: create_singlethread_workqueue failed.\n", __func__);
        return -1;
    }

    INIT_WORK(&dev->dump_work, rdr_system_err_dump_work);
    INIT_WORK(&dev->err_work, rdr_system_err_work);
    INIT_LIST_HEAD(&dev->err_list);
    INIT_LIST_HEAD(&dev->dump_list);

    spin_lock_init(&dev->err_list_lock);
    spin_lock_init(&dev->dump_list_lock);
    dev->offline_rdrlevel = ISPCPU_DEFAULT_RDR_LEVEL;
    for(i=0;i<IRQ_NUM;i++)
        dev->irq[i]=0;

    pr_info("[%s] -\n", __func__);

    return ret;
}

void rdr_isp_exit(void)
{
    struct rdr_isp_device *dev = &rdr_isp_dev;

    destroy_workqueue(dev->wq);
    return;
}

MODULE_LICENSE("GPL v2");

