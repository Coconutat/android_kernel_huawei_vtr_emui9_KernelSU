#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/cpu.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <asm/compiler.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/string.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/version.h>
#include <chipset_common/security/hw_kernel_stp_interface.h>
#include <linux/cpumask.h>


#include <huawei_platform/log/imonitor.h>
#define IMONITOR_TA_CRASH_EVENT_ID      (901002003)
#include "securec.h"
#include "tc_ns_log.h"

/*#define TC_DEBUG*/
/*#define TC_VERBOSE*/
#include "smc.h"
#include "teek_client_constants.h"
#include "tc_ns_client.h"
#include "agent.h"
#include "teek_ns_client.h"
#include "tui.h"
#include "mailbox_mempool.h"

#include "tlogger.h"
#include <linux/crc32.h>
#include "security_auth_enhance.h"
#include <linux/hisi/secs_power_ctrl.h>
#include "dynamic_mem.h"
struct session_crypto_info *g_session_root_key = NULL;

/*lint -save -e750 -e529*/
#define MAX_EMPTY_RUNS		100

/* Current state of the system */
static uint8_t sys_crash;

enum SPI_CLK_MODE {
	SPI_CLK_OFF = 0,
	SPI_CLK_ON,
};

struct shadow_work {
	struct kthread_work kthwork;
	struct work_struct work;
	uint64_t target;
};
unsigned long g_shadow_thread_id = 0;

static struct task_struct *siq_thread;
static struct task_struct *smc_svc_thread;

static struct task_struct *ipi_helper_thread;
static DEFINE_KTHREAD_WORKER(ipi_helper_worker);

static struct cpumask m;
static int mask_flag = 0;

#define MAX_SMC_CMD 19

typedef uint32_t smc_buf_lock_t;

typedef struct __attribute__ ((__packed__)) TC_NS_SMC_QUEUE {
	/* set when CA send cmd_in, clear after cmd_out return */
	DECLARE_BITMAP(in_bitmap, MAX_SMC_CMD);
	/* set when gtask get cmd_in, clear after cmd_out return */
	DECLARE_BITMAP(doing_bitmap, MAX_SMC_CMD);
	/* set when gtask get cmd_out, clear after cmd_out return */
	DECLARE_BITMAP(out_bitmap, MAX_SMC_CMD); /* */

	smc_buf_lock_t smc_lock;

	uint32_t last_in;
	TC_NS_SMC_CMD in[MAX_SMC_CMD];
	uint32_t last_out;
	TC_NS_SMC_CMD out[MAX_SMC_CMD];

} TC_NS_SMC_QUEUE;

TC_NS_SMC_QUEUE *cmd_data;
phys_addr_t cmd_phys;

static struct list_head g_pending_head;
static spinlock_t pend_lock;

/* helpers */
static inline void acquire_smc_buf_lock(smc_buf_lock_t *lock)
{
	int ret;
	preempt_disable();
	do {
		ret = cmpxchg(lock, 0, 1);
	} while (ret);
}

static inline void release_smc_buf_lock(smc_buf_lock_t *lock)
{
	(void)cmpxchg(lock, 1, 0);
	preempt_enable();
}

static inline int occupy_free_smc_in_entry(TC_NS_SMC_CMD *cmd)
{
	int idx = -1, i;

	/* Note:
	 * acquire_smc_buf_lock will disable preempt and kernel will forbid
	 * call mutex_lock in preempt disabled scenes.
	 * To avoid such case(update_timestamp and update_chksum will call
	 * mutex_lock), only cmd copy is done when preempt is disable,
	 * then do update_timestamp and update_chksum.
	 * As soon as this idx of in_bitmap is set, gtask will see this
	 * cmd_in, but the cmd_in is not ready that lack of update_xxx,
	 * so we make a tricky here, set doing_bitmap and in_bitmap both
	 * at first, after update_xxx is done, clear doing_bitmap.
	 */
	acquire_smc_buf_lock(&cmd_data->smc_lock);
	for (i=0; i<MAX_SMC_CMD; i++) {
		if (test_bit(i, cmd_data->in_bitmap))
			continue;
		if (EOK != memcpy_s(&cmd_data->in[i], sizeof(TC_NS_SMC_CMD),
						cmd, sizeof(TC_NS_SMC_CMD))) {
			tloge("memcpy_s failed,%s line:%d", __func__, __LINE__);
			break;
		}
		cmd_data->in[i].event_nr = i;
		isb();
		wmb();
		set_bit(i, cmd_data->in_bitmap);
		set_bit(i, cmd_data->doing_bitmap);
		idx = i;
		break;
	}
	release_smc_buf_lock(&cmd_data->smc_lock);

	if (idx == -1) {
		tloge("can't get any free smc entry\n");
		return -1;
	}

	if (update_timestamp(&cmd_data->in[idx])) {
		tloge("update_timestamp failed !\n");
		goto clean;
	}
	if (update_chksum(&cmd_data->in[idx])) {
		tloge("update_chksum failed.\n");
		goto clean;
	}

	acquire_smc_buf_lock(&cmd_data->smc_lock);
	isb();
	wmb();
	clear_bit(idx, cmd_data->doing_bitmap);
	release_smc_buf_lock(&cmd_data->smc_lock);

	return idx;
clean:
	acquire_smc_buf_lock(&cmd_data->smc_lock);
	clear_bit(i, cmd_data->in_bitmap);
	clear_bit(i, cmd_data->doing_bitmap);
	release_smc_buf_lock(&cmd_data->smc_lock);

	return -1;
}

static int reuse_smc_in_entry(uint32_t idx)
{
	int rc = 0;

	acquire_smc_buf_lock(&cmd_data->smc_lock);
	if (!(test_bit(idx, cmd_data->in_bitmap)
			&& test_bit(idx, cmd_data->doing_bitmap))) {
		tloge("invalid cmd to reuse\n");
		rc = -1;
		goto out;
	}
	if (EOK != memcpy_s(&cmd_data->in[idx], sizeof(TC_NS_SMC_CMD),
			&cmd_data->out[idx], sizeof(TC_NS_SMC_CMD))) {
		tloge("memcpy_s failed,%s line:%d", __func__, __LINE__);
		rc = -1;
		goto out;
	}
	release_smc_buf_lock(&cmd_data->smc_lock);

	if (update_timestamp(&cmd_data->in[idx])) {
		tloge("update_timestamp failed !\n");
		return -1;
	}
	if (update_chksum(&cmd_data->in[idx])) {
		tloge("update_chksum failed.\n");
		return -1;
	}

	acquire_smc_buf_lock(&cmd_data->smc_lock);
	isb();
	wmb();
	clear_bit(idx, cmd_data->doing_bitmap);
out:
	release_smc_buf_lock(&cmd_data->smc_lock);

	return rc;
}

