/*
 * Process number limiting controller for cgroups.
 *
 * Used to allow a cgroup hierarchy to stop any new processes
 * from fork()ing after a certain limit is reached.
 *
 * Since it is trivial to hit the task limit without hitting
 * any kmemcg limits in place, PIDs are a fundamental resource.
 * As such, PID exhaustion must be preventable in the scope of
 * a cgroup hierarchy by allowing resource limiting of the
 * number of tasks in a cgroup.
 *
 * In order to use the `pids` controller, set the maximum number
 * of tasks in pids.max (this is not available in the root cgroup
 * for obvious reasons). The number of processes currently
 * in the cgroup is given by pids.current. Organisational operations
 * are not blocked by cgroup policies, so it is possible to have
 * pids.current > pids.max. However, fork()s will still not work.
 *
 * To set a cgroup to have no limit, set pids.max to "max". fork()
 * will return -EBUSY if forking would cause a cgroup policy to be
 * violated.
 *
 * pids.current tracks all child cgroup hierarchies, so
 * parent/pids.current is a superset of parent/child/pids.current.
 *
 * Copyright (C) 2015 Aleksa Sarai <cyphar@cyphar.com>
 *
 */

#include <linux/kernel.h>
#include <linux/threads.h>
#include <linux/atomic.h>
#include <linux/cgroup.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/eventfd.h>
#include <linux/poll.h>
#include <linux/file.h>

#define PIDS_MAX (PID_MAX_LIMIT + 1ULL)

struct pids_cgroup {
	struct cgroup_subsys_state	css;

	/*
	 * Use 64-bit types so that we can safely represent "max" as
	 * (PID_MAX_LIMIT + 1).
	 */
	atomic64_t			counter;
	int64_t				limit;

	/*each group_pids is default limit*/
	int64_t				group_soft_limit;
	int64_t				group_limit;
	/* The list of pid_event structs. */
	struct		list_head event_list;
	/* Have to grab the lock on events traversal or modifications. */
	spinlock_t		event_list_lock;
	struct list_head		group_pids_list;

	int64_t				token;
};

static struct pids_cgroup *css_pids(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct pids_cgroup, css) : NULL;
}

static struct pids_cgroup *task_pids(struct task_struct *task)
{
	return css_pids(task_css(task, pids_cgrp_id));
}

static struct pids_cgroup *parent_pids(struct pids_cgroup *pids)
{
		return css_pids(pids->css.parent);
}

struct group_pids {
	/* All the tasks in the same QoS group are in this list */
	struct list_head	head;
	/* Linked to the global group_pids_list */
	struct list_head	node;
	int64_t				soft_limit;
	int64_t				limit;

	atomic64_t	counter;
	int64_t		token;
	/*true: has signal to user space, at default, it is false, no */
	bool	signal;
};

static void pids_event(struct pids_cgroup *pids, struct group_pids *gp);
static DEFINE_SPINLOCK(group_pids_lock);

static int group_pids_try_charge(struct group_pids *gp, int num)
{
	int64_t new;
	int ret;

	if (!gp)
		return 0;

	new = atomic64_add_return(num, &gp->counter);
	if (new > gp->limit) {
		ret = atomic64_add_negative(-num, &gp->counter);
		return -ENOMEM;
	}
	if (new > gp->soft_limit) {
		/*send a pid over soft limit event to user space*/
		return -EDQUOT;
	}
	return 0;
}

static void group_pids_uncharge(struct group_pids *gp, int num)
{
	if (!gp)
		return;
	WARN_ON_ONCE(atomic64_add_negative(-num, &gp->counter));
}

static void group_pids_migrate(struct task_struct *tsk, struct group_pids *gp)
{
	struct group_pids *old_gp = tsk->group_pids;

	if (old_gp) {
		list_del(&tsk->group_pids_list);
		atomic64_dec(&old_gp->counter);

		if (list_empty(&old_gp->head)) {
			list_del(&old_gp->node);
			kfree(old_gp);
		}
	}

	list_add_tail(&tsk->group_pids_list, &gp->head);
	atomic64_inc(&gp->counter);
	tsk->group_pids = gp;
}

