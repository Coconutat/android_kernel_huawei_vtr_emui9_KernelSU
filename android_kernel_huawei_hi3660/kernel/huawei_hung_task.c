/*
 * Detect Hung Task
 *
 * kernel/hisi_hung_task.c - kernel thread for detecting tasks stuck in D state
 *
 */

#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/nmi.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/lockdep.h>
#include <linux/export.h>
#include <linux/sysctl.h>
#include <linux/utsname.h>
#include <trace/events/sched.h>

#include <huawei_platform/log/log_jank.h>
#include <linux/ptrace.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <asm/traps.h>
#include <linux/suspend.h>
#ifdef CONFIG_HISI_BB
#include <linux/hisi/rdr_hisi_ap_hook.h>
#endif
#ifdef CONFIG_TZDRIVER
#include <chipset_common/security/itrustee.h>
#endif
#ifdef CONFIG_HW_ZEROHUNG
#include <chipset_common/hwzrhung/zrhung.h>
#endif
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/slab.h>
/****************************MICRO DEFINITION*********************************/
#define ENABLE_SHOW_LEN         8
#define WHITELIST_STORE_LEN     200
#define IGNORELIST_LENGTH       100
#define CONCERNLIST_LENGTH       11

#define WHITE_LIST              1
#define BLACK_LIST              2

#define HT_ENABLE               1
#define HT_DISABLE              0

#define JANK_TASK_MAXNUM        8
#define HEARTBEAT_TIME          3
#define MAX_LOOP_NUM            (CONFIG_DEFAULT_HUNG_TASK_TIMEOUT/\
						HEARTBEAT_TIME)
#define ONE_MINUTE              (60/HEARTBEAT_TIME)
#define ONE_AND_HALF_MINUTE     (90/HEARTBEAT_TIME)
#define TWO_MINUTES         (120/HEARTBEAT_TIME)
#define THREE_MINUTES		(180/HEARTBEAT_TIME)
#define TWENTY_SECONDS		(21/HEARTBEAT_TIME)
#define THIRTY_SECONDS		(30/HEARTBEAT_TIME)
#define HUNG_ONE_HOUR		(3600/HEARTBEAT_TIME)
#define HUNG_TEN_MINUTES	(600/HEARTBEAT_TIME)
#define HUNGTASK_REPORT_TIMECOST	TWENTY_SECONDS

#define HUNGTASK_DUMP_IN_PANIC_LOOSE	3
#define HUNGTASK_DUMP_IN_PANIC_STRICT	2
#define MAX_DUMP_TIMES		10
#define REFRESH_INTERVAL	THREE_MINUTES

#define FLAG_NONE               0
#define FLAG_DUMP_WHITE         1
#define FLAG_DUMP_APP           2
#define FLAG_DUMP_NOSCHEDULE    4
#define FLAG_DUMP_JANK          8
#define FLAG_PANIC              16
#define FLAG_PF_FROZEN			64

#define TASK_TYPE_IGNORE        0
#define TASK_TYPE_WHITE         1
#define TASK_TYPE_APP           2	/*android java process, and should
				not be TASK_TYPE_WHITE at the same time*/
#define TASK_TYPE_JANK          4	/*can combined with TASK_TYPE_APP or
					TASK_TYPE_WHITE at the same time*/
#define TASK_TYPE_KERNEL        8	/*it's kernel task*/
#define TASK_TYPE_NATIVE        16	/*it's native task*/
#define TASK_TYPE_NOSCHEDULE    32	/*it's android watchdog task*/
#define TASK_TYPE_FROZEN		64	/*task is FROZEN */

#define PID_INIT                1	/*PID for init process,always 1*/
#define PID_KTHREAD             2	/*PID for kernel kthreadd, always 2*/

/****************************JANK REPORT CONTROLLERS******************/
#define JANK_REPORT_LIMIT_PERDAY	5
#define JANK_REPORT_TRESHOLD		1
/********************DEFAULT DUMP OR PANIC TIME SETTINGS**************/
#define DEFAULT_SHUT_THIS_DOWN		0
#define DEFAULT_WHITE_DUMP_CNT          MAX_LOOP_NUM
#define DEFAULT_WHITE_PANIC_CNT         MAX_LOOP_NUM
#define DEFAULT_APP_DUMP_CNT            MAX_LOOP_NUM

#define DEFAULT_APP_PANIC_CNT		DEFAULT_SHUT_THIS_DOWN
#define DEFAULT_NOSCHEDULE_DUMP_CNT     (ONE_MINUTE - HUNGTASK_REPORT_TIMECOST)
#define DEFAULT_NOSCHEDULE_PANIC_CNT    TWO_MINUTES
#define DEFAULT_JANK_DUMP_CNT           JANK_REPORT_TRESHOLD
#define DEFAULT_OTHER_LOG_CNT		MAX_LOOP_NUM
#define HUNG_TASK_RECORDTIME_CNT	4
#define HUNG_TASK_UPLOAD_ONCE		1

#define IGN_STATE_INIT          1
#define IGN_STATE_FIRST         2
#define IGN_STATE_DONE          3
#define IGNORELIST_EMPTY        0
#define IGNORELIST_NORMAL       1
#define IGNORELIST_FULL         2
#define TOPAPP_HUNG_INIT		0
#define TOPAPP_HUNG_FOUND		1
#define TOPAPP_HUNG_RECORDED	2
#define WATCHDOG_THREAD_NAME    "watchdog"
/*
 * Limit number of tasks checked in a batch.
 * This value controls the preemptibility of khungtaskd since preemption
 * is disabled during the critical section. It also controls the size of
 * the RCU grace period. So it needs to be upper-bound.
 */
#define HUNG_TASK_BATCHING      1024
#define TIME_REFRESH_PIDS	20
#define HASH_ERROR              -1
#define HASH_FIND               0
#define HASH_INSERT             1
#define HASH_CONTINUE           0
#define HASH_END                1
#define HASH_FIRST              47
#define HASH_SECOND             23
#define HASH_THIRD              17
/****************************ZRHUNG CONTROLLERS***********************/
#define HUNGTASK_EVENT_WHITELIST	1
#define HUNGTASK_EVENT_LAZYWATCHDOG	2
#define HUNGTASK_EVENT_TOPAPP		4
#define REPORT_MSGLENGTH        100
#define ZRHUNG_CONFIG_LENGTH    10
#define MAX_ZRHUNG_CMD_BUF_SIZE 20
/****************************ZRHUNG CONFIGS***************************/
#define ZRHUNG_HUNGTASK_ENABLE  49
/****************************STRUCTURE DEFINITION*********************/
struct task_item {
	struct rb_node node;
	pid_t pid;
	pid_t tgid;
	char name[TASK_COMM_LEN + 1];
	unsigned long switchCount;
	unsigned int task_type;
	int dump_wa;
	int panic_wa;
	int dump_jank;
	int time_in_D_state;
	bool isDonewa;
	bool isDonejank;
	bool isReportjank;
};
struct hungtask_concernlist_table {
	pid_t pid;
	int end;
};
struct hungtask_concernlist_tmptable {
	pid_t pid;
	char  name[TASK_COMM_LEN + 1];
};
struct hungtask_ignorelist_table {
	pid_t pid;
	int end;
	int exist;
};
struct task_hung_upload {
	char  name[TASK_COMM_LEN + 1];
	pid_t pid;
	pid_t tgid;
	int   flag;
	int   duration;
};
/************************GLOBAL VARIABLE DEFINITION**************************/
static struct rb_root list_tasks = RB_ROOT;	/*pid*/
static struct hungtask_concernlist_table        whitelist[CONCERNLIST_LENGTH];
static struct hungtask_concernlist_tmptable     whitetmplist[CONCERNLIST_LENGTH];
static struct hungtask_concernlist_table        janklist[CONCERNLIST_LENGTH];
static struct hungtask_ignorelist_table         ignorelist[IGNORELIST_LENGTH];
static bool whitelistEmpty = true;
static bool janklistEmpty = true;