enum cmd_reuse {
	clear,        /* clear this cmd index */
	resend,       /* use this cmd index resend */
	reserve       /* not clear, wait cmd answer */
};

static inline int copy_smc_out_entry(uint32_t idx, TC_NS_SMC_CMD *copy, enum cmd_reuse *usage)
{
	acquire_smc_buf_lock(&cmd_data->smc_lock);
	if (!test_bit(idx, cmd_data->out_bitmap)) {
		tloge("cmd out %d is not ready\n", idx);
		show_cmd_bitmap();
		release_smc_buf_lock(&cmd_data->smc_lock);
		return -1;
	}

	if (memcpy_s(copy, sizeof(TC_NS_SMC_CMD), &cmd_data->out[idx],
			sizeof(TC_NS_SMC_CMD))) {
		tloge("copy smc out failed\n");
		release_smc_buf_lock(&cmd_data->smc_lock);
		return -1;
	}

	isb();
	wmb();
	/* In TEEC_PENDIDG2 case, there're 2 case: slog and ssa.
	 * 1. ssa's request use a new cmd_index and send back, after agent work
	 *    done the reponse will not update the cmd_in in current session;
	 * 2. slog will use the session's cmd in to send TEEC_PENDING2 back,
	 *    after agent work done, it will update the cmd_in in current session.
	 * So we need clear bitmap in slog case and not clear in ssa call case.
	 */
	if (cmd_data->out[idx].cmd_id == SS_AGENT_MSG
		|| cmd_data->out[idx].cmd_id == MT_AGENT_MSG) {
		if (usage)
			*usage = reserve;
	} else if (cmd_data->out[idx].ret_val == TEEC_PENDING2
		|| cmd_data->out[idx].ret_val == TEEC_PENDING) {
		if (usage)
			*usage = resend;
	} else {
		clear_bit(idx, cmd_data->in_bitmap);
		clear_bit(idx, cmd_data->doing_bitmap);
		if (usage)
			*usage = clear;
	}
	clear_bit(idx, cmd_data->out_bitmap);

	release_smc_buf_lock(&cmd_data->smc_lock);

	return 0;
}

static inline void clear_smc_in_entry(uint32_t idx)
{
	acquire_smc_buf_lock(&cmd_data->smc_lock);
	clear_bit(idx, cmd_data->in_bitmap);
	release_smc_buf_lock(&cmd_data->smc_lock);
}

static inline void release_smc_entry(uint32_t idx)
{
	acquire_smc_buf_lock(&cmd_data->smc_lock);
	clear_bit(idx, cmd_data->in_bitmap);
	clear_bit(idx, cmd_data->doing_bitmap);
	clear_bit(idx, cmd_data->out_bitmap);
	release_smc_buf_lock(&cmd_data->smc_lock);
}

static inline int copy_shadow_task_agent_cmd(TC_NS_SMC_CMD *copy)
{
	uint32_t i;

	acquire_smc_buf_lock(&cmd_data->smc_lock);
	for (i=0; i<MAX_SMC_CMD; i++) {
		if (test_bit(i, cmd_data->out_bitmap)
			&& (cmd_data->out[i].ret_val == TEEC_PENDING2
				&& cmd_data->out[i].cmd_id & (SHADOW_AGENT_FLAG))) {
			if (memcpy_s(copy, sizeof(TC_NS_SMC_CMD), &cmd_data->out[i],
					sizeof(TC_NS_SMC_CMD))) {
				tloge("copy smc out failed\n");
				release_smc_buf_lock(&cmd_data->smc_lock);
				return -1;
			}
			clear_bit(i, cmd_data->in_bitmap);
			clear_bit(i, cmd_data->doing_bitmap);
			clear_bit(i, cmd_data->out_bitmap);

			release_smc_buf_lock(&cmd_data->smc_lock);

			return 0;
		}
	}
	release_smc_buf_lock(&cmd_data->smc_lock);

	return -1;
}

static inline int is_cmd_working_done(uint32_t idx)
{
	bool ret = false;

	acquire_smc_buf_lock(&cmd_data->smc_lock);
	if (test_bit(idx, cmd_data->out_bitmap))
		ret = true;
	release_smc_buf_lock(&cmd_data->smc_lock);

	return ret;
}

void show_cmd_bitmap(void)
{
	uint32_t idx;
	char bitmap[MAX_SMC_CMD+1];
	int cmd_in[MAX_SMC_CMD] = {-1}, cmd_out[MAX_SMC_CMD] = {-1};
	uint32_t in=0, out=0;

	if (memset_s(cmd_in, sizeof(cmd_in), -1, sizeof(cmd_in))
		|| memset_s(cmd_out, sizeof(cmd_out), -1, sizeof(cmd_out))) {
		tloge("memset failed\n");
		return;
	}

	for (idx = 0; idx < MAX_SMC_CMD; idx++) {
		if (test_bit(idx, cmd_data->in_bitmap)) {
			bitmap[idx] = '1';
			cmd_in[in++] = idx;
		} else {
			bitmap[idx] = '0';
		}
	}
	bitmap[MAX_SMC_CMD] = '\0';
	TCERR("in_bitmap: %s\n", bitmap);
	for (idx = 0; idx < MAX_SMC_CMD; idx++) {
		if (test_bit(idx, cmd_data->doing_bitmap)) {
			bitmap[idx] = '1';
		} else {
			bitmap[idx] = '0';
		}
	}
	bitmap[MAX_SMC_CMD] = '\0';
	TCERR("doing_bitmap: %s\n", bitmap);
	for (idx = 0; idx < MAX_SMC_CMD; idx++) {
		if (test_bit(idx, cmd_data->out_bitmap)) {
			bitmap[idx] = '1';
			cmd_out[out++] = idx;
		} else {
			bitmap[idx] = '0';
		}
	}
	bitmap[MAX_SMC_CMD] = '\0';
	TCERR("out_bitmap: %s\n", bitmap);

	tlogi("cmd_in value:\n");
	for (idx=0; idx<MAX_SMC_CMD; idx++) {
		if (cmd_in[idx] == -1)
			break;
		tlogi("cmd[%d]: cmd_id=%d, read_pid=%d, dev_id = 0x%x, event_nr=%d, ret_val=0x%x\n",
			cmd_in[idx],
			cmd_data->in[cmd_in[idx]].cmd_id,
			cmd_data->in[cmd_in[idx]].ca_pid,
			cmd_data->in[cmd_in[idx]].dev_file_id,
			cmd_data->in[cmd_in[idx]].event_nr,
			cmd_data->in[cmd_in[idx]].ret_val
			);
	}

	tlogi("cmd_out value:\n");
	for (idx=0; idx<MAX_SMC_CMD; idx++) {
		if (cmd_out[idx] == -1)
			break;
		tlogi("cmd[%d]: cmd_id=%d, read_pid=%d, dev_id = 0x%x, event_nr=%d, ret_val=0x%x\n",
			cmd_out[idx],
			cmd_data->out[cmd_out[idx]].cmd_id,
			cmd_data->out[cmd_out[idx]].ca_pid,
			cmd_data->out[cmd_out[idx]].dev_file_id,
			cmd_data->out[cmd_out[idx]].event_nr,
			cmd_data->out[cmd_out[idx]].ret_val
			);
	}
}

