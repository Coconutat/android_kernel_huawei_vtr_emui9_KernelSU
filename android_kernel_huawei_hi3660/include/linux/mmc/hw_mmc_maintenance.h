/*
 * hw_mmc_maintenance.h
 *
 * add maintenance function for mmc
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Authors:
 * Wangtao <wangtao126@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef HW_MMC_MAINTENANCE_H
#define HW_MMC_MAINTENANCE_H

#define MMC_CMD_BUF_SIZE      (0x20000)

extern void mmc_cmd_sequence_rec_init(void);
extern void mmc_cmd_sequence_rec_exit(void);

#ifdef CONFIG_HW_MMC_MAINTENANCE_DATA
#define MMC_BOOTMEM_SIZE      (0x6400000)

extern char *mmc_bootmem;
extern void mmc_bootmem_init(char *mmc_bootmem);
#endif

#endif