/*the number of tasks checked*/
static int __read_mostly sysctl_hung_task_check_count = PID_MAX_LIMIT;
/*zero means infinite timeout - no checking done*/
static unsigned long __read_mostly huawei_hung_task_timeout_secs = CONFIG_DEFAULT_HUNG_TASK_TIMEOUT;
extern unsigned long __read_mostly sysctl_hung_task_timeout_secs;

/*static int __read_mostly sysctl_hung_task_warnings = MAX_DUMP_TIMES;*/
static int did_panic = 0;	/*if not 0, then check no more*/
/*static struct task_struct *watchdog_task;*/
/*should we panic (and reboot, if panic_timeout= is set)
when a hung task is detected:*/
extern unsigned int __read_mostly sysctl_hung_task_panic;
/*control the switch on/off at "/sys/kernel/hubngtask/enable"*/
static unsigned int hungtask_enable = HT_DISABLE;
static unsigned int hungtask_watchdog_nosched_enable = 0;
static unsigned int hungtask_watchdog_nosched_firstkick = 0;
static unsigned int hungtask_watchdog_nosched_status = 1;
static unsigned int hungtask_watchdog_nosched_vmrebooting = 0;

/*used for white list D state detect*/
static unsigned int whitelist_type = WHITE_LIST;
static int whitelist_dump_cnt = DEFAULT_WHITE_DUMP_CNT;
static int whitelist_panic_cnt = DEFAULT_WHITE_PANIC_CNT;
/*used for jank D state dectect*/
static int jankproc_pids[JANK_TASK_MAXNUM];
static int jankproc_pids_size;
static int topapp_pid;
//static int last_topapp_pid;
static int found_in_topapp = TOPAPP_HUNG_INIT;
static int topapp_hung_times = 0;
static int janklist_dump_cnt = DEFAULT_JANK_DUMP_CNT;
static int jank_report_limit;
/*used for app D state detect*/
static int zygote64_pid;
static int zygote_pid;
static int systemserver_pid;
static int watchdog_pid;
static int applist_dump_cnt = DEFAULT_APP_DUMP_CNT;
static int applist_panic_cnt = DEFAULT_APP_PANIC_CNT;
/*used for S state task that not be schedule for long time*/
static int noschedule_dump_cnt = DEFAULT_NOSCHEDULE_DUMP_CNT;
static int noschedule_panic_cnt = DEFAULT_NOSCHEDULE_PANIC_CNT;
static int topapp_hung_cnt = DEFAULT_WHITE_DUMP_CNT;
static int other_log_cnt = DEFAULT_OTHER_LOG_CNT;
static int watchdog_nosched_cnt = 0;
/*indicate ignorelist is generated or not, the list is used
to avoid too much D state task printing during dump*/
static int ignore_state = IGN_STATE_INIT;
static int ignorelist_state = IGNORELIST_EMPTY;
/*how many heartbeat happend after power on*/
static unsigned long cur_heartbeat;
static int report_zrhung_ID;
static int hung_task_dump_and_upload = 0;
static int time_since_upload = 0;
static int hung_task_must_panic = 0;
static int zrhung_WPhungtask_enable;
static struct task_hung_upload upload_hungtask;
static bool in_suspend = false;
static struct rtc_time jank_last_tm = {0};
static struct rtc_time jank_tm = {0};
enum hash_turn {
	HASH_TURN_END = 0,
	HASH_TURN_FIRST = 1,
	HASH_TURN_SECOND,
	HASH_TURN_THIRD,
	HASH_TURN_FOURTH,
	HASH_TURN_FIFTH
};

/****************************FUNCTION DEFINITION*************************/
static struct task_item *find_task(pid_t pid, struct rb_root *root);
static bool rcu_lock_break(struct task_struct *g, struct task_struct *t);
extern void sysrq_sched_debug_show(void);
#if defined(CONFIG_HW_CGROUP_INFO) && defined(CONFIG_HW_ZEROHUNG)
extern int freezer_info_show_messages(void);
#else
int freezer_info_show_messages(void) {return 0;}
#endif
extern int hwhungtask_get_backlight(void);

