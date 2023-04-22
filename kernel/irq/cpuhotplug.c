/*
 * Generic cpu hotunplug interrupt migration code copied from the
 * arch/arm implementation
 *
 * Copyright (C) Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/interrupt.h>
#include <linux/ratelimit.h>
#include <linux/irq.h>
#ifdef CONFIG_HISI_CPU_ISOLATION
#include <linux/cpumask.h>
#endif

#include "internals.h"

static bool migrate_one_irq(struct irq_desc *desc)
{
	struct irq_data *d = irq_desc_get_irq_data(desc);
	const struct cpumask *affinity = d->common->affinity;
	struct irq_chip *c;
	bool ret = false;
#ifdef CONFIG_HISI_CPU_ISOLATION
	struct cpumask available_cpus;
#endif

	/*
	 * If this is a per-CPU interrupt, or the affinity does not
	 * include this CPU, then we have nothing to do.
	 */
	if (irqd_is_per_cpu(d) ||
	    !cpumask_test_cpu(smp_processor_id(), affinity))
		return false;

#ifdef CONFIG_HISI_CPU_ISOLATION
	cpumask_copy(&available_cpus, affinity);
	cpumask_andnot(&available_cpus, &available_cpus, cpu_isolated_mask);

	/* keep affinity first when conflict with isolation */
	if (cpumask_intersects(cpu_online_mask, &available_cpus))
		affinity = &available_cpus;

	if (cpumask_any_and(affinity, cpu_online_mask) >= nr_cpu_ids) {
		/*
		 * The order of preference for selecting a fallback CPU is
		 *
		 * (1) online and un-isolated CPU from default affinity
		 * (2) online and un-isolated CPU
		 * (3) online CPU
		 */
		cpumask_andnot(&available_cpus, cpu_online_mask,
			       cpu_isolated_mask);
		affinity = &available_cpus;

		if (cpumask_intersects(&available_cpus, irq_default_affinity))
			cpumask_and(&available_cpus, &available_cpus,
				    irq_default_affinity);
		else if (cpumask_empty(&available_cpus))
			affinity = cpu_online_mask;

		ret = true;
	}
#else
	if (cpumask_any_and(affinity, cpu_online_mask) >= nr_cpu_ids) {
		affinity = cpu_online_mask;
		ret = true;
	}
#endif

	c = irq_data_get_irq_chip(d);
	if (!c->irq_set_affinity) {
		pr_debug("IRQ%u: unable to set affinity\n", d->irq);
	} else {
		int r = irq_do_set_affinity(d, affinity, false);
		if (r)
			pr_warn_ratelimited("IRQ%u: set affinity failed(%d).\n",
					    d->irq, r);
	}

	return ret;
}

/**
 * irq_migrate_all_off_this_cpu - Migrate irqs away from offline cpu
 *
 * The current CPU has been marked offline.  Migrate IRQs off this CPU.
 * If the affinity settings do not allow other CPUs, force them onto any
 * available CPU.
 *
 * Note: we must iterate over all IRQs, whether they have an attached
 * action structure or not, as we need to get chained interrupts too.
 */
void irq_migrate_all_off_this_cpu(void)
{
	unsigned int irq;
	struct irq_desc *desc;
	unsigned long flags;

	local_irq_save(flags);

	for_each_active_irq(irq) {
		bool affinity_broken;

		desc = irq_to_desc(irq);
		raw_spin_lock(&desc->lock);
		affinity_broken = migrate_one_irq(desc);
		raw_spin_unlock(&desc->lock);

		if (affinity_broken)
			pr_warn_ratelimited("IRQ%u no longer affine to CPU%u\n",
					    irq, smp_processor_id());
	}

	local_irq_restore(flags);
}
