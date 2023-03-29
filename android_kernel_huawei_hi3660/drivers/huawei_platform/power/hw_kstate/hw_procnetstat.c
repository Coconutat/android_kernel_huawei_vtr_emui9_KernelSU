/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2017. All rights reserved.
 *
 * File name: hw_procnetstat.c
 * Description: This file use to statistics net info, including mobile and wifi bytes and packets.
 * Author: yangqinghua@huawei.com
 * Version: 1.0
 * Date:  2017/05/11
 */
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/fdtable.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/crc32.h>
#include <net/sock.h>
#include <net/inet_sock.h>
#include <linux/string.h>

#define MAX_PATH_LENGTH 255
#define PROC_NAME_LENGTH 16
#define KEY_LENGTH 80
#define STAT_HASHTABLE_SIZE 10
#define MAX_CTRL_CMD_LENGTH 512
#define MAX_SHARED_UID_NUM 80
#define CTRL_SET_SHARED_UID 1
#define DEFAULT_SHARED_UID_NUMBER 4

#define PROC_NET_INFO_NODE "proc_netstat"
#define STATS "stats"
#define CTRL "ctrl"
#define DEFAULT "default"

/* max store 80 shared uids */
static int shared_uids[MAX_SHARED_UID_NUM] = {0};
static int shared_uid_num = 0;

/* stat_splock used to stat data */
static DEFINE_RWLOCK(stat_splock);

/* iface list, such as wlan0, rmnet0 */
static struct list_head iface_stat_list;

/* to store bytes and packets */
struct data_entry {
	uint64_t rx_bytes;
	uint64_t tx_bytes;
	uint64_t rx_packets;
	uint64_t tx_packets;
};

/* pid info */
struct pid_stat {
	/* hash list node */
	struct hlist_node node;
	/* key for hash list */
	pid_t pid;
	uid_t uid;
	struct data_entry data;
};

/* proc info */
struct proc_stat {
	/* hash list node */
	struct hlist_node node;
	/* key transfer from procName + uid */
	uint32_t hash_code;
	pid_t pid;
	uid_t uid;
	char proc_name[PROC_NAME_LENGTH];
	struct data_entry data;
};

/* iface info */
struct iface_stat {
	struct list_head list;
	DECLARE_HASHTABLE(pid_stat_table, STAT_HASHTABLE_SIZE);
	DECLARE_HASHTABLE(proc_stat_table, STAT_HASHTABLE_SIZE);
	char if_name[IFNAMSIZ + 1];
};

static struct iface_stat *find_or_create_iface_entry(const char *if_name)
{
	struct iface_stat *iface_entry;

	list_for_each_entry(iface_entry, &iface_stat_list, list) {
		if (!strncmp(if_name, iface_entry->if_name, IFNAMSIZ)) {
			return iface_entry;
		}
	}

	iface_entry = kzalloc(sizeof(struct iface_stat), GFP_ATOMIC);
	if (!iface_entry) {
		pr_err("%s, no mem\n", __func__);
		return NULL;
	}

	strlcpy(iface_entry->if_name, if_name, IFNAMSIZ);
	hash_init(iface_entry->pid_stat_table);
	hash_init(iface_entry->proc_stat_table);
	INIT_LIST_HEAD(&iface_entry->list);
	list_add(&iface_entry->list, &iface_stat_list);
	return iface_entry;
}

/*lint -e666*/
static struct proc_stat *find_or_create_proc_entry(struct iface_stat *iface_entry,
													const char *proc_name, uid_t uid)
{
	struct proc_stat *proc_entry;
	uint32_t hash_code;
	char key[KEY_LENGTH] = {0};

	/* proc name _ uid is the key */
	snprintf(key, (KEY_LENGTH - 1), "%s_%d", proc_name, uid);
	/* use crc32 of proc_name for hash */
	hash_code = crc32(0, key, strlen(key));

	hash_for_each_possible(iface_entry->proc_stat_table, proc_entry, node, hash_code) {
		if (proc_entry->hash_code == hash_code) {
			return proc_entry;
		}
	}
	proc_entry = kzalloc(sizeof(struct proc_stat), GFP_ATOMIC);
	if (!proc_entry) {
		pr_err("%s, no mem\n", __func__);
		return NULL;
	}

	strlcpy(proc_entry->proc_name, proc_name, PROC_NAME_LENGTH);
	memset(&proc_entry->data, 0, sizeof(proc_entry->data));
	proc_entry->hash_code = hash_code;
	proc_entry->pid = -1;
	proc_entry->uid = uid;
	INIT_HLIST_NODE(&proc_entry->node);
	hash_add(iface_entry->proc_stat_table, &proc_entry->node, hash_code);

	return proc_entry;
}
/*lint +e666*/