static void hungtask_print_all_ignorelist(void)
{
	pr_err("hungtask: the ignorelist is full which is abnormal for hungtask!\n");
}
static int ignorelist_hash_locate(pid_t pid, int goal, struct hungtask_ignorelist_table *whichlist)
{
	int ret = HASH_ERROR;
	int mid = 0, turn =  HASH_TURN_FIRST, end = 0;
	int valueExpect = 0, valueDisappoint = 0;

	if (pid <= 0 || !hungtask_enable)
		return HASH_ERROR;
	valueExpect = (goal == HASH_FIND) ? pid : 0;
	valueDisappoint = (goal == HASH_INSERT) ? pid : 0;
	ret = pid % HASH_FIRST;
	mid = pid / HASH_FIRST;
	while (turn) {
		if (whichlist[ret].pid == valueExpect) {
			whichlist[ret].pid = pid;
			if (valueDisappoint)
				whichlist[ret].end = HASH_END;
			return ret;
		} else if (whichlist[ret].pid == valueDisappoint ||
			(valueExpect && whichlist[ret].end == HASH_END)) {
			return HASH_ERROR;
		} else if (valueDisappoint)
			whichlist[ret].end = HASH_CONTINUE;
		turn++;
		if(turn == HASH_TURN_SECOND)
			ret = HASH_FIRST + (pid - mid) % HASH_SECOND;
		else if (turn == HASH_TURN_THIRD)
			ret = HASH_FIRST + HASH_SECOND + pid % HASH_THIRD;
		else if (turn == HASH_TURN_FOURTH)
			ret = HASH_FIRST + HASH_SECOND + HASH_THIRD;
		else if (turn == HASH_TURN_FIFTH) {
			ret++;
			turn = HASH_TURN_FOURTH;
			if (ret == IGNORELIST_LENGTH) {
				ret = 0;
				end++;
			}
			if(end == HASH_TURN_SECOND) {
				if (HASH_INSERT == goal) {
					pr_err("hungtask: No room to store another hung task!\n");
					ignorelist_state = IGNORELIST_FULL;
				}
				turn = HASH_TURN_END;
			}
		}
	}
	return HASH_ERROR;
}
static int hunglist_hash_locate(pid_t pid, int goal, struct hungtask_concernlist_table *whichlist)
{
	int ret = HASH_ERROR, turn = HASH_TURN_FIRST;
	int valueExpect = 0, valueDisappoint = 0;

	if (pid <= 0)
		return HASH_ERROR;
	valueExpect = (goal == HASH_FIND) ? pid : 0;
	valueDisappoint = (goal == HASH_INSERT) ? pid : 0;
	ret = pid % CONCERNLIST_LENGTH;
	while (turn) {
		if (whichlist[ret].pid == valueExpect) {
			whichlist[ret].pid = pid;
			if (valueDisappoint)
				whichlist[ret].end = HASH_END;
			return ret;
		} else if (whichlist[ret].pid == valueDisappoint ||
			(valueExpect && whichlist[ret].end == HASH_END)) {
			return HASH_ERROR;
		} else if (valueDisappoint)
			whichlist[ret].end = HASH_CONTINUE;
		ret++;
		if (ret == CONCERNLIST_LENGTH) {
			ret = 0;
			turn++;
		}
		if (turn == HASH_TURN_THIRD)
			turn = 0;
	}
	pr_err("hungtask: No room to store another list item.\n");
	return HASH_ERROR;
}
static void hash_lists_init(void)
{
	memset(whitelist, 0, sizeof(whitelist));
	memset(whitetmplist, 0, sizeof(whitetmplist));
	memset(janklist, 0, sizeof(janklist));
	memset(ignorelist, 0, sizeof(ignorelist));
}
static void empty_whitelist(void)
{
	memset(whitelist, 0 , sizeof(whitelist));
}
static void empty_janklist(void)
{
	memset(janklist, 0 , sizeof(janklist));
}
static pid_t get_pid_by_name(const char *name)
{
	int max_count = sysctl_hung_task_check_count;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g, *t;
	int pid = 0;

	rcu_read_lock();
	do_each_thread(g, t) {
		if (!max_count--)
			goto unlock_f;
		if (!--batch_count) {
			batch_count = HUNG_TASK_BATCHING;
			if (!rcu_lock_break(g, t))
				goto unlock_f;
		}
		if (!strncmp(t->comm, name, TASK_COMM_LEN)) {
/*the function is used to match whitelist, janklist, for system_server,
some thread has the same name as main thread, so we use tgid*/
			pid = t->tgid;
			goto unlock_f;
		}
	} while_each_thread(g, t);

unlock_f:
	rcu_read_unlock();
	return pid;
}
static int get_task_type(pid_t pid, pid_t tgid, struct task_struct *parent)
{
	unsigned int flag = TASK_TYPE_IGNORE;

	/*check tgid of it's parent as PPID*/
	if (parent) {
		pid_t ppid = parent->tgid;

		if (PID_KTHREAD == ppid) {
			flag |= TASK_TYPE_KERNEL;
		} else if ((ppid == zygote_pid || ppid == zygote64_pid) &&
			   tgid != systemserver_pid) {
			flag |= TASK_TYPE_APP;
		} else if (ppid == PID_INIT) {
			flag |= TASK_TYPE_NATIVE;
		}
	}
	if (!whitelistEmpty && hunglist_hash_locate(tgid, HASH_FIND, whitelist) != HASH_ERROR) {
		flag |= TASK_TYPE_WHITE;
		/*pr_err("hungtask: Task: %d in whitelist found in D state\n", pid);*/
	}
	if (!janklistEmpty && hunglist_hash_locate(tgid, HASH_FIND, janklist) != HASH_ERROR)
		flag |= TASK_TYPE_JANK;

	return flag;
}
static void refresh_zygote_pids(void)
{
	int max_count = sysctl_hung_task_check_count;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g, *t;

	hungtask_watchdog_nosched_vmrebooting = 1;
	rcu_read_lock();
	do_each_thread(g, t) {
		if (!max_count--)
			goto unlock_f;
		if (!--batch_count) {
			batch_count = HUNG_TASK_BATCHING;
			if (!rcu_lock_break(g, t))
				goto unlock_f;
		}
		if (!strncmp(t->comm, "main", TASK_COMM_LEN) &&
			systemserver_pid) {
			if (zygote64_pid && !zygote_pid && t->tgid != zygote64_pid) {
				zygote_pid = t->tgid;
				pr_err("hungtask: zygote pid-%d.\n", zygote_pid);
			}
		} else if (!strncmp(t->comm, "system_server", TASK_COMM_LEN)) {
			systemserver_pid = t->tgid;
			if (t->pid == t->tgid) {
				zygote64_pid = t->real_parent->tgid;
				zygote_pid = 0;
				pr_err("hungtask: zygote64 pid-%d, system_server pid-%d.\n", zygote64_pid, systemserver_pid);
			}
		} else if (!strncmp(t->comm, "watchdog", TASK_COMM_LEN)) {
			if (systemserver_pid && systemserver_pid == t->tgid) {
				hungtask_watchdog_nosched_vmrebooting = 0;
				watchdog_pid = t->pid;
				pr_err("hungtask: watchdog pid-%d.\n", watchdog_pid);
			}
		}
	} while_each_thread(g, t);
unlock_f:
	rcu_read_unlock();
}
static int refresh_pids(void)
{
	int i = 0, ret = 0, hash_index = 0;

	empty_whitelist();
	for (i = 0; i < CONCERNLIST_LENGTH; i++) {
		if(strlen(whitetmplist[i].name) > 0) {
			pr_err("hungtask: whitetmplist[%d].name %s.\n", i, whitetmplist[i].name);
			whitetmplist[i].pid = get_pid_by_name(whitetmplist[i].name);
			hash_index = hunglist_hash_locate(whitetmplist[i].pid, HASH_INSERT, whitelist);
			if (hash_index != HASH_ERROR)
				pr_err("hungtask: whitelist member%d--%s-%d.\n",i ,
					whitetmplist[i].name, whitelist[hash_index].pid);
		}
	}

    /*refresh janklist*/
	empty_janklist();
	if (topapp_pid) {
		hunglist_hash_locate(topapp_pid, HASH_INSERT, janklist);
	}
	for (i = 0; i < jankproc_pids_size; i++) {
		hash_index = hunglist_hash_locate(jankproc_pids[i], HASH_INSERT, janklist);
		if (hash_index > 0 && janklist[hash_index].pid > 0)
			janklistEmpty = false;
	}
	/*refresh zygote_pid and zygote64_pid*/
	refresh_zygote_pids();
	return ret;
}

static struct task_item *find_task(pid_t pid, struct rb_root *root)
{
	struct rb_node **p = &root->rb_node;
	struct task_item *cur = NULL;

	while (*p) {
		struct rb_node *parent = NULL;

		parent = *p;
		cur = rb_entry(parent, struct task_item, node);
		if (!cur)
			return NULL;
		if (pid < cur->pid)
			p = &(*p)->rb_left;
		else if (pid > cur->pid)
			p = &(*p)->rb_right;
		else
			return cur;
	}
	return NULL;
}
static bool insert_task(struct task_item *item, struct rb_root *root)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent = NULL;
	struct task_item *cur = NULL;

	while (*p) {
		parent = *p;

		cur = rb_entry(parent, struct task_item, node);
		if (!cur)
			return false;
		if (item->pid < cur->pid)
			p = &(*p)->rb_left;
		else if (item->pid > cur->pid)
			p = &(*p)->rb_right;
		else {
			pr_info("hungtask: insert failed due to already"
				" exist pid=%d,tgid=%d,name=%s,type=%d\n",
				item->pid, item->tgid, item->name,
				item->task_type);
			return false;
		}
	}
	rb_link_node(&item->node, parent, p);
	rb_insert_color(&item->node, root);
	/*if (item->task_type & (TASK_TYPE_WHITE | TASK_TYPE_JANK)) {
		pr_info("hungtask: insert success pid=%d,tgid=%d,name=%s,"
		"type=%d\n", item->pid, item->tgid, item->name, item->task_type);
	}*/
	return true;
}

void show_state_filter_ext(unsigned long state_filter)
{
	struct task_struct *g, *p;
	struct task_item *taskitem;
	int    exist_frozen = 0;

#if BITS_PER_LONG == 32
	printk(KERN_INFO "  task                PC stack   pid father\n");
#else
	printk(KERN_INFO
	       "  task                        PC stack   pid father\n");
#endif
	rcu_read_lock();
	for_each_process_thread(g, p) {
		/*
		 * reset the NMI-timeout, listing all files on a slow
		 * console might take a lot of time:
		 */
		touch_nmi_watchdog();
		if (((p->state == TASK_RUNNING) || (p->state & state_filter))
		    && (ignorelist_hash_locate(p->pid, HASH_FIND, ignorelist) == HASH_ERROR)) {
			taskitem = find_task(p->pid, &list_tasks);
			if (unlikely(p->flags & PF_FROZEN)) {
				exist_frozen++;
				if (taskitem) {
					pr_err("hungtask:name=%s,PID=%d,tgid=%d,tgname=%s FROZEN for %ds,"
						"type=%d,SP=0x%08lx,la:%llu/lq:%llu.\n",
						p->comm, p->pid, p->tgid, p->group_leader->comm,
						taskitem->time_in_D_state * HEARTBEAT_TIME,
						taskitem->task_type, p->thread.cpu_context.sp,
						p->sched_info.last_arrival,
						p->sched_info.last_queued);
				} else {
					pr_err("hungtask:name=%s,PID=%d,tgid=%d,tgname=%s a new FROZEN task"
						",SP=0x%08lx,la:%llu/lq:%llu.\n",
						p->comm, p->pid, p->tgid, p->group_leader->comm,
						p->thread.cpu_context.sp,
						p->sched_info.last_arrival,
						p->sched_info.last_queued);
				}
			} else {
				if (taskitem) {
					pr_err("hungtask:name=%s,PID=%d,tgid=%d,tgname=%s,type=%d blocked %ds,"
						"SP=0x%08lx,la:%llu/lq:%llu.\n", taskitem->name,
						taskitem->pid, p->tgid, p->group_leader->comm, taskitem->task_type,
						taskitem->time_in_D_state * HEARTBEAT_TIME,
						p->thread.cpu_context.sp,
						p->sched_info.last_arrival, p->sched_info.last_queued);
				} else {
					pr_err("hungtask:name=%s,PID=%d,tgid=%d,tgname=%s,SP=0x%08lx,la:%llu/lq:%llu.\n",
						p->comm, p->pid, p->tgid,
						p->group_leader->comm, p->thread.cpu_context.sp,
						p->sched_info.last_arrival, p->sched_info.last_queued);
				}
				sched_show_task(p);
			}
		}
	}
	touch_all_softlockup_watchdogs();
#ifdef CONFIG_SCHED_DEBUG
#endif
	rcu_read_unlock();
	/*
	 * Only show locks if all tasks are dumped:
	 */
	if (TASK_UNINTERRUPTIBLE == state_filter || !state_filter)
		debug_show_all_locks();
	if (exist_frozen)
		freezer_info_show_messages();
	exist_frozen = 0;
}

