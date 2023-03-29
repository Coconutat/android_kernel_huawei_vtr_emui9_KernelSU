#ifdef CONFIG_HW_VIP_THREAD
/*lint -save -e578 -e695 -e571*/

#include <linux/sched.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <trace/events/sched.h>
#include <../kernel/sched/sched.h>

int vip_min_sched_delay_granularity; /*vip thread delay upper bound(ms)*/
int vip_max_dynamic_granularity = 32;    /*vip dynamic max exist time(ms)*/
int vip_min_migration_delay = 10;        /*vip min migration delay time(ms)*/
#define S2NS_T 1000000

static int entity_before(struct sched_entity *a,
				struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

void enqueue_vip_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	bool exist = false;

	if (!rq || !p || !list_empty(&p->vip_entry)) {
		return;
	}
	p->enqueue_time = rq->clock;
	if (p->static_vip || atomic64_read(&p->dynamic_vip)) {
		list_for_each_safe(pos, n, &rq->vip_thread_list) {
			if (pos == &p->vip_entry) {
				exist = true;
				break;
			}
		}
		if (!exist) {
			list_add_tail(&p->vip_entry, &rq->vip_thread_list);
			get_task_struct(p);
			trace_sched_vip_queue_op(p, "vip_enqueue_succ");
		}
	}
}

void dequeue_vip_thread(struct rq *rq, struct task_struct *p)
{
	struct list_head *pos, *n;
	u64 now =  jiffies_to_nsecs(jiffies);

	if (!rq || !p) {
		return;
	}
	p->enqueue_time = 0;
	if (!list_empty(&p->vip_entry)) {
		list_for_each_safe(pos, n, &rq->vip_thread_list) {
			if (atomic64_read(&p->dynamic_vip) && (now - p->dynamic_vip_start) > (u64)vip_max_dynamic_granularity * S2NS_T) {
				atomic64_set(&p->dynamic_vip, 0);
			}
		}
		list_for_each_safe(pos, n, &rq->vip_thread_list) {
			if (pos == &p->vip_entry) {
				list_del_init(&p->vip_entry);
				put_task_struct(p);
				trace_sched_vip_queue_op(p, "vip_dequeue_succ");
				return;
			}
		}
	}
}

static struct task_struct *pick_first_vip_thread(struct rq *rq)
{
	struct list_head *vip_thread_list = &rq->vip_thread_list;
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct task_struct *temp = NULL;
	struct task_struct *leftmost_task = NULL;
	list_for_each_safe(pos, n, vip_thread_list) {
		temp = list_entry(pos, struct task_struct, vip_entry);
		/*ensure vip task in current rq cpu otherwise delete it*/
		if (unlikely(task_cpu(temp) != rq->cpu)) {
			printk(KERN_WARNING "task(%s,%d,%d) does not belong to cpu%d", temp->comm, task_cpu(temp), temp->policy, rq->cpu);
			list_del_init(&temp->vip_entry);
			continue;
		}
		if (leftmost_task == NULL) {
			leftmost_task = temp;
		} else if (entity_before(&temp->se, &leftmost_task->se)) {
			leftmost_task = temp;
		}
	}

	return leftmost_task;
}

void pick_vip_thread(struct rq *rq, struct task_struct **p, struct sched_entity **se)
{
	struct task_struct *ori_p;
	struct task_struct *key_task;
	struct sched_entity *key_se;
	if (!rq || !p || !se) {
		return;
	}
	ori_p = *p;
	if (ori_p && !ori_p->static_vip && !atomic64_read(&ori_p->dynamic_vip)) {
		if (!list_empty(&rq->vip_thread_list)) {
			key_task = pick_first_vip_thread(rq);
			if (key_task) {
				key_se = &key_task->se;
				if (key_se && (rq->clock >= key_task->enqueue_time) &&
				rq->clock - key_task->enqueue_time >= ((u64)vip_min_sched_delay_granularity * S2NS_T)) {
					trace_sched_vip_sched(key_task, "pick_vip");
					*p = key_task;
					*se = key_se;
				}
			}
		}
	}
}

