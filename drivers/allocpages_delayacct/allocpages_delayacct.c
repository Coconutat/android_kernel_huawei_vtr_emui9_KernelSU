#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/taskstats.h>
#include <linux/time.h>
#include <linux/sysctl.h>
#include <linux/delayacct.h>
#include <linux/module.h>

#ifdef CONFIG_HW_MEMORY_MONITOR

#include <chipset_common/allocpages_delayacct/allocpages_delayacct.h>

void __delayacct_allocpages_start(void)
{
	current->delays->allocpages_start = ktime_get_ns();
}

void __delayacct_allocpages_end(unsigned int order)
{
	u64 ns = ktime_get_ns() - current->delays->allocpages_start;
	unsigned long flags;

	if (ns > 0) {
		spin_lock_irqsave(&current->delays->allocpages_lock, flags);
		current->delays->allocpages_delay += ns;
		current->delays->allocpages_count++;
		if (current->delays->allocpages_delay_max < ns) {
			current->delays->allocpages_delay_max = ns;
			current->delays->allocpages_delay_max_order = order;
		}
		current->delays->allocuser_delay += ns;
		current->delays->allocuser_count++;
		if (current->delays->allocuser_delay_max < ns) {
			current->delays->allocuser_delay_max = ns;
			current->delays->allocuser_delay_max_order = order;
		}
		spin_unlock_irqrestore(&current->delays->allocpages_lock, flags);
	}
}
#endif