void hwhungtask_show_state_filter(unsigned long state_filter)
{
	show_state_filter_ext(state_filter);
}
EXPORT_SYMBOL(hwhungtask_show_state_filter);

static void jank_print_task_wchan(struct task_struct *task)
{
	unsigned long wchan = 0;
	char symname[KSYM_NAME_LEN] = {0};
	char report_jank_text[REPORT_MSGLENGTH] = {0};

	wchan = get_wchan(task);
	if (lookup_symbol_name(wchan, symname) < 0) {
		if (!ptrace_may_access(task, PTRACE_MODE_READ_FSCREDS))
			return;
		pr_err("hungtask: Task %s pid:%d tgid: %d wchan:[<%08lx>].\n",
				task->comm, task->pid, task->tgid, wchan);
	} else {
		pr_err("hungtask: Task %s pid:%d tgid: %d wchan:[<%08lx>]-%s.\n",
				task->comm, task->pid, task->tgid, wchan, symname);
	}
	snprintf(report_jank_text, sizeof(report_jank_text),
			"hungtask-janklist Blocked task %s pid:%d tgid:%d tgname:%s wchan %s.\n",
			task->comm, task->pid, task->tgid, task->group_leader->comm, symname);
	zrhung_send_event(ZRHUNG_WP_HTSK_WARNING, NULL, report_jank_text);
}

static void do_dump_task(struct task_struct *task)
{
	sched_show_task(task);
	debug_show_held_locks(task);
}
static void do_dump(struct task_struct *task, int flag, int time_in_D_state)
{
	pr_err("hungtask: do_dump, flag=%d\n", flag);

	rcu_read_lock();
	if (!pid_alive(task)) {
		rcu_read_unlock();
		return;
	}
#ifdef CONFIG_HUAWEI_PRINTK_CTRL
	printk_level_setup(LOGLEVEL_DEBUG);
#endif

	if (flag & (FLAG_DUMP_WHITE | FLAG_DUMP_APP)) {
		int cnt = 0;

		trace_sched_process_hang(task);
		/*if (!sysctl_hung_task_warnings)
			return;
		if (sysctl_hung_task_warnings > 0)
			sysctl_hung_task_warnings--;*/
		cnt = time_in_D_state;
		pr_err("INFO: task %s:%d tgid:%d blocked more than %d seconds"
			" in %s.\n", task->comm, task->pid, task->tgid,
			(HEARTBEAT_TIME * cnt),
			(flag & FLAG_DUMP_WHITE) ? "whitelist" : "applist");
		/*should this RDR hook bind with dump or panic---TBD ??*/
#ifdef CONFIG_HISI_BB
		hung_task_hook((void *)task,
			       (u32) sysctl_hung_task_timeout_secs);
#endif
		pr_err("      %s %s %.*s\n",
		       print_tainted(), init_utsname()->release,
		       (int)strcspn(init_utsname()->version, " "),
		       init_utsname()->version);
		pr_err("\"echo 0 > /proc/sys/kernel/hung_task_timeout_secs\""
		       " disables this message.\n");
		do_dump_task(task);
		touch_nmi_watchdog();
		if (flag & FLAG_DUMP_WHITE && (!hung_task_dump_and_upload)) {
			hung_task_dump_and_upload++;
			upload_hungtask.pid = task->pid;
			upload_hungtask.tgid = task->tgid;
			upload_hungtask.duration = time_in_D_state;
			memset(upload_hungtask.name, 0,sizeof(upload_hungtask.name));
			strncpy(upload_hungtask.name, task->comm, sizeof(task->comm));
			upload_hungtask.flag = flag;
			if (task->flags & PF_FROZEN)
				upload_hungtask.flag |= FLAG_PF_FROZEN;
		}
	} else if (flag & FLAG_DUMP_JANK) {
		do_dump_task(task);
	}

#ifdef CONFIG_HUAWEI_PRINTK_CTRL
	printk_level_setup(sysctl_printk_level);
#endif
	rcu_read_unlock();
}
static void do_panic(void)
{
	if (sysctl_hung_task_panic) {
		trigger_all_cpu_backtrace();
		panic("hungtask: blocked tasks");
	}
}