#define DYNAMIC_VIP_SEC_WIDTH   8
#define DYNAMIC_VIP_MASK_BASE   0x00000000ff

#define dynamic_vip_offset_of(type) (type * DYNAMIC_VIP_SEC_WIDTH)
#define dynamic_vip_mask_of(type) ((u64)(DYNAMIC_VIP_MASK_BASE) << (dynamic_vip_offset_of(type)))
#define dynamic_vip_get_bits(value, type) ((value & dynamic_vip_mask_of(type)) >> dynamic_vip_offset_of(type))
#define dynamic_vip_one(type) ((u64)1 << dynamic_vip_offset_of(type))


bool test_dynamic_vip(struct task_struct *task, int type)
{
	u64 dynamic_vip;
	if (!task) {
		return false;
	}
	dynamic_vip = atomic64_read(&task->dynamic_vip);
	return dynamic_vip_get_bits(dynamic_vip, type) > 0;
}

static bool test_task_exist(struct task_struct *task, struct list_head *head)
{
	struct list_head *pos, *n;
	list_for_each_safe(pos, n, head) {
		if (pos == &task->vip_entry) {
			return true;
		}
	}
	return false;
}

static inline void dynamic_vip_dec(struct task_struct *task, int type)
{
	atomic64_sub(dynamic_vip_one(type), &task->dynamic_vip);
}

static inline void dynamic_vip_inc(struct task_struct *task, int type)
{
	atomic64_add(dynamic_vip_one(type), &task->dynamic_vip);
}

static void __dynamic_vip_dequeue(struct task_struct *task, int type)
{
#ifdef CONFIG_HISI_EAS_SCHED
        unsigned long flags;
#else
        struct rq_flags flags;
#endif
	bool exist = false;
	struct rq *rq = NULL;
	u64 dynamic_vip = 0;

	rq = task_rq_lock(task, &flags);
	dynamic_vip = atomic64_read(&task->dynamic_vip);
	if (dynamic_vip <= 0) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
	dynamic_vip_dec(task, type);
	dynamic_vip = atomic64_read(&task->dynamic_vip);
	if (dynamic_vip > 0) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
	task->vip_depth = 0;

	exist = test_task_exist(task, &rq->vip_thread_list);
	if (exist) {
		trace_sched_vip_queue_op(task, "dynamic_vip_dequeue_succ");
		list_del_init(&task->vip_entry);
		put_task_struct(task);
	}
	task_rq_unlock(rq, task, &flags);
}

void dynamic_vip_dequeue(struct task_struct *task, int type)
{
	if (!task || type >= DYNAMIC_VIP_MAX) {
		return;
	}
	__dynamic_vip_dequeue(task, type);
}

extern const struct sched_class fair_sched_class;
static void __dynamic_vip_enqueue(struct task_struct *task, int type, int depth)
{
#ifdef CONFIG_HISI_EAS_SCHED
        unsigned long flags;
#else
        struct rq_flags flags;
#endif
	bool exist = false;
	struct rq *rq = NULL;

	rq = task_rq_lock(task, &flags);
	if (task->sched_class != &fair_sched_class) {
		task_rq_unlock(rq, task, &flags);
		return;
	}
	if (unlikely(!list_empty(&task->vip_entry))) {
		printk(KERN_WARNING "task(%s,%d,%d) is already in another list", task->comm, task->pid, task->policy);
		task_rq_unlock(rq, task, &flags);
		return;
	}

	dynamic_vip_inc(task, type);
	task->dynamic_vip_start = jiffies_to_nsecs(jiffies);
	task->vip_depth = task->vip_depth > depth + 1 ? task->vip_depth : depth + 1;
	if (task->state == TASK_RUNNING) {
		exist = test_task_exist(task, &rq->vip_thread_list);
		if (!exist) {
			get_task_struct(task);
			list_add_tail(&task->vip_entry, &rq->vip_thread_list);
			trace_sched_vip_queue_op(task, "dynamic_vip_enqueue_succ");
		}
	}
	task_rq_unlock(rq, task, &flags);
}

