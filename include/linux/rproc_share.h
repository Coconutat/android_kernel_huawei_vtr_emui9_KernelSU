/*
 * Remote Processor - Histar ISP remoteproc platform data.
 *
 * Copyright (c) 2013-2014 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _INCLUDE_LINUX_RPROC_SHARE_H
#define _INCLUDE_LINUX_RPROC_SHARE_H

#include <linux/platform_device.h>
#include <linux/platform_data/remoteproc-hisi.h>

enum rdr_enable_mask
{
    RDR_LOG_TRACE       = 0x1,
    RDR_LOG_LAST_WORD   = 0x2,
    RDR_LOG_ALGO        = 0x4,
    RDR_LOG_CVDR        = 0x8,
    RDR_LOG_IRQ         = 0x10,
    RDR_LOG_TASK        = 0x20,
    RDR_LOG_CPUP        = 0x40,
    RDR_LOG_MONITOR     = 0x80,
};
#define ISPCPU_RDR_USE_APCTRL        (1UL << 31)
#define ISPCPU_RDR_RESERVE_30        (1 << 30)
#define ISPCPU_RDR_RESERVE_29        (1 << 29)
#define ISPCPU_RDR_RESERVE_28        (1 << 28)
#define ISPCPU_RDR_RESERVE_27        (1 << 27)
#define ISPCPU_RDR_RESERVE_26        (1 << 26)
#define ISPCPU_RDR_RESERVE_25        (1 << 25)
#define ISPCPU_RDR_RESERVE_24        (1 << 24)
#define ISPCPU_RDR_RESERVE_23        (1 << 23)
#define ISPCPU_RDR_RESERVE_22        (1 << 22)
#define ISPCPU_RDR_RESERVE_21        (1 << 21)
#define ISPCPU_RDR_RESERVE_20        (1 << 20)
#define ISPCPU_RDR_RESERVE_19        (1 << 19)
#define ISPCPU_RDR_RESERVE_18        (1 << 18)
#define ISPCPU_RDR_RESERVE_17        (1 << 17)
#define ISPCPU_RDR_RESERVE_16        (1 << 16)
#define ISPCPU_RDR_RESERVE_15        (1 << 15)
#define ISPCPU_RDR_RESERVE_14        (1 << 14)
#define ISPCPU_RDR_RESERVE_13        (1 << 13)
#define ISPCPU_RDR_RESERVE_12        (1 << 12)
#define ISPCPU_RDR_RESERVE_11        (1 << 11)
#define ISPCPU_RDR_RESERVE_10               (1 << 10)
#define ISPCPU_RDR_RESERVE_9                 (1 << 9)
#define ISPCPU_RDR_RESERVE_8                 (1 << 8)
#define ISPCPU_RDR_RESERVE_7                 (1 << 7)
#define ISPCPU_RDR_LEVEL_CPUP                 (1 << 6)
#define ISPCPU_RDR_LEVEL_TASK                (1 << 5)
#define ISPCPU_RDR_LEVEL_IRQ                  (1 << 4)
#define ISPCPU_RDR_LEVEL_CVDR                (1 << 3)
#define ISPCPU_RDR_LEVEL_ALGO                (1 << 2)
#define ISPCPU_RDR_LEVEL_LAST_WORD     (1 << 1)
#define ISPCPU_RDR_LEVEL_TRACE              (1 << 0)
#define ISPCPU_RDR_LEVEL_MASK                 (0x7FFFFFFF)
#define RDR_CHOOSE_TYPE                         (RDR_LOG_TRACE | RDR_LOG_LAST_WORD)
#define ISPCPU_DEFAULT_RDR_LEVEL             RDR_CHOOSE_TYPE

#define ISPCPU_LOG_USE_APCTRL        (1UL << 31)
#define ISPCPU_LOG_TIMESTAMP_FPGAMOD (1 << 30)
#define ISPCPU_LOG_FORCE_UART        (1 << 29)
#define ISPCPU_LOG_LEVEL_WATCHDOG    (1 << 28)
#define ISPCPU_LOG_RESERVE_27        (1 << 27)
#define ISPCPU_LOG_RESERVE_26        (1 << 26)
#define ISPCPU_LOG_RESERVE_25        (1 << 25)
#define ISPCPU_LOG_RESERVE_24        (1 << 24)
#define ISPCPU_LOG_RESERVE_23        (1 << 23)
#define ISPCPU_LOG_RESERVE_22        (1 << 22)
#define ISPCPU_LOG_RESERVE_21        (1 << 21)
#define ISPCPU_LOG_RESERVE_20        (1 << 20)
#define ISPCPU_LOG_RESERVE_19        (1 << 19)
#define ISPCPU_LOG_RESERVE_18        (1 << 18)
#define ISPCPU_LOG_RESERVE_17        (1 << 17)
#define ISPCPU_LOG_RESERVE_16        (1 << 16)
#define ISPCPU_LOG_RESERVE_15        (1 << 15)
#define ISPCPU_LOG_RESERVE_14        (1 << 14)
#define ISPCPU_LOG_RESERVE_13        (1 << 13)
#define ISPCPU_LOG_RESERVE_12        (1 << 12)
#define ISPCPU_LOG_RESERVE_11        (1 << 11)
#define ISPCPU_LOG_RESERVE_10        (1 << 10)
#define ISPCPU_LOG_RESERVE_09        (1 << 9)
#define ISPCPU_LOG_RESERVE_08        (1 << 8)
#define ISPCPU_LOG_LEVEL_DEBUG_ALGO  (1 << 7)
#define ISPCPU_LOG_LEVEL_ERR_ALGO    (1 << 6)
#define ISPCPU_LOG_LEVEL_TRACE       (1 << 5)
#define ISPCPU_LOG_LEVEL_DUMP        (1 << 4)
#define ISPCPU_LOG_LEVEL_DBG         (1 << 3)
#define ISPCPU_LOG_LEVEL_INFO        (1 << 2)
#define ISPCPU_LOG_LEVEL_WARN        (1 << 1)
#define ISPCPU_LOG_LEVEL_ERR         (1 << 0)
#define ISPCPU_LOG_LEVEL_MASK        (0x1FFFFFFF)
#define ISPCPU_DEFAULT_LOG_LEVEL     (0x0)

#define ISPCPU_PERF_RECORD          (1UL << 31)
#define ISPCPU_PERF_RESERVE_30      (1 << 30)
#define ISPCPU_PERF_RESERVE_29      (1 << 29)
#define ISPCPU_PERF_YUVNF           (1 << 28)
#define ISPCPU_PERF_WARP            (1 << 27)
#define ISPCPU_PERF_TNR             (1 << 26)
#define ISPCPU_PERF_STAT3A          (1 << 25)
#define ISPCPU_PERF_SCALER          (1 << 24)
#define ISPCPU_PERF_RGB2YUV         (1 << 23)
#define ISPCPU_PERF_RAWNF           (1 << 22)
#define ISPCPU_PERF_PDAF            (1 << 21)
#define ISPCPU_PERF_OIS             (1 << 20)
#define ISPCPU_PERF_MINILSC         (1 << 19)
#define ISPCPU_PERF_LUT3D           (1 << 18)
#define ISPCPU_PERF_LSC             (1 << 17)
#define ISPCPU_PERF_LASER           (1 << 16)
#define ISPCPU_PERF_IF              (1 << 15)
#define ISPCPU_PERF_GCD             (1 << 14)
#define ISPCPU_PERF_GAMMA           (1 << 13)
#define ISPCPU_PERF_FLASH           (1 << 12)
#define ISPCPU_PERF_FD              (1 << 11)
#define ISPCPU_PERF_DRC             (1 << 10)
#define ISPCPU_PERF_DPCC            (1 << 9)
#define ISPCPU_PERF_DIS             (1 << 8)
#define ISPCPU_PERF_DEGAMMA         (1 << 7)
#define ISPCPU_PERF_CE              (1 << 6)
#define ISPCPU_PERF_CC              (1 << 5)
#define ISPCPU_PERF_BLC             (1 << 4)
#define ISPCPU_PERF_BAS             (1 << 3)
#define ISPCPU_PERF_AWB             (1 << 2)
#define ISPCPU_PERF_AF              (1 << 1)
#define ISPCPU_PERF_AE              (1 << 0)
#define ISPCPU_PERF_MASK            (0xFFFFFFFF)
#define ISPCPU_PERF_DEFAULT         (0)

#define IRQ_NUM             (64)

#define MONITOR_CHAN_NUM    4
/*
 * struct rproc_shared_para - shared parameters for debug
 * @rdr_enable: the rdr function status
 */