static void create_taskitem(struct task_item *taskitem, struct task_struct *task)
{
	taskitem->pid = task->pid;
	taskitem->tgid = task->tgid;
	memset(taskitem->name, 0, sizeof(taskitem->name));
	strncpy(taskitem->name, task->comm, sizeof(task->comm));
	taskitem->switchCount = task->nvcsw + task->nivcsw;
	taskitem->dump_wa = 0;	/*whitelist or applist task dump times*/
	taskitem->panic_wa = 0;	/*whitelist or applist task panic times*/
	taskitem->dump_jank = 0;	/*janklist task dump times*/
	taskitem->time_in_D_state = -1;/*D time_cnt is 1 less than dump_wa or dump_jank*/
	taskitem->isDonewa = true;	/*if task in  white or app dealed */
	taskitem->isDonejank = true;	/*if task in  jank dealed */
	taskitem->isReportjank = false;
}
static bool refresh_task(struct task_item *taskitem, struct task_struct *task)
{
	bool is_called = false;

	if (taskitem->switchCount != (task->nvcsw + task->nivcsw)) {
		taskitem->switchCount = task->nvcsw + task->nivcsw;
		is_called = true;
		return is_called;
	}

	if (taskitem->task_type & TASK_TYPE_WHITE) {
		taskitem->isDonewa = false;
		taskitem->dump_wa++;
		taskitem->panic_wa++;
	} else if (taskitem->task_type & TASK_TYPE_APP) {
		taskitem->isDonewa = false;
		taskitem->dump_wa++;
		taskitem->panic_wa++;
	}
	if (taskitem->task_type & TASK_TYPE_JANK) {
		taskitem->isDonejank = false;
		taskitem->dump_jank++;
	} else if (!(taskitem->task_type & (TASK_TYPE_WHITE | TASK_TYPE_APP))) {
		taskitem->dump_wa++;
		taskitem->isDonewa = false;
	}
	taskitem->time_in_D_state++;

	if (task->flags & PF_FROZEN) {
		taskitem->task_type |= TASK_TYPE_FROZEN;
	}
	return is_called;
}
static void generate_ignorelist(struct task_struct *t)
{
	int hashIndex = 0;

	if (IGN_STATE_INIT == ignore_state && cur_heartbeat > ONE_MINUTE) {
		ignorelist_hash_locate(t->pid, HASH_INSERT, ignorelist);
	} else if (IGN_STATE_FIRST == ignore_state &&
		cur_heartbeat <= ONE_AND_HALF_MINUTE) {
			hashIndex = ignorelist_hash_locate(t->pid, HASH_FIND, ignorelist);
			if (hashIndex != HASH_ERROR)
				ignorelist[hashIndex].exist++;
	}
}
static void remove_list_tasks(struct task_item *item)
{
	/*if(item->task_type & (TASK_TYPE_WHITE | TASK_TYPE_JANK)) {
		pr_info("hungtask: remove from list_tasks pid=%d,tgid=%d,"
			"name=%s\n", item->pid, item->tgid, item->name);
	}*/
	rb_erase(&item->node, &list_tasks);
	kfree(item);
}
static void shrink_list_tasks(void)
{
	bool found = false;

	do {
		struct rb_node *n;
		struct task_item *item = NULL;

		found = false;
		for (n = rb_first(&list_tasks); n != NULL; n = rb_next(n)) {
			item = rb_entry(n, struct task_item, node);
			if (!item)
				continue;
			if (item->isDonewa && item->isDonejank) {
				found = true;
				break;
			}
		}
		if (found)
			remove_list_tasks(item);
	} while (found);
}
static void check_parameters(void)
{
	if (whitelist_dump_cnt < 0 || whitelist_dump_cnt > DEFAULT_WHITE_DUMP_CNT)
		whitelist_dump_cnt = DEFAULT_WHITE_DUMP_CNT;
	if (whitelist_panic_cnt <= 0 || whitelist_panic_cnt > DEFAULT_WHITE_PANIC_CNT)
		whitelist_panic_cnt = DEFAULT_WHITE_PANIC_CNT;
	if (applist_dump_cnt < 0 || applist_dump_cnt > MAX_LOOP_NUM)
		applist_dump_cnt = DEFAULT_APP_DUMP_CNT;
	if (applist_panic_cnt != DEFAULT_APP_PANIC_CNT)
		applist_panic_cnt = DEFAULT_APP_PANIC_CNT;
	if (noschedule_panic_cnt < 0 /*DEFAULT_NOSCHEDULE_PANIC_CNT*/ ||
		noschedule_panic_cnt > MAX_LOOP_NUM) {
		noschedule_panic_cnt = DEFAULT_NOSCHEDULE_PANIC_CNT;
	}
	if (janklist_dump_cnt < 0 || janklist_dump_cnt > MAX_LOOP_NUM)
		janklist_dump_cnt = DEFAULT_JANK_DUMP_CNT;
}
#ifdef CONFIG_HW_ZEROHUNG
static void hungtask_report_zrhung(int event)
{
	bool report_load = false;
	char report_buf_tag[REPORT_MSGLENGTH] = {0};
	char report_buf_tag_cmd[REPORT_MSGLENGTH + 2] = {0};
	char report_buf_text[REPORT_MSGLENGTH] = {0};
	char report_name[TASK_COMM_LEN + 1] = {0};
	char cmd[MAX_ZRHUNG_CMD_BUF_SIZE] = {0};
	int report_pid = 0, report_hungtime = 0, report_tasktype = 0;

	if (zrhung_WPhungtask_enable != ZRHUNG_HUNGTASK_ENABLE)
		return;
	if (!event)
		return;

	if (event & HUNGTASK_EVENT_WHITELIST) {
		snprintf(report_buf_tag, sizeof(report_buf_tag), "hungtask_whitelist_%d", report_zrhung_ID);
		strncpy(report_name, upload_hungtask.name, TASK_COMM_LEN);
		report_pid = upload_hungtask.pid;
		report_tasktype = TASK_TYPE_WHITE;
		report_hungtime = whitelist_dump_cnt * HEARTBEAT_TIME;
		report_load = true;
	} else if (event & HUNGTASK_EVENT_LAZYWATCHDOG) {
		snprintf(report_buf_tag, sizeof(report_buf_tag), "hungtask_lazydog_%d", report_zrhung_ID);
		snprintf(report_name, TASK_COMM_LEN, "vmwatchdog");
		report_pid = watchdog_pid;
		report_tasktype = TASK_TYPE_NOSCHEDULE;
		report_hungtime = watchdog_nosched_cnt * HEARTBEAT_TIME;
		upload_hungtask.flag = FLAG_DUMP_NOSCHEDULE;
		report_load = true;
	} else if (event & HUNGTASK_EVENT_TOPAPP) {
		snprintf(report_buf_tag, sizeof(report_buf_tag), "hungtask_topapp_%d", report_zrhung_ID);
		strncpy(report_name, upload_hungtask.name, TASK_COMM_LEN);
		report_pid = upload_hungtask.pid;
		report_tasktype = TASK_TYPE_APP;
		report_hungtime = topapp_hung_cnt * HEARTBEAT_TIME;
		if (upload_hungtask.duration < TWO_MINUTES || (0 == upload_hungtask.duration % HUNG_ONE_HOUR))
			report_load = true;
	} else
		pr_err("hungtask: No such event report to zerohung!");

	pr_err("hungtask: %s start.\n", report_buf_tag);
	if (event & HUNGTASK_EVENT_WHITELIST)
		pr_err("hungtask: HUNGTASK_EVENT_WHITELIST happens , report to zrhung!\n");
	if (event & HUNGTASK_EVENT_LAZYWATCHDOG) {
		pr_err("hungtask: HUNGTASK_EVENT_LAZYWATCHDOG happens in this cycle!\n");
		snprintf(cmd, MAX_ZRHUNG_CMD_BUF_SIZE, "P=%d", systemserver_pid);
		zrhung_send_event(ZRHUNG_WP_HUNGTASK, cmd, "hungtask: vmreboot watchdog not scheduled for 39s.");
	}
	if (event & HUNGTASK_EVENT_TOPAPP)
		pr_err("hungtask: HUNGTASK_EVENT_TOPAPP happens in this cycle!\n");
	if (upload_hungtask.flag & FLAG_PF_FROZEN) {
		snprintf(report_buf_text, sizeof(report_buf_text),
				"Task %s(%s) pid %d type %d blocked %ds.",
				report_name, "FROZEN", report_pid,
				report_tasktype, report_hungtime);
	} else
		snprintf(report_buf_text, sizeof(report_buf_text),
				"Task %s pid %d type %d blocked %ds.",
				report_name, report_pid, report_tasktype, report_hungtime);

	if (report_load) {
		show_state_filter_ext(TASK_UNINTERRUPTIBLE);
		pr_err("hungtask: %s end.\n", report_buf_tag);
		snprintf(report_buf_tag_cmd, sizeof(report_buf_tag_cmd), "R=%s", report_buf_tag);
		zrhung_send_event(ZRHUNG_WP_HUNGTASK, report_buf_tag_cmd, report_buf_text);
		report_zrhung_ID++;
	}
}
#endif
static int hungtask_check_topapp(void)
{
	if (found_in_topapp == TOPAPP_HUNG_INIT)
		topapp_hung_times = 0;
	if (topapp_hung_times > topapp_hung_cnt) {
		topapp_hung_times = 0;
		if (hwhungtask_get_backlight() > 0)
			return HUNGTASK_EVENT_TOPAPP;
	}
	return 0;
}
static int watchdog_nosched_check(void)
{
	int ret = 0;
#ifdef CONFIG_HW_ZEROHUNG
	char cmd[MAX_ZRHUNG_CMD_BUF_SIZE] = {0};
#endif
	if (!hungtask_watchdog_nosched_enable || !hungtask_watchdog_nosched_firstkick)
		return ret;

	if(watchdog_nosched_cnt >= noschedule_dump_cnt) {
		refresh_zygote_pids();
		if (hungtask_watchdog_nosched_vmrebooting) {
			pr_err("hungtask: watchdog nowhere probably because vmrebooting.\n");
			watchdog_nosched_cnt = 0;
			hungtask_watchdog_nosched_vmrebooting = 0;
			return ret;
		}
		if (watchdog_nosched_cnt == noschedule_dump_cnt) {
			pr_err("hungtask: vmreboot watchdog not scheduled for more"
				"than %d seconds, notify zerohung.\n",
				noschedule_dump_cnt * HEARTBEAT_TIME);
			watchdog_nosched_cnt++;
			return HUNGTASK_EVENT_LAZYWATCHDOG;
		}
		if(watchdog_nosched_cnt > noschedule_panic_cnt) {
			pr_err("hungtask: vmreboot watchdog not scheduled for more"
				"than %d seconds.\n", noschedule_panic_cnt * HEARTBEAT_TIME);
#ifdef CONFIG_HW_ZEROHUNG
			snprintf(cmd, MAX_ZRHUNG_CMD_BUF_SIZE, "S,P=%d", systemserver_pid);
			zrhung_send_event(ZRHUNG_WP_HUNGTASK, cmd, "hungtask: vmreboot watchdog not scheduled for 120s.");
#endif
			watchdog_nosched_cnt = 0;
			pr_err("hungtask: print all running cpu stack and"
				" D state stack due to systemserver watchdog"
				" thread not schedule\n");
			show_state_filter_ext(TASK_UNINTERRUPTIBLE);
			do_panic();
		}
	}
	if (hungtask_watchdog_nosched_status) {
		watchdog_nosched_cnt = 0;
		hungtask_watchdog_nosched_status = 0;
	}
	watchdog_nosched_cnt++;
	return ret;
}
static void post_process(void)
{
	struct rb_node *n;
	int i = 0;
	int hungevent = 0, err = 0;
	char config_str[ZRHUNG_CONFIG_LENGTH] = {0};

	if (hung_task_dump_and_upload == HUNG_TASK_UPLOAD_ONCE) {
		hungevent |= HUNGTASK_EVENT_WHITELIST;
		hung_task_dump_and_upload++;
	}
	if (hung_task_dump_and_upload > 0) {
		time_since_upload++;
		if (time_since_upload > (whitelist_panic_cnt - whitelist_dump_cnt)) {
			hung_task_dump_and_upload = 0;
			time_since_upload = 0;
		}
	}
	if (hung_task_must_panic) {
		show_state_filter_ext(TASK_UNINTERRUPTIBLE);
		hung_task_must_panic = 0;
		pr_err("hungtask: The whitelist task %s:%d blocked more than %ds is causing panic.\n",
			upload_hungtask.name, upload_hungtask.pid,
			whitelist_panic_cnt * HEARTBEAT_TIME);
		do_panic();
	}

	if (IGN_STATE_INIT == ignore_state && cur_heartbeat > ONE_MINUTE) {
		ignore_state = IGN_STATE_FIRST;
	} else if (IGN_STATE_FIRST == ignore_state && cur_heartbeat <=
		ONE_AND_HALF_MINUTE) {
		for (i = 0; i < IGNORELIST_LENGTH; i++) {
			if(ignorelist[i].exist == 0) {
				ignorelist[i].pid = 0;
			} else
				ignorelist[i].exist = 0;
		}
	} else if (ignore_state == IGN_STATE_FIRST && cur_heartbeat > ONE_AND_HALF_MINUTE) {
		ignore_state = IGN_STATE_DONE;
	}

	shrink_list_tasks();
	for (n = rb_first(&list_tasks); n != NULL; n = rb_next(n)) {
		struct task_item *item = rb_entry(n, struct task_item, node);
		item->isDonewa = true;
		item->isDonejank = true;
	}

	hungevent |= hungtask_check_topapp();
	hungevent |= watchdog_nosched_check();
#ifdef CONFIG_HW_ZEROHUNG
	if (hungevent) {
		err = zrhung_get_config(ZRHUNG_WP_HUNGTASK, config_str, sizeof(config_str));
		if (err)
			pr_err("hungtask: get zrhung config failed.\n");
		else
			zrhung_WPhungtask_enable = (int)config_str[0];
		hungtask_report_zrhung(hungevent);
	}
#endif
}

