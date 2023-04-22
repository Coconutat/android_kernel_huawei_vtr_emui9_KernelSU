/*
 * hisi_bootup_keypoint
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <linux/of.h>

#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/mfd/hisi_pmic.h>
#include <mntn_public_interface.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BOOTUP_KEYPOINT_TAG
#include "blackbox/rdr_print.h"

static u32 g_last_bootup_keypoint;
static u64 g_bootup_keypoint_addr;
static struct semaphore clear_dfx_sem;
static u32 fpga_flag;

/*******************************************************************************
Function:       set_boot_keypoint
Description:    set bootup_keypoint, record last_bootup_keypoint,
Input:          value:the value that need to set
Output:         NA
Return:         NA
********************************************************************************/
void set_boot_keypoint(u32 value)
{
	if (value < STAGE_KERNEL_BOOTANIM_COMPLETE || value > STAGE_END) {
		BB_PRINT_ERR("value[%d] is invilad.\n", value);
		return;
	}
	if (STAGE_BOOTUP_END == value) {
		up(&clear_dfx_sem);
	}
	if (fpga_flag == FPGA) {
		if (!g_bootup_keypoint_addr) {
			return;
		}
		writel(value, (void *)g_bootup_keypoint_addr);
	} else {
		hisi_pmic_reg_write(BOOTUP_KEYPOINT_OFFSET, value);/*lint !e747*/
	}
}

/*******************************************************************************
Function:       get_boot_keypoint
Description:    get bootup_keypoint
Input:          NA
Output:         NA
Return:         the current bootup_keypoint
********************************************************************************/
u32 get_boot_keypoint(void)
{
	u32 value = 0;

	if (fpga_flag == FPGA) {
		if (!g_bootup_keypoint_addr) {
			return 0;
		}
		value = readl((void *)g_bootup_keypoint_addr);
	} else {
		value = hisi_pmic_reg_read(BOOTUP_KEYPOINT_OFFSET);/*lint !e747*/
	}
	return value;
}

/*******************************************************************************
Function:       get_last_boot_keypoint
Description:    get bootup_keypoint
Input:          NA
Output:         NA
Return:         the last bootup_keypoint
********************************************************************************/
u32 get_last_boot_keypoint(void)
{
	return g_last_bootup_keypoint;
}

