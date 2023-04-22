/*
 * hisi_log.h
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __HISI_LOG_H__
#define __HISI_LOG_H__

#undef  pr_fmt
#define pr_fmt(fmt) "["HISI_LOG_TAG"]:" fmt


#define MNTN_DUMP_TAG "mntn_dump"
#define HISI_PMIC_TAG "pmic"
#define HISI_PMIC_MNTN_TAG "pmic_mntn"
#define HISI_PMIC_REGULATOR_TAG "pmic_regulator"
#define HISI_PMIC_REGULATOR_DEBUG_TAG "pmic_regulator_debug"
#define HISI_SPMI_TAG "spmi"
#define HISI_SPMI_DBGFS_TAG "spmi_dbgfs"
#define HISI_DMA_TAG "hisi_dma"
#define HISI_DMA64_TAG "hisi_dma64"
#define HISI_AMBA_PL011_TAG "hisi_amba_pl011"
#define HISI_LED_TAG "hisi_led"
#define HISI_POWERKEY_TAG "hisi_powerkey"
#define MNTN_BBOX_DIAGINFO  "bbox_diaginfo"
#define HISI_NOC_TRACE_TAG "noc_trace"
#define HISI_STM_TRACE_TAG "stm_trace"
#define HISI_AXI_TAG "hisi_axi"
#define MEMORY_DUMP_TAG "memory_dump"
#define HISI_BLACKBOX_TAG "blackbox"
#define HISI_BOOTTIME_TAG "boottime"
#define HISI_CODE_PROTECT_TAG "code_protect"
#define HISI_DUMP_TAG "dump"
#define HISI_FASTBOOTLOG_TAG "fastbootlog"
#define HISI_BOOTUP_KEYPOINT_TAG "hisi_bootup_keypoint"
#define HISI_FIQ_TAG "hisi_fiq"
#define HISI_POWEROFF_TAG "hisi_poweroff"
#define HISI_FILESYS_TAG "hisi_filesys"
#define MNTN_13CACHE_ECC_TAG "mntn_l3cache_ecc"
#define HISI_UTIL_TAG "util"
#define VIRT_TO_PHYS_TAG "virt_to_phys"
#define HISI_NOC_TAG "hisi_noc"
#define HISI_SP805_WDT_TAG "hisi_sp805_wdt"
#define HISI_MNTN_TEST_TAG "hisi_mntn_test"
#define HISI_EASY_SHELL_TAG "hisi_easy_shell"
#define MNTN_BC_PANIC_TAG "mntn_bc_panic"
#define MNTN_RECORD_SP_TAG "mntn_record_sp"
#define HISI_EAGLE_EYE_TAG "eagle_eye"
#define HISI_GPIO_TAG "gpio"
#define HISI_PINCTRL_TAG "pinctrl"
#define HISI_HWSPINLOCK_TAG "hwspinlock"
#define HISI_HWSPINLOCK_DEBUGFS_TAG "hwspinlock_debugfs"

#define AP_MAILBOX_TAG "ap_ipc"


#endif /* end of hisi_log.h */
