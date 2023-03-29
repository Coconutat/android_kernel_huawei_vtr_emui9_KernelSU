#include <linux/irq.h>

/*
 * Remove a task from the runqueue and pretend that it's migrating. This
 * should prevent migrations for the detached task and disallow further
 * changes to tsk_cpus_allowed.
 */
static void
detach_one_task(struct task_struct *p, struct rq *detach_rq, struct list_head *tasks)
{
	lockdep_assert_held(&detach_rq->lock);

	p->on_rq = TASK_ON_RQ_MIGRATING;
	deactivate_task(detach_rq, p, 0);
	list_add(&p->se.group_node, tasks);
}

static void attach_tasks(struct list_head *tasks, struct rq *attach_rq)
{
	struct task_struct *p;

	lockdep_assert_held(&attach_rq->lock);

	while (!list_empty(tasks)) {
		p = list_first_entry(tasks, struct task_struct, se.group_node);
		list_del_init(&p->se.group_node);

		BUG_ON(task_rq(p) != attach_rq);/*lint !e58*/
		activate_task(attach_rq, p, 0);
		p->on_rq = TASK_ON_RQ_QUEUED;
	}
}


static void set_rq_online(struct rq *rq);
static void set_rq_offline(struct rq *rq);
static void migrate_tasks(struct rq *dead_rq, bool migrate_pinned_tasks);

int do_isolation_work_cpu_stop(void *data)
{
	unsigned int cpu = smp_processor_id();
	struct rq *iso_rq = cpu_rq(cpu);

	watchdog_disable(cpu);

	irq_migrate_all_off_this_cpu();

	local_irq_disable();

	sched_ttwu_pending();

	raw_spin_lock(&iso_rq->lock);

	/*
	 * Temporarily mark the rq as offline. This will allow us to
	 * move tasks off the CPU.
	 */
	if (iso_rq->rd) {
		BUG_ON(!cpumask_test_cpu(cpu, iso_rq->rd->span));
		set_rq_offline(iso_rq);
	}

	migrate_tasks(iso_rq, false);

	if (iso_rq->rd)
		set_rq_online(iso_rq);
	raw_spin_unlock(&iso_rq->lock);

	local_irq_enable();
	return 0;
}

int do_unisolation_work_cpu_stop(void *data)
{
	watchdog_enable(smp_processor_id());
	return 0;
}

static void init_sched_groups_capacity(int cpu, struct sched_domain *sd);

static void sched_update_group_capacities(int cpu)
{
	struct sched_domain *sd;

	mutex_lock(&sched_domains_mutex);
	rcu_read_lock();

	for_each_domain(cpu, sd) {/*lint !e666*/
		int balance_cpu = group_balance_cpu(sd->groups);

		init_sched_groups_capacity(cpu, sd);
		/*
		 * Need to ensure this is also called with balancing
		 * cpu.
		 */
		if (cpu != balance_cpu)
			init_sched_groups_capacity(balance_cpu, sd);
	}

	rcu_read_unlock();
	mutex_unlock(&sched_domains_mutex);
}

static unsigned int cpu_isolation_vote[NR_CPUS];

int sched_isolate_count(const cpumask_t *mask, bool include_offline)
{
	cpumask_t count_mask = CPU_MASK_NONE;

	if (include_offline) {
		cpumask_complement(&count_mask, cpu_online_mask);
		cpumask_or(&count_mask, &count_mask, cpu_isolated_mask);
		cpumask_and(&count_mask, &count_mask, mask);
	} else {
		cpumask_and(&count_mask, mask, cpu_isolated_mask);
	}

	return cpumask_weight(&count_mask);
}

static void calc_load_migrate(struct rq *rq);
/*
 * 1) CPU is isolated and cpu is offlined:
 *     Unisolate the core.
 * 2) CPU is not isolated and CPU is offlined:
 *     No action taken.
 * 3) CPU is offline and request to isolate
 *     Request ignored.
 * 4) CPU is offline and isolated:
 *     Not a possible state.
 * 5) CPU is online and request to isolate
 *     Normal case: Isolate the CPU
 * 6) CPU is not isolated and comes back online
 *     Nothing to do
 *
 * Note: The client calling sched_isolate_cpu() is repsonsible for ONLY
 * calling sched_unisolate_cpu() on a CPU that the client previously isolated.
 * Client is also responsible for unisolating when a core goes offline
 * (after CPU is marked offline).
 */
