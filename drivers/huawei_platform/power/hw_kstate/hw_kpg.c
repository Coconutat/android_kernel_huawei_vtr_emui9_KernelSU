/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
 *
 * File name: hw_kpg.c
 * Description: monitor kernel wakelock.
 * Author: zhangguiwen@huawei.com
 * Version: 0.01
 * Date: 2018/04/02
 */
#include <linux/time.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>
#include <linux/err.h>
#include <huawei_platform/power/hw_kstate.h>


typedef enum {
	SET_WAKELOCK_TIMEOUT = 0,
	CANCEL_WAKELOCK_TIMEOUT = 1,
	SET_ALL_WAKELOCK_TIMEOUT = 2,
	CANCEL_ALL_WAKELOCK_TIMEOUT = 3,
	STOP_WAKELOCK = 4,
	STOP_ALL_KERNEL_WAKELOCK = 5
} PG_COMMAND_ORDER;


#define HW_PG_LOCK_NAME_MAX_LEN (64)
/*command received*/
typedef struct {
	u8 cmd;
	u8 lock_timeout;
	char name[HW_PG_LOCK_NAME_MAX_LEN];
} PG_CMD;

/*
  * Function: pg_cb
  * Description: manage wakelock
  * Return: -1 -- failed, 0 -- success
**/
static int pg_cb(CHANNEL_ID src, PACKET_TAG tag, const char *data, size_t len)
{
	int ret = 0;
	PG_CMD pg_cmd;

	if (IS_ERR_OR_NULL(data)) {
		pr_err("pg_cb %s: invalid data or len:%d\n", __func__, (int)len);
		return -1;
	}

	/* set wakelock */
	memset(&pg_cmd, 0, sizeof(PG_CMD));
	memcpy(&pg_cmd, data, len<(sizeof(PG_CMD)-1)?len:(sizeof(PG_CMD)-1));

	switch (pg_cmd.cmd) {
		case SET_WAKELOCK_TIMEOUT:
			ret = wakeup_source_set(pg_cmd.name, pg_cmd.lock_timeout);
			break;
		case CANCEL_WAKELOCK_TIMEOUT:
			ret = wakeup_source_set(pg_cmd.name, 0);
			break;
		case SET_ALL_WAKELOCK_TIMEOUT:

			break;
		case CANCEL_ALL_WAKELOCK_TIMEOUT:
			ret = wakeup_source_set_all(0);
			break;
		case STOP_WAKELOCK:
			ret = wake_unlockByName(pg_cmd.name);
			break;
       case STOP_ALL_KERNEL_WAKELOCK:
           ret = wake_unlockAll(30*60*1000);
           break;
		default:
			return -1;
	}

	pr_debug("pg_cb %s: src=%d tag=%d len=%d \n", __func__, src, tag, (int) len);
	return ret;
}

static struct kstate_opt kpg_opt = {
	.name = "kpg",
	.tag = PACKET_TAG_KPG,
	.dst = CHANNEL_ID_NETLINK,
	.hook = pg_cb,
};


static int __init kpg_init(void)
{
	int ret = -1;

	ret = kstate_register_hook(&kpg_opt);
	if (ret < 0) {
		pr_err("hw_kpg %s: kstate_register_hook error\n", __func__);
	} else {
		pr_info("hw_kpg %s: kstate_register_hook success\n", __func__);
	}
	return ret;
}

static void __exit kpg_exit(void)
{
	kstate_unregister_hook(&kpg_opt);
}

late_initcall(kpg_init);
module_exit(kpg_exit);

MODULE_LICENSE("Dual BSD/GPL");
