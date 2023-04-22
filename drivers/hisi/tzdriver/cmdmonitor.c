#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/kernel.h>

#include "tc_ns_log.h"
#include "securec.h"
#include "teek_ns_client.h"
#include "smc.h"
#include <huawei_platform/log/imonitor.h>
#define IMONITOR_TA_CRASH_EVENT_ID      (901002003)
#include "tlogger.h"
#include "tui.h"
const char g_cmd_monitor_white_table[][TASK_COMM_LEN]={
	{"FIAgentThread"},
	{"AIAgentThread"},
	{"tee_test_ut"},
};
const uint32_t g_white_table_thread_num = sizeof(g_cmd_monitor_white_table) / TASK_COMM_LEN;

extern void wakeup_tc_siq(void);

static int cmd_need_archivelog = 0;
static LIST_HEAD(cmd_monitor_list);
static int cmd_monitor_list_size = 0;
#define MAX_CMD_MONITOR_LIST 200
static DEFINE_MUTEX(cmd_monitor_lock);
struct cmd_monitor {
	struct list_head list;
	struct timespec sendtime;
	int count ;
	bool returned;
	bool isReported;
	unsigned int pid;
	unsigned int tid;
	char pname[TASK_COMM_LEN];
	char tname[TASK_COMM_LEN];
	unsigned int lastcmdid;
	long timetotal;
};
static struct delayed_work cmd_monitor_work;
static struct delayed_work cmd_monitor_work_archive;
static int g_tee_detect_ta_crash = 0;
enum {
	TYPE_CRASH_TA 	= 1,
	TYPE_CRASH_TEE = 2,
};

void tzdebug_archivelog(void)
{
	schedule_delayed_work(&cmd_monitor_work_archive, usecs_to_jiffies(0));
}
void cmd_monitor_ta_crash(int32_t type)
{
	g_tee_detect_ta_crash = ((type == TYPE_CRASH_TEE) ? TYPE_CRASH_TEE : TYPE_CRASH_TA);
	tzdebug_archivelog();
}
static int get_pid_name(pid_t pid,char* comm,size_t size)
{
	struct task_struct *task;
	int sret;
	if (size <= TASK_COMM_LEN - 1 || !comm) {
		return -1;
	}
	rcu_read_lock();
	task = find_task_by_vpid(pid);
	if (task != NULL) {
		get_task_struct(task);
	}
	rcu_read_unlock();
	if (task != NULL) {
		sret = strncpy_s(comm, size, task->comm, strlen(task->comm));
		if (sret != 0) {
			tloge("strncpy_s faild: errno = %d.\n", sret);
		}
		put_task_struct(task);
		return sret;
	}
	return -1;

}

static bool is_thread_in_white_table(char *tname)
{
	uint32_t i;
	if (!tname)
		return false;

	for (i=0; i < g_white_table_thread_num; i++) {
		if (!strcmp(tname, g_cmd_monitor_white_table[i])) { /*lint !e421 */
			return true;
		}
	}
	return false;
}


static void cmd_monitor_tick(void)
{
	long timedif;
	struct cmd_monitor *monitor = NULL;
	struct cmd_monitor *tmp = NULL;
	struct timespec nowtime = current_kernel_time();
	mutex_lock(&cmd_monitor_lock);
	list_for_each_entry_safe(monitor, tmp, &cmd_monitor_list, list) {
		if (monitor->returned ==true) {
			tloge("[cmd_monitor_tick] pid=%d,pname=%s,tid=%d,tname=%s,lastcmdid=%d,count=%d timetotal=%ld us returned, remained command(s)=%d\n", monitor->pid, monitor->pname, monitor->tid,  monitor->tname    ,monitor->lastcmdid, monitor->count, monitor->timetotal, cmd_monitor_list_size);
			list_del(&monitor->list);
			kfree(monitor);
			cmd_monitor_list_size--;
			continue;
		}
		/* not return, we need to check  */
		timedif = 1000*nowtime.tv_sec-1000*monitor->sendtime.tv_sec+nowtime.tv_nsec/1000000-monitor->sendtime.tv_nsec/1000000;

		/* Temporally change timeout to 25s, we log the teeos log,and report*/
		if (timedif > 25*1000 && !monitor->isReported) {
			monitor->isReported = true;
			/* print tee stask*/
			tloge("[cmd_monitor_tick] pid=%d,pname=%s,tid=%d,tname=%s,lastcmdid=%d,timedif=%ld ms and report\n", monitor->pid, monitor->pname, monitor->tid, monitor->tname,monitor->lastcmdid, timedif);
			/* threads out of white table need info dump*/
			tloge("monitor: pid-%d", monitor->pid);
			if((!(is_thread_in_white_table(monitor->tname))) && (!tui_pid_status(monitor->pid))){
				cmd_need_archivelog = 1;
				wakeup_tc_siq();
			}
		} else if (timedif > 1*1000) {
			tloge("[cmd_monitor_tick] pid=%d,pname=%s,tid=%d,timedif=%ld ms\n", monitor->pid, monitor->pname, monitor->tid, timedif);
		}
        }
        tlogi("[cmd_monitor_tick] cmd_monitor_list_size=%d\n",cmd_monitor_list_size);
        if (cmd_monitor_list_size > 0) {
		/* if have cmd in monitor list, we need tick*/
		schedule_delayed_work(&cmd_monitor_work, usecs_to_jiffies(1000000));
        }
        mutex_unlock(&cmd_monitor_lock);
}
static void cmd_monitor_tickfn(struct work_struct *work)
{
        (void)(work);
        cmd_monitor_tick();

		/* check tlogcat if have new log  */
		tz_log_write();
}
static void cmd_monitor_archivefn(struct work_struct *work)
{
        (void)(work);
	if( tlogger_store_lastmsg() < 0)
		tloge("[cmd_monitor_tick]tlogger_store_lastmsg failed\n");

	if (g_tee_detect_ta_crash == TYPE_CRASH_TA) {

		if (0 > teeos_log_exception_archive(IMONITOR_TA_CRASH_EVENT_ID, "ta crash"))
			tloge("log_exception_archive failed\n");

	}

	g_tee_detect_ta_crash = 0;
}