static int group_pids_max_show(struct seq_file *m, void *V)
{
	struct pids_cgroup *pids = css_pids(seq_css(m));
	struct group_pids *gp;
	struct task_struct *task;

	spin_lock(&group_pids_lock);

	list_for_each_entry(gp, &pids->group_pids_list, node) {
		WARN_ON(list_empty(&gp->head));

		task = list_first_entry(&gp->head, struct task_struct,
					group_pids_list);
		seq_printf(m, "%llu %lu %llu %llu %llu\n",
			   (long long)gp->token,
			   (long)task_tgid_vnr(task),
			   (long long)gp->soft_limit,
			   (long long)gp->limit,
			   (long long)atomic64_read(&gp->counter));
	}

	spin_unlock(&group_pids_lock);
	return 0;
}

static int group_pids_limit_show(struct seq_file *m, void *V)
{
	struct pids_cgroup *pids = css_pids(seq_css(m));

	seq_printf(m, "%llu,%llu\n",
			   (long long)pids->group_soft_limit,
			   (long long)pids->group_limit);
	return 0;
}

static ssize_t group_pids_limit_write(struct kernfs_open_file *of,
				    char *buf, size_t nbytes, loff_t off)
{
	struct pids_cgroup *pids = css_pids(of_css(of));
	struct group_pids *gp;
	int64_t soft_limit, max;

	if (sscanf(buf, "%llu,%llu", &soft_limit, &max) != 2)
		return -EINVAL;

	if (soft_limit > max)
		return -EINVAL;

	pids->group_soft_limit = soft_limit;
	pids->group_limit = max;

	spin_lock(&group_pids_lock);

	list_for_each_entry(gp, &pids->group_pids_list, node) {
		gp->soft_limit = soft_limit;
		gp->limit = max;
	}

	spin_unlock(&group_pids_lock);
	return nbytes;
}


static struct cgroup_subsys_state *pids_css_alloc(
				    struct cgroup_subsys_state *parent_css)
{
	struct pids_cgroup *pids;

	pids = kzalloc(sizeof(struct pids_cgroup), GFP_KERNEL);
	if (!pids)
		return ERR_PTR(-ENOMEM);

	pids->limit = PIDS_MAX;
	pids->group_soft_limit = PIDS_MAX;
	pids->group_limit = PIDS_MAX;
	atomic64_set(&pids->counter, 0);
	INIT_LIST_HEAD(&pids->group_pids_list);
	INIT_LIST_HEAD(&pids->event_list);
	spin_lock_init(&pids->event_list_lock);
	return &pids->css;
}

static void pids_css_free(struct cgroup_subsys_state *css)
{
	kfree(css_pids(css));
}


/**
 * pids_cancel - uncharge the local pid count
 * @pids: the pid cgroup state
 * @num: the number of pids to cancel
 *
 * This function will WARN if the pid count goes under 0,
 * because such a case is a bug in the pids controller proper.
 */
static void pids_cancel(struct pids_cgroup *pids, int num)
{
	/*
	 * A negative count (or overflow for that matter) is invalid,
	 * and indicates a bug in the pids controller proper.
	 */
	WARN_ON_ONCE(atomic64_add_negative(-num, &pids->counter));
}

/**
 * pids_uncharge - hierarchically uncharge the pid count
 * @pids: the pid cgroup state
 * @num: the number of pids to uncharge
 */
static void pids_uncharge(struct pids_cgroup *pids, int num)
{
	struct pids_cgroup *p;

	for (p = pids; p; p = parent_pids(p))
		pids_cancel(p, num);
}

/**
 * pids_charge - hierarchically charge the pid count
 * @pids: the pid cgroup state
 * @num: the number of pids to charge
 *
 * This function does *not* follow the pid limit set. It cannot
 * fail and the new pid count may exceed the limit, because
 * organisational operations cannot fail in the unified hierarchy.
 */
static void pids_charge(struct pids_cgroup *pids, int num)
{
	struct pids_cgroup *p;

	for (p = pids; p; p = parent_pids(p))
		atomic64_add(num, &p->counter);
}

/**
 * pids_try_charge - hierarchically try to charge the pid count
 * @pids: the pid cgroup state
 * @num: the number of pids to charge
 *
 * This function follows the set limit. It will fail if the charge
 * would cause the new value to exceed the hierarchical limit.
 * Returns 0 if the charge succeded, otherwise -EAGAIN.
 */