/* get data entry to store net data */
/*lint -e666 -e429*/
static struct data_entry *get_data_entry(struct iface_stat *iface_entry,
											pid_t pid, uid_t uid)
{
	struct pid_stat *pid_entry;
	struct proc_stat *proc_entry;
	ulong bkt;

	/* first check pid table */
	hash_for_each_possible(iface_entry->pid_stat_table, pid_entry, node, pid) {
		if (pid_entry->pid == pid && pid_entry->uid == uid) {
			return &(pid_entry->data);
		}
	}

	/* then check proc table, for some just dead pid. */
	hash_for_each(iface_entry->proc_stat_table, bkt, proc_entry, node) {
		if (proc_entry->pid == pid && proc_entry->uid == uid) {
			return &(proc_entry->data);
		}
	}

	/* create new pid entry */
	pid_entry = kzalloc(sizeof(struct pid_stat), GFP_ATOMIC);
	if (!pid_entry) {
		pr_err("%s, no mem\n", __func__);
		return NULL;
	}

	pid_entry->pid = pid;
	pid_entry->uid = uid;
	memset(&pid_entry->data, 0, sizeof(pid_entry->data));
	INIT_HLIST_NODE(&pid_entry->node);
	hash_add(iface_entry->pid_stat_table, &pid_entry->node, pid);

	return &(pid_entry->data);
}
/*lint +e666 +e429*/

static void update_data_entry(struct data_entry *data_entry, uint hooknum, uint len)
{
	switch (hooknum) {
		case NF_INET_LOCAL_IN: {
			data_entry->rx_bytes += len;
			data_entry->rx_packets++;
			break;
		}
		case NF_INET_LOCAL_OUT: {
			data_entry->tx_bytes += len;
			data_entry->tx_packets++;
			break;
		}
		default:
			break;
	}
}

/* add to default data entry of this uid */
static void add_to_default_data_entry(const char *if_name, uint len, uint hooknum, uid_t uid)
{
	struct iface_stat *iface_entry;
	struct proc_stat *proc_entry;

	if (!if_name) {
		pr_err("%s, null dev_name\n", __func__);
		return;
	}

	iface_entry = find_or_create_iface_entry(if_name);
	if (!iface_entry) {
		pr_err("%s, no mem\n", __func__);
		return;
	}

	write_lock_bh(&stat_splock);
	proc_entry = find_or_create_proc_entry(iface_entry, DEFAULT, uid);
	if (!proc_entry) {
		pr_err("%s, no mem\n", __func__);
		write_unlock_bh(&stat_splock);
		return;
	}
	update_data_entry(&proc_entry->data, hooknum, len);
	write_unlock_bh(&stat_splock);
}

/* add data to pid or proc hash table */
static void add_to_pid_stat(struct iface_stat *iface_entry,
							pid_t pid,
							uid_t uid,
							uint len,
							uint hooknum)
{
	struct data_entry *data_entry;

	write_lock_bh(&stat_splock);
	data_entry = get_data_entry(iface_entry, pid, uid);
	if (!data_entry) {
		write_unlock_bh(&stat_splock);
		pr_err("%s, no mem\n", __func__);
		return;
	}

	update_data_entry(data_entry, hooknum, len);
	write_unlock_bh(&stat_splock);
}

/* check whether is shared uid */
static bool is_shared_uid(uid_t uid)
{
	int i;
	bool ret = false;

	read_lock_bh(&stat_splock);
	for (i = 0; i < MAX_SHARED_UID_NUM; i++) {
		if (shared_uids[i] == uid) {
			ret = true;
			break;
		}

		if (i >= (shared_uid_num - 1)) {
			break;
		}
	}
	read_unlock_bh(&stat_splock);

	return ret;
}