static struct pending_entry *init_pending_entry(pid_t pid)
{
	struct pending_entry *pe;

	pe = kmalloc(sizeof(*pe), GFP_KERNEL);
	if (!pe) {
		tloge("alloc pe failed\n");
		return NULL;
	}
	atomic_set(&pe->users, 1);
	pe->pid = pid;
	init_waitqueue_head(&pe->wq);
	atomic_set(&pe->run, 0);
	INIT_LIST_HEAD(&pe->list);
	spin_lock(&pend_lock);
	list_add_tail(&pe->list, &g_pending_head);
	spin_unlock(&pend_lock);

	return pe;
}

struct pending_entry *find_pending_entry(pid_t pid)
{
	struct pending_entry *pe;

	spin_lock(&pend_lock);
	list_for_each_entry(pe, &g_pending_head, list) {
		if (pe->pid == pid) {
			atomic_inc(&pe->users);
			spin_unlock(&pend_lock);
			return pe;
		}
	}
	spin_unlock(&pend_lock);

	return NULL;
}

void foreach_pending_entry(void (*func)(struct pending_entry *))
{
	struct pending_entry *pe;

	if (!func)
		return;

	spin_lock(&pend_lock);
	list_for_each_entry(pe, &g_pending_head, list) {
		func(pe);
	}
	spin_unlock(&pend_lock);
}

void put_pending_entry(struct pending_entry *pe)
{
	if (atomic_dec_and_test(&pe->users))
		kfree(pe);
}

static void release_pending_entry(struct pending_entry *pe)
{
	spin_lock(&pend_lock);
	list_del(&pe->list);
	spin_unlock(&pend_lock);
	put_pending_entry(pe);
}

static DECLARE_WAIT_QUEUE_HEAD(siq_th_wait);
static DECLARE_WAIT_QUEUE_HEAD(ipi_th_wait);

static atomic_t siq_th_run;

extern int spi_init_secos(unsigned int spi_bus_id);
extern int spi_exit_secos(unsigned int spi_bus_id);

extern void cmd_monitor_log(TC_NS_SMC_CMD *cmd);
extern void cmd_monitor_logend(TC_NS_SMC_CMD *cmd);
extern void init_cmd_monitor(void);
extern void do_cmd_need_archivelog(void);
enum {
	TYPE_CRASH_TA 	= 1,
	TYPE_CRASH_TEE = 2,
};
extern void cmd_monitor_ta_crash(int32_t type);

enum smc_ops_exit {
	SMC_OPS_NORMAL		= 0x0,
	SMC_OPS_SCHEDTO		= 0x1,
	SMC_OPS_START_SHADOW    = 0x2,
	SMC_OPS_START_FIQSHD    = 0x3,
	SMC_OPS_PROBE_ALIVE	= 0x4,

	SMC_EXIT_NORMAL		= 0x0,
	SMC_EXIT_PREEMPTED	= 0x1,
	SMC_EXIT_SHADOW		= 0x2,

	SMC_EXIT_MAX		= 0x3,
};

#define SHADOW_EXIT_RUN		(0x1234dead)

#define SMC_EXIT_TARGET_SHADOW_EXIT	(0x1)

static inline bool is_shadow_exit(uint64_t target)
{
	return target & SMC_EXIT_TARGET_SHADOW_EXIT;
}

typedef struct smc_cmd_ret {
	u64 exit;
	u64 ta;
	u64 target;
} smc_cmd_ret_t;

static inline void secret_fill(smc_cmd_ret_t *ret, u64 exit, u64 ta, u64 target)
{
	ret->exit = exit;
	ret->ta = ta;
	ret->target = target;
}

static noinline int smp_smc_send(uint32_t cmd, u64 ops, u64 ca,
					smc_cmd_ret_t *secret)
{
	u64 x0 = cmd;
	u64 x1 = ops;
	u64 x2 = ca;
	u64 x3 = 0;
	u64 x4 = 0;
	u64 ret = 0, exit_reason = 0, ta = 0, target = 0;

retry:
	if (secret && SMC_EXIT_PREEMPTED == secret->exit) {
		x0 = cmd;
		x1 = SMC_OPS_SCHEDTO;
		x2 = ca;
		x3 = secret->ta;
		x4 = secret->target;
	}
	if (secret && (SMC_OPS_SCHEDTO == ops || SMC_OPS_START_FIQSHD == ops))
		x4 = secret->target;
	tlogd("[cpu %d] start to send x0=%llx x1=%llx x2=%llx x3=%llx x4=%llx\n",
			raw_smp_processor_id(), x0, x1, x2, x3, x4);
	isb();
	wmb();

	do {
		asm volatile(
                "mov x0, %[fid]\n"
                "mov x1, %[a1]\n"
                "mov x2, %[a2]\n"
                "mov x3, %[a3]\n"
                "mov x4, %[a4]\n"
                "smc	#0\n"
                "str x0, [%[re0]]\n"
                "str x1, [%[re1]]\n"
                "str x2, [%[re2]]\n"
                "str x3, [%[re3]]\n"
			: [fid] "+r" (x0), [a1] "+r" (x1), [a2] "+r" (x2),
			  [a3] "+r" (x3), [a4] "+r" (x4)
			: [re0] "r" (&ret), [re1] "r" (&exit_reason),
			  [re2] "r" (&ta), [re3] "r" (&target)
			: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
				"x8", "x9",	"x10", "x11", "x12", "x13",
				"x14", "x15", "x16", "x17");
	} while (0);

	isb();
	wmb();
        tlogd("[cpu %d] return val %llx exit_reason %llx ta %llx targ %llx\n",
                        raw_smp_processor_id(), ret, exit_reason, ta, target);
	if (!secret)
		return ret;
	secret_fill(secret, exit_reason, ta, target);
	if (SMC_EXIT_PREEMPTED == exit_reason)
		goto retry;

	return ret;
}