static int pids_try_charge(struct pids_cgroup *pids, int num)
{
	struct pids_cgroup *p, *q;

	for (p = pids; p; p = parent_pids(p)) {
		int64_t new = atomic64_add_return(num, &p->counter);

		if (new > p->limit)
			goto revert;
	}

	return 0;

revert:
	for (q = pids; q != p; q = parent_pids(q))
		pids_cancel(q, num);
	pids_cancel(p, num);

	return -EAGAIN;
}

/* This is protected by cgroup lock */
static struct group_pids *tmp_gp;
static struct pids_cgroup *pids_attach_old_cs;

static int pids_can_attach(struct cgroup_taskset *tset)
{
	struct cgroup_subsys_state *css;
	struct pids_cgroup *pids;

	/* used later by pids_attach() */
	pids_attach_old_cs = task_pids(cgroup_taskset_first(tset, &css));
	pids = css_pids(css);

	WARN_ON(tmp_gp);
	tmp_gp = kzalloc(sizeof(*tmp_gp), GFP_KERNEL);
	if (!tmp_gp)
		return -ENOMEM;

	tmp_gp->token = pids->token++;
	tmp_gp->limit = PIDS_MAX;
	atomic64_set(&tmp_gp->counter, 0);
	INIT_LIST_HEAD(&tmp_gp->head);
	tmp_gp->soft_limit = pids->group_soft_limit;
	tmp_gp->limit = pids->group_limit;

	return 0;
}

static void pids_cancel_attach(struct cgroup_taskset *tset)
{
	kfree(tmp_gp);
	tmp_gp = NULL;
}

static void pids_attach(struct cgroup_taskset *tset)
{ 
	struct cgroup_subsys_state *css;
	struct pids_cgroup *pids;
	struct task_struct *task;
	int64_t num = 0;

	cgroup_taskset_first(tset,&css);
	pids = css_pids(css);

	spin_lock(&group_pids_lock);

	cgroup_taskset_for_each(task,css,tset) {
		num++;
		pids_uncharge(pids_attach_old_cs, 1);

		group_pids_migrate(task, tmp_gp);
	}
	list_add(&tmp_gp->node, &pids->group_pids_list);

	spin_unlock(&group_pids_lock);

	/*
	 * Attaching to a cgroup is allowed to overcome the
	 * the PID limit, so that organisation operations aren't
	 * blocked by the `pids` cgroup controller.
	 */
	pids_charge(pids, num);

	tmp_gp = NULL;
}

int cgroup_pids_can_fork(void)
{
	struct pids_cgroup *pids = NULL;
	int ret;

	rcu_read_lock();
	pids = task_pids(current);
	rcu_read_unlock();
	ret = pids_try_charge(pids, 1);
	if (ret)
		return ret;

	spin_lock(&group_pids_lock);
	ret = group_pids_try_charge(current->group_pids, 1);
	if (ret == -ENOMEM) {
		pr_warn("Pid %d(%s) over pids cgroup hard_limit\n",
		     task_tgid_vnr(current), current->comm);
		pids_uncharge(pids, 1);
	} else if (ret == -EDQUOT) {
		pids_event(pids, current->group_pids);
		ret = 0;
	}
	spin_unlock(&group_pids_lock);
	return ret;

}

void cgroup_pids_cancel_fork(void)
{

	struct pids_cgroup *pids = NULL;

	rcu_read_lock();
	pids = task_pids(current);
	rcu_read_unlock();
	pids_uncharge(pids, 1);

	spin_lock(&group_pids_lock);
	group_pids_uncharge(current->group_pids, 1);
	spin_unlock(&group_pids_lock);
}

static void pids_fork(struct task_struct *tsk,void **priv)
{
	spin_lock(&group_pids_lock);
	if (current->group_pids) {
		tsk->group_pids = current->group_pids;
		list_add_tail(&tsk->group_pids_list,
			      &current->group_pids->head);
	}
	spin_unlock(&group_pids_lock);
}

static void pids_exit(struct task_struct *task)
{
	struct css_set *cset;
	struct cgroup_subsys_state *old_css;
	struct pids_cgroup *pids;
 
	cset = task_css_set(task);
	if (!cset)
	return;

	old_css = cset->subsys[pids_cgrp_id];
	pids = css_pids(old_css);
	pids_uncharge(pids, 1);

	spin_lock(&group_pids_lock);

	if (task->group_pids) {
		atomic64_dec(&task->group_pids->counter);
		list_del(&task->group_pids_list);

		if (list_empty(&task->group_pids->head)) {
			list_del(&task->group_pids->node);
			kfree(task->group_pids);
		}
	}

	spin_unlock(&group_pids_lock);
}