/* account data to hash table */
void account_data(pid_t pid, uint len, const char *if_name, uint hooknum, int uid)
{
	struct iface_stat *iface_entry;

	if (!if_name) {
		pr_err("%s, null dev_name\n", __func__);
		return;
	}

	iface_entry = find_or_create_iface_entry(if_name);
	if (!iface_entry) {
		pr_err("%s, no mem\n", __func__);
		return;
	}

	add_to_pid_stat(iface_entry, pid, uid, len, hooknum);
}

/* hook to get data to account */
static unsigned int hook_datastat(void *priv,
							struct sk_buff *skb,
							const struct nf_hook_state *state)
{
	struct socket *socket;
	struct file *filp;
	struct sock *sk;
	struct net_device *dev = NULL;
	uid_t uid = 0;
	uint hook;
	pid_t pid = -1;
	bool isDefault = false;

	if (0 == shared_uid_num) {
		return NF_ACCEPT;
	}

	/* partial atomic, !in_atomic() */
	if (!skb || !skb->len || !state) {
		pr_err("%s, ignore empty buffer\n", __func__);
		return NF_ACCEPT;
	}

	hook = state->hook;
	sk = skb_to_full_sk(skb);

	/* check run-time state */
	if (NF_INET_LOCAL_OUT == hook) {
		dev = state->out;
	} else if (hook == NF_INET_LOCAL_IN) {
		dev = state->in;
	}
	if (NULL == dev || NULL == dev->name) {
		goto exit;
	}

	/*
	* When in TCP_TIME_WAIT the sk is not a "struct sock" but
	* "struct inet_timewait_sock" which is missing fields.
	* So we ignore it.
	* Otherwise if you visit sk->sk_socket->file, it may cause reboot.
	*/
	if (NULL == sk || (TCP_TIME_WAIT == sk->sk_state)) {
		goto account_default_and_exit;
	}

	read_lock_bh(&sk->sk_callback_lock);
	socket = sk->sk_socket;
	if (NULL != socket) {
		filp = socket->file;
		pid = socket->pid;
		if (NULL != filp && NULL != filp->f_cred) {
			uid = from_kuid(&init_user_ns, filp->f_cred->fsuid);
		}
	} else {
		/* sk->sk_socket is null, should account to default*/
		isDefault = true;
	}
	read_unlock_bh(&sk->sk_callback_lock);

	if (isDefault) {
		goto account_default_and_exit;
	}

	if (is_shared_uid(uid) == false) {
		return NF_ACCEPT;
	}
	account_data(pid, skb->len, dev->name, hook, uid);
	return NF_ACCEPT;

account_default_and_exit:
	if (is_shared_uid(uid) == false) {
		return NF_ACCEPT;
	}
	add_to_default_data_entry(dev->name, skb->len, hook, uid);
exit:
	/* must return NF_ACCEPT, so skb can be processed by other flow */
	return NF_ACCEPT;
}

/* read stats node will call this func. */
static int data_stat_show(struct seq_file *m, void *v)
{
	struct iface_stat *iface_entry;
	struct proc_stat *proc_entry;
	struct pid_stat *pid_entry;
	ulong bkt;

	/* not atomic, !in_atomic() */
	seq_printf(m, "if_name\tuid\tpid\tproc_name\trx_bytes\ttx_bytes\trx_packets\ttx_packets\n");
	read_lock_bh(&stat_splock);
	list_for_each_entry(iface_entry, &iface_stat_list, list) {
		hash_for_each(iface_entry->pid_stat_table, bkt, pid_entry, node) {
			seq_printf(m, "%s\t%d\t%d\t%s\t%llu\t%llu\t%llu\t%llu\n",
						iface_entry->if_name, pid_entry->uid, pid_entry->pid, "",
						pid_entry->data.rx_bytes, pid_entry->data.tx_bytes,
						pid_entry->data.rx_packets, pid_entry->data.tx_packets);
		}

		hash_for_each(iface_entry->proc_stat_table, bkt, proc_entry, node) {
			seq_printf(m, "%s\t%d\t%d\t%s\t%llu\t%llu\t%llu\t%llu\n",
						iface_entry->if_name, proc_entry->uid, -1, proc_entry->proc_name,
						proc_entry->data.rx_bytes, proc_entry->data.tx_bytes,
						proc_entry->data.rx_packets, proc_entry->data.tx_packets);
		}
	}
	read_unlock_bh(&stat_splock);

	return 0;
}