int raw_smc_send(uint32_t cmd, phys_addr_t cmd_addr,
			     uint32_t cmd_type, uint8_t wait)
{
	/*tlogd("start to send smc to secure\n");*/
	register u64 x0 asm("x0") = cmd;
	register u64 x1 asm("x1") = cmd_addr;
	register u64 x2 asm("x2") = cmd_type;
	register u64 x3 asm("x3") = cmd_addr >> 32;

	do {
		asm volatile(
				__asmeq("%0", "x0")
				__asmeq("%1", "x0")
				__asmeq("%2", "x1")
				__asmeq("%3", "x2")
				__asmeq("%4", "x3")
				"smc	#0\n"
				: "+r" (x0)
				: "r" (x0), "r" (x1), "r" (x2), "r" (x3));
	} while (x0 == TSP_REQUEST && wait);

	return x0;
}

void siq_dump(int mode)
{
	raw_smc_send(TSP_REE_SIQ, (phys_addr_t)mode, 0, false);
	tz_log_write();
	do_cmd_need_archivelog();
}

static int siq_thread_fn(void *arg)
{
	int ret;
	while(1) {
		ret = wait_event_interruptible(siq_th_wait,/*lint !e666*/
				atomic_read(&siq_th_run));
		if (ret) {
			tloge("wait_event_interruptible failed!\n");
			return -EINTR;
		}

		atomic_set(&siq_th_run, 0);/*lint !e1058*/
		siq_dump(1);
	}
}

#define MAX_UPLOAD_INFO_LEN		4
static void upload_audit_event(unsigned int eventindex)
{
	struct stp_item item;
	int ret = 0;
	char att_info[MAX_UPLOAD_INFO_LEN + 1] = {0};

	att_info[0] = (unsigned char)(eventindex>>24);
	att_info[1] = (unsigned char)(eventindex>>16);
	att_info[2] = (unsigned char)(eventindex>>8);
	att_info[3] = (unsigned char)eventindex;
	att_info[MAX_UPLOAD_INFO_LEN] = '\0';

	item.id = item_info[ITRUSTEE].id;// 0x00000185;
	item.status = STP_RISK;
	item.credible = STP_REFERENCE;//STP_CREDIBLE;
	item.version = 0;

	ret = strncpy_s(item.name, STP_ITEM_NAME_LEN, item_info[ITRUSTEE].name, sizeof(item_info[ITRUSTEE].name));
	if(EOK != ret)
		tloge("strncpy_s failed  %x . \n", ret);
	tlogd("stp get size %x succ \n", sizeof(item_info[ITRUSTEE].name));

	ret = kernel_stp_upload(item, att_info);
	if (ret != 0) {
		tloge("stp %x event upload failed \n", eventindex);
	}
	else {
		tloge("stp %x event upload succ \n", eventindex);
	}
}

static void cmd_result_check(TC_NS_SMC_CMD* cmd)
{
	if ((TEEC_SUCCESS == cmd->ret_val) && (TEEC_SUCCESS
		!= verify_chksum(cmd))) {
		cmd->ret_val = (uint32_t)TEEC_ERROR_GENERIC;
		tloge("verify_chksum failed.\n");
	}
	if (cmd->ret_val == TEEC_PENDING
			  || cmd->ret_val == TEEC_PENDING2) { /*lint !e650 */
		tlogd("wakeup command %d\n", cmd->event_nr);
	}
	if (TEE_ERROR_TAGET_DEAD == cmd->ret_val) { /*lint !e650 !e737 */
		tloge("error smc call: ret = %x and cmd.err_origin=%x\n",
				cmd->ret_val, cmd->err_origin);
		if(TEEC_ORIGIN_TRUSTED_APP_TUI == cmd->err_origin){
			do_ns_tui_release();
		}
		cmd_monitor_ta_crash(TYPE_CRASH_TA);
	}
	if(TEE_ERROR_AUDIT_FAIL == cmd->ret_val){ /*lint !e650 !e737 */
		tloge("error smc call: ret = %x and cmd.err_origin=%x\n",
				cmd->ret_val, cmd->err_origin);
		tloge("error smc call: status = %x and cmd.err_origin=%x\n",
				cmd->eventindex, cmd->err_origin);
		upload_audit_event(cmd->eventindex);
	}
}

static int shadow_thread_fn(void *arg)
{
	u64 x0 = TSP_REQUEST;
	u64 x1 = SMC_OPS_START_SHADOW;
	u64 x2 = current->pid;
	u64 x3 = 0;
	u64 x4 = *(u64 *)arg;
	u64 ret = 0, exit_reason = SMC_EXIT_MAX, ta = 0, target = 0;
	int n_preempted = 0, n_idled = 0, ret_val = 0;
	struct pending_entry *pe;
	TC_NS_SMC_CMD cmd = {0};
	bool agent_request = false;

	pe = init_pending_entry(current->pid);
	if (!pe) {
		tloge("init pending entry failed\n");
		kfree(arg);
		return -ENOMEM;
	}

	isb();
	wmb();
	tlogd("%s: [cpu %d] x0=%llx x1=%llx x2=%llx x3=%llx x4=%llx\n", __func__,
		raw_smp_processor_id(), x0, x1, x2, x3, x4);

	if (hisi_secs_power_on()) {
		tloge("hisi_secs_power_on failed\n");
		kfree(arg);
		release_pending_entry(pe);
		return -EINVAL;
	}
retry:
	if (exit_reason == SMC_EXIT_PREEMPTED) {
		x0 = TSP_REQUEST;
		x1 = SMC_OPS_SCHEDTO;
		x2 = current->pid;
		x3 = ta;
		x4 = target;
	} else if (exit_reason == SMC_EXIT_NORMAL) {
		if (!agent_request) {
			x0 = TSP_REQUEST;
			x1 = SMC_OPS_SCHEDTO;
			x2 = current->pid;
			x3 = 0;
			x4 = 0;
			if (n_idled > 100) {
				n_idled = 0;
				x1 = SMC_OPS_PROBE_ALIVE;
			}
		} else {
			uint32_t cmd_index;

			agent_request = false;
			x0 = TSP_REQUEST;
			x1 = SMC_OPS_NORMAL;
			x2 = current->pid;
			x3 = 0;
			x4 = 0;

			/* TODO: need retry? */
			cmd_index = occupy_free_smc_in_entry(&cmd);
			if (-1 == cmd_index) {
				tloge("there's no more smc entry\n");
				ret_val = -1;
				goto clean;
			}
		}
	}

	isb();
	wmb();
	tlogd("%s: [cpu %d] x0=%llx x1=%llx x2=%llx x3=%llx x4=%llx\n", __func__,
		raw_smp_processor_id(), x0, x1, x2, x3, x4);

	do {
		asm volatile(
			"mov x0, %[fid]\n"
			"mov x1, %[a1]\n"
			"mov x2, %[a2]\n"
			"mov x3, %[a3]\n"
			"mov x4, %[a4]\n"
			"smc	#0\n"
			"str x0, [%[re0]]\n"
			"str x1, [%[re1]]\n"
			"str x2, [%[re2]]\n"
			"str x3, [%[re3]]\n"
			: [fid] "+r" (x0), [a1] "+r" (x1), [a2] "+r" (x2),
			[a3] "+r" (x3), [a4] "+r" (x4)
			: [re0] "r" (&ret), [re1] "r" (&exit_reason),
			[re2] "r" (&ta), [re3] "r" (&target)
			: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			"x8", "x9",	"x10", "x11", "x12", "x13",
			"x14", "x15", "x16", "x17");
	} while (0);

        isb();
        wmb();
        tlogd("shadow thread return %lld\n", exit_reason);
        if (SMC_EXIT_PREEMPTED == exit_reason) {
		n_idled = 0;
		if (++n_preempted > 10000) {
			tlogi("%s: retry 10K times on CPU%d\n",
				__func__, smp_processor_id());
			n_preempted = 0;
		}
                goto retry;
        } else if (SMC_EXIT_NORMAL == exit_reason) {
		if (!copy_shadow_task_agent_cmd(&cmd)) {
			/* if it's a agent request */
			unsigned int agent_id = cmd.agent_id;

			n_idled = 0;
			n_preempted = 0;

			/* If the agent does not exist post
			 * the answer right back to the TEE */
			if (agent_process_work(&cmd, agent_id) != TEEC_SUCCESS) {
				tloge("agent process work failed\n");

				agent_request = true;
				/* cmd will be reused */
				goto retry;
			} else {
				/* resume from agent */
				agent_request = true;
				/* cmd will be reused */
				goto retry;
			}
		} else {
			int rc, timeout;

			n_preempted = 0;
			timeout = HZ * (10 + (current->pid & 0xF));
			rc = wait_event_timeout(pe->wq, atomic_read(&pe->run), timeout);
			if (!rc) {
				n_idled++;
			}
			if (SHADOW_EXIT_RUN == atomic_read(&pe->run)) {
				tlogd("shadow thread work quit, be killed\n");
			} else {
				atomic_set(&pe->run, 0);
				goto retry;
			}
		}
        } else if (SMC_EXIT_SHADOW == exit_reason) {
		tlogd("shadow thread exit, it self\n");
	} else {
		tlogd("shadow thread exit with unknown code %ld\n", (long)exit_reason);
        }

clean:
	if (hisi_secs_power_down()) {
		tloge("hisi_secs_power_down failed\n");
		ret_val = -1;
	}
	kfree(arg);
	release_pending_entry(pe);

	return ret_val;
}