/*
 * To avoid extending the RCU grace period for an unbounded amount of time,
 * periodically exit the critical section and enter a new one.
 *
 * For preemptible RCU it is sufficient to call rcu_read_unlock in order
 * to exit the grace period. For classic RCU, a reschedule is required.
 */
static bool rcu_lock_break(struct task_struct *g, struct task_struct *t)
{
	bool can_cont = false;

	get_task_struct(g);
	get_task_struct(t);
	rcu_read_unlock();
	cond_resched();
	rcu_read_lock();
	can_cont = pid_alive(g) && pid_alive(t);
	put_task_struct(t);
	put_task_struct(g);

	return can_cont;
}
static int dump_task_wa(struct task_item *item, int dump_cnt,
				struct task_struct *task,  int flag)
{
	int ret = 0;

	if (item->time_in_D_state > TWO_MINUTES &&
		(item->time_in_D_state % TWO_MINUTES != 0))
		return ret;
	if (item->time_in_D_state > HUNG_TEN_MINUTES &&
		(item->time_in_D_state % HUNG_TEN_MINUTES != 0))
		return ret;
	if (item->time_in_D_state > HUNG_ONE_HOUR &&
                (item->time_in_D_state % HUNG_ONE_HOUR != 0))
		return ret;
	if (dump_cnt && item->dump_wa > dump_cnt) {
		item->dump_wa = 1;
		if (!hung_task_dump_and_upload) {
			if (task->flags & PF_FROZEN) {
#ifndef CONFIG_FINAL_RELEASE
				pr_err("hungtask: Task %s:%d tgid:%d type:%d is FROZEN for %ds.\n",
						item->name, item->pid, item->tgid, item->task_type,
						item->time_in_D_state * HEARTBEAT_TIME);
#endif
			} else {
				pr_err("hungtask: Ready to dump a task %s.\n", item->name);
				do_dump(task, flag, item->time_in_D_state);
				ret++;
			}
		}
	}
	return ret;
}
static void check_topapp_hung(struct task_item *item)
{
	if (item->pid == topapp_pid) {
		if (found_in_topapp == TOPAPP_HUNG_INIT) {
			found_in_topapp = TOPAPP_HUNG_FOUND;
			if (!hung_task_dump_and_upload) {
				upload_hungtask.pid = item->pid;
				upload_hungtask.tgid = item->tgid;
				upload_hungtask.duration = item->time_in_D_state;
				memset(upload_hungtask.name, 0,sizeof(upload_hungtask.name));
				strncpy(upload_hungtask.name, item->name, sizeof(item->name));
				if (item->task_type & TASK_TYPE_FROZEN)
					upload_hungtask.flag |= FLAG_PF_FROZEN;
			}
		}
		if (found_in_topapp == TOPAPP_HUNG_FOUND) {
			found_in_topapp = TOPAPP_HUNG_RECORDED;
			topapp_hung_times++;
		}
	}
}
static void deal_task(struct task_item *item, struct task_struct *task, bool is_called)
{
	int any_dumped_num = 0;

	if (is_called) {
		item->dump_wa = 1;
		item->panic_wa = 1;
		item->dump_jank = 1;
		item->time_in_D_state = 0;
		return;
	}
	if (item->task_type & TASK_TYPE_WHITE) {
		any_dumped_num =
			dump_task_wa(item, whitelist_dump_cnt, task, FLAG_DUMP_WHITE);
	} else if (item->task_type & TASK_TYPE_APP) {
		any_dumped_num =
			dump_task_wa(item, applist_dump_cnt, task, FLAG_DUMP_APP);
	}
	if (item->task_type & TASK_TYPE_JANK && janklist_dump_cnt) {
		if (item->dump_jank > janklist_dump_cnt
				&& false == item->isReportjank
				&& !hung_task_dump_and_upload) {
			LOG_JANK_D(JLID_KERNEL_HUNG_TASK,
				"#ARG1:<%s>#ARG2:<%d>", item->name,
				(item->dump_jank - 1) * HEARTBEAT_TIME);
			item->isReportjank = true;
			if (jank_report_limit < JANK_REPORT_LIMIT_PERDAY) {
				jank_print_task_wchan(task);
				do_dump(task, FLAG_DUMP_JANK, item->time_in_D_state);
				jank_report_limit++;
			}
		}
	} else if (!(item->task_type & (TASK_TYPE_WHITE | TASK_TYPE_APP))) {
		if (item->dump_wa > other_log_cnt && item->time_in_D_state < HUNG_ONE_HOUR) {
#ifndef CONFIG_FINAL_RELEASE
			pr_err("hungtask: Unconcerned task %s:%d blocked more than %d seconds\n",
				item->name, item->pid, item->time_in_D_state * HEARTBEAT_TIME);
#endif
			item->dump_wa = 1;
			any_dumped_num++;
		}
	}
#ifdef CONFIG_TZDRIVER
	if (any_dumped_num && is_tee_hungtask(task)) {
		pr_info("hungtask: related to teeos was detected, dump status of teeos\n");
		wakeup_tc_siq();
	}
#endif

	if (!is_called && (item->task_type & TASK_TYPE_WHITE)) {
		if(whitelist_panic_cnt && item->panic_wa > whitelist_panic_cnt) {
			pr_err("hungtask: A whitelist task %s is about to cause panic.\n", item->name);
			item->panic_wa = 0;
			hung_task_must_panic++;
		} else
			item->isDonewa = false;
	}
	check_topapp_hung(item);
	if (item->isDonewa && item->isDonejank)
		remove_list_tasks(item);
}

