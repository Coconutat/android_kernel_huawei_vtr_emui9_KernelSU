#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <asm/cacheflush.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include "teek_client_constants.h"
#include "tc_ns_client.h"
#include "teek_ns_client.h"
#include "tc_ns_log.h"
#include "gp_ops.h"
#include "mailbox_mempool.h"
#include "smc.h"
#include "libhwsecurec/securec.h"
#include "tz_spi_notify.h"


#define MAX_CALLBACK_COUNT 100
struct TEEC_timer_property;

typedef unsigned int (*rtc_timer_callback_func)(struct TEEC_timer_property *);
static int g_timer_type;

enum timer_class_type {
	/* timer event using timer10 */
	TIMER_GENERIC,
	/* timer event using RTC */
	TIMER_RTC
};

struct TEEC_timer_property {
	unsigned int type;
	unsigned int timer_id;
	unsigned int timer_class;
	unsigned int reserved2;
};

struct notify_context_timer {
	unsigned int dev_file_id;
	unsigned char uuid[16];
	unsigned int session_id;
	struct TEEC_timer_property property;
	uint32_t expire_time;
};

struct notify_context_wakeup {
	pid_t ca_thread_id;
};

struct notify_context_shadow {
	uint64_t target_tcb;
};

struct notify_context_stats {
	uint32_t send_s;
	uint32_t recv_s;
	uint32_t send_w;
	uint32_t recv_w;
	uint32_t missed;
};

union notify_context {
	struct notify_context_timer timer;
	struct notify_context_wakeup wakeup;
	struct notify_context_shadow shadow;
	struct notify_context_stats  meta;
};

struct notify_data_entry {
	uint32_t entry_type : 31;
	uint32_t filled     : 1;
	union notify_context context;
};

#define NOTIFY_DATA_ENTRY_COUNT \
	((PAGE_SIZE/sizeof(struct notify_data_entry)) - 1)
struct notify_data_struct{
	struct notify_data_entry entry[NOTIFY_DATA_ENTRY_COUNT];
	struct notify_data_entry meta;
};

static struct notify_data_struct *g_notify_data;
static struct notify_data_entry *notify_data_entry_timer;
static struct notify_data_entry *notify_data_entry_rtc;
static struct notify_data_entry *notify_data_entry_shadow;

enum notify_data_type {
	NOTIFY_DATA_ENTRY_UNUSED,
	NOTIFY_DATA_ENTRY_TIMER,
	NOTIFY_DATA_ENTRY_RTC,
	NOTIFY_DATA_ENTRY_WAKEUP,
	NOTIFY_DATA_ENTRY_SHADOW,
	NOTIFY_DATA_ENTRY_FIQSHD,
	NOTIFY_DATA_ENTRY_SHADOW_EXIT,
	NOTIFY_DATA_ENTRY_MAX,
};

struct TC_NS_Callback {
	unsigned char uuid[16];
	struct mutex callback_lock;
	void (*callback_func)(void *);
	struct list_head head;
};

struct TC_NS_Callback_List {
	unsigned int callback_count;
	struct mutex callback_list_lock;
	struct list_head callback_list;
};

static void tc_notify_fn(struct work_struct *dummy);

static struct TC_NS_Callback_List g_ta_callback_func_list;
static DECLARE_WORK(tc_notify_work, tc_notify_fn);
static struct workqueue_struct *tz_spi_wq;