static void shadow_work_func(struct kthread_work *work)
{
	struct task_struct *shadow_thread;
	struct shadow_work *s_work = container_of(work, struct shadow_work, kthwork);
	uint64_t *target_arg = kmalloc(sizeof(uint64_t), GFP_KERNEL);

	if (!target_arg) {
		tloge("%s: kmalloc(8 bytes) failed\n", __func__);
		return;
	}

	*target_arg = s_work->target;
	shadow_thread = kthread_create(shadow_thread_fn,
				target_arg, "shadow_th/%lu", g_shadow_thread_id++);
	if (IS_ERR(shadow_thread)) {
		kfree(target_arg);
		tloge("couldn't create shadow_thread %ld\n", PTR_ERR(shadow_thread));
		return;
	}

	tlogd("%s: create shadow thread %lu for target %llx\n",
		__func__, g_shadow_thread_id, *target_arg);
	wake_up_process(shadow_thread);
}

static int __smc_wakeup_ca(pid_t ca, int which)
{
	if (ca == 0) {
		tlogw("wakeup for ca = 0\n");
	} else {
		struct pending_entry *pe = find_pending_entry(ca);

		if (!pe) {
			tloge("invalid ca pid=%d for pending entry\n", (int)ca);
			return -1;
		}
		atomic_set(&pe->run, which);
		wake_up(&pe->wq);
		tlogd("wakeup pending thread %ld\n", (long)ca);
		put_pending_entry(pe);
	}
	return 0;
}

void wakeup_pe(struct pending_entry *pe)
{
	atomic_set(&pe->run, 1);
	wake_up(&pe->wq);
}

int smc_wakeup_broadcast(void)
{
	foreach_pending_entry(wakeup_pe);
	return 0;
}

int smc_wakeup_ca(pid_t ca)
{
	return __smc_wakeup_ca(ca, 1);
}

int smc_shadow_exit(pid_t ca)
{
	return __smc_wakeup_ca(ca, SHADOW_EXIT_RUN);
}

void fiq_shadow_work_func(uint64_t target)
{
	smc_cmd_ret_t secret = { SMC_EXIT_MAX, 0, target};

	if (spi_init_secos(0)) {
		pr_err("spi0 for spi_init_secos error\n");
		return;
	}
	if (spi_init_secos(1)) {
		pr_err("spi1 for spi_init_secos error\n");
		(void)spi_exit_secos(0);
		return;
	}

	if (hisi_secs_power_on()) {
		tloge("hisi_secs_power_on failed\n");
		goto secs_power_err;
	}
	smp_smc_send(TSP_REQUEST, SMC_OPS_START_FIQSHD, current->pid, &secret);
	if (hisi_secs_power_down()) {
		tloge("hisi_secs_power_down failed\n");
		goto secs_power_err;
	}

	if (spi_exit_secos(0)) {
		pr_err("spi0 for spi_exit_secos error\n");
		(void)spi_exit_secos(1);
		return;
	}
	if (spi_exit_secos(1)) {
		pr_err("spi1 for spi_exit_secos error\n");
		return;
	}

	return;

secs_power_err:
	(void)spi_exit_secos(1);
	(void)spi_exit_secos(0);
	return;
}

