/*
 *  drivers/huawei/device/hwlog_kernel.c
 *
 *  Copyright (C) 2014 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if defined(CONFIG_HWLOG_KERNEL)
#include <linux/kernel.h>
#include <huawei_platform/log/hwlog_kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/aio.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <uapi/linux/uio.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <linux/version.h>
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#include <linux/module.h>
#endif

#define MAX_WORK_COUNT   20
#define MAX_TAG_SIZE     64
#define MAX_MSG_SIZE     256
#define HWLOG_TAG    hwlog_kernel
#define HWLOG_EXCEPTION_FS    "hwlog_exception"
#define HWLOG_JANK_FS    "hwlog_jank"
#define HWLOG_DUBAI_FS "hwlog_dubai"
HWLOG_REGIST();
static struct file* filp[HW_LOG_ID_MAX] = { NULL, NULL, NULL};
static const char *log_name[HW_LOG_ID_MAX] = {
	[HW_LOG_ID_EXCEPTION]   = "/dev/" HWLOG_EXCEPTION_FS,
	[HW_LOG_ID_JANK]   = "/dev/" HWLOG_JANK_FS,
	[HW_LOG_ID_DUBAI]   = "/dev/" HWLOG_DUBAI_FS,
};
static int CHECK_CODE = 0x7BCDABCD;
/**
*  tag: the tag of this command
*  msg: concrete command string to write to /dev/hwlog_jank
*  return: on success return the bytes writed successfully, on error return <0
*
*/
struct my_work_struct{
    int   bWaiting;
    u16   nbufid;
    u16   nPrio;
    int   taglen;
    u16   janktagid;
    u64   uptime;
    u64   realtime;
    char  ntagid[MAX_MSG_SIZE];
    int   msglen;
    char  msg[MAX_MSG_SIZE];
    struct work_struct writelog;
};
static int             bInited = 0;
static int              nwork_index = 0;

struct my_work_struct   hwlog_work[MAX_WORK_COUNT];
struct workqueue_struct *hwlog_workqueue = NULL;

static int __write_hwlog_to_kernel_init(struct my_work_struct  *phwlog);
static int __write_to_kernel_kernel(struct my_work_struct  *phwlog);
static int (*write_hwlog_to_kernel)(struct my_work_struct  *phwlog) = __write_hwlog_to_kernel_init;
static DEFINE_MUTEX(hwlogfile_mutex);
static DEFINE_RAW_SPINLOCK(hwlog_spinlock);

void hwlog_write(struct work_struct *p_work)
{
	struct my_work_struct *p_hwlog_work = container_of(p_work, struct my_work_struct, writelog);
	write_hwlog_to_kernel(p_hwlog_work);
	p_hwlog_work->bWaiting = 0;
}

static int __init hwlog_workqueue_init(void)
{
	int   i;

    if (bInited) {
		hwlog_err("create hwlog_workqueue again\n");
		return 1;
	}
	hwlog_info("hw_log: create hwlog_workqueue\n");
	hwlog_workqueue = create_singlethread_workqueue("hwlog_workqueue");
	if (!hwlog_workqueue) {
		hwlog_err("failed to create hwlog_workqueue\n");
		return 1;
	}
	nwork_index = 0;
	for (i = 0; i < MAX_WORK_COUNT; i++) {
		INIT_WORK(&(hwlog_work[i].writelog), hwlog_write);
		hwlog_work[i].bWaiting = 0;
	}
	bInited = 1;
	return 0;
}

static void __exit hwlog_workqueue_destroy(void)
{
	int i = 0;

	if (hwlog_workqueue) {
		destroy_workqueue(hwlog_workqueue);
		hwlog_workqueue = NULL;
	}
	for (i = 0; i < HW_LOG_ID_MAX; i++) {
		if (0 == IS_ERR(filp[i])) {
			filp_close(filp[i], NULL);
			filp[i] = NULL;
		}
	}
}