static void tc_notify_timer_fn(struct notify_data_entry *notify_data_entry)
{
	TC_NS_DEV_File *temp_dev_file;
	TC_NS_Service *temp_svc;
	TC_NS_Session *temp_ses = NULL;
	int enc_found = 0;
	rtc_timer_callback_func callback_func;
	struct TC_NS_Callback *callback_func_t;
	struct notify_context_timer *tc_notify_data_timer;

	tc_notify_data_timer =
		&(notify_data_entry->context.timer);
	notify_data_entry->filled = 0;

	TC_TIME_DEBUG("notify_data timer type is 0x%x, timer ID is 0x%x\n",
			tc_notify_data_timer->property.type,
			tc_notify_data_timer->property.timer_id);

	/* timer callback process */
	mutex_lock(&g_ta_callback_func_list.callback_list_lock);
	list_for_each_entry(callback_func_t,
			&g_ta_callback_func_list.callback_list, head) {
		if (0 == memcmp(callback_func_t->uuid,
					tc_notify_data_timer->uuid, 16)) {
			if (TIMER_RTC ==
					tc_notify_data_timer->property.timer_class) {
				TC_TIME_DEBUG("start to call callback func\n");
				callback_func =	(rtc_timer_callback_func)
					(callback_func_t->callback_func);
				(void)(callback_func)
					(&(tc_notify_data_timer->property));
				TC_TIME_DEBUG("end to call callback func\n");
			} else if (TIMER_GENERIC ==
					tc_notify_data_timer->property.timer_class) {
				TC_TIME_DEBUG("timer60 no callback func\n");
			}
		}
	}
	mutex_unlock(&g_ta_callback_func_list.callback_list_lock);

	mutex_lock(&g_tc_ns_dev_list.dev_lock);
	list_for_each_entry(temp_dev_file,
			&g_tc_ns_dev_list.dev_file_list, head) {
		TCDEBUG("dev file id1 = %d, id2 = %d\n",
				temp_dev_file->dev_file_id,
				tc_notify_data_timer->dev_file_id);
		if (temp_dev_file->dev_file_id ==
		    tc_notify_data_timer->dev_file_id) {
			mutex_lock(&temp_dev_file->service_lock);
			temp_svc =
				tc_find_service(&temp_dev_file->services_list,
						tc_notify_data_timer->uuid); /*lint !e64 */
			get_service_struct(temp_svc);
			mutex_unlock(&temp_dev_file->service_lock);
			if (temp_svc) {
				mutex_lock(&temp_svc->session_lock);
				temp_ses =
					tc_find_session(&temp_svc->session_list,
							tc_notify_data_timer->
							session_id);
				get_session_struct(temp_ses);
				mutex_unlock(&temp_svc->session_lock);
				put_service_struct(temp_svc);
				if (temp_ses) {
					TCDEBUG("send cmd ses id %d\n",
							temp_ses->session_id);
					enc_found = 1;
					break;
				}
				break;
			}
			break;
		}

	}
	mutex_unlock(&g_tc_ns_dev_list.dev_lock);
	if (TIMER_GENERIC == tc_notify_data_timer->property.timer_class) {
		TC_TIME_DEBUG("timer60 wake up event\n");
		if (enc_found && temp_ses) {
			temp_ses->wait_data.send_wait_flag = 1;
			wake_up(&temp_ses->wait_data.send_cmd_wq);
			put_session_struct(temp_ses);
		}
	} else {
		TC_TIME_DEBUG("RTC do not need to wakeup\n");
	}
}

static noinline int get_notify_data_entry(struct notify_data_entry *copy)
{
	uint32_t i;
	int ret = -1, filled;

	/* TIMER and RTC use fix entry, skip them. */
	for (i=NOTIFY_DATA_ENTRY_WAKEUP-1; i<NOTIFY_DATA_ENTRY_COUNT; i++) {
		struct notify_data_entry *e;

		e = &g_notify_data->entry[i];
		filled = e->filled;
		smp_mb();
		if (!filled)
			continue;
		switch (e->entry_type) {
		case NOTIFY_DATA_ENTRY_SHADOW:
		case NOTIFY_DATA_ENTRY_SHADOW_EXIT:
		case NOTIFY_DATA_ENTRY_FIQSHD:
			g_notify_data->meta.context.meta.recv_s++;
			break;
		case NOTIFY_DATA_ENTRY_WAKEUP:
			g_notify_data->meta.context.meta.recv_w++;
			break;
		default:
			tloge("invalid notify type=%d\n", e->entry_type);
			goto exit;
		}

		if (memcpy_s(copy, sizeof(struct notify_data_entry),
				e, sizeof(struct notify_data_entry))) {
			tloge("memcpy entry failed\n");
			break;
		}

		smp_mb();
		e->filled = 0;
		ret = 0;
		break;
	}

exit:
	return ret;
}

static void tc_notify_wakeup_fn(struct notify_data_entry *entry)
{
	struct notify_context_wakeup *tc_notify_wakeup;

	tc_notify_wakeup = &(entry->context.wakeup);
	smc_wakeup_ca(tc_notify_wakeup->ca_thread_id);
	tlogd("notify_data_entry_wakeup ca: %llx\n", tc_notify_wakeup->ca_thread_id);
}

static void tc_notify_shadow_fn(struct notify_data_entry *entry)
{
	struct notify_context_shadow *tc_notify_shadow;

	tc_notify_shadow = &(entry->context.shadow);
	smc_queue_shadow_worker(tc_notify_shadow->target_tcb);
}