static struct cmd_monitor* init_monitor_locked(void)
{
        struct cmd_monitor* newitem;
        int pidnameresult;
	int tidnameresult;
        newitem = kzalloc(sizeof(struct cmd_monitor),GFP_KERNEL);/*lint !429 !593*/
        if (newitem == NULL) {
                tloge("[cmd_monitor_tick]kmalloc faild\n");
                return NULL;
        }
        newitem->sendtime = current_kernel_time();
        newitem->count = 1;
        newitem->returned = false;
        newitem->isReported = false;
        newitem->pid = current->tgid;
        newitem->tid = current->pid;
        pidnameresult = get_pid_name(newitem->pid, newitem->pname, sizeof(newitem->pname));
        if (pidnameresult != 0) {
                newitem->pname[0] = '\0';
        }
	tidnameresult = get_pid_name(newitem->tid, newitem->tname, sizeof(newitem->tname));
        if (tidnameresult != 0) {
                newitem->pname[0] = '\0';
        }
        INIT_LIST_HEAD(&newitem->list);
        list_add_tail(&newitem->list, &cmd_monitor_list);
        cmd_monitor_list_size++;
        return newitem;
}

void cmd_monitor_log(TC_NS_SMC_CMD *cmd)
{
	int foundFlag =0;
	unsigned int pid;
	unsigned int tid;
	struct cmd_monitor *monitor = NULL;
	struct cmd_monitor* newitem;
	if (cmd == NULL)
	{
		return;
	}
	pid = current->tgid;
	tid = current->pid;
	mutex_lock(&cmd_monitor_lock);
	do {
		list_for_each_entry(monitor, &cmd_monitor_list, list) {
			if(monitor->pid == pid && monitor->tid == tid){
				foundFlag = 1;
				/* restart*/
				monitor->sendtime = current_kernel_time();
				monitor->count++;
				monitor->returned = false;
				monitor->isReported = false;
				monitor->lastcmdid = cmd->cmd_id;
				break;
			}
		}
		if (foundFlag == 0) {
			if (cmd_monitor_list_size > MAX_CMD_MONITOR_LIST-1) {
				tloge("[cmd_monitor_tick]MAX_CMD_MONITOR_LIST\n");
				break;
			}
			newitem = init_monitor_locked();
			if (newitem == NULL) {
				tloge("[cmd_monitor_tick]init_monitor failed\n");
				break;
			}
			newitem->lastcmdid = cmd->cmd_id;
			/* the first cmd will cause timer*/
			if (cmd_monitor_list_size == 1) {
				schedule_delayed_work(&cmd_monitor_work, usecs_to_jiffies(1000000));
			}
		}
        }while(0);
        mutex_unlock(&cmd_monitor_lock);
}

void cmd_monitor_logend(TC_NS_SMC_CMD *cmd)
{
	unsigned int pid;
	unsigned int tid;
	struct cmd_monitor *monitor = NULL;
	if (cmd == NULL)
	{
		return;
	}
	pid = current->tgid;
	tid = current->pid;
	mutex_lock(&cmd_monitor_lock);
	list_for_each_entry(monitor, &cmd_monitor_list, list) {
		if(monitor->pid == pid && monitor->tid == tid && monitor->returned ==false){
			struct timespec nowtime = current_kernel_time();
			long timedif = 1000000*nowtime.tv_sec-1000000*monitor->sendtime.tv_sec+nowtime.tv_nsec/1000-monitor->sendtime.tv_nsec/1000;
			monitor->timetotal += timedif;
			monitor->returned = true;
			/* we need set all monitor.returned = true ,dont break;*/
		}
	}
	mutex_unlock(&cmd_monitor_lock);
}
void do_cmd_need_archivelog(void)
{
	if(cmd_need_archivelog == 1) {
		cmd_need_archivelog =0;
		schedule_delayed_work(&cmd_monitor_work_archive, usecs_to_jiffies(1000000));
	}
}
void init_cmd_monitor(void)
{
	INIT_DEFERRABLE_WORK(&cmd_monitor_work, cmd_monitor_tickfn);
	INIT_DEFERRABLE_WORK(&cmd_monitor_work_archive, cmd_monitor_archivefn);
}