static int __write_hwlog_to_kernel_init(struct my_work_struct  *phwlog)
{
	int i = 0;
	int err_count = 0;
	mm_segment_t oldfs;
	if (phwlog->nbufid >= HW_LOG_ID_MAX)
		return 0;

	mutex_lock(&hwlogfile_mutex);
	if (__write_hwlog_to_kernel_init == write_hwlog_to_kernel) {
		oldfs = get_fs();
		set_fs(get_ds());
		for (i = 0; i < HW_LOG_ID_MAX; i++) {
			filp[i] = filp_open(log_name[i], O_WRONLY, 0);
			if (IS_ERR(filp[i]))
				err_count++;
		}
	set_fs(oldfs);
	if (err_count == HW_LOG_ID_MAX) {
		hwlog_err("failed to open \n");
		mutex_unlock(&hwlogfile_mutex);
		return 0;
	} else
		write_hwlog_to_kernel = __write_to_kernel_kernel;
	}
	mutex_unlock(&hwlogfile_mutex);
	return write_hwlog_to_kernel(phwlog);
}
/*
static int __write_to_kernel_null(char* tag, char* msg)
{
    return -1;
}
*/
static int __write_to_kernel_kernel(struct my_work_struct  *phwlog)
{
	struct iovec vec[6];
	unsigned long vcount = 0;
	mm_segment_t oldfs;
	int ret;
	if (phwlog->nbufid >= HW_LOG_ID_MAX)
		return 0;

	if (unlikely(!phwlog)) {
		hwlog_err("invalid arguments\n");
		return -1;
	}

	if (IS_ERR(filp[phwlog->nbufid])) {
		hwlog_err("file descriptor to %s is invalid: %ld\n", log_name[phwlog->nbufid], PTR_ERR(filp[phwlog->nbufid]));
		return -1;
	}

	vcount = 0;
	vec[vcount].iov_base = &CHECK_CODE;
	vec[vcount++].iov_len  = sizeof(CHECK_CODE);
	if (HW_LOG_ID_JANK == phwlog->nbufid) {
		vec[vcount].iov_base = &phwlog->nPrio;
		vec[vcount++].iov_len  = 2;
		vec[vcount].iov_base = &phwlog->janktagid;
		vec[vcount++].iov_len  = 2;
		vec[vcount].iov_base =&phwlog->uptime;
		vec[vcount++].iov_len  = sizeof(phwlog->uptime);
		vec[vcount].iov_base = &phwlog->realtime;
		vec[vcount++].iov_len  = sizeof(phwlog->realtime);
		vec[vcount].iov_base = phwlog->msg;
		vec[vcount++].iov_len  = phwlog->msglen;
	} else {
		vec[vcount].iov_base = &phwlog->nPrio;
		vec[vcount++].iov_len  = 1;
		vec[vcount].iov_base = phwlog->ntagid;
		vec[vcount++].iov_len  = phwlog->taglen;
		vec[vcount].iov_base = phwlog->msg;
		vec[vcount++].iov_len  = phwlog->msglen;
	}
	oldfs = get_fs();
	set_fs(get_ds());
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	ret = vfs_writev(filp[phwlog->nbufid], vec, vcount,
	        &(filp[phwlog->nbufid])->f_pos, 0);
#else
	ret = vfs_writev(filp[phwlog->nbufid], vec, vcount,
	        &(filp[phwlog->nbufid])->f_pos);
#endif
	set_fs(oldfs);
	if (unlikely(ret < 0))
		hwlog_err("failed to write %s\n", log_name[phwlog->nbufid]);
	return ret;
}

int hwlog_to_write(int prio, int bufid, const char *tag, const char *fmt, ...)
{
	unsigned long           flags;
	va_list                 args;
	struct my_work_struct  *pWork = NULL;
	int                     len;
	if (strlen(tag) >= MAX_MSG_SIZE)
		return 0;
	raw_spin_lock_irqsave(&hwlog_spinlock, flags);
	if (!bInited) {
		hwlog_err("hwlog_workqueue is null.\n");
		raw_spin_unlock_irqrestore(&hwlog_spinlock, flags);
		return -1;
	}
	pWork = &hwlog_work[nwork_index];
	if (0 == pWork->bWaiting) {
		pWork->bWaiting = 1;
		nwork_index++;
		if (nwork_index >= MAX_WORK_COUNT)
		nwork_index = 0;
	} else {
		pWork = NULL;
	}
	raw_spin_unlock_irqrestore(&hwlog_spinlock, flags);

	if (NULL == pWork) {
		hwlog_err("hwlog_workqueue is full, failed.\n");
		return -1;
	}
	pWork->nbufid = bufid;
	pWork->nPrio = prio;
	memcpy(pWork->ntagid, tag, strlen(tag)+1);
	pWork->taglen = strlen(tag) + 1;
	va_start(args, fmt);
	len = vscnprintf(pWork->msg, MAX_MSG_SIZE, fmt, args);
	va_end(args);
	pWork->msglen = len + 1;

	queue_work(hwlog_workqueue, &(pWork->writelog));

	return 0;
}
int hwlog_to_jank(int tag, int prio, const char* fmt, ...)
{
	unsigned long           flags;
	va_list                 args;
	struct my_work_struct  *pWork = NULL;
	int                     len;
	struct timespec         upts, realts;
	u64                     uptime;
	u64                     realtime;

	ktime_get_ts(&upts);
	realts = upts;
	get_monotonic_boottime(&realts);
	uptime = (u64)upts.tv_sec*1000 + upts.tv_nsec/1000000;
	realtime = (u64)realts.tv_sec*1000 + realts.tv_nsec/1000000;

	raw_spin_lock_irqsave(&hwlog_spinlock, flags);
	if (!bInited) {
		hwlog_err("hwlog_workqueue is null.\n");
		raw_spin_unlock_irqrestore(&hwlog_spinlock, flags);
		return -1;
	}
	pWork = &hwlog_work[nwork_index];
	if (0 == pWork->bWaiting) {
		pWork->bWaiting = 1;
		nwork_index++;
		if (nwork_index >= MAX_WORK_COUNT)
			nwork_index = 0;
	} else {
		pWork = NULL;
	}
	raw_spin_unlock_irqrestore(&hwlog_spinlock, flags);

	if (NULL == pWork) {
		hwlog_err("hwlog_workqueue is full, logd_to_jank failed.\n");
		return -1;
	}
	pWork->nbufid = HW_LOG_ID_JANK;
	pWork->nPrio = prio;
	pWork->janktagid = tag;
	pWork->uptime = uptime;
	pWork->realtime = realtime;

	va_start(args, fmt);
	len = vscnprintf(pWork->msg, MAX_MSG_SIZE, fmt, args);
	va_end(args);
	pWork->msglen = len + 1;
	queue_work(hwlog_workqueue, &(pWork->writelog));

	return 0;
}

EXPORT_SYMBOL(hwlog_to_jank);
EXPORT_SYMBOL(hwlog_to_write);
module_init(hwlog_workqueue_init);
module_exit(hwlog_workqueue_destroy);
#endif