static bool check_conditions(struct task_struct *task, unsigned int task_type)
{
	bool needNoCheck = true;

	if (task->flags & PF_FROZEN && in_suspend) {
		return needNoCheck;
	}

	if (task_type & TASK_TYPE_WHITE && (whitelist_dump_cnt ||
						whitelist_panic_cnt))
		needNoCheck = false;
	else if (task_type & TASK_TYPE_APP && (applist_dump_cnt ||
						applist_panic_cnt))
		needNoCheck = false;
	if (task_type & TASK_TYPE_JANK && janklist_dump_cnt)
		needNoCheck = false;
	else if (!(task_type & (TASK_TYPE_WHITE | TASK_TYPE_APP)) && ignore_state == IGN_STATE_DONE)
		needNoCheck = false;

	return needNoCheck;
}
static void get_localtime(struct rtc_time *tm)
{
	struct timex  txc;

	do_gettimeofday(&(txc.time));
	txc.time.tv_sec -= (int)(sys_tz.tz_minuteswest * 60);
	rtc_time_to_tm(txc.time.tv_sec,tm);
}
static void jank_refresh_limit_check(void)
{
	get_localtime(&jank_tm);
	if((jank_tm.tm_year != jank_last_tm.tm_year)
			||(jank_tm.tm_mon != jank_last_tm.tm_mon)
			|| (jank_tm.tm_mday != jank_last_tm.tm_mday)){
		jank_report_limit = 0;
	}
	memcpy(&jank_last_tm, &jank_tm, sizeof(struct rtc_time));
}
void check_hung_tasks_proposal(unsigned long timeout)
{
	int max_count = sysctl_hung_task_check_count;
	int batch_count = HUNG_TASK_BATCHING;
	struct task_struct *g, *t;
	static int do_refresh = 0;

	if(!hungtask_enable)
		return;
	if (test_taint(TAINT_DIE) || did_panic) {
		pr_err("hungtask: heartbeart=%ld, it's going to panic, "
			"ignore this heartbeart\n", cur_heartbeat);
		return;
	}
	cur_heartbeat++;
	if((cur_heartbeat % REFRESH_INTERVAL) == 0) {
		pr_info("hungtask: The huawei hungtask detect is running.\n");
		if (unlikely(IGNORELIST_FULL == ignorelist_state)) {
			hungtask_print_all_ignorelist();
		}
		do_refresh = 1;
	} else
		do_refresh = 0;

	if(do_refresh || cur_heartbeat < TIME_REFRESH_PIDS) {
		refresh_pids();
		check_parameters();
	}
	found_in_topapp = TOPAPP_HUNG_INIT;

	rcu_read_lock();
	for_each_process_thread(g, t) {
		bool is_called = false;

		if (!max_count--)
			goto unlock;
		if (!--batch_count) {
			batch_count = HUNG_TASK_BATCHING;
			if (!rcu_lock_break(g, t))
				goto unlock;
		}

		if (t->state == TASK_UNINTERRUPTIBLE) {
			unsigned int task_type = TASK_TYPE_IGNORE;
			unsigned long switch_count = t->nvcsw + t->nivcsw;
			struct task_item *taskitem;
			int hashIndex  = HASH_ERROR;

			/*
			* When a freshly created task is scheduled once,
			*changes its state to TASK_UNINTERRUPTIBLE without
			*having ever been switched out once, it musn't
			*be checked.
			*/
			if (unlikely(!switch_count)) {
				pr_info("hungtask: heartbeart=%ld, switch_count"
					" is zero, ignore this heartbeart\n", cur_heartbeat);
				continue;
			}
			if (ignore_state == IGN_STATE_DONE) {
				hashIndex = ignorelist_hash_locate(t->pid, HASH_FIND, ignorelist);
				if (hashIndex != HASH_ERROR)
					continue;
			} else
				generate_ignorelist(t);
			taskitem = find_task(t->pid, &list_tasks);
			if (taskitem) {
				if (check_conditions(t, taskitem->task_type))
					continue;
				is_called = refresh_task(taskitem, t);
			} else {
				task_type = get_task_type(t->pid,
						t->tgid, t->real_parent);
				if (check_conditions(t, task_type))
					continue;
				taskitem = kmalloc(sizeof(*taskitem),
								GFP_ATOMIC);
				if (!taskitem) {
					pr_err("hungtask: kmalloc failed");
					continue;
				}
				memset(taskitem, 0, sizeof(*taskitem));
				taskitem->task_type = task_type;
				create_taskitem(taskitem, t);
				is_called = refresh_task(taskitem, t);
				insert_task(taskitem, &list_tasks);
			}
			deal_task(taskitem, t, is_called);
		}
	}
unlock:
	rcu_read_unlock();
	post_process();
	if (do_refresh)
		jank_refresh_limit_check();
}
static ssize_t watchdog_nosched_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	if (hungtask_watchdog_nosched_status)
		return snprintf(buf, ENABLE_SHOW_LEN, "kick\n");
	if (hungtask_watchdog_nosched_enable)
		return snprintf(buf, ENABLE_SHOW_LEN, "on\n");
	else
		return snprintf(buf, ENABLE_SHOW_LEN, "off\n");
}
static ssize_t watchdog_nosched_store(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	char tmp[6];
	size_t len = 0;
	char *p;

	if ((count < 2) || (count > (sizeof(tmp) - 1))) {
		pr_err("hungtask: string too long or too short.\n");
		return -EINVAL;
	}
	if (!buf)
		return -EINVAL;
	p = memchr(buf, '\n', count);
	len = p ? (size_t) (p - buf) : count;
	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, buf, len);
	if (strncmp(tmp, "on", strlen(tmp)) == 0) {
		hungtask_watchdog_nosched_enable = 1;
		pr_err("hungtask: watchdog_nosched_enable is set to enable.\n");
	} else if (unlikely(strncmp(tmp, "off", strlen(tmp)) == 0)) {
		hungtask_watchdog_nosched_enable = 0;
		pr_err("hungtask: watchdog_nosched_enable is set to disable.\n");
	} else if (likely(strncmp(tmp, "kick", strlen(tmp)) == 0)) {
		hungtask_watchdog_nosched_firstkick = 1;
		hungtask_watchdog_nosched_status = 1;
		/*pr_err("hungtask: watchdog_nosched_enable is kick.\n");*/
	} else
		pr_err("hungtask: only accept on off or kick !\n");

	return (ssize_t) count;
}
static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	if (hungtask_enable)
		return snprintf(buf, ENABLE_SHOW_LEN, "on\n");
	else
		return snprintf(buf, ENABLE_SHOW_LEN, "off\n");
}
static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	char tmp[6];
	size_t len = 0;
	char *p;

	if ((count < 2) || (count > (sizeof(tmp) - 1))) {
		pr_err("hungtask: string too long or too short.\n");
		return -EINVAL;
	}
	if (!buf)
		return -EINVAL;
	p = memchr(buf, '\n', count);
	len = p ? (size_t) (p - buf) : count;
	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, buf, len);
	if (strncmp(tmp, "on", strlen(tmp)) == 0) {
		hungtask_enable = HT_ENABLE;
		pr_info("hungtask: hungtask_enable is set to enable.\n");
	} else if (strncmp(tmp, "off", strlen(tmp)) == 0) {
		hungtask_enable = HT_DISABLE;
		pr_info("hungtask: hungtask_enable is set to disable.\n");
	} else
		pr_err("hungtask: only accept on or off !\n");

	return (ssize_t) count;
}
static ssize_t monitorlist_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	char *start = buf;
	char all_buf[WHITELIST_STORE_LEN - 20];
	int i = 0;
	unsigned long len = 0;

	memset(all_buf, 0, sizeof(all_buf));
	for (i = 0; i < CONCERNLIST_LENGTH; i++) {
		if (whitetmplist[i].pid > 0) {
			len += snprintf(all_buf + len, sizeof(all_buf) - len, "%s-%d,",
			whitetmplist[i].name, whitetmplist[i].pid);
			if (!(len < sizeof(all_buf))) {
				len = sizeof(all_buf) - 1;
				break;
			}
		}
	}
	if (len > 0)
		all_buf[len] = 0;
	/*pr_info("hungtask: show all_buf=%s\n", all_buf);*/
	if (whitelist_type == WHITE_LIST)
		buf += snprintf(buf, WHITELIST_STORE_LEN, "whitelist: [%s]\n",
				all_buf);
	else if (whitelist_type == BLACK_LIST)
		buf += snprintf(buf, WHITELIST_STORE_LEN, "blacklist: [%s]\n",
				all_buf);
	else
		buf += snprintf(buf, WHITELIST_STORE_LEN, "\n");

	return  buf - start;
}
/*
 *    monitorlist_store    -  Called when 'write/echo' method is
 *    used on entry '/sys/kernel/hungtask/monitorlist'.
 */