static int pids_max_write(struct cgroup_subsys_state *css,
		     struct cftype *cft, s64 max)
{
	struct pids_cgroup *pids = css_pids(css);

	if (max < 0 || max > INT_MAX)
		return -EINVAL;

	/*
	 * Limit updates don't need to be mutex'd, since it isn't
	 * critical that any racing fork()s follow the new limit.
	 */
	pids->limit = max;
	return 0;
}

static s64 pids_max_show(struct cgroup_subsys_state *css,
		     struct cftype *cft)
{
	struct pids_cgroup *pids = css_pids(css);

	return pids->limit;
}

static s64 pids_current_read(struct cgroup_subsys_state *css,
		     struct cftype *cft)
{
	struct pids_cgroup *pids = css_pids(css);

	return atomic64_read(&pids->counter);
}

/**
 *a notify eventfd to report which process to over the process number limit
 */
struct pids_event {
	struct eventfd_ctx *efd;
	struct list_head node;
};

/**
 * when a process have soft limit
 */
static void pids_event(struct pids_cgroup *pids, struct group_pids *gp)
{
	struct pids_event *ev;

	/*is gp has signal to user space, protect by group_pids_lock*/
	if (gp->signal)
		return;

	spin_lock(&pids->event_list_lock);
	list_for_each_entry(ev, &pids->event_list, node) {
		eventfd_signal(ev->efd, 1);
		gp->signal = true;
	}
	spin_unlock(&pids->event_list_lock);

}

static int group_pids_events_show(struct seq_file *m, void *V)
{
	struct pids_cgroup *pids = css_pids(seq_css(m));
	struct group_pids *gp;
	struct task_struct *task;
	int64_t count;

	spin_lock(&group_pids_lock);
	list_for_each_entry(gp, &pids->group_pids_list, node) {
		WARN_ON(list_empty(&gp->head));

		task = list_first_entry(&gp->head, struct task_struct,
					group_pids_list);

		count = atomic64_read(&gp->counter);
		if (count < gp->soft_limit)
			continue;

		seq_printf(m, "%llu %lu %lu %llu\n",
			    (long long)gp->token,
			    (long)task_tgid_vnr(task),
			    (long)from_kuid_munged(current_user_ns(), task_uid(task)),
			    (long long)count);
	}
	spin_unlock(&group_pids_lock);
	return 0;
}


/*
 * cgroup_event represents events which userspace want to receive.
 */
struct pids_cgroup_event {
	/*
	 * memcg which the event belongs to.
	 */
	struct pids_cgroup *pids;
	/*
	 * eventfd to signal userspace about the event.
	 */
	struct eventfd_ctx *eventfd;
	/*
	 * Each of these stored in a list by the cgroup.
	 */
	struct list_head list;
	/*
	 * register_event() callback will be used to add new userspace
	 * waiter for changes related to this event.  Use eventfd_signal()
	 * on eventfd to send notification to userspace.
	 */
	int (*register_event)(struct pids_cgroup *pids,
			      struct eventfd_ctx *eventfd, const char *args);
	/*
	 * unregister_event() callback will be called when userspace closes
	 * the eventfd or on cgroup removing.  This callback must be set,
	 * if you want provide notification functionality.
	 */
	void (*unregister_event)(struct pids_cgroup *pids,
				 struct eventfd_ctx *eventfd);
	/*
	 * All fields below needed to unregister event when
	 * userspace closes eventfd.
	 */
	poll_table pt;
	wait_queue_head_t *wqh;
	wait_queue_t wait;
	struct work_struct remove;
};

static int pids_cgroup_usage_register_event(struct pids_cgroup *pids,
			    struct eventfd_ctx *eventfd, const char *args)
{
	struct pids_event *ev;

	ev = kzalloc(sizeof(*ev), GFP_KERNEL);
	if (!ev)
		return -ENOMEM;
	ev->efd = eventfd;

	spin_lock(&pids->event_list_lock);
	list_add(&ev->node, &pids->event_list);
	spin_unlock(&pids->event_list_lock);

