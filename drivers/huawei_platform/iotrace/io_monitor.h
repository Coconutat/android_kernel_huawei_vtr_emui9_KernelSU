/* io_monitor.h
 *
 * Copyright (C) 2014 - 2015 Huawei, Inc.
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
#ifdef CONFIG_HUAWEI_IO_MONITOR

#ifndef IO_MONITOR_H
#define IO_MONITOR_H

#include "huawei_platform/log/imonitor.h"

struct io_module_template;
enum io_monitor_module {
	IO_MONITOR_VFS = 0,
	IO_MONITOR_BLK,
	IO_MONITOR_LOAD,
#ifdef CONFIG_HUAWEI_UFS_HEALTH_INFO
	IO_MONITOR_UFS_HEALTH,
#endif
	IO_MONITOR_CNT,
};

struct io_module_ops {
	int (*log_record)(unsigned char *buff, int len);
	int (*log_output)(unsigned char *buff, int len);
	int (*log_set_param)(struct imonitor_eventobj *obj, unsigned char *buf);
	int (*log_upload)(struct io_module_template *mod, unsigned char *buf);
};

typedef void (*io_timer_fn)(unsigned long arg);

struct io_module_template {
	/*public*/
	unsigned int mod_id;
	unsigned int enable;
	unsigned int event_id;
	unsigned long base_interval; /*ms*/
	struct io_module_ops ops;
	/*private*/
	struct list_head upload_node;
	struct list_head wait_node;
	io_timer_fn fn;
	unsigned long expires;
	long upload_time_sec;
};

extern int io_monitor_mod_register(int mod_id, struct io_module_template *mngt);
extern int vfs_report_to_iotrace(unsigned char *buff, int len);
extern int dev_report_to_iotrace(unsigned char *buff, int len);

#endif

#endif
