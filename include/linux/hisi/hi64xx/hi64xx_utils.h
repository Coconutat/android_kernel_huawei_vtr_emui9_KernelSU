/*
 * hi64xx_util.h -- hisilicon codec controller
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _HI64XX_UTILS_H
#define _HI64XX_UTILS_H

#include <sound/soc.h>
#include <linux/hisi/hi64xx/hi_cdc_ctrl.h>

struct utils_config {
	/* functions to dump codec registers */
	void (*hi64xx_dump_reg)(char *, unsigned int );
};

struct hi64xx_resmgr;

int hi64xx_update_bits(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int mask, unsigned int value);
int hi64xx_utils_init(struct snd_soc_codec*, struct hi_cdc_ctrl*,
	const struct utils_config*, struct hi64xx_resmgr*, unsigned int codec_type);

void hi64xx_dump_debug_info(void);

void hi64xx_utils_deinit(void);

#ifdef CONFIG_HISI_DIEID
int hisi_codec_get_dieid(char *dieid, unsigned int len);
#endif

#endif
