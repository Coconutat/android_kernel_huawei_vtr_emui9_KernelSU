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

#ifndef _BLACKBOX_PLATFORM_ISP_RDR_HISI_ISP_H
#define _BLACKBOX_PLATFORM_ISP_RDR_HISI_ISP_H

/* sync data */
#define RDR_ISP_MAGIC       0x66668888
#define RDR_ISP_SYNC        0x88886666
#define RDR_SYNC_WORD_OFF   0x4

#define ISP_WDT_IRQ     304
#define MODULE_NAME     "RDR ISP"
#define SYSCTL_ADDR     0xfff0a000
#define SC_WDT_OFFSET   0x33c
#define SC_ISP_WDT_BIT  6
#define PCTRL_ADDR      0xe8a09000
#define PCTRL_ISP_FIQ   0x70
#define WDT_ISP_ADDR    0xe8580000
#define WDT_ENABLE_CTL  0x008
#define WDT_LOCK_CTL    0xc00
#define WDT_LOCK        0x1
#define WDT_UNLOCK      0x1acce551

#define SHOW_SIZE       0x1000

struct level_switch_s {
    unsigned int level;
    char enable_cmd[8];
    char disable_cmd[8];
    char info[32];
};

extern u64 get_isprdr_addr(void);

#endif /* _BLACKBOX_PLATFORM_ISP_RDR_HISI_ISP_H */
