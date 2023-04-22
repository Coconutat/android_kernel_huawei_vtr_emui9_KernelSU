/*
 *  Copyright (C) 2017 Hisilicon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/of.h>

#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>


#define KHz		(1000)
#define LOCAL_BUF_MAX		(128)



MODULE_LICENSE("GPL");