	return 0;
}

static void pids_cgroup_usage_unregister_event(struct pids_cgroup *pids,
			    struct eventfd_ctx *eventfd)
{
	struct pids_event *ev;

	spin_lock(&pids->event_list_lock);
	list_for_each_entry(ev, &pids->event_list, node) {
		if (ev->efd != eventfd)
			continue;
		list_del(&ev->node);
		kfree(ev);
		break;
	}
	spin_unlock(&pids->event_list_lock);
}

/*
 * DO NOT USE IN NEW FILES.
 *
 * "cgroup.event_control" implementation.
 *
 * This is way over-engineered.  It tries to support fully configurable
 * events for each user.  Such level of flexibility is completely
 * unnecessary especially in the light of the planned unified hierarchy.
 *
 * Please deprecate this and replace with something simpler if at all
 * possible.
 */

/*
 * Unregister event and free resources.
 *
 * Gets called from workqueue.
 */
static void pids_event_remove(struct work_struct *work)
{
	struct pids_cgroup_event *event =
		container_of(work, struct pids_cgroup_event, remove);
	struct pids_cgroup *pids = event->pids;

	remove_wait_queue(event->wqh, &event->wait);

	event->unregister_event(pids, event->eventfd);

	/* Notify userspace the event is going away. */
	eventfd_signal(event->eventfd, 1);

	eventfd_ctx_put(event->eventfd);
	kfree(event);
	css_put(&pids->css);
}

/*
 * Gets called on POLLHUP on eventfd when user closes it.
 *
 * Called with wqh->lock held and interrupts disabled.
 */
static int pids_event_wake(wait_queue_t *wait, unsigned mode,
			    int sync, void *key)
{
	struct pids_cgroup_event *event =
		container_of(wait, struct pids_cgroup_event, wait);
	struct pids_cgroup *pids = event->pids;
	unsigned long flags = (unsigned long)key;

	if (flags & POLLHUP) {
		/*
		 * If the event has been detached at cgroup removal, we
		 * can simply return knowing the other side will cleanup
		 * for us.
		 *
		 * We can't race against event freeing since the other
		 * side will require wqh->lock via remove_wait_queue(),
		 * which we hold.
		 */
		spin_lock(&pids->event_list_lock);
		if (!list_empty(&event->list)) {
			list_del_init(&event->list);
			/*
			 * We are in atomic context, but cgroup_event_remove()
			 * may sleep, so we have to call it in workqueue.
			 */
			schedule_work(&event->remove);
		}
		spin_unlock(&pids->event_list_lock);
	}

	return 0;
}

static void pids_event_ptable_queue_proc(struct file *file,
		wait_queue_head_t *wqh, poll_table *pt)
{
	struct pids_cgroup_event *event =
		container_of(pt, struct pids_cgroup_event, pt);

	event->wqh = wqh;
	add_wait_queue(wqh, &event->wait);
}

/*
 * DO NOT USE IN NEW FILES.
 *
 * Parse input and register new cgroup event handler.
 *
 * Input must be in format '<event_fd> <control_fd> <args>'.
 * Interpretation of args is defined by control file implementation.
 */