void dynamic_vip_enqueue(struct task_struct *task, int type, int depth)
{
	if (!task || type >= DYNAMIC_VIP_MAX) {
		return;
	}
	__dynamic_vip_enqueue(task, type, depth);
}

inline bool test_task_vip(struct task_struct *task)
{
	return task && (task->static_vip || atomic64_read(&task->dynamic_vip));
}

inline bool test_task_vip_depth(int vip_depth)
{
	return vip_depth < VIP_DEPTH_MAX;
}

inline bool test_set_dynamic_vip(struct task_struct *tsk)
{
	return test_task_vip(tsk) && test_task_vip_depth(tsk->vip_depth);
}

static struct task_struct *check_vip_delayed(struct rq *rq)
{
	struct list_head *pos = NULL;
	struct list_head *n = NULL;
	struct task_struct *tsk = NULL;
	struct list_head *vip_thread_list = NULL;
	vip_thread_list = &rq->vip_thread_list;

	list_for_each_safe(pos, n, vip_thread_list) {
		tsk = list_entry(pos, struct task_struct, vip_entry);
		if (tsk && (rq->clock - tsk->enqueue_time) >= (u64)vip_min_migration_delay * S2NS_T)
			return tsk;
	}
	return NULL;
}

static int vip_task_hot(struct task_struct *p, struct rq *src_rq, struct rq *dst_rq)
{
	s64 delta;

	lockdep_assert_held(&src_rq->lock);

	if (p->sched_class != &fair_sched_class)
		return 0;

	if (unlikely(p->policy == SCHED_IDLE))
		return 0;

	if (sched_feat(CACHE_HOT_BUDDY) && dst_rq->nr_running &&
	    (&p->se == src_rq->cfs.next || &p->se == src_rq->cfs.last))
		return 1;

	if (sysctl_sched_migration_cost == (unsigned int)-1)
		return 1;
	if (sysctl_sched_migration_cost == 0)
		return 0;

	delta = src_rq->clock_task - p->se.exec_start;
	return delta < (s64)sysctl_sched_migration_cost;
}

static void detach_task(struct task_struct *p, struct rq *src_rq, struct rq *dst_rq)
{
	trace_sched_vip_queue_op(p, "detach_task");
	lockdep_assert_held(&src_rq->lock);
	deactivate_task(src_rq, p, 0);
	p->on_rq = TASK_ON_RQ_MIGRATING;
	double_lock_balance(src_rq, dst_rq);
	set_task_cpu(p, dst_rq->cpu);
	double_unlock_balance(src_rq, dst_rq);
}

static void attach_task(struct rq *dst_rq, struct task_struct *p)
{
	trace_sched_vip_queue_op(p, "attach_task");
	raw_spin_lock(&dst_rq->lock);
	BUG_ON(task_rq(p) != dst_rq); /*lint !e58*/
	p->on_rq = TASK_ON_RQ_QUEUED;
	activate_task(dst_rq, p, 0);
	check_preempt_curr(dst_rq, p, 0);
	raw_spin_unlock(&dst_rq->lock);
}

static int vip_can_migrate(struct task_struct *p, struct rq *src_rq, struct rq *dst_rq)
{
	if (task_running(src_rq, p))
		return 0;
	if (vip_task_hot(p, src_rq, dst_rq))
		return 0;
	if (task_rq(p) != src_rq) /*lint !e58*/
		return 0;
	if (!test_task_exist(p, &src_rq->vip_thread_list))
		return 0;
	return 1;
}

extern void hisi_get_fast_cpus(struct cpumask *cpumask);
extern void hisi_get_slow_cpus(struct cpumask *cpumask);
#ifdef CONFIG_HISI_EAS_SCHED
static struct cpumask hisi_slow_cpu_mask;
#endif

