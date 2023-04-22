#ifndef _HWCFS_FORK_H_
#define _HWCFS_FORK_H_

static inline void init_task_vip_info(struct task_struct *p)
{
	p->static_vip = 0;
	atomic64_set(&(p->dynamic_vip), 0);
	INIT_LIST_HEAD(&p->vip_entry);
	p->vip_depth = 0;
	p->enqueue_time = 0;
	p->dynamic_vip_start = 0;
}
#endif
