/*
 * arch/arm/mach-k3v2/include/mach/util.h
 *
 * balong platform misc utilities function
 *
 * Copyright (C) 2012 Hisilicon, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_UTIL_H__
#define __MACH_UTIL_H__

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include "mntn_public_interface.h"

#define HIMNTN_VALID_SIZE   (64)

extern int check_himntn(int feature);
extern u32 atoi(char *s);
extern int himntn_set(char *himntn_temp);
#endif