static int __do_vip_balance(void *data)
{
	struct rq *src_rq = data;
	struct rq *dst_rq = NULL;
	int src_cpu = cpu_of(src_rq);
	int i;
	struct task_struct *p = NULL;
#ifdef CONFIG_HISI_EAS_SCHED
	struct cpumask cluster = CPU_MASK_NONE;
	struct cpumask cpus_allowed = CPU_MASK_NONE;
#endif
	bool is_mig = false;

	/*find a delayed vip task*/
	raw_spin_lock_irq(&src_rq->lock);
	if (unlikely(src_cpu != smp_processor_id() || !src_rq->active_vip_balance)) {
		src_rq->active_vip_balance = 0;
		raw_spin_unlock_irq(&src_rq->lock);
		return 0;
	}
	p = check_vip_delayed(src_rq);
	if (!p) {
		src_rq->active_vip_balance = 0;
		raw_spin_unlock_irq(&src_rq->lock);
		return 0;
	}

	/*find a free-cpu*/
#ifdef CONFIG_HISI_EAS_SCHED
	hisi_get_fast_cpus(&cluster);
	hisi_get_slow_cpus(&hisi_slow_cpu_mask);
	if (cpumask_test_cpu(src_cpu, &cluster))
		cpumask_and(&cpus_allowed, &cluster, &(p->cpus_allowed));
	else
		cpumask_and(&cpus_allowed, &hisi_slow_cpu_mask, &(p->cpus_allowed));

	raw_spin_unlock(&src_rq->lock);
	for (i = 0; i < nr_cpu_ids; i++) {
		if (i == src_cpu)
			continue;
		if (cpumask_test_cpu(i, &cpus_allowed)) {
			dst_rq = cpu_rq(i);
			raw_spin_lock(&dst_rq->lock);
			if (!dst_rq->rt.rt_nr_running || list_empty(&dst_rq->vip_thread_list)) {
				raw_spin_unlock(&dst_rq->lock);
				break;
			} else {
				raw_spin_unlock(&dst_rq->lock);
			}
		}
	}
#else
	raw_spin_unlock(&src_rq->lock);
	for_each_cpu_and(i, tsk_cpus_allowed(p), cpu_coregroup_mask(src_cpu)) {
		if (i == src_cpu || !cpu_online(i))
			continue;
		dst_rq = cpu_rq(i);
		raw_spin_lock(&dst_rq->lock);
		if (!dst_rq->rt.rt_nr_running || list_empty(&dst_rq->vip_thread_list)) {
			raw_spin_unlock(&dst_rq->lock);
			break;
		} else {
			raw_spin_unlock(&dst_rq->lock);
		}
	}
#endif
	/*move p from src to dst cpu*/
	raw_spin_lock(&src_rq->lock);
	if (i != nr_cpu_ids && p != NULL && dst_rq != NULL) {
		if (vip_can_migrate(p, src_rq, dst_rq)) {
			detach_task(p, src_rq, dst_rq);
			is_mig = true;
		}
	}
	src_rq->active_vip_balance = 0;
	raw_spin_unlock(&src_rq->lock);
	if (is_mig) {
		attach_task(dst_rq, p);
	}
	local_irq_enable();
	return 0;
}

void trigger_vip_balance(struct rq *rq)
{
	struct task_struct *task = NULL;
	int active_vip_balance = 0;
	if (!rq) {
		return;
	}
	raw_spin_lock(&rq->lock);
	task = check_vip_delayed(rq);
	/*
	 * active_vip_balance synchronized accesss to vip_balance_work
	 * 1 means vip_balance_work is ongoing, and reset to 0 after
	 * vip_balance_work is done
	 */
	if (!rq->active_vip_balance && task) {
		active_vip_balance = 1;
		rq->active_vip_balance = 1;
	}
	raw_spin_unlock(&rq->lock);
	if (active_vip_balance) {
		stop_one_cpu_nowait(cpu_of(rq), __do_vip_balance, rq, &rq->vip_balance_work);
	}
}

void vip_init_rq_data(struct rq *rq)
{
	if (!rq) {
		return;
	}
	rq->active_vip_balance = 0;
	INIT_LIST_HEAD(&rq->vip_thread_list);
}
/*lint -restore*/
#endif
