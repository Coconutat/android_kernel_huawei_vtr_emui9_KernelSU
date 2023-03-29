#ifndef _HWCFS_MUTEX_H_
#define _HWCFS_MUTEX_H_
extern void mutex_list_add(struct task_struct *task, struct list_head *entry, struct list_head *head, struct mutex *lock);
extern void mutex_dynamic_vip_enqueue(struct mutex *lock, struct task_struct *task);
extern void mutex_dynamic_vip_dequeue(struct mutex *lock, struct task_struct *task);
#endif