int sched_isolate_cpu_unlocked(int cpu)
{
	cpumask_t avail_cpus;
	int ret_code = 0;
	u64 start_time = 0;

	if (trace_sched_isolate_enabled())
		start_time = sched_clock();

	cpumask_andnot(&avail_cpus, cpu_online_mask, cpu_isolated_mask);

	/* We cannot isolate ALL cpus in the system */
	if (cpumask_weight(&avail_cpus) == 1) {
		ret_code = -EINVAL;
		goto out;
	}

	if (!cpu_online(cpu)) {
		ret_code = -EINVAL;
		goto out;
	}

	/*
	 * There is a race between watchdog being enabled by hotplug and
	 * core isolation disabling the watchdog. When a CPU is hotplugged in
	 * and the hotplug lock has been released the watchdog thread might
	 * not have run yet to enable the watchdog.
	 */
	if (!watchdog_configured(cpu)) {
		ret_code = -EBUSY;
		goto out;
	}

	if (++cpu_isolation_vote[cpu] > 1)
		goto out;

	set_cpu_isolated(cpu, true);
	cpumask_clear_cpu(cpu, &avail_cpus);

	/* Migrate timers */
	smp_call_function_any(&avail_cpus, hrtimer_quiesce_cpu, &cpu, 1);
	smp_call_function_any(&avail_cpus, timer_quiesce_cpu, &cpu, 1);

	walt_migrate_sync_cpu(cpu, cpumask_first(&avail_cpus));
	stop_cpus(cpumask_of(cpu), do_isolation_work_cpu_stop, 0);

	calc_load_migrate(cpu_rq(cpu));
	update_max_interval();
	sched_update_group_capacities(cpu);

out:
	trace_sched_isolate(cpu, cpumask_bits(cpu_isolated_mask)[0],
			start_time, 1);
	return ret_code;
}

int sched_isolate_cpu(int cpu)
{
	int ret_code;

	cpu_maps_update_begin();
	ret_code = sched_isolate_cpu_unlocked(cpu);
	cpu_maps_update_done();
	return ret_code;
}

/*
 * Note: The client calling sched_isolate_cpu() is repsonsible for ONLY
 * calling sched_unisolate_cpu() on a CPU that the client previously isolated.
 * Client is also responsible for unisolating when a core goes offline
 * (after CPU is marked offline).
 */
int sched_unisolate_cpu_unlocked(int cpu, bool reset_vote)
{
	int ret_code = 0;
	struct rq *iso_rq = cpu_rq(cpu);
	u64 start_time = 0;

	if (trace_sched_isolate_enabled())
		start_time = sched_clock();

	if (!cpu_isolation_vote[cpu]) {
		ret_code = -EINVAL;
		goto out;
	}

	if (reset_vote)
		cpu_isolation_vote[cpu] = 0;
	else if (--cpu_isolation_vote[cpu])
		goto out;

	if (cpu_online(cpu)) {
		unsigned long flags;

		raw_spin_lock_irqsave(&iso_rq->lock, flags);
		iso_rq->age_stamp = sched_clock_cpu(cpu);
		raw_spin_unlock_irqrestore(&iso_rq->lock, flags);
	}

	set_cpu_isolated(cpu, false);

	if (cpu_online(cpu)) {
		update_max_interval();
		sched_update_group_capacities(cpu);
		stop_cpus(cpumask_of(cpu), do_unisolation_work_cpu_stop, 0);

		/* Kick CPU to immediately do load balancing */
		if (!test_and_set_bit(NOHZ_BALANCE_KICK, nohz_flags(cpu)))
			smp_send_reschedule(cpu);
	}

out:
	trace_sched_isolate(cpu, cpumask_bits(cpu_isolated_mask)[0],
			start_time, 0);
	return ret_code;
}

int sched_unisolate_cpu(int cpu)
{
	int ret_code;

	cpu_maps_update_begin();
	ret_code = sched_unisolate_cpu_unlocked(cpu, false);
	cpu_maps_update_done();
	return ret_code;
}

