#ifndef _HISI_HUNG_TASK_H_
#define _HISI_HUNG_TASK_H_

#ifndef HEARTBEAT_TIME

#define HEARTBEAT_TIME 3

#endif

extern void check_hung_tasks_proposal(unsigned long timeout);

extern void fetch_hung_task_panic(int new_did_panic);

extern void fetch_task_timeout_secs(unsigned long new_sysctl_hung_task_timeout_secs);

extern int create_sysfs_hungtask(void);

extern void hwhungtask_show_state_filter(unsigned long state_filter);
#endif
