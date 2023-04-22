#ifndef _CGROUP_IOLIMIT_H
#define _CGROUP_IOLIMIT_H

#include <linux/cgroup.h>
#include <linux/atomic.h>
#include <linux/timer.h>

#ifdef CONFIG_CGROUP_IOLIMIT

struct iolimit_cgroup {
	struct cgroup_subsys_state      css;
	atomic64_t                      switching;

	atomic64_t			write_limit;
        s64                             write_part_nbyte;
        s64                             write_already_used;
        struct timer_list               write_timer;
        spinlock_t                      write_lock;
        wait_queue_head_t               write_wait;

        atomic64_t                      read_limit;
        s64                             read_part_nbyte;
        s64                             read_already_used;
        struct timer_list               read_timer;
        spinlock_t                      read_lock;
        wait_queue_head_t               read_wait;
};

static inline struct iolimit_cgroup *css_iolimit(struct cgroup_subsys_state *css)
{
        return css ? container_of(css, struct iolimit_cgroup, css) : NULL;
}

static inline struct iolimit_cgroup *task_iolimitcg(struct task_struct *tsk)
{
        return css_iolimit(task_css(tsk, iolimit_cgrp_id));
}

void do_io_write_bandwidth_control(size_t count);

void do_io_read_bandwidth_control(size_t count);

static void inline io_read_bandwidth_control(size_t count)
{
	if (likely(task_css_is_root(current, iolimit_cgrp_id)))
		return;
	else
		do_io_read_bandwidth_control(count);
}

static void inline io_write_bandwidth_control(size_t count)
{
	if (likely(task_css_is_root(current, iolimit_cgrp_id)))
		return;
	else
		do_io_write_bandwidth_control(count);
}

static void inline io_generic_read_bandwidth_control(size_t count)
{
	if (likely(task_css_is_root(current, iolimit_cgrp_id))) {
		return;
	}
	else {
		task_set_in_pagefault(current);
		do_io_read_bandwidth_control(count);
		task_clear_in_pagefault(current);
	}
}
#else  /* !CONFIG_CGROUP_IOLIMIT */

static inline void io_write_bandwidth_control(size_t count)
{
}

static inline void io_read_bandwidth_control(size_t count)
{
}

static void inline io_generic_read_bandwidth_control(size_t count)
{
}
#endif /* !CONFIG_CGROUP_IOLIMIT */

#endif /* _CGROUP_IOLIMIT_H */

