/*
 * hi64xx_hifi_om.h -- om module for hi64xx
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HI64XX_HIFI_OM_H__
#define __HI64XX_HIFI_OM_H__

#include <linux/hisi/hi64xx/asp_dma.h>
#include <linux/hisi/hi64xx/hi64xx_irq.h>

extern int hi64xx_hifi_om_init(struct hi64xx_irq *irqmgr, unsigned int codec_type);
extern void hi64xx_hifi_om_deinit(void);
extern int hi64xx_hifi_om_hook_start(void);
extern void hi64xx_hifi_om_hook_stop(void);
extern int hi64xx_hifi_om_set_hook_path(char *path, unsigned int size);
extern int hi64xx_hifi_om_set_bw(unsigned short bandwidth);
extern int hi64xx_hifi_om_set_sponsor(unsigned short sponsor);
extern void hi64xx_hifi_dump_to_file(char *buf, unsigned int size, char *path);
extern int hi64xx_hifi_create_hook_dir(const char *path);
extern int hi64xx_hifi_om_set_dir_count(unsigned int count);

#endif