int smc_queue_shadow_worker(uint64_t target)
{
	struct shadow_work work = {
		KTHREAD_WORK_INIT(work.kthwork, shadow_work_func),
		.target = target,
	};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (!queue_kthread_work(&ipi_helper_worker, &work.kthwork))
#else
	if (!kthread_queue_work(&ipi_helper_worker, &work.kthwork))
#endif
	{
		tloge("ipi helper work did't queue successfully, was already pending.\n");
		return -1;

	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	flush_kthread_work(&work.kthwork);
#else
	kthread_flush_work(&work.kthwork);
#endif

    return 0;
}

static void set_drm_strategy(void)
{
	if (!mask_flag) {
	        cpumask_clear(&m);
	        cpumask_set_cpu(4, &m);
	        cpumask_set_cpu(5, &m);
	        cpumask_set_cpu(6, &m);
	        cpumask_set_cpu(7, &m);
	        mask_flag = 1;
	}

	if (current->group_leader && strstr(current->group_leader->comm, "drm@1.")) {
	        set_cpus_allowed_ptr(current, &m);
	        set_user_nice(current, -5);
	}
}

static unsigned int smp_smc_send_func(TC_NS_SMC_CMD *in, uint32_t cmd_type,
				  uint8_t flags, bool reuse)
{
	int cmd_index = 0, last_index = 0;
	smc_cmd_ret_t cmd_ret = {0};
	int ret;
	TC_NS_SMC_CMD cmd = {0};
	struct pending_entry *pe;
	u64 ops;
	enum cmd_reuse cmd_usage = clear;

	set_drm_strategy();

	pe = init_pending_entry(current->pid);
	if (!pe) {
		tloge("init pending entry failed\n");
		return -ENOMEM;
	}

	in->ca_pid = current->pid;
	if (!reuse) {
		if (memcpy_s(&cmd, sizeof(TC_NS_SMC_CMD), in,
				sizeof(TC_NS_SMC_CMD))) {
			tloge("memcpy_s failed,%s line:%d", __func__, __LINE__);
			release_pending_entry(pe);
			return -1;
		}
	} else {
		last_index = cmd_index = in->event_nr;
		cmd_usage = resend;
	}

	ops = SMC_OPS_NORMAL;

retry:
	if (ops == SMC_OPS_NORMAL) {
		if (cmd_usage == resend) {
			if (reuse_smc_in_entry(cmd_index)) {
				tloge("reuse smc entry failed\n");
				release_smc_entry(cmd_index);
				release_pending_entry(pe);
				return -1;
			}
		} else {
			cmd_index = occupy_free_smc_in_entry(&cmd);
			if (-1 == cmd_index) {
				tloge("there's no more smc entry\n");
				release_pending_entry(pe);
				return -1;
			}
		}

		if (cmd_usage != clear) {
			cmd_index = last_index;
			cmd_usage = clear;
		} else
			last_index = cmd_index;

		tlogd("submit new cmd: cmd.ca=%d cmd-id=%x ev-nr=%d cmd-index=%d last-index=%d\n",
				cmd.ca_pid,
				cmd.cmd_id,
				cmd_data->in[cmd_index].event_nr,
				cmd_index,
				last_index
				);
	}

retry_with_filled_cmd:
	tlogd("smp_smc_send start cmd_id = %d, ca = %d\n", cmd.cmd_id, cmd.ca_pid);
	if (hisi_secs_power_on()) {
		tloge("hisi_secs_power_on failed\n");
		cmd.ret_val = -1;
		goto clean;
	}
	ret = smp_smc_send(TSP_REQUEST, ops, current->pid, &cmd_ret);
	if (hisi_secs_power_down()) {
		tloge("hisi_secs_power_down failed\n");
		cmd.ret_val = -1;
		goto clean;
	}
	tlogd("smp_smc_send ret = %x, cmd_ret.exit=%ld, cmd_index=%d\n",
		ret, (long)cmd_ret.exit, cmd_index);

	isb();
	wmb();

	if (ret == TSP_CRASH) {
		tloge("TEEOS has crashed!\n");
		sys_crash = 1;
		cmd_monitor_ta_crash(TYPE_CRASH_TEE);
		rdr_system_error(0x83000001, 0, 0);

		cmd.ret_val = -1;
		goto clean;
	}

	if (!is_cmd_working_done(cmd_index)) {
		
		tlogd("smc-cmd NOT-done: smc-ret=%x smc-exit=%d cmd-ca=%d cmd-id=%x ev-nr=%d cmd-index=%d last-index=%d\n",
			ret, (int)cmd_ret.exit,
			cmd_data->in[cmd_index].ca_pid,
			cmd_data->in[cmd_index].cmd_id,
			cmd_data->in[cmd_index].event_nr,
			cmd_index,
			last_index
			);

		if (cmd_ret.exit == SMC_EXIT_NORMAL) {
			/* task pending exit */
			tlogd("goto sleep, exit_reason=%lld\n", cmd_ret.exit);
			wait_event(pe->wq, atomic_read(&pe->run));
			atomic_set(&pe->run, 0);

			if (is_cmd_working_done(cmd_index)) {
				tlogd("cmd done\n");
				goto cmd_done;
			}

			ops = SMC_OPS_SCHEDTO;
			goto retry_with_filled_cmd;
		} else {
			tloge("invalid cmd work state\n");
			cmd.ret_val = -1;
			goto clean;
		}
	} else {
cmd_done:
		if (copy_smc_out_entry(cmd_index, &cmd, &cmd_usage)) {
			cmd.ret_val = -1;
			goto clean;
		}

		tlogd("smc-cmd done: smc-ret=%x smc-exit=%d cmd-ca=%d cmd-id=%x ev-nr=%d cmd-index=%d last-index=%d cmd.ret=%d\n",
			ret, (int)cmd_ret.exit,
			cmd_data->out[cmd_index].ca_pid,
			cmd_data->out[cmd_index].cmd_id,
			cmd_data->out[cmd_index].event_nr,
			cmd_index,
			last_index,
			cmd_data->out[cmd_index].ret_val
			);

		cmd_result_check(&cmd);

		switch (cmd.ret_val) {
		case TEEC_PENDING2: {
			unsigned int agent_id = cmd.agent_id;

			/* If the agent does not exist post
			 * the answer right back to the TEE */
			if (agent_process_work(&cmd, agent_id) != TEEC_SUCCESS) {
				tloge("agent process work failed\n");

				ops = SMC_OPS_NORMAL;
				/* cmd will be reused */
				goto retry;
			} else {

				ops = SMC_OPS_NORMAL;
				/* cmd will be reused */
				goto retry;
			}
		}
		case TEE_ERROR_TAGET_DEAD:
			break;
		case TEEC_PENDING:
			/* just copy out, and let out to proceed */
		default:
			if (EOK != memcpy_s(in,
					sizeof(TC_NS_SMC_CMD),
					&cmd,
					sizeof(TC_NS_SMC_CMD))) {
				tloge("memcpy_s failed,%s line:%d", __func__, __LINE__);
				cmd.ret_val = -1;
			}
			break;
		}
	}

clean:
	ret = (cmd_usage != clear && cmd.ret_val != TEEC_PENDING);
	if (ret)
		release_smc_entry(cmd_index);
	release_pending_entry(pe);

	return cmd.ret_val;
}

static int smc_svc_thread_fn(void *arg)
{
	struct mb_cmd_pack *mb_pack = NULL;

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc mailbox failed\n");
		return TEEC_ERROR_GENERIC;
	}
	mb_pack->uuid[0] = 1;

	while (!kthread_should_stop()) {
		TC_NS_SMC_CMD smc_cmd = {0};
		int ret;

		smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
		smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
		smc_cmd.cmd_id = GLOBAL_CMD_ID_SET_SERVE_CMD;

		if (spi_init_secos(0)) {
			pr_err("spi0 for spi_init_secos error\n");
			goto spi_err;
		}
		if (spi_init_secos(1)) {
			pr_err("spi1 for spi_init_secos error\n");
			(void)spi_exit_secos(0);
			goto spi_err;
		}

		ret = smp_smc_send_func(&smc_cmd, TC_NS_CMD_TYPE_NS_TO_SECURE, 0, false);

		if (spi_exit_secos(0)) {
			pr_err("spi0 for spi_exit_secos error\n");
			(void)spi_exit_secos(1);
			goto spi_err;
		}
		if (spi_exit_secos(1)) {
			pr_err("spi1 for spi_exit_secos error\n");
			goto spi_err;
		}
		tlogd("smc svc return 0x%x\n", ret);
	}

	tloge("smc_svc_thread stop ...\n");
	mailbox_free(mb_pack);

	return 0;

spi_err:
	mailbox_free(mb_pack);
	return -EINVAL;
}