static void tc_notify_fiqshd_fn(struct notify_data_entry *entry)
{
	struct notify_context_shadow *tc_notify_shadow;

	tc_notify_shadow = &(entry->context.shadow);
	fiq_shadow_work_func(tc_notify_shadow->target_tcb);
}

static void tc_notify_shadowexit_fn(struct notify_data_entry *entry)
{
	struct notify_context_wakeup *tc_notify_wakeup;

	tc_notify_wakeup = &(entry->context.wakeup);
	if(smc_shadow_exit(tc_notify_wakeup->ca_thread_id) != 0) {
		tloge("shadow ca exit failed: %d\n", (int)tc_notify_wakeup->ca_thread_id);
	}
}

static void spi_broadcast_notifications(void)
{
	uint32_t missed;

	smp_mb();
	missed = __xchg(0, &g_notify_data->meta.context.meta.missed, 4);
	if (!missed)
		return;
	if (missed & (1U<<NOTIFY_DATA_ENTRY_WAKEUP)) {
		smc_wakeup_broadcast();
		missed &= ~(1U<<NOTIFY_DATA_ENTRY_WAKEUP);
	}
	if (missed & (1U<<NOTIFY_DATA_ENTRY_FIQSHD)) {
		tc_notify_fiqshd_fn(NULL);
		missed &= ~(1U<<NOTIFY_DATA_ENTRY_FIQSHD);
	}
	if (missed)
		tloge("missed spi notification mask %x\n", missed);
}

static void tc_notify_other_fun(void)
{
	struct notify_data_entry copy = {0};

	while (0 == get_notify_data_entry(&copy)) {
		switch (copy.entry_type) {
		case NOTIFY_DATA_ENTRY_WAKEUP:
			tc_notify_wakeup_fn(&copy);
			break;
		case NOTIFY_DATA_ENTRY_SHADOW:
			tc_notify_shadow_fn(&copy);
			break;
		case NOTIFY_DATA_ENTRY_FIQSHD:
			tc_notify_fiqshd_fn(&copy);
			break;
		case NOTIFY_DATA_ENTRY_SHADOW_EXIT:
			tc_notify_shadowexit_fn(&copy);
			break;
		default:
			tloge("invalid entry type = %d\n", copy.entry_type);
		}

		if (memset_s(&copy, sizeof(copy), 0, sizeof(copy)))
			tloge("memset copy failed\n");
	}
	spi_broadcast_notifications();
}

static void tc_notify_fn(struct work_struct *dummy)
{
	if (notify_data_entry_timer->filled) {
		tc_notify_timer_fn(notify_data_entry_timer);
	}
	if (notify_data_entry_rtc->filled) {
		tc_notify_timer_fn(notify_data_entry_rtc);
	}
	tc_notify_other_fun();
}

static irqreturn_t tc_secure_notify(int irq, void *dev_id)
{
#define N_WORK	8
	int i;
	static struct work_struct tc_notify_works[N_WORK];
	static int init;

	if (!init) {
		for (i=0; i<N_WORK; i++) {
			INIT_WORK(&tc_notify_works[i], tc_notify_fn);
		}
		init = 1;
	}

	for (i=0; i<N_WORK; i++) {
		if (queue_work(tz_spi_wq, &tc_notify_works[i]))
			break;
	}
#undef N_WORK

	return IRQ_HANDLED;
}

static void TST_get_timer_type(int *type)
{
	*type = g_timer_type;
}

int TC_NS_RegisterServiceCallbackFunc(char *uuid, void *func,
				      void *private_data)
{
	struct TC_NS_Callback *callback_func = NULL;
	struct TC_NS_Callback *new_callback = NULL;
	int ret = 0;
	errno_t sret;

	if (NULL == uuid || NULL == func)
		return -EINVAL;

	mutex_lock(&g_ta_callback_func_list.callback_list_lock);
	if (g_ta_callback_func_list.callback_count > MAX_CALLBACK_COUNT) {
		mutex_unlock(&g_ta_callback_func_list.callback_list_lock);
		tloge("callback_count is out\n");
		return -ENOMEM;
	}
	list_for_each_entry(callback_func,
			&g_ta_callback_func_list.callback_list, head) {
		if (0 == memcmp(callback_func->uuid, uuid, 16)) {
			callback_func->callback_func = (void (*)(void *))func; /*lint !e611 */
			TCDEBUG("succeed to find uuid ta_callback_func_list\n");
			goto find_callback;
		}
	}

	/*create a new callback struct if we couldn't find it in list */
	new_callback = kzalloc(sizeof(struct TC_NS_Callback), GFP_KERNEL);
	if (!new_callback) {
		TCERR("kmalloc failed\n");
		ret = -ENOMEM;
		goto find_callback;
	}

	sret = memcpy_s(new_callback->uuid, 16, uuid, 16);
	if (EOK != sret) {
		kfree(new_callback);
		ret = -ENOMEM;
		goto find_callback;
	}
	g_ta_callback_func_list.callback_count++;
	TCDEBUG("ta_callback_func_list.callback_count is %d\n",
		g_ta_callback_func_list.callback_count);
	INIT_LIST_HEAD(&new_callback->head);
	new_callback->callback_func = (void (*)(void *))func; /*lint !e611 */
	mutex_init(&new_callback->callback_lock);
	list_add_tail(&new_callback->head,
		      &g_ta_callback_func_list.callback_list);

find_callback:
	mutex_unlock(&g_ta_callback_func_list.callback_list_lock);
	return ret; /*lint !e593 */
}
EXPORT_SYMBOL(TC_NS_RegisterServiceCallbackFunc);