static ssize_t pids_write_event_control(struct kernfs_open_file *of,
					 char *buf, size_t nbytes, loff_t off)
{
	struct cgroup_subsys_state *css = of_css(of);
	struct pids_cgroup *pids = css_pids(css);
	struct pids_cgroup_event *event;
	struct cgroup_subsys_state *cfile_css;
	unsigned int efd, cfd;
	struct fd efile;
	struct fd cfile;
	const char *name;
	char *endp;
	int ret;

	buf = strstrip(buf);

	efd = simple_strtoul(buf, &endp, 10);
	if (*endp != ' ')
		return -EINVAL;
	buf = endp + 1;

	cfd = simple_strtoul(buf, &endp, 10);
	if ((*endp != ' ') && (*endp != '\0'))
		return -EINVAL;
	buf = endp + 1;

	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!event)
		return -ENOMEM;

	event->pids = pids;
	INIT_LIST_HEAD(&event->list);
	init_poll_funcptr(&event->pt, pids_event_ptable_queue_proc);
	init_waitqueue_func_entry(&event->wait, pids_event_wake);
	INIT_WORK(&event->remove, pids_event_remove);

	efile = fdget(efd);
	if (!efile.file) {
		ret = -EBADF;
		goto out_kfree;
	}

	event->eventfd = eventfd_ctx_fileget(efile.file);
	if (IS_ERR(event->eventfd)) {
		ret = PTR_ERR(event->eventfd);
		goto out_put_efile;
	}

	cfile = fdget(cfd);
	if (!cfile.file) {
		ret = -EBADF;
		goto out_put_eventfd;
	}

	/* the process need read permission on control file */
	/* AV: shouldn't we check that it's been opened for read instead? */
	ret = inode_permission(file_inode(cfile.file), MAY_READ);
	if (ret < 0)
		goto out_put_cfile;

	/*
	 * Determine the event callbacks and set them in @event.  This used
	 * to be done via struct cftype but cgroup core no longer knows
	 * about these events.  The following is crude but the whole thing
	 * is for compatibility anyway.
	 *
	 * DO NOT ADD NEW FILES.
	 */
	name = cfile.file->f_path.dentry->d_name.name;

	if (!strcmp(name, "pids.group_event")) {
		event->register_event = pids_cgroup_usage_register_event;
		event->unregister_event = pids_cgroup_usage_unregister_event;
	} else {
		ret = -EINVAL;
		goto out_put_cfile;
	}

	/*
	 * Verify @cfile should belong to @css.  Also, remaining events are
	 * automatically removed on cgroup destruction but the removal is
	 * asynchronous, so take an extra ref on @css.
	 */
	cfile_css = css_tryget_online_from_dir(cfile.file->f_path.dentry->d_parent,
					       &pids_cgrp_subsys);
	ret = -EINVAL;
	if (IS_ERR(cfile_css))
		goto out_put_cfile;
	if (cfile_css != css) {
		css_put(cfile_css);
		goto out_put_cfile;
	}

	ret = event->register_event(pids, event->eventfd, buf);
	if (ret)
		goto out_put_css;

	efile.file->f_op->poll(efile.file, &event->pt);

	spin_lock(&pids->event_list_lock);
	list_add(&event->list, &pids->event_list);
	spin_unlock(&pids->event_list_lock);

	fdput(cfile);
	fdput(efile);

	return nbytes;

out_put_css:
	css_put(css);
out_put_cfile:
	fdput(cfile);
out_put_eventfd:
	eventfd_ctx_put(event->eventfd);
out_put_efile:
	fdput(efile);
out_kfree:
	kfree(event);

	return ret;
}

static void pids_css_offline(struct cgroup_subsys_state *css)
{
	struct pids_cgroup *pids = css_pids(css);
	struct pids_cgroup_event *event, *tmp;

	/*
	 * Unregister events and notify userspace.
	 * Notify userspace about cgroup removing only after rmdir of cgroup
	 * directory to avoid race between userspace and kernelspace.
	 */
	spin_lock(&pids->event_list_lock);
	list_for_each_entry_safe(event, tmp, &pids->event_list, list) {
		list_del_init(&event->list);
		schedule_work(&event->remove);
	}
	spin_unlock(&pids->event_list_lock);
}

static struct cftype files[] = {
	{
		.name = "max",
		.write_s64 = pids_max_write,
		.read_s64 = pids_max_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{
		.name = "current",
		.read_s64 = pids_current_read,
	},
	{
		.name = "group_limit",
		.seq_show = group_pids_limit_show,
		.write = group_pids_limit_write,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{
		.name = "group_tasks",
		.seq_show = group_pids_max_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{
		.name = "cgroup.event_control",		/* XXX: for compat */
		.write = pids_write_event_control,
		.flags = CFTYPE_NO_PREFIX | CFTYPE_WORLD_WRITABLE,
	},
	{
		.name = "group_event",
		.seq_show = group_pids_events_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{ }	/* terminate */
};

struct cgroup_subsys pids_cgrp_subsys = {
	.css_alloc = pids_css_alloc,
	.css_offline = pids_css_offline,
	.css_free = pids_css_free,
	.can_attach = pids_can_attach,
	.cancel_attach = pids_cancel_attach,
	.attach = pids_attach,
	.fork = pids_fork,
	.exit = pids_exit,
	.legacy_cftypes = files,
	.early_init = 0,
};
