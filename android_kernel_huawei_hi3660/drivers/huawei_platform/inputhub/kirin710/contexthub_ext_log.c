#include <linux/err.h>
#include "linux/spinlock_types.h"
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include "contexthub_route.h"
#include "contexthub_ext_log.h"
#include <huawei_platform/log/hw_log.h>


static struct inputhub_ext_log_notifier inputhub_ext_log_mag;
static bool inited = false;

int inputhub_ext_log_register_handler(int tag, int (*notify)(const pkt_header_t* head))
{
	struct inputhub_ext_notifier_node* pnode, *n;
	int ret = 0;
	unsigned long flags = 0;

	if (!inited) {
		hwlog_err("%s not inited\n", __func__);
		return -EINVAL;
	}

	if ((!(TAG_BEGIN <= tag && tag < TAG_END)) || (NULL == notify))
	{ return -EINVAL; }

	spin_lock_irqsave(&inputhub_ext_log_mag.lock, flags);
	/*avoid regist more than once*/
	list_for_each_entry_safe(pnode, n, &inputhub_ext_log_mag.head, entry) {
		if (tag == pnode->tag) {
			hwlog_warn
			("inputhub_ext_log tag = %d, notify = %pf has already registed in %s\n!",
			 tag, notify, __func__);
			goto out;   /*return when already registed*/
		}
	}

	/*make mcu_notifier_node*/
	pnode = kzalloc(sizeof(struct inputhub_ext_notifier_node), GFP_ATOMIC);

	if (NULL == pnode) {
		ret = -ENOMEM;
		goto out;
	}

	pnode->tag = tag;
	pnode->notify = notify;
	hwlog_info("%s tag %d registered\n", __func__, tag);
	/*add to list*/
	list_add(&pnode->entry, &inputhub_ext_log_mag.head);
out:
	spin_unlock_irqrestore(&inputhub_ext_log_mag.lock, flags);

	return ret;
}

int is_inputhub_ext_log_notify(const pkt_header_t* head)
{
	return (head->tag == TAG_LOG_BUFF)
		   && (CMD_EXT_LOG_FLUSH == head->cmd);
}

static int inputhub_ext_log_process(const pkt_header_t* head)
{
	int found = 0;
	ext_logger_req_t* pkt_ext = (ext_logger_req_t*)head;

	/*search mcu_notifier, call all call_backs*/
	struct inputhub_ext_notifier_node* pos, *n;
	list_for_each_entry_safe(pos, n, &inputhub_ext_log_mag.head, entry) {
		if (pos->tag == pkt_ext->tag && pos->notify) {
			hwlog_info("%s tag %d handled\n", __func__, head->tag);
			pos->notify(head);
			found = 1;
		}
	}

	if (!found) {
		hwlog_err("inputhub_ext_log_process tag %d not found\n", head->tag);
	}

	return 0;
}

int inputhub_ext_log_init(void)
{
	INIT_LIST_HEAD(&inputhub_ext_log_mag.head);
	spin_lock_init(&inputhub_ext_log_mag.lock);
	register_mcu_event_notifier(TAG_LOG_BUFF, CMD_EXT_LOG_FLUSH, inputhub_ext_log_process);
	inited = true;
	//hwlog_info("inputhub_ext_log_init success !\n");
	return 0;
}