static ssize_t monitorlist_store(struct kobject *kobj,
				 struct kobj_attribute *attr, const char *buf,
				 size_t n)
{
	size_t len = 0;
	char *p;
	char all_buf[WHITELIST_STORE_LEN];
	char *cur = all_buf;
	int index = 0;

	if ((n < 2) || (n > (sizeof(all_buf) - 1))) {
		pr_err("hungtask: whitelist input string too long or too short.\n");
		return -EINVAL;
	}
	if (!buf)
		return -EINVAL;
	/*input format:
	*write /sys/kernel/hungtask/monitorlist "whitelist,
	*system_server,surfaceflinger"*/
	p = memchr(buf, '\n', n);
	len = p ? (size_t) (p - buf) : n;	/*exclude the '\n'*/

	memset(all_buf, 0, sizeof(all_buf));
	strncpy(all_buf, buf,
		len > WHITELIST_STORE_LEN ? WHITELIST_STORE_LEN : len);
	p = strsep(&cur, ",");
	if (!cur) {
		pr_err("hungtask: input string is not correct!\n");
		return -EINVAL;
	}
	if (!strncmp(p, "whitelist", n)) {
		whitelist_type = WHITE_LIST;
	} else{
		if (!strncmp(p, "blacklist", n))
			/*whitelist_type = BLACK_LIST;*/
			pr_err("hungtask: blacklist is not support!\n");
		else
			pr_err("hungtask: wrong list type is set!\n");
		return -EINVAL;
	}
	if (!strlen(cur)) {
		pr_err("hungtask: at least one process need to be set!\n");
		return -EINVAL;
	}
	pr_err("hungtask: whitelist is %s.\n", cur);
	hash_lists_init();

	/*generate the new whitelist*/
	for (;;) {
		char *token;

		token = strsep(&cur, ",");
		if (token && strlen(token)) {
			strncpy(whitetmplist[index].name, token, TASK_COMM_LEN);
			if(strlen(whitetmplist[index].name) > 0)
				whitelistEmpty = false;
			index++;
		}
		if (!cur)
			break;
	}
	/*check again in case user input "whitelist,,,,,,"*/
	if (whitelistEmpty) {
		pr_err("hungtask: at least one process need to be set!\n");
		return -EINVAL;
	}

	return (ssize_t) n;
}

static struct notifier_block pm_event = {0};
static int pm_sr_event(struct notifier_block *this,
                unsigned long event, void *ptr)
{
	switch (event) {
		case PM_SUSPEND_PREPARE:
			in_suspend = true;
			break;
		case PM_POST_SUSPEND:
			in_suspend = false;
			break;
		default:
			return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}
/*used for sysctl at "/proc/sys/kernel/hung_task_timeout_secs"*/
void fetch_task_timeout_secs(unsigned long new_sysctl_hung_task_timeout_secs)
{
	if (new_sysctl_hung_task_timeout_secs > CONFIG_DEFAULT_HUNG_TASK_TIMEOUT
		|| (new_sysctl_hung_task_timeout_secs % HEARTBEAT_TIME))
		return;

 	huawei_hung_task_timeout_secs = new_sysctl_hung_task_timeout_secs;

	/*if user change panic timeout value, we sync it to dump value
	defaultly, user can set it diffrently*/
	whitelist_panic_cnt = (int)(huawei_hung_task_timeout_secs / HEARTBEAT_TIME);
	if (whitelist_panic_cnt > THIRTY_SECONDS) {
		whitelist_dump_cnt = whitelist_panic_cnt / HUNGTASK_DUMP_IN_PANIC_LOOSE;
	} else {
		whitelist_dump_cnt = whitelist_panic_cnt / HUNGTASK_DUMP_IN_PANIC_STRICT;
	}
	applist_dump_cnt = whitelist_dump_cnt;
	topapp_hung_cnt = whitelist_dump_cnt;
}

void fetch_hung_task_panic(int new_did_panic)
{
	did_panic = new_did_panic;
}
/*used as main thread of khungtaskd*/
static struct kobj_attribute watchdog_nosched_attribute = {
	.attr = {
		.name = "vm_heart",
		.mode = 0640,
		},
	.show = watchdog_nosched_show,
	.store = watchdog_nosched_store,
};
static struct kobj_attribute timeout_attribute = {
	.attr = {
		 .name = "enable",
		 .mode = 0640,
		 },
	.show = enable_show,
	.store = enable_store,
};
static struct kobj_attribute monitorlist_attr = {
	.attr = {
		 .name = "monitorlist",
		 .mode = 0640,
		 },
	.show = monitorlist_show,
	.store = monitorlist_store,
};
static struct attribute *attrs[] = {
	&timeout_attribute.attr,
	&monitorlist_attr.attr,
	&watchdog_nosched_attribute.attr,
	NULL
};
static struct attribute_group hungtask_attr_group = {
	.attrs = attrs,
};
struct kobject *hungtask_kobj;
int create_sysfs_hungtask(void)
{
	int retval = 0;

	while (kernel_kobj == NULL)
		msleep(1000);
	/*Create kobject named "hungtask" located at /sys/kernel/huangtask */
	hungtask_kobj = kobject_create_and_add("hungtask", kernel_kobj);
	if (!hungtask_kobj)
		return -ENOMEM;

	retval = sysfs_create_group(hungtask_kobj, &hungtask_attr_group);
	if (retval)
		kobject_put(hungtask_kobj);

	pm_event.notifier_call = pm_sr_event;
	pm_event.priority = -1;
	if (register_pm_notifier(&pm_event))
	{
		pr_err("hungtask: register pm notifier failed\n");
		return -EFAULT;
	}
	return retval;
}

int get_fg_pid(void)
{
    return topapp_pid;
}
EXPORT_SYMBOL(get_fg_pid);

/*all parameters located under "/sys/module/huawei_hung_task/parameters/"*/
module_param_array_named(jankproc_pids, jankproc_pids, int,
				&jankproc_pids_size, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(jankproc_pids, "jankproc_pids state");
module_param_named(topapp_pid, topapp_pid, int, (S_IRUGO | S_IWUSR));
