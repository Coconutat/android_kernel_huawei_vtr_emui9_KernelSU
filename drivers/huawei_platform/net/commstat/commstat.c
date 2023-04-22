#include <linux/types.h>
#include <linux/socket.h>
#include <linux/jhash.h>
#include <linux/slab.h>
#include <linux/hash.h>

#include <net/sock.h>

#include "commstat.h"

#define MAX_COMM_STATS 1024

static int num_comm_stats;

static LIST_HEAD(comm_stat_list);
static DEFINE_SPINLOCK(comm_stat_list_lock);

struct hlist_head *comm_head;

struct comm_stat {
	struct list_head list;
	struct hlist_node hlist;
	char *comm;
	uid_t uid;
	uint64_t tx_bytes;
	uint64_t rx_bytes;
};

#define COMM_HASHBITS    8
#define COMM_HASHENTRIES (1 << COMM_HASHBITS)

static struct hlist_head *comm_create_hash(void)
{
	int i;
	struct hlist_head *hash;

	hash = kmalloc(sizeof(*hash) * COMM_HASHENTRIES, GFP_KERNEL);
	if (hash != NULL)
		for (i = 0; i < COMM_HASHENTRIES; i++)
			INIT_HLIST_HEAD(&hash[i]);

	return hash;
}

static inline struct hlist_head *comm_hash(const char *name)
{
	u32 hash = jhash(name, strnlen(name, TASK_COMM_LEN), 0);

	return &comm_head[hash_32(hash, COMM_HASHBITS)];
}

struct comm_stat *get_comm_stat_entry(const char *comm)
{
	struct comm_stat *proc_entry;
	struct hlist_head *head = comm_hash(comm);

	hlist_for_each_entry(proc_entry, head, hlist)
		if (!strncmp(comm, proc_entry->comm, TASK_COMM_LEN))
			return proc_entry;

	return NULL;
}

static struct comm_stat *alloc_comm_stat_entry(const char *comm)
{
	struct comm_stat *entry;

	if (num_comm_stats + 1 > MAX_COMM_STATS) {
		pr_err("%s(): no room to add entry %s\n", __func__, comm);
		return NULL;
	}

	entry = kzalloc(sizeof(struct comm_stat), GFP_ATOMIC);
	if (entry == NULL) {
		pr_err("%s(): kzalloc fail for %s\n", __func__, comm);
		return NULL;
	}

	entry->comm = kstrdup(comm, GFP_ATOMIC);
	if (entry->comm == NULL) {
		pr_err("%s(): kstrdup fail for %s\n", __func__, comm);
		kfree(entry);
		return NULL;
	}

	num_comm_stats++;
	list_add(&entry->list, &comm_stat_list);
	hlist_add_head(&entry->hlist, comm_hash(comm));

	return entry;
}

void inet_save_comm_stat(struct socket *sock, int tx, int len)
{
	char comm[TASK_COMM_LEN];
	kuid_t uid;
	struct comm_stat *entry;

	get_task_comm(comm, current->group_leader);
	if (sock->file && sock->file->f_cred)
		uid = sock->file->f_cred->fsuid;
	else
		uid = make_kuid(&init_user_ns, 0);

	/*
	pr_info("%s(): comm_stat: %s %s %d bytes\n",
		__func__, comm, tx ? "send" : "recv", len);
	*/

	spin_lock_bh(&comm_stat_list_lock);

	entry = get_comm_stat_entry(comm);
	if (entry) {
		entry->tx_bytes += tx ? len : 0;
		entry->rx_bytes += tx ? 0 : len;
		spin_unlock_bh(&comm_stat_list_lock);
		return;
	}

	entry = alloc_comm_stat_entry(comm);
	if (!entry) {
		spin_unlock_bh(&comm_stat_list_lock);
		return;
	}
	entry->uid = from_kuid(&init_user_ns, uid);
	entry->tx_bytes = tx ? len : 0;
	entry->rx_bytes = tx ? 0 : len;

	spin_unlock_bh(&comm_stat_list_lock);

	return;
}

static struct proc_dir_entry *comm_stats_procdir;
static struct proc_dir_entry *comm_stats_procfile;

static void *comm_stats_proc_start(struct seq_file *m, loff_t *pos)
{
	loff_t n = *pos;

	spin_lock_bh(&comm_stat_list_lock);

	return seq_list_start(&comm_stat_list, n);
}

static void *comm_stats_proc_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &comm_stat_list, pos);
}

static void comm_stats_proc_stop(struct seq_file *m, void *p)
{
	spin_unlock_bh(&comm_stat_list_lock);
}

static int comm_stats_proc_show(struct seq_file *m, void *v)
{
	struct comm_stat *proc_entry;

	proc_entry = list_entry(v, struct comm_stat, list);

	seq_printf(m, "%s %u %llu %llu\n",
			proc_entry->comm,
			proc_entry->uid,
			proc_entry->rx_bytes,
			proc_entry->tx_bytes);

	return 0;
}

static const struct seq_operations proc_comm_stats_seqops = {
	.start = comm_stats_proc_start,
	.next = comm_stats_proc_next,
	.stop = comm_stats_proc_stop,
	.show = comm_stats_proc_show,
};

static int proc_comm_stats_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &proc_comm_stats_seqops);
}

static const struct file_operations comm_stats_fops = {
	.open		= proc_comm_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release_private,
};

int __init comm_stat_init(void)
{
	int err;
	comm_head = comm_create_hash();

	comm_stats_procdir = proc_mkdir("comm", init_net.proc_net);
	if (!comm_stats_procdir) {
		pr_err("comm_stat: failed to create /proc/net/comm\n");
		err = -1;
		goto err;
	}

	comm_stats_procfile = proc_create_data("stats",
						   S_IRUGO,
						   comm_stats_procdir,
						   &comm_stats_fops,
						   NULL);
	if (!comm_stats_procfile) {
		pr_err("comm_stat: failed to create /proc/net/comm/stats\n");
		err = -1;
		goto no_stats_entry;
	}

	return 0;
no_stats_entry:
	remove_proc_entry("comm", init_net.proc_net);
err:
	return err;
}