static void calc_pid_to_proc(struct pid_stat *pid_entry, struct proc_stat *proc_entry)
{
	proc_entry->data.rx_bytes += pid_entry->data.rx_bytes;
	proc_entry->data.tx_bytes += pid_entry->data.tx_bytes;
	proc_entry->data.rx_packets += pid_entry->data.rx_packets;
	proc_entry->data.tx_packets += pid_entry->data.tx_packets;
	/* mark this pid, becuase some data may on the road, we will add them to proc entry. */
	proc_entry->pid = pid_entry->pid;
}

/* move dead pid to proc */
static void move_dead_pid_to_proc(struct iface_stat *iface_entry, struct pid_stat *pid_entry, char *proc_name)
{
	struct proc_stat *proc_entry;

	proc_entry = find_or_create_proc_entry(iface_entry, proc_name, pid_entry->uid);
	if (!proc_entry) {
		pr_err("%s, no mem\n", __func__);
		return;
	}

	calc_pid_to_proc(pid_entry, proc_entry);
}

/* pid exit callback func */
static int process_exit_callback(struct notifier_block *self, ulong cmd, void *v)
{
	struct task_struct *task = v;
	struct iface_stat *iface_entry;
	struct pid_stat *pid_entry;
	struct hlist_node *tmp;
	unsigned long bkt;
	uid_t uid;

	if (0 == shared_uid_num) {
		return NOTIFY_OK;
	}

	/* ignore child threads, only care about main process. */
	if (!task || task != task->group_leader) {
		return NOTIFY_OK;
	}

	/* get the uid of this packet. */
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	if (is_shared_uid(uid) == false) {
		return NOTIFY_OK;
	}

	/*
	* dead process's task_struct will be invalid.
	* need to update pid_stat to proc_stat if its task points to this task.
	*/
	write_lock_bh(&stat_splock);
	list_for_each_entry(iface_entry, &iface_stat_list, list) {
		/* must use safe iter func, because we need delete node. */
		hash_for_each_safe(iface_entry->pid_stat_table, bkt, tmp, pid_entry, node) {
			if (pid_entry->pid != task->pid) {
				continue;
			}
			move_dead_pid_to_proc(iface_entry, pid_entry, task->comm);
			hash_del(&pid_entry->node);
			kfree(pid_entry);
			break;
		}
	}
	write_unlock_bh(&stat_splock);

	return NOTIFY_OK;
}

static void set_default_shared_uid(void) {
	shared_uids[0] = 0;
	shared_uids[1] = 1000;
	shared_uids[2] = 1001;
	shared_uids[3] = 1010;
	shared_uid_num = DEFAULT_SHARED_UID_NUMBER;
}

/* write ctrl node will call this func. */
static ssize_t ctrl_stat_write(struct file *file, const char __user *buffer, size_t count, loff_t *offp)
{
	char input_buf[MAX_CTRL_CMD_LENGTH] = {0};
	int cmd;
	int val;
	int index = DEFAULT_SHARED_UID_NUMBER;
	char *curr_pointer;
	char *blank;
	int ret;
	int i;
	bool valid;

	if (count >= MAX_CTRL_CMD_LENGTH) {
		pr_err("%s, exceed max count %ld\n", __func__, count);
		return -EINVAL;
	}

	if (copy_from_user(input_buf, buffer, count)) {
		pr_err("%s, copy_from_user error\n", __func__);
		return -EFAULT;
	}

	curr_pointer = input_buf;
	blank = strchr(curr_pointer, ' ');
	if (NULL == blank) {
		pr_err("%s, invald buffer: %s\n", __func__, curr_pointer);
		return -EINVAL;
	}

	ret = sscanf(curr_pointer, "%d", &cmd);
	if (-1 == ret) { /* -1 means no num found */
		pr_err("%s, invald cmd, buffer: %s\n", __func__, curr_pointer);
		return -EINVAL;
	}

	curr_pointer = ++blank;

	write_lock_bh(&stat_splock);
	switch (cmd) {
		case CTRL_SET_SHARED_UID: {
			while ((blank = strchr(curr_pointer, ' ')) != NULL) {
				ret = sscanf(curr_pointer, "%d", &val);
				if (-1 == ret) {/* -1 means no num found */
					pr_err("%s, invald uid, buffer: %s\n", __func__, curr_pointer);
					set_default_shared_uid();
					write_unlock_bh(&stat_splock);
					return -EINVAL;
				}
				valid = false;
				for (i = 0; i < DEFAULT_SHARED_UID_NUMBER; i++) {
					if (shared_uids[i] == val) {
						valid = true;
						break;
					}
				}
				if (!valid && index >= DEFAULT_SHARED_UID_NUMBER && index < MAX_SHARED_UID_NUM) {
					shared_uids[index] = val;
					index++;
				}
				if (index >= MAX_SHARED_UID_NUM) {
					break;
				}
				curr_pointer = ++blank;
			}
			shared_uid_num = index;
			break;
		}
		default:
			pr_err("%s, unexpected cmd: %d\n", __func__, cmd);
			break;
	}
	write_unlock_bh(&stat_splock);

	return count;
}

