#ifndef _HWCFS_COMMON_H_
#define _HWCFS_COMMON_H_
struct rq;
extern void enqueue_vip_thread(struct rq *rq, struct task_struct *p);
extern void dequeue_vip_thread(struct rq *rq, struct task_struct *p);
extern void pick_vip_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se);
extern bool test_dynamic_vip(struct task_struct *task, int type);
extern void dynamic_vip_dequeue(struct task_struct *task, int type);
extern void dynamic_vip_enqueue(struct task_struct *task, int type, int depth);
extern bool test_task_vip(struct task_struct *task);
extern bool test_task_vip_depth(int vip_depth);
extern bool test_set_dynamic_vip(struct task_struct *tsk);
extern void trigger_vip_balance(struct rq *rq);
extern void vip_init_rq_data(struct rq *rq);
#endif