#define HUNGTASK_LIST_LEN	13
static const char* g_hungtask_monitor_list[HUNGTASK_LIST_LEN] = {
	"system_server","fingerprintd", "atcmdserver", "keystore", "gatekeeperd",
	"volisnotd", "secure_storage", "secure_storage_s", "mediaserver",
	"vold", "tee_test_ut", "tee_test_secure_timer", "IFAAPluginThrea"};

bool is_tee_hungtask(struct task_struct *t)
{
	uint32_t i;
	if (!t)
		return false;

	for (i=0; i < HUNGTASK_LIST_LEN; i++) {
		if (!strcmp(t->comm, g_hungtask_monitor_list[i])) { /*lint !e421 */
			tloge("tee_hungtask detected:the hungtask is %s\n",t->comm);
			return true;
		}
	}
	return false;

}

void wakeup_tc_siq(void)
{
	atomic_set(&siq_th_run, 1);/*lint !e1058*/
	wake_up_interruptible(&siq_th_wait);
}

/*
 * Function:     TC_NS_SMC
 * Description:   This function first power on crypto cell,
 *					then send smc cmd to trustedcore.
 *					After finished, power off crypto cell.
 * Parameters:   cmd_addr
 * Return:      0:smc cmd handled succefully
 *              0xFFFFFFFF:power on or power off failed.
 *              0xFFFFxxxx:smc cmd handler failed or cmd pending.
 */
static unsigned int __TC_NS_SMC(TC_NS_SMC_CMD *cmd, uint8_t flags, bool reuse)
{
	unsigned int ret = (unsigned int)TEEC_ERROR_GENERIC;
	int spi_ret;

	if (sys_crash) {
		tloge("ERROR: sys crash happened!!!\n");
		return (unsigned int)TEEC_ERROR_GENERIC;
	}

	if (!cmd) {
		tloge("invalid cmd\n");
		return (unsigned int)TEEC_ERROR_GENERIC;
	}

	spi_ret = spi_init_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_init_secos error : %d\n", spi_ret);
		goto spi_err;
	}
	spi_ret = spi_init_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_init_secos error : %d\n", spi_ret);
		(void)spi_exit_secos(0);
		goto spi_err;
	}

	tlogd(KERN_INFO "***TC_NS_SMC call start on cpu %d ***\n",
		raw_smp_processor_id());

	cmd_monitor_log(cmd);
	ret = smp_smc_send_func(cmd, TC_NS_CMD_TYPE_NS_TO_SECURE, flags, reuse);
	cmd_monitor_logend(cmd);

	spi_ret = spi_exit_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_exit_secos error : %d\n", spi_ret);
		(void)spi_exit_secos(1);
		if (!ret)
			ret = (unsigned int)TEEC_ERROR_GENERIC;
		goto spi_err;
	}
	spi_ret = spi_exit_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_exit_secos error : %d\n", spi_ret);
		if (!ret)
			ret = (unsigned int)TEEC_ERROR_GENERIC;
		goto spi_err;
	}

spi_err:
	return ret;
}

unsigned int TC_NS_SMC(TC_NS_SMC_CMD *cmd, uint8_t flags)
{
	return __TC_NS_SMC(cmd, flags, false);
}

unsigned int TC_NS_SMC_WITH_NO_NR(TC_NS_SMC_CMD *cmd, uint8_t flags)
{
	return __TC_NS_SMC(cmd, flags, true);
}

static void smc_work_no_wait(uint32_t type)
{
	raw_smc_send(TSP_REQUEST, cmd_phys, type, true);
}

static void smc_work_set_cmd_buffer(void)
{
	smc_work_no_wait(TC_NS_CMD_TYPE_SECURE_CONFIG);
}

static void smc_work_init_secondary_cpus(void)
{
	smc_work_no_wait(TC_NS_CMD_TYPE_NS_TO_SECURE);
}

static int smc_set_cmd_buffer(void)
{
	struct work_struct work;

	INIT_WORK_ONSTACK(&work, smc_work_set_cmd_buffer);
	/* Run work on CPU 0 */
	schedule_work_on(0, &work);
	flush_work(&work);
	tlogd("smc_set_cmd_buffer done\n");

	return 0;
}

static int smc_init_secondary_cpus(void)
{
	int i = 0;
	struct work_struct work;

	INIT_WORK_ONSTACK(&work, smc_work_init_secondary_cpus);
	/* Run work on all secondary cpus */
	get_online_cpus();
	for_each_online_cpu(i) {
		if (i != 0) {
			schedule_work_on(i, &work);
			flush_work(&work);
			tlogd("init secondary cpu %d done\n", i);
		}
	}
	put_online_cpus();

	return 0;
}