static const struct file_operations ctrl_stat_fops = {
	.write = ctrl_stat_write,
};

static int data_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, data_stat_show, PDE_DATA(inode));
}

static struct notifier_block netstat_process_nb = {
	.notifier_call = process_exit_callback,
};

static const struct file_operations data_stat_fops = {
	.open = data_stat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

// priority should be bigger than NF_IP_PRI_FILTER, some iface name may change
static struct nf_hook_ops netstat_nf_hooks[] = {
	{
		.hook = hook_datastat,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_FILTER + 1,
	},
	{
		.hook = hook_datastat,
		.pf = NFPROTO_IPV6,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP6_PRI_FILTER + 1,
	},
	{
		.hook = hook_datastat,
		.pf = NFPROTO_IPV4,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FILTER + 1,
	},
	{
		.hook = hook_datastat,
		.pf = NFPROTO_IPV6,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP6_PRI_FILTER + 1,
	}
};

/* module init func. */
static int __init procnetstat_init(void)
{
	struct proc_dir_entry *parent;
	struct proc_dir_entry *file;
	int err;

	/* stats uid 1000 for default */
	set_default_shared_uid();

	INIT_LIST_HEAD(&iface_stat_list);

	/* create root dir */
	parent = proc_mkdir(PROC_NET_INFO_NODE, init_net.proc_net);
	if (!parent) {
		pr_err("mk_proc_dir_err\n");
		goto mk_proc_dir_err;
	}

	/* create data bytes node */
	file = proc_create_data(STATS, S_IRUGO, parent, &data_stat_fops, NULL);
	if (!file) {
		pr_err("mk_proc_stats_err\n");
		goto mk_proc_stats_err;
	}

	/* create ctrl node */
	file = proc_create_data(CTRL, S_IRUGO | S_IWUGO, parent, &ctrl_stat_fops, NULL);
	if (!file) {
		pr_err("mk_proc_ctrl_err\n");
		goto mk_proc_ctrl_err;
	}

	/* register process exit notifier function */
	profile_event_register(PROFILE_TASK_EXIT, &netstat_process_nb);

	/* register data hook function */
	err = nf_register_hooks(netstat_nf_hooks, ARRAY_SIZE(netstat_nf_hooks));
	if (err < 0) {
		pr_err("nf_register_hooks_err\n");
		goto nf_register_hooks_err;
	}
	return 0;

nf_register_hooks_err:
	profile_event_unregister(PROFILE_TASK_EXIT, &netstat_process_nb);
	remove_proc_entry(CTRL, parent);
mk_proc_ctrl_err:
	remove_proc_entry(STATS, parent);
mk_proc_stats_err:
	remove_proc_entry(PROC_NET_INFO_NODE, init_net.proc_net);
mk_proc_dir_err:
	return -1;
}

module_init(procnetstat_init);
MODULE_AUTHOR("yangqinghua <yangqinghua@huawei.com>");
MODULE_DESCRIPTION("ProcNetStat: network flow statistics via process name and pid.");
