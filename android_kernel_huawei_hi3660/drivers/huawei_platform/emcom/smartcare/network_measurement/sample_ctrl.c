#include <net/sock.h>
#include "huawei_platform/emcom/smartcare/network_measurement/nm.h"

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
#define DECIMAL			10

struct sample_uid_list {
	unsigned int 		*uid;
	int			nums;
	struct rcu_head 	rcu;
};

static struct sample_uid_list *gbl_list;
static DEFINE_SPINLOCK(sample_uid_list_lock);

/*
 * Validate changes from /proc interface.
 * Change global valve of sampling: 1 upon open and 0 upon closed.
 */
int proc_sample_valve(struct ctl_table *ctl, int write, void __user *buffer,
		      size_t *lenp, loff_t *ppos)
{
	struct ctl_table tbl;
	int err = 0;
	int val = 0;
	char data[NM_SAMPLE_VALVE_BUF_MAX];

	if (write) {
		tbl.data = &val;
		tbl.maxlen = sizeof(int);
		tbl.procname = "netmeasure";
		err = proc_dointvec(&tbl, write, buffer, lenp, ppos);
		if (!err)
			network_measure = val ? nm_turn_on_valve(NULL) :
						nm_shut_off_valve(NULL);
	} else {
		val = network_measure;
		snprintf(data, sizeof(data), "Valve: %s\tUnix epoch: %u",
			 (val & VALVE_OPEN) ? "on" : "off",
			 (unsigned int)val >> VALVE_TS_SHIFT);
		tbl.data = data;
		tbl.maxlen = sizeof(data);
		tbl.procname = "netmeasure";
		err = proc_dostring(&tbl, write, buffer, lenp, ppos);
	}

	return err;
}
EXPORT_SYMBOL(proc_sample_valve);

static int compare_uid(const void *a, const void *b)
{
	return *(unsigned int *)a - *(unsigned int *)b; /* Ascending order */
}

static void sample_uid_list_reclaim(struct rcu_head *rp)
{
	struct sample_uid_list *p = NULL;

	p = container_of(rp, struct sample_uid_list, rcu);
	if (p) {
		if (p->uid)
			kfree(p->uid);

		kfree(p);
	}
}

/* Change the list of sample uids. */
static int set_sample_uid_list(char *val)
{
	struct sample_uid_list *new_list = NULL;
	struct sample_uid_list *old_list = NULL;
	char *clone = NULL;
	char *saved_clone = NULL;
	char *uid_str = NULL;
	int index = 0;
	int err = -ENOMEM; /* Out of memory */
	unsigned int uid = 0;

	if (!val)
		return -EFAULT; /* Bad address */

	clone = kstrdup(val, GFP_USER);
	if (unlikely(!clone))
		goto error;

	saved_clone = clone;
	new_list = kzalloc(sizeof(*new_list), GFP_USER);
	if (unlikely(!new_list))
		goto error;

	spin_lock(&sample_uid_list_lock);
	/* Must lock to obtain the backup of gbl_list. */
	old_list = gbl_list;

	/* Pass 1: check for bad entries. */
	while ((uid_str = strsep(&clone, " ")) && *uid_str) {
		err = kstrtouint((const char *)uid_str, DECIMAL, &uid);
		if (err)
			goto error_unlock;

		new_list->nums++;
	}

	kfree(saved_clone);
	clone = NULL;
	saved_clone = NULL;

	if (!new_list->nums)
		goto error_unlock;

	/* Pass 2: create the new list. */
	new_list->uid = kmalloc(sizeof(int) * new_list->nums, GFP_ATOMIC);
	if (!new_list->uid)
		goto error_unlock;

	while ((uid_str = strsep(&val, " ")) && *uid_str)
		err = kstrtouint((const char *)uid_str, DECIMAL,
				 &new_list->uid[index++]);

	/* Pass 3: sort the new list. */
	sort((void *)new_list->uid, (size_t)new_list->nums, sizeof(int),
	     compare_uid, NULL);

	rcu_assign_pointer(gbl_list, new_list);
	spin_unlock(&sample_uid_list_lock);
	if (old_list)
		call_rcu(&old_list->rcu, sample_uid_list_reclaim); /* reclaim */

	return err;

error_unlock:
	spin_unlock(&sample_uid_list_lock);

error:
	if (saved_clone)
		kfree(saved_clone);
	if (new_list)
		kfree(new_list);

	return err;
}

/* Build string with the list of sample uids */
static void get_sample_uid_list(char *buf, size_t maxlen)
{
	struct sample_uid_list *cur_list = NULL;
	int i = 0;
	size_t offs = 0;

	if (unlikely(!buf))
		return;

	rcu_read_lock();
	cur_list = rcu_dereference(gbl_list);

	if (!cur_list) {
		snprintf(buf, maxlen, "%s", "");
		rcu_read_unlock();
		return;
	}

	for (i = 0; i < cur_list->nums; i++)
		offs += snprintf(buf + offs, maxlen - offs, "%u ",
				 cur_list->uid[i]);

	rcu_read_unlock();
}

/* Validate changes from /proc interface. */
int proc_sample_uid_list(struct ctl_table *ctl, int write, void __user *buffer,
			 size_t *lenp, loff_t *ppos)
{
	char val[NM_SAMPLE_UID_BUF_MAX];
	struct ctl_table tbl = {
		.data = val,
		.maxlen = NM_SAMPLE_UID_BUF_MAX,
	};
	int err = 0;

	if (write) {
		err = proc_dostring(&tbl, write, buffer, lenp, ppos);
		if (!err)
			err = set_sample_uid_list(val);
	} else {
		get_sample_uid_list(val, NM_SAMPLE_UID_BUF_MAX);
		err = proc_dostring(&tbl, write, buffer, lenp, ppos);
	}

	return err;
}
EXPORT_SYMBOL(proc_sample_uid_list);

bool in_sample_uid_list(unsigned int uid)
{
	struct sample_uid_list *cur_list = NULL;
	void *res = NULL;
	bool ret = false;

	rcu_read_lock();
	cur_list = rcu_dereference(gbl_list);
	if (!cur_list) {
		rcu_read_unlock();
		return false;
	}

	res = bsearch((const void *)&uid, (const void *)cur_list->uid,
		      (size_t)cur_list->nums, sizeof(int), compare_uid);
	ret = res ? true : false;
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL(in_sample_uid_list);
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
