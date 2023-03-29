/*
 *  linux/drivers/mmc/core/mmc-kirin-lib.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/hisi/kirin_partition.h>

static struct mmc_card *mmc_card_tmp = NULL;

int mmc_set_boot_partition_type(int boot_partition_type)
{
	int ret;
	unsigned char ext_csd_boot_partition_value;

	emmc_boot_partition_type = (enum AB_PARTITION_TYPE)boot_partition_type;
	ext_csd_boot_partition_value = mmc_card_tmp->ext_csd.partition_config & 0xc7;
	ext_csd_boot_partition_value = (ext_csd_boot_partition_value | (boot_partition_type << 3));

	mmc_card_tmp->ext_csd.partition_config = ext_csd_boot_partition_value;

	pr_info("%s: card_ext_csd value = 0x%x\n", __func__, mmc_card_tmp->ext_csd.partition_config);

	mmc_get_card(mmc_card_tmp);

	ret = mmc_switch(mmc_card_tmp, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_PART_CONFIG,
			 ext_csd_boot_partition_value,
			 0);

	mmc_put_card(mmc_card_tmp);
	return ret;
}

int mmc_get_boot_partition_type(struct mmc_card *card, u8 *ext_csd)
{
	/*decode boot partition type : XLOADER_A or XLOADER_B*/
	card->ext_csd.partition_config = ext_csd[EXT_CSD_PART_CONFIG];
	emmc_boot_partition_type = (enum AB_PARTITION_TYPE)((card->ext_csd.partition_config & 0x38) >> 3);
	mmc_card_tmp = card;

	pr_info("%s: emmc boot partition type is %s\n",
			 __func__, (emmc_boot_partition_type == 0x1) ? "XLOADER_A" : "XLOADER_B");

	return 0;
}

