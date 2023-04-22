/*
 * Copyright (c) 2017-2018 HiSilicon Technologies Co., Ltd.
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Author:       HiSilicon Kirin Storage DEV
 * Created Time: Sat 25 Feb 2017 11:12:58 AM CST
 * File Name:    pagecache_manage.h
 *
 */

#ifndef _LINUX_PAGECACHE_MANAGE_H
#define _LINUX_PAGECACHE_MANAGE_H

#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/version.h>

#define PCH_DEV_NAME "pagecache_helper"

#define USRDATA_IS_MOUNTED 	(0x1 << 0)
#define USRDATA_IS_UMOUNTING 	(0x1 << 1)
#define USRDATA_IS_UMOUNTED 	(0x1 << 2)
#define SYSTEM_IS_MOUNTED 		(0x1 << 4)
#define SYSTEM_IS_UMOUNTING 	(0x1 << 5)
#define SYSTEM_IS_UMOUNTED 	(0x1 << 6)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
#define MAX_RA_PAGES   ((512*4096)/PAGE_CACHE_SIZE)
#else
#define MAX_RA_PAGES   ((512*4096)/PAGE_SIZE)
#endif

#define MAX_MMAP_LOTSAMISS  (100)

#define USER_SPACE_IS_LOW 		1
#define USER_SPACE_IS_HIGH 		0

#define MEM_NORMAL_WMARK 		0
#define MEM_HIGH_WMARK 		1
#define MEM_LOW_WMARK 		2

// the number of the page
#define STORAGE_IS_1G 		(1024*256)

#define TIMER_WORK_IS_RUNING 		(1)

#define PCH_IOCTL_CMD_Z 	(0)
#define PCH_IOCTL_CMD_O 	(1)

#ifdef CONFIG_HISI_PAGECACHE_HELPER
void mount_fs_register_pch(struct vfsmount *mnt);
void umounting_fs_register_pch(struct super_block *sb);
void umounted_fs_register_pch(struct super_block *sb);
#else
static inline void mount_fs_register_pch(struct vfsmount *mnt) {}
static inline void umounting_fs_register_pch(struct super_block *sb) {}
static inline void umounted_fs_register_pch(struct super_block *sb) {}
#endif

#endif
