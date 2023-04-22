/*
 *  linux/drivers/mmc/core/cache_enable.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kern_levels.h>
#include <stddef.h>
#include <linux/mmc/card.h>
#include <linux/kernel.h>

#define CID_MANFID_SAMSUNG             0x15
extern int is_hisi_battery_exist(void);
#ifdef CONFIG_HISI_CMDLINE_PARSE
extern unsigned int runmode_is_factory(void);
#endif
int mmc_screen_test_cache_enable(struct mmc_card *card)
{
	int need_cache_on = 1;
	int runmode_normal = 1;
	int batterystate_exist = 0;
	if(card == NULL) {
	   printk(KERN_INFO "card is null and then return\n");
	   return need_cache_on;
	}

#ifdef CONFIG_HISI_CMDLINE_PARSE
	runmode_normal = !runmode_is_factory();
#endif

#ifdef CONFIG_HISI_COUL
	batterystate_exist = is_hisi_battery_exist();
#ifdef CONFIG_HLTHERM_RUNTEST
	batterystate_exist = 1;
	printk(KERN_INFO "CONFIG_HLTHERM_RUNTEST is enable, set the batterystate_exist to 1\n");
#endif
#else
	batterystate_exist = 0;
#endif
	printk(KERN_INFO "runmode_normal=%d batterystate_exist=%d card->cid.manfid=%d\n", runmode_normal, batterystate_exist, card->cid.manfid);
	if(runmode_normal ||  ( batterystate_exist && (card->cid.manfid != CID_MANFID_SAMSUNG) ) )
	{
		printk(KERN_INFO "need_cache_on\n");
		need_cache_on = 1;
	}
	else
	{
		printk(KERN_INFO "need_cache_off\n");
		need_cache_on = 0;
	}
	return need_cache_on;
}
