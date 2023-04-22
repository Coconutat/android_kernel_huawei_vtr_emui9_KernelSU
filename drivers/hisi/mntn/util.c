
/*
 * arch/arm/mach-hi6620/util.c
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
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/hisi/util.h>
#include <linux/uaccess.h>	/* For copy_to_user */
#include <linux/pstore_ram.h>
#include <linux/delay.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_UTIL_TAG
#include "blackbox/rdr_print.h"

static char himntn[HIMNTN_VALID_SIZE + 1] = { '\0' };

/*******************************************************************************
函 数 名: check_himntn
功能描述: 判断nv项是否打开
输入参数:feature:himntn中数据的位置

输出参数: 无
返 回 值: 只有himntn[feature]为'1'，函数返回1；其他均返回0
*******************************************************************************/
int check_himntn(int feature)
{
	int ret = 0;

	if (feature >= HIMNTN_BOTTOM || feature < 0)
		goto out;

	if ('1' == himntn[feature])
		ret = 1;

out:
	return ret;
}

/*******************************************************************************
函 数 名: check_himntn
功能描述: 返回对应的nv值
输入参数:feature:himntn中数据的位置

输出参数: 无
返 回 值: 只有himntn[feature]为'1'，函数返回1；其他均返回0
*******************************************************************************/
int get_himntn_value(int feature)
{
	int ret = 0;

	if (feature >= HIMNTN_BOTTOM || feature < 0)
		goto out;

	ret = himntn[feature];

out:
	return ret;
}

static int __init early_parse_himntn_cmdline(char *himntn_cmdline)
{
	memset(himntn, 0x0, HIMNTN_VALID_SIZE + 1);
	if (strlen(himntn_cmdline) > HIMNTN_VALID_SIZE) {
		BB_PRINT_ERR("error: invalid himn cmdline size!\n");
		return -1;
	}
	memcpy(himntn, himntn_cmdline, strlen(himntn_cmdline));

	return 0;
}

early_param("himntn", early_parse_himntn_cmdline);

void mntn_print_to_ramconsole(const char *fmt, ...)
{
	char pbuf[128] = { 0 };
	va_list ap;

	if (NULL == fmt) {
		return;
	}
	memset(pbuf, 0, 128);
	va_start(ap, fmt);
	vsnprintf(pbuf, 128, fmt, ap);
	va_end(ap);
	ramoops_console_write_buf(pbuf, strlen(pbuf));
}

/*******************************************************************************
Function:       atoi
Description:    transfer string to int
Input:          string
Output:         NA
Return:         u32
********************************************************************************/
u32 atoi(char *s)
{
	char *p = s;
	char c;
	u64 ret = 0;

	if (s == NULL)
		return 0;
	while ((c = *p++) != '\0') {
		if ('0' <= c && c <= '9') {
			ret *= 10;
			ret += (u64) ((unsigned char)c - '0');
			if (ret > U32_MAX)
				return 0;
		} else {
			break;
		}
	}
	return (u32) ret;
}

/*create balong proc fs directory */
static struct proc_dir_entry *proc_balong_entry;	/*proc/balong */
static struct proc_dir_entry *proc_balong_stats_entry;	/*proc/balong/stats */
static struct proc_dir_entry *proc_balong_memory_entry;	/*proc/balong/memory */
static struct proc_dir_entry *proc_balong_log_entry;	/*proc/balong/log */
static struct proc_dir_entry *proc_balong_pstore;	/*proc/balong/pstore */
static int __init balong_proc_fs_init(void)
{
	proc_balong_entry = proc_mkdir("balong", NULL);
	if (!proc_balong_entry) {
		panic("cannot create balong proc entry");
	}

	proc_balong_stats_entry = proc_mkdir("stats", proc_balong_entry);
	if (!proc_balong_stats_entry) {
		panic("cannot create balong sys proc entry");
	}

	proc_balong_memory_entry = proc_mkdir("memory", proc_balong_entry);
	if (!proc_balong_memory_entry) {
		panic("cannot create balong memory proc entry");
	}

	proc_balong_log_entry = proc_mkdir("log", proc_balong_entry);
	if (!proc_balong_log_entry) {
		panic("cannot create balong log proc entry");
	}

	proc_balong_pstore = proc_mkdir("pstore", proc_balong_entry);
	if (!proc_balong_pstore) {
		panic("cannot create balong pstore proc entry");
	}
	return 0;
}

core_initcall(balong_proc_fs_init);

static inline struct proc_dir_entry *balong_create_proc_entry(const char *name,
							      mode_t mode,
							      struct
							      proc_dir_entry
							      *parent,
							      const struct
							      file_operations
							      *proc_fops,
							      void *data)
{
	return proc_create_data(name, mode, parent, proc_fops, data);

	return NULL; /*lint !e527*/
}

static inline void balong_remove_proc_entry(const char *name,
					    struct proc_dir_entry *parent)
{
	remove_proc_entry(name, parent);

	return;
}

/* cppcheck-suppress * */
#define CREATE_PROC_ENTRY_DECLARE(NAME, PARENT)\
struct proc_dir_entry *balong_create_ ## NAME ## _proc_entry(const char *name, \
	mode_t mode, const struct file_operations *proc_fops, void *data)\
{\
	return balong_create_proc_entry(name, mode, PARENT, proc_fops, data);\
} \
EXPORT_SYMBOL(balong_create_ ## NAME ## _proc_entry);\
\
void balong_remove_ ## NAME ## _proc_entry(const char *name)\
{\
	balong_remove_proc_entry(name, PARENT);\
\
	return;\
} \
EXPORT_SYMBOL(balong_remove_ ## NAME ## _proc_entry);

CREATE_PROC_ENTRY_DECLARE(stats, proc_balong_stats_entry)
    CREATE_PROC_ENTRY_DECLARE(memory, proc_balong_memory_entry)
    CREATE_PROC_ENTRY_DECLARE(log, proc_balong_log_entry)
    CREATE_PROC_ENTRY_DECLARE(pstore, proc_balong_pstore)