static void timer_callback_func(struct TEEC_timer_property *timer_property)
{
	TC_TIME_DEBUG
	("timer_property->type = %x, timer_property->timer_id = %x\n",
	 timer_property->type, timer_property->timer_id);
	g_timer_type = (int)timer_property->type;
}

static void callback_demo_main(char *uuid)
{
	int ret = 0;

	TC_TIME_DEBUG("step into callback_demo_main\n");

	ret = TC_NS_RegisterServiceCallbackFunc(uuid,
						(void *)&timer_callback_func, /*lint !e611 */
						NULL);
	if (ret != 0)
		TCERR("failed to TC_NS_RegisterServiceCallbackFunc\n");
}

int TC_NS_TST_CMD(TC_NS_DEV_File *dev_id, void *argp)
{
	int ret = 0;
	TC_NS_ClientContext client_context;
	int cmd_id;
	int timer_type;
	TEEC_UUID secure_timer_uuid = {
		0x19b39980, 0x2487, 0x7b84,
		{0xf4, 0x1a, 0xbc, 0x89, 0x22, 0x62, 0xbb, 0x3d}
	};

	if (!argp) {
		TCERR("argp is NULL input buffer\n");
		ret = -EINVAL;
		return ret;
	}

	if (copy_from_user(&client_context, argp,
			   sizeof(TC_NS_ClientContext))) {
		TCERR("copy from user failed\n");
		ret = -ENOMEM;
		return ret;
	}

	if (tc_user_param_valid(&client_context, 0)) {
		TCERR("param 0 is invalid\n");
		ret = -EFAULT;
		return ret;
	}

	/*a_addr contain the command id*/
	if (copy_from_user
	    (&cmd_id, (void *)client_context.params[0].value.a_addr,
	     sizeof(cmd_id))) {
		TCERR("copy from user failed:cmd_id\n");
		ret = -ENOMEM;
		return ret;
	}

	if (memcmp((char *)client_context.uuid,
		   (char *)&secure_timer_uuid, sizeof(TEEC_UUID))) {
		TCERR("request not from secure_timer\n");
		TCERR("request uuid: %x %x %x %x\n",
		      *(client_context.uuid + 0),
		      *(client_context.uuid + 1),
		      *(client_context.uuid + 2),
		      *(client_context.uuid + 3));
		ret = -EACCES;
		return ret;
	}

	switch (cmd_id) {
	case TST_CMD_01:
		callback_demo_main((char *)client_context.uuid);
		break;
	case TST_CMD_02:
		TST_get_timer_type(&timer_type);
		if (tc_user_param_valid(&client_context, 1)) {
			TCERR("param 1 is invalid\n");
			ret = -EFAULT;
			return ret;
		}
		if (copy_to_user
		    ((void *)client_context.params[1].value.a_addr, &timer_type,
		     sizeof(timer_type))) {
			TCERR("copy to user failed:timer_type\n");
			ret = -ENOMEM;
			return ret;
		}
		break;
	default:
		ret = -EINVAL;
		return ret;
	}

	if (copy_to_user(argp, (void *)&client_context,
			 sizeof(client_context))) {
		TCERR("copy to user failed:client context\n");
		ret = -ENOMEM;
		return ret;
	}

	return ret;
}