/*******************************************************************************
Function:       get_last_boot_stage
Description:    get last bootup_keypoint from cmdline
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
static int __init early_last_bootup_keypoint_cmdline(char *last_bootup_keypoint_cmdline)
{
	g_last_bootup_keypoint = atoi(last_bootup_keypoint_cmdline);
	pr_debug("g_last_bootup_keypoint is [%d]\n", g_last_bootup_keypoint);
	return 0;
}

early_param("last_bootup_keypoint", early_last_bootup_keypoint_cmdline);

/*******************************************************************************
Function:       clear_dfx_happen
Description:    clear dfx tempbuffer
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int clear_dfx_happen(void *arg)
{
	pr_debug("clear_dfx_happen start\n");
	down(&clear_dfx_sem);
	clear_dfx_tempbuffer();

	return 0;
}

/*******************************************************************************
Function:       hisi_bootup_keypoint_init
Description:    sema_init clear_dfx_sem and kthread_run clear_dfx task
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init hisi_bootup_keypoint_init(void)
{
	sema_init(&clear_dfx_sem, 0);
	if (!kthread_run(clear_dfx_happen, NULL, "clear_dfx_happen")) {
		BB_PRINT_ERR("create thread clear_dfx_happen faild.\n");
	}

	return 0;
}
module_init(hisi_bootup_keypoint_init)

/*******************************************************************************
Function:       bootup_keypoint_addr_init
Description:    init bootup_keypoint_addr
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
static void bootup_keypoint_addr_init(void)
{
	int ret;
	struct device_node *np;
	u64 bootup_keypoint_addr;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hisifb");
	if (!np) {
		BB_PRINT_ERR("NOT FOUND device node 'hisilicon,hisifb'!\n");
		return;
	}
	ret = of_property_read_u32(np, "fpga_flag", &fpga_flag);
	if (ret) {
		BB_PRINT_ERR("failed to get fpga_flag resource.\n");
		return;
	}
	if (fpga_flag == FPGA) {
		bootup_keypoint_addr = FPGA_BOOTUP_KEYPOINT_ADDR;
		g_bootup_keypoint_addr = (unsigned long long)
			ioremap_wc(bootup_keypoint_addr, sizeof(int));
		if (!g_bootup_keypoint_addr) {
			BB_PRINT_ERR(KERN_ERR "get bootup_keypoint_addr error\n");
			return;
		}
		pr_debug("bootup_keypoint_addr is %llx\n", g_bootup_keypoint_addr);
	}

	return;
}

/*******************************************************************************
Function:       early_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init early_stage_init(void)
{
	bootup_keypoint_addr_init();
	set_boot_keypoint(STAGE_KERNEL_EARLY_INITCALL);
	return 0;
}
early_initcall(early_stage_init);

/*******************************************************************************
Function:       pure_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init pure_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_PURE_INITCALL);
	return 0;
}
pure_initcall(pure_stage_init);

/*******************************************************************************
Function:       core_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init core_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_CORE_INITCALL);
	return 0;
}
core_initcall(core_stage_init);

/*******************************************************************************
Function:       core_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init core_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_CORE_INITCALL_SYNC);
	return 0;
}
core_initcall_sync(core_sync_stage_init);

/*******************************************************************************
Function:       postcore_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init postcore_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_POSTCORE_INITCALL);
	return 0;
}
postcore_initcall(postcore_stage_init);

/*******************************************************************************
Function:       postcore_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init postcore_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_POSTCORE_INITCALL_SYNC);
	return 0;
}
postcore_initcall_sync(postcore_sync_stage_init);

/*******************************************************************************
Function:       arch_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init arch_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_ARCH_INITCALL);
	return 0;
}
arch_initcall(arch_stage_init);

/*******************************************************************************
Function:       arch_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init arch_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_ARCH_INITCALLC);
	return 0;
}
arch_initcall_sync(arch_sync_stage_init);

/*******************************************************************************
Function:       subsys_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init subsys_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_SUBSYS_INITCALL);
	return 0;
}
subsys_initcall(subsys_stage_init);

/*******************************************************************************
Function:       subsys_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init subsys_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_SUBSYS_INITCALL_SYNC);
	return 0;
}
subsys_initcall_sync(subsys_sync_stage_init);

/*******************************************************************************
Function:       fs_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init fs_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_FS_INITCALL);
	return 0;
}
fs_initcall(fs_stage_init);

/*******************************************************************************
Function:       fs_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init fs_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_FS_INITCALL_SYNC);
	return 0;
}
fs_initcall_sync(fs_sync_stage_init);

/*******************************************************************************
Function:       rootfs_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init rootfs_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_ROOTFS_INITCALL);
	return 0;
}
rootfs_initcall(rootfs_stage_init);

/*******************************************************************************
Function:       device_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init device_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_DEVICE_INITCALL);
	return 0;
}
device_initcall(device_stage_init);

/*******************************************************************************
Function:       device_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init device_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_DEVICE_INITCALL_SYNC);
	return 0;
}
device_initcall_sync(device_sync_stage_init);

/*******************************************************************************
Function:       late_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init late_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_LATE_INITCALL);
	return 0;
}
late_initcall(late_stage_init);

/*******************************************************************************
Function:       late_sync_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init late_sync_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_LATE_INITCALL_SYNC);
	return 0;
}
late_initcall_sync(late_sync_stage_init);

/*******************************************************************************
Function:       console_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init console_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_CONSOLE_INITCALL);
	return 0;
}
console_initcall(console_stage_init);

/*******************************************************************************
Function:       security_stage_init
Description:    set bootup_keypoint
Input:          NA
Output:         NA
Return:         OK:success
********************************************************************************/
static int __init security_stage_init(void)
{
	set_boot_keypoint(STAGE_KERNEL_SECURITY_INITCALL);
	return 0;
}
security_initcall(security_stage_init);
