/*
 * Hisilicon DDR TEST driver (master only).
 *
 * Copyright (c) 2012-2013 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

#include "hisi_ddr_secprotect.h"
#include "../ap/platform/hi3660/global_ddr_map.h"
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/hisi_ddr.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/hisi_drmdriver.h>

#define DMR_SHARE_MEM_PHY_BASE (HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE + DRM_SHARE_MEM_OFFSET)

u64 *g_dmss_intr_fiq;
struct semaphore modemddrc_happen_sem;

void dmss_ipi_handler(void)
{
	pr_crit("dmss_ipi_inform start\n");
	up(&modemddrc_happen_sem);
}

int modemddrc_happen(void *arg)
{
	printk(KERN_ERR "modemddrc_happen start\n");
	down(&modemddrc_happen_sem);
	printk(KERN_ERR "modemddrc_happen goto rdr_system_error\n");
	rdr_system_error(RDR_MODEM_DMSS_MOD_ID, 0, 0);
	return 0;
}

void fiq_print_src(u64 intrsrc)
{
	unsigned int intr = intrsrc & 0xFF;

	/* check with FIQ number */
	if (IRQ_WDT_INTR_FIQ == intr) {
		printk(KERN_ERR "fiq triggered by: Watchdog\n");
	} else if (IRQ_DMSS_INTR_FIQ == intr) {
		smp_send_stop();
		printk(KERN_ERR "fiq triggered by: DMSS\n");
	} else {
		printk(KERN_ERR "fiq triggered by: Unknown, intr=0x%x\n", (unsigned int)intrsrc);
	}

}

void dmss_fiq_handler(void)
{
	if (NULL == g_dmss_intr_fiq) {
		printk(KERN_ERR "fiq_handler intr ptr is null.\n");
		g_dmss_intr_fiq = ioremap_nocache(HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE, 8);
		if (NULL == g_dmss_intr_fiq) {
			printk(KERN_ERR "fiq_handler ioremap_nocache fail\n");
			return;
		}
	}
	fiq_print_src(*g_dmss_intr_fiq);

	if ((NULL != g_dmss_intr_fiq )&&((u64)(DMSS_INTR_FIQ_FLAG|IRQ_DMSS_INTR_FIQ) == (*g_dmss_intr_fiq))) {
		printk(KERN_ERR "dmss_fiq_handler\n");
		printk(KERN_ERR "dmss intr happened. please see bl31 log.\n");

		/*rdr reboot  because of DDR_SEC*/
		rdr_syserr_process_for_ap(MODID_AP_S_DDRC_SEC, 0ULL, 0ULL);
	}
}

int hisi_sec_ddr_set(DRM_SEC_CFG *sec_cfg, DYNAMIC_DDR_SEC_TYPE type)
{
	int ret;
	DRM_SEC_CFG *p_sec_cfg = NULL;
	p_sec_cfg = (DRM_SEC_CFG*)ioremap_nocache(DMR_SHARE_MEM_PHY_BASE, sizeof(DRM_SEC_CFG));
	if (NULL == p_sec_cfg || NULL == sec_cfg) {
		return -1;
	}
	p_sec_cfg->start_addr = sec_cfg->start_addr;
	p_sec_cfg->sub_rgn_size = sec_cfg->sub_rgn_size;
	p_sec_cfg->bit_map = sec_cfg->bit_map;
	p_sec_cfg->sec_port = sec_cfg->sec_port;

	ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
		(u64)DMR_SHARE_MEM_PHY_BASE, (u64)type, ACCESS_REGISTER_FN_SUB_ID_DDR_DRM_SET);
	iounmap(p_sec_cfg);
	return ret;
}

int hisi_sec_ddr_clr(DYNAMIC_DDR_SEC_TYPE type)
{
	return atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
		(u64)DMR_SHARE_MEM_PHY_BASE, (u64)type, ACCESS_REGISTER_FN_SUB_ID_DDR_DRM_CLR);
}

static int hisi_ddr_secprotect_probe(struct platform_device *pdev)
{
	g_dmss_intr_fiq = ioremap_nocache(HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE, 8);
	if (NULL == g_dmss_intr_fiq) {
		printk(" ddr ioremap_nocache fail\n");
		return -ENOMEM;
	}

	sema_init(&modemddrc_happen_sem, 0);
	if (!kthread_run(modemddrc_happen, NULL, "modemddrc_emit"))
		pr_err("create thread modemddrc_happen faild.\n");

	return 0;
}

static int hisi_ddr_secprotect_remove(struct platform_device *pdev)
{
	if (g_dmss_intr_fiq)
	{
		iounmap(g_dmss_intr_fiq);
	}
	g_dmss_intr_fiq = NULL;
	return 0;
}

static const struct of_device_id hs_ddr_of_match[] = {
	{ .compatible = "hisilicon,ddr_secprotect", },
	{},
};
MODULE_DEVICE_TABLE(of, hs_ddr_of_match);

static struct platform_driver hisi_ddr_secprotect_driver = {
	.probe		= hisi_ddr_secprotect_probe,
	.remove		= hisi_ddr_secprotect_remove,
	.driver		= {
		.name	= "ddr_secprotect",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(hs_ddr_of_match),
	},
};
module_platform_driver(hisi_ddr_secprotect_driver);

MODULE_DESCRIPTION("hisi ddr secprotect driver");
MODULE_ALIAS("hisi ddr_secprotect module");
MODULE_LICENSE("GPL");