static int TC_NS_register_notify_data_memery(void)
{
	TC_NS_SMC_CMD smc_cmd = { 0 };
	int ret;
	struct mb_cmd_pack *mb_pack;

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack)
		return TEEC_ERROR_GENERIC;

	mb_pack->operation.paramTypes =
		TEE_PARAM_TYPE_VALUE_INPUT | TEE_PARAM_TYPE_VALUE_INPUT << 4;
	mb_pack->operation.params[0].value.a = virt_to_phys(g_notify_data);
	mb_pack->operation.params[0].value.b = virt_to_phys(g_notify_data) >> 32;
	mb_pack->operation.params[1].value.a = SZ_4K;

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys((void *)mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys((void *)mb_pack->uuid) >> 32;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_NOTIFY_MEMORY;
	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32;

	TCDEBUG("cmd. context_phys:%x\n", smc_cmd.context_id);
	ret = TC_NS_SMC(&smc_cmd, 0);

	mailbox_free(mb_pack);

	return ret;
}

static int TC_NS_unregister_notify_data_memory(void)
{

	TC_NS_SMC_CMD smc_cmd = { 0 };
	int ret;
	struct mb_cmd_pack *mb_pack;

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack)
		return TEEC_ERROR_GENERIC;

	mb_pack->operation.paramTypes =
		TEE_PARAM_TYPE_VALUE_INPUT | TEE_PARAM_TYPE_VALUE_INPUT << 4;
	mb_pack->operation.params[0].value.a = virt_to_phys(g_notify_data);
	mb_pack->operation.params[0].value.b = virt_to_phys(g_notify_data) >> 32;
	mb_pack->operation.params[1].value.a = SZ_4K;

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys((void *)mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys((void *)mb_pack->uuid) >> 32;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_UNREGISTER_NOTIFY_MEMORY;
	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32;
	TCDEBUG("cmd. context_phys:%x\n", smc_cmd.context_id);

	ret = TC_NS_SMC(&smc_cmd, 0);

	mailbox_free(mb_pack);

	return ret;
}

int tz_spi_init(struct device *class_dev, struct device_node *np)
{
	int ret = 0;
	unsigned int irq = 0;

	tz_spi_wq = alloc_ordered_workqueue("tz_spi_wq", WQ_HIGHPRI);
	if (!tz_spi_wq) {
		TCERR("it failed to create workqueue tz_spi_wq\n");
		return -ENOMEM;
	}

	/* Map IRQ 0 from the OF interrupts list */
	irq = irq_of_parse_and_map(np, 0);
	ret = devm_request_irq(class_dev, irq, tc_secure_notify,
			       0, TC_NS_CLIENT_DEV, NULL);
	if (ret < 0) {
		TCERR("device irq %u request failed %u", irq, ret);
		goto clean;
	}

	ret = memset_s(&g_ta_callback_func_list,
			sizeof(g_ta_callback_func_list), 0,
			sizeof(g_ta_callback_func_list));
	if (EOK != ret) {
		ret = -EFAULT;
		goto clean;
	}

	g_ta_callback_func_list.callback_count = 0;
	INIT_LIST_HEAD(&g_ta_callback_func_list.callback_list);
	mutex_init(&g_ta_callback_func_list.callback_list_lock);

	if (!g_notify_data) {
		g_notify_data =
			(struct notify_data_struct *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
		if (!g_notify_data) {
			TCERR("__get_free_page failed for notification data\n");
			ret = -ENOMEM;
			goto clean;
		}
		ret = TC_NS_register_notify_data_memery();
		if (ret != TEEC_SUCCESS) {
			TCERR("Shared memory failed ret is 0x%x\n", ret);
			ret = -EFAULT;
			free_page((unsigned long)g_notify_data);
			g_notify_data = NULL;
			goto clean;
		}

		notify_data_entry_timer = &g_notify_data->entry[NOTIFY_DATA_ENTRY_TIMER-1];
		notify_data_entry_rtc = &g_notify_data->entry[NOTIFY_DATA_ENTRY_RTC-1];
		notify_data_entry_shadow = &g_notify_data->entry[NOTIFY_DATA_ENTRY_SHADOW-1];
                tlogi("test target is: %llx\n", notify_data_entry_shadow->context.shadow.target_tcb);
	}

	return 0;
clean:
	tz_spi_exit();
	return ret;
}

void tz_spi_exit(void)
{
	if (g_notify_data) {
		TC_NS_unregister_notify_data_memory();
		free_page((unsigned long)g_notify_data);
		g_notify_data = NULL;
	}
	if (tz_spi_wq) {
		flush_workqueue(tz_spi_wq);
		destroy_workqueue(tz_spi_wq);
		tz_spi_wq = NULL;
	}
}