struct isp_plat_cfg {
    unsigned int platform_id;
    unsigned int isp_local_timer;
    unsigned int perf_power;
};

struct bw_boot_status {
    unsigned int entry:1;
    unsigned int invalid_tlb:1;
    unsigned int enable_mmu:1;
    unsigned int reserved:29;
};

struct fw_boot_status {
    unsigned int entry:1;
    unsigned int hard_boot_init:1;
    unsigned int hard_drv_init:1;
    unsigned int app_init:1;
    unsigned int reserved:28;
};

struct rproc_shared_para {
    struct isp_plat_cfg plat_cfg;
    int rdr_enable;
    unsigned int rdr_enable_type;
    unsigned char irq[IRQ_NUM];
    int log_flush_flag;
    unsigned int log_head_size;
    unsigned int log_cache_write;
    struct bw_boot_status bw_stat;
    struct fw_boot_status fw_stat;
    unsigned int logx_switch;
    u64 bootware_paddr;
    u64 dynamic_pgtable_base;
    unsigned long long  coresight_addr_da;
    unsigned long long  coresight_addr_vir;
    unsigned int perf_switch;
    u64 perf_addr;
    u32 coredump_addr;
    u32 exc_flag;/*bit 0:exc cur ;bit 1:ion flag ; bit 2:rtos dump over  bit3:handle over*/
    u64 mdc_pa_addr;
    u64 sec_smmuerr_addr;
    unsigned int monitor_addr[MONITOR_CHAN_NUM];
    unsigned int monitor_ctrl;
    unsigned int monitor_pa_va;
    unsigned int monitor_hit_flag;
    unsigned int clk_value[ISP_CLK_MAX];
    unsigned char isp_efuse;
};

extern struct rproc_shared_para *isp_share_para;
extern struct rproc_shared_para *rproc_get_share_para(void);
extern int rproc_set_shared_para(void);
extern void rproc_set_shared_clk_value(int type,unsigned int value);
extern void hisp_lock_sharedbuf(void);
extern void hisp_unlock_sharedbuf(void);

#endif /* _INCLUDE_LINUX_RPROC_SHARE_H */
