/*
 * hisi_flash_hisee_otp.c
 *
 * provides interfaces for writing hisee otp, set hisee_otp secure when
 * writing securitydebug value
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * chenli <chenli24@huawei.com>
 *
 */

#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/hisi/hisi_efuse.h>
#include "hisi_flash_hisee_otp.h"

static struct task_struct *flash_hisee_otp_task = NULL;
int (*g_write_hisee_otp_fn) (void) = NULL;
static struct semaphore g_hisee_sem;
static efuse_log_level_t g_efuse_print_level = log_level_error;

#define efuse_print_info(level, fmt, ...) do {\
    if (level <= g_efuse_print_level) { \
        printk(KERN_ERR fmt, ##__VA_ARGS__); \
    } \
} while (0)

static int flash_hisee_otp_value(void)
{
	if (NULL != g_write_hisee_otp_fn)
		return g_write_hisee_otp_fn();
	else
		return OK;
}

static int flash_otp_task(void *arg)
{
	int ret = OK;

	ret = down_timeout(&g_hisee_sem, MAX_SCHEDULE_TIMEOUT);
	if (-ETIME == ret) {
		efuse_print_info(log_level_error, "wait hisee factory test finished semphore timeout.\n");
	} else {
		ret = flash_hisee_otp_value();
	}
	flash_hisee_otp_task = NULL;
	return ret;
}

void release_hisee_semphore(void)
{
	up(&g_hisee_sem);
	return;
}

bool flash_otp_task_is_started(void)
{
	if (NULL == flash_hisee_otp_task) {
		return false;
	}

	if (IS_ERR(flash_hisee_otp_task)) {
		return false;
	}

	return true;
}

void register_flash_hisee_otp_fn(int (*fn_ptr) (void))
{
	if (NULL != fn_ptr)
		g_write_hisee_otp_fn = fn_ptr;
	return;
}

void creat_flash_otp_thread(void)
{
	if (!flash_hisee_otp_task) {
		flash_hisee_otp_task = kthread_run(flash_otp_task, NULL, "flash_otp_task");
		if (IS_ERR(flash_hisee_otp_task)) {
			flash_hisee_otp_task = NULL;
			efuse_print_info(log_level_error, "%s:create flash_otp_task failed\n", __func__);
		}
	}
}

void creat_flash_otp_init(void)
{
	sema_init(&g_hisee_sem, 0);
}
