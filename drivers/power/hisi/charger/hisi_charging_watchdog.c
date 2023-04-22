/*
 * watchdog for preventing the charging work dead
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kern_levels.h>
#include <huawei_platform/log/hw_log.h>
#include "soc_rtctimerwdtv100_interface.h"
#include "hisi_charging_watchdog.h"
#include "securec.h"
#ifdef CONFIG_HISI_BB
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#endif
#define CHGWDT_LOG_INFO
#ifndef CHGWDT_LOG_INFO
#define chgwdt_debug(fmt, args...)do {} while (0)
#define chgwdt_info(fmt, args...) do {} while (0)
#define chgwdt_warn(fmt, args...) do {} while (0)
#define chgwdt_err(fmt, args...)  do {} while (0)
#else
#define chgwdt_debug(fmt, args...)do { printk(KERN_DEBUG   "[hisi_chgwdt]" fmt, ## args); } while (0)
#define chgwdt_info(fmt, args...) do { printk(KERN_INFO    "[hisi_chgwdt]" fmt, ## args); } while (0)
#define chgwdt_warn(fmt, args...) do { printk(KERN_WARNING"[hisi_chgwdt]" fmt, ## args); } while (0)
#define chgwdt_err(fmt, args...)  do { printk(KERN_ERR   "[hisi_chgwdt]" fmt, ## args); } while (0)
#endif
static struct hisi_chgwdg_device *g_di;
static unsigned char g_watchdog_enable;
static unsigned int watchdog_kick_cpu;
static u64  watchdog_kick_time;
static inline void __chg_wdt_lock(void __iomem *base, unsigned int lock_key)
{
	__raw_writel(lock_key, CHG_WATHDOG_WDLOCK_ADDR((char *)base));
}

static inline void __chg_wdt_unlock(void __iomem *base, unsigned int value)
{
	__raw_writel(value, CHG_WATHDOG_WDLOCK_ADDR((char *)base));
}

static inline unsigned int __chg_wdt_unlock_check(void __iomem *base)
{
	return __raw_readl(CHG_WATHDOG_WDLOCK_ADDR((char *)base));
}

static inline void __chg_wdt_load(void __iomem *base, unsigned int timeout)
{
	__raw_writel(WATCHDOG_CLOCK_COUNT*timeout, CHG_WATHDOG_WDLOAD_ADDR((char *)base));
}

static inline unsigned int __chg_wdt_get_cnt(void __iomem *base)
{
	return __raw_readl(CHG_WATHDOG_WDVALUE_ADDR((char *)base));
}

static inline void __chg_wdt_intclr(void __iomem *base, unsigned int value)
{
	__raw_writel(value, CHG_WATHDOG_WDINTCLR_ADDR((char *)base));
}

static inline void __chg_wdt_control(void __iomem *base, unsigned int value)
{
	__raw_writel(value, CHG_WATHDOG_WDCTRL_ADDR((char *)base));
}

/*******************************************************************************
Function:       rdr_charge_syswdt_init
Description:    init charger sys watchdog rdr
Input:          NA
Output:         NA
Return:         success:0;fail:-1
********************************************************************************/
#ifdef CONFIG_HISI_BB
static int rdr_charge_syswdt_init(void)
{
	struct rdr_exception_info_s einfo;
	unsigned int ret;
	memset_s(&einfo, sizeof(struct rdr_exception_info_s), 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = MODID_CHARGER_S_WDT;
	einfo.e_modid_end = MODID_CHARGER_S_WDT;
	/*处理级别最高. */
	einfo.e_process_priority = RDR_ERR;
	/* 立即重启 */
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	/* 通知AP保存日志.待修改 */
	einfo.e_notify_core_mask = RDR_AP;
	/* 通知AP重置状态.并且通知ap复位全系统 */
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_from_core = RDR_AP;
	/* 不允许本异常重入(多次发生不重复处理). */
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	/* 异常类型初始化失败 */
	einfo.e_exce_type = CHARGER_S_WDT;
	einfo.e_upload_flag = (u32)RDR_UPLOAD_YES;
	memcpy_s(einfo.e_from_module, sizeof("RDR_CHG_SYSWDT"), "RDR_CHG_SYSWDT", sizeof("RDR_CHG_SYSWDT"));
	memcpy_s(einfo.e_desc, sizeof("RDR_CHG_SYSWDT"), "RDR_CHG_SYSWDT", sizeof("RDR_CHG_SYSWDT"));

	ret = rdr_register_exception(&einfo);
	if (ret != MODID_CHARGER_S_WDT) {
		chgwdt_err(" register rdr_charge_syswdt failed.\n");
		return -1;
	}
	return 0;
}
#endif

/**********************************************************
*  Function:       charge_watchdog_interrupt
*  Description:    callback function for chargerIC fault irq in charging process
*  Parameters:   irq:chargerIC fault interrupt
*                      _di:hi6523_device_info
*  return value:  IRQ_HANDLED-success or others
**********************************************************/
static irqreturn_t charger_watchdog_interrupt(int irq, void *_di)
{
#ifdef CONFIG_HISI_BB
    u64 a_wdt_kick_time = rdr_get_last_wdt_kick_slice();

	if (NULL == _di || 0 == irq ) {
		chgwdt_err("[charger_watchdog_interrupt]:di is null.\n");
	}
	chgwdt_err("charge wdt timeout,last kick cpu[%d],time:%llu ns, a_wdt_kick_time:%llu s\n", watchdog_kick_cpu, watchdog_kick_time, (a_wdt_kick_time/32768));
	rdr_syserr_process_for_ap((u32)MODID_CHARGER_S_WDT, (u64)0, (u64)0);
#endif
	return IRQ_HANDLED;
}

/*******************************************************************************
Function:       charge_feed_sys_wdt
Description:    set timeout for charge watchdog
Input:          timeout:watchdog timeout
Output:         NA
Return:         NA
********************************************************************************/
void charge_feed_sys_wdt(unsigned int timeout)
{
	unsigned int cnt;

	if (NULL == g_di || NULL == g_di->base) {
		chgwdt_err("%s:NULL POINTER\n", __func__);
		return;
	}
	if (0 == g_watchdog_enable) {
		return;
	}
	__chg_wdt_unlock(g_di->base, UNLOCK);
	if (0 != __chg_wdt_unlock_check(g_di->base)) {
		chgwdt_err("wdt unlock fail\n");
	}
	cnt = __chg_wdt_get_cnt(g_di->base);
	__chg_wdt_load(g_di->base, timeout);
	__chg_wdt_lock(g_di->base, LOCK);
	watchdog_kick_cpu = get_cpu();
	put_cpu();
	watchdog_kick_time = hisi_getcurtime();
	chgwdt_info("++charge watchdog feed, current cnt:%d, timeout is:%d ++\n",cnt/WATCHDOG_CLOCK_COUNT, timeout);
}
EXPORT_SYMBOL(charge_feed_sys_wdt);
/*******************************************************************************
Function:       charge_enable_sys_wdt
Description:    enable charge watchdog
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
void charge_enable_sys_wdt(void)
{
	if (NULL == g_di || NULL == g_di->base) {
		chgwdt_err("%s:NULL POINTER\n", __func__);
		return;
	}
	if (0 == g_watchdog_enable) {
		return;
	}
	__chg_wdt_unlock(g_di->base, UNLOCK);
	if (0 != __chg_wdt_unlock_check(g_di->base)) {
		chgwdt_err("wdt unlock fail\n");
	}
	__chg_wdt_load(g_di->base, CHG_WATCHDOG_TIME);
	__chg_wdt_intclr(g_di->base, 0x0);
	__chg_wdt_control(g_di->base, CHG_WATCHDOG_EN);
	__chg_wdt_lock(g_di->base, LOCK);
	chgwdt_info("++charge watchdog start++\n");
}
EXPORT_SYMBOL(charge_enable_sys_wdt);
/*******************************************************************************
Function:       chargr_stop_sys_wdt
Description:    disable charge watchdog
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
void charge_stop_sys_wdt(void)
{
	if (NULL == g_di || NULL == g_di->base) {
		chgwdt_err("%s:NULL POINTER\n", __func__);
		return;
	}
	__chg_wdt_unlock(g_di->base, UNLOCK);
	if (0 != __chg_wdt_unlock_check(g_di->base)) {
		chgwdt_err("wdt unlock fail\n");
	}
	__chg_wdt_intclr(g_di->base, 0x0);
	__chg_wdt_control(g_di->base, CHG_WATCHDOG_DIS);
	__chg_wdt_lock(g_di->base, LOCK);
	chgwdt_info("++charge watchdog stop++\n");
}
EXPORT_SYMBOL(charge_stop_sys_wdt);

static int hisi_chg_wdg_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int ret = 0;

	g_di = kzalloc(sizeof(*g_di), GFP_KERNEL);
	if (!g_di) {
		chgwdt_err("no mem for chg watchdog resouce\n");
		ret = -ENOMEM;
		goto out;
	}
	g_di->base = of_iomap(node, 0);
	if (!g_di->base) {
		chgwdt_err("iomap error\n");
		ret = -ENOMEM;
		goto free_dev;
	}

	g_di->irq = irq_of_parse_and_map(node, 0);
	if (0 == g_di->irq) {
		chgwdt_err("charge wdt irq is %d\n", g_di->irq);
		goto free_iomap;
	}
	charge_stop_sys_wdt();
	ret = request_irq(g_di->irq, charger_watchdog_interrupt,
			(unsigned long)IRQF_NO_SUSPEND, "charge_syswdt_irq", g_di);
	if (ret) {
		chgwdt_err("register charge wdt irq failed!\n");
		goto free_iomap;
	}
#ifdef CONFIG_HISI_BB
    if (!strstr(saved_command_line, "androidboot.swtype=factory") && check_himntn(HIMNTN_GLOBAL_WDT))
            g_watchdog_enable = 1;
    else
            g_watchdog_enable = 0;
	ret = rdr_charge_syswdt_init();
	if (ret) {
		chgwdt_err("init charge wdt rdr failed!\n");
		goto free_iomap;
	}
#else
    disable_irq_nosync((unsigned int)g_di->irq);
#endif
	platform_set_drvdata(pdev, g_di);
	chgwdt_info("Hisi charge watchdog ready\n");
	return 0;
free_iomap:
	iounmap(g_di->base);
free_dev:
	kfree(g_di);
out:
	return ret;
}

static int hisi_chg_wdg_remove(struct platform_device *pdev)
{
	struct hisi_chgwdg_device *di = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	if (di) {
		iounmap(di->base);
		free_irq(di->irq, pdev);
		kfree(di);
	}
	return 0;
}

static const struct of_device_id hisi_chgwdg_of_match[] = {
	{
	.compatible = "hisilicon,chgwdg",
	.data = NULL
	},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_chgwdg_of_match);

static struct platform_driver hisi_chgwdg_driver = {
	.probe = hisi_chg_wdg_probe,
	.remove = hisi_chg_wdg_remove,
	.driver = {
			   .name = "Chg-watchdog",
			   .owner = THIS_MODULE,
			   .of_match_table = of_match_ptr(hisi_chgwdg_of_match),
		},
};

static int __init hisi_chgwdg_init(void)
{
	platform_driver_register(&hisi_chgwdg_driver);
	return 0;
}

module_init(hisi_chgwdg_init);

static void __exit hisi_chgwdg_exit(void)
{
	platform_driver_unregister(&hisi_chgwdg_driver);
}

module_exit(hisi_chgwdg_exit);

MODULE_DESCRIPTION("CHG watchdog driver");
MODULE_LICENSE("GPL V2");