static int get_session_root_key(void)
{
	int ret;
	uint32_t *buffer = (uint32_t *)(cmd_data->in);
	uint32_t tee_crc = 0;
	uint32_t crc = 0;
	if (!buffer || ((uint64_t)buffer & 0x3)) {
		tloge("Session root key must be 4bytes aligned.\n");
		return -EFAULT;
	}

	g_session_root_key = kzalloc(sizeof(struct session_crypto_info),
				     GFP_KERNEL);
	if (!g_session_root_key) {
		tloge("No memory to store session root key.\n");
		return -ENOMEM;
	}

	tee_crc = *buffer;
	if (memcpy_s(g_session_root_key,
		     sizeof(struct session_crypto_info),
		     (void *)(buffer + 1),
		     sizeof(struct session_crypto_info))) {
		tloge("Copy session root key from TEE failed.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	crc = crc32(0, (void *)g_session_root_key,
		    sizeof(struct session_crypto_info));
	if (crc != tee_crc) {
		tloge("Session root key has been modified.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	if (memset_s((void *)(cmd_data->in), sizeof(cmd_data->in),
		     0, sizeof(cmd_data->in))) {
		tloge("Clean the command buffer failed.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	return 0;

free_mem:
	kfree(g_session_root_key);
	g_session_root_key = NULL;
	return ret;
}
extern char __cfc_rules_start[];
extern char __cfc_rules_stop[];
extern char __cfc_area_start[];
extern char __cfc_area_stop[];
unsigned int *cfc_seqlock;

static void smc_set_cfc_info(void)
{
	struct cfc_rules_pos {
		u64 magic;
		u64 address;
		u64 size;
		u64 cfc_area_start;
		u64 cfc_area_stop;
	} __attribute__((packed)) *buffer = (void *)(cmd_data->out);

	buffer->magic = 0xCFC00CFC00CFCCFC;
	buffer->address = virt_to_phys(__cfc_rules_start);
	buffer->size = (void *)__cfc_rules_stop - (void *)__cfc_rules_start;
	buffer->cfc_area_start = virt_to_phys(__cfc_area_start);
	buffer->cfc_area_stop = virt_to_phys(__cfc_area_stop);
	cfc_seqlock = (u32 *)__cfc_rules_start;
}

/* Sync with trustedcore_src TEE/cfc.h */
enum cfc_info_to_linux {
       CFC_TO_LINUX_IS_ENABLED = 0xCFC0CFC1,
       CFC_TO_LINUX_IS_DISABLED = 0xCFC0CFC2,
};

/* default is false */
bool cfc_is_enabled;

static void smc_get_cfc_info(void)
{
	uint32_t cfc_flag = ((uint32_t *)(cmd_data->out))[0];

	if (cfc_flag == (uint32_t) CFC_TO_LINUX_IS_ENABLED)
		cfc_is_enabled = true;
}


#define compile_time_assert(cond, msg) \
    typedef char ASSERT_##msg[(cond) ? 1 : -1]
compile_time_assert(sizeof(TC_NS_SMC_QUEUE) <= PAGE_SIZE,
		size_of_TC_NS_SMC_QUEUE_too_large);
int smc_init_data(struct device *class_dev)
{
	int ret = 0;
	struct cpumask new_mask;

	if (NULL == class_dev || IS_ERR(class_dev))
		return -ENOMEM;

	cmd_data = (TC_NS_SMC_QUEUE *) __get_free_page(GFP_KERNEL | __GFP_ZERO);
	if (!cmd_data)
		return -ENOMEM;

	smc_set_cfc_info();

	cmd_phys = virt_to_phys(cmd_data);

	/* Send the allocated buffer to TrustedCore for init */
	if (smc_set_cmd_buffer()) {
		ret = -EINVAL;
		goto free_mem;
	}
	if (smc_init_secondary_cpus()) {
		ret = -EINVAL;
		goto free_mem;
	}
	if (get_session_root_key()) {
		ret = -EFAULT;
		goto free_mem;
	}
	smc_get_cfc_info();

	siq_thread = kthread_create(siq_thread_fn, NULL, "siqthread/%d", 0);

	if (unlikely(IS_ERR(siq_thread))) {
		dev_err(class_dev, "couldn't create siqthread %ld\n",
			PTR_ERR(siq_thread));
		ret = (int)PTR_ERR(siq_thread);
		goto free_mem;
	}

	/*
	 * TEE Dump will disable IRQ/FIQ for about 500 ms, it's not
	 * a good choice to ask CPU0/CPU1 to do the dump.
	 *
	 * So, bind this kernel thread to other CPUs rather than CPU0/CPU1.
	 */
	cpumask_setall(&new_mask);
	cpumask_clear_cpu(0, &new_mask);
	cpumask_clear_cpu(1, &new_mask);
	kthread_bind_mask(siq_thread, &new_mask);

	ipi_helper_thread =
		kthread_create(kthread_worker_fn, &ipi_helper_worker, "ipihelper");
	if (IS_ERR(ipi_helper_thread)) {
		dev_err(class_dev, "couldn't create ipi_helper_threads %ld\n",
			PTR_ERR(ipi_helper_thread));
		ret = (int)PTR_ERR(ipi_helper_thread);
		goto free_siq_worker;
	}

	wake_up_process(ipi_helper_thread);
	wake_up_process(siq_thread);
	init_cmd_monitor();

	INIT_LIST_HEAD(&g_pending_head);
	spin_lock_init(&pend_lock);

	return 0;

free_siq_worker:
	kthread_stop(siq_thread);
	siq_thread = NULL;

free_mem:
	free_page((unsigned long)cmd_data);
	cmd_data = NULL;
	if (!IS_ERR_OR_NULL(g_session_root_key)) {
		kfree(g_session_root_key);
		g_session_root_key = NULL;
	}
	return ret;
}

int init_smc_svc_thread(void)
{
	smc_svc_thread = kthread_create(smc_svc_thread_fn, NULL,
		"smc_svc_thread");
	if (unlikely(IS_ERR(smc_svc_thread))) {
		tloge("couldn't create smc_svc_thread %ld\n",
			PTR_ERR(smc_svc_thread));
		return PTR_ERR(smc_svc_thread);
	}
	wake_up_process(smc_svc_thread);

	return 0;
}

/*lint -e838*/
int teeos_log_exception_archive(unsigned int eventid,const char* exceptioninfo)
{
	int ret;
	struct imonitor_eventobj *teeos_obj;

	teeos_obj = imonitor_create_eventobj(eventid);
	if ( exceptioninfo!=NULL ) {
	    ret = imonitor_set_param(teeos_obj, 0, (long)exceptioninfo);
	} else {
	    ret = imonitor_set_param(teeos_obj, 0, (long)"teeos something crash");
	}
	if (0 != ret){
		tloge("imonitor_set_param failed\n");
		imonitor_destroy_eventobj(teeos_obj);
		return ret;
	}

	ret = imonitor_add_dynamic_path(teeos_obj, "/data/vendor/log/hisi_logs/tee");
	if (0 != ret) {
		tloge("add path  failed\n");
		imonitor_destroy_eventobj(teeos_obj);
		return ret;
	}
	ret = imonitor_add_dynamic_path(teeos_obj, "/data/log/tee");
	if (0 != ret) {
		tloge("add path  failed\n");
		imonitor_destroy_eventobj(teeos_obj);
		return ret;
	}
	ret = imonitor_send_event(teeos_obj);
	imonitor_destroy_eventobj(teeos_obj);
	return ret;
}

void smc_free_data(void)
{
	free_page((unsigned long)cmd_data);

	if (!IS_ERR_OR_NULL(smc_svc_thread)) {
		kthread_stop(smc_svc_thread);
		smc_svc_thread = NULL;
	}

	if (!IS_ERR_OR_NULL(g_session_root_key)) {
		kfree(g_session_root_key);
		g_session_root_key = NULL;
	}
}
/*lint -restore*/
