/* io_monitor.c
 *
 * Copyright (C) 2014 - 2015 Huawei, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef CONFIG_HUAWEI_IO_MONITOR

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/time.h>
#include <linux/writeback.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/uio.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/genhd.h>
#include <linux/statfs.h>
#include <linux/vfs.h>
#include <linux/namei.h>

#include <iotrace/io_monitor.h>
/*lint -save -e550 -e64 -e647 -e529 -e438 -e1072 -e50 -e1058 -e838 -e774 -e712 -e730 -e737 -e715 -e845*/
/*lint -save -e1514 -e570 -e574 -e530 -e40 -e409 -e151 -e48 -e501 -e732 -e747 -e826 -e785 -e713 -e734*/
#define IO_MONITOR_DIR  "/data/log/jank/iotrace/"
enum{
	IO_DEBUG = 0,
	IO_ERROR,
};

#define IO_PRINT_LEVEL  IO_ERROR
#define io_monitor_print(level, fmt, arg...) do { \
	if (level >= IO_PRINT_LEVEL) \
		pr_err("[IO_MON:%s:%d]"fmt, __func__, __LINE__, ##arg); \
} while (0)

#define IO_MONITOR_BUFF_SIZE    8192

#define KB  (1024)
#define MB  (KB * KB)
#define GB  (KB * MB)

struct io_monitor_ctl {
	spinlock_t ctl_spinlock;
	struct list_head  upload_list;
	struct list_head  wait_list;
	unsigned char all_equal;
	unsigned char in_memory;
	unsigned short reserved;
	struct timer_list   file_timer;

	struct io_module_template *log_module[IO_MONITOR_CNT];
	struct task_struct *io_work_thread;
	atomic_t  reg_cnt;
	unsigned char *io_buf;
	struct file *io_file;
};
static struct io_monitor_ctl *monitor_ctl;

#define IO_MONITOR_MOD_SIZE  1024
#define IO_BLK_UPLOAD_INTER  3600000
#define IO_MONITOR_UPLOAD_INTER	(1 * 24 * 3600)

#define IO_BLK_DMD_CODE	914009000
struct io_monitor_file_header {
	unsigned int magic;
	unsigned int file_size;
};

struct io_monitor_file_end {
	unsigned int magic;
};
#define MAGIC_HEAD	0x5A5A
#define MAGIC_END	0x5A5B
#define MAGIC_BLK	0x5A5C
#define MAGCI_IO	0x5A5D

#define IO_MONITOR_MOD_OFFSET(mod_id) ((mod_id) * IO_MONITOR_MOD_SIZE)
#define GET_MOD_OFFSET(mod_id) ((sizeof(struct io_monitor_file_header)) + \
		IO_MONITOR_MOD_OFFSET(mod_id))
#define GET_HEAD_OFFSET     0
#define GET_END_OFFSET  (GET_MOD_OFFSET(IO_MONITOR_CNT))

#define GET_FILE_HEAD(buf)  (((struct io_monitor_file_header *)(buf)))
#define GET_FILE_END(buf) ((struct io_monitor_file_end *)((buf) + \
			GET_MOD_OFFSET(IO_MONITOR_CNT)))
#define GET_FILE_MOD_OFFSET(buf, mod_id) ((buf) + GET_MOD_OFFSET(mod_id))

static struct file *open_log_file(unsigned char *name)
{
	mm_segment_t oldfs;
	struct file *filp = NULL;

	if (name == NULL)
		return NULL;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(name, O_CREAT | O_RDWR, 0644);

	set_fs(oldfs);

	if (IS_ERR(filp)) {
		io_monitor_print(IO_ERROR, "failed to open %s: %ld\n",
			name, PTR_ERR(filp));
		return NULL;
	}
	return filp;
}

static int write_log_file(struct file *filp, unsigned char *buf, int len)
{
	int ret;
	mm_segment_t oldfs;
	struct iovec vec[1];

	if (filp == NULL || buf == NULL)
		return -1;

	vec[0].iov_base = buf;
	vec[0].iov_len  = len;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_writev(filp, vec, 1, &filp->f_pos);
	set_fs(oldfs);
	if (unlikely(ret < 0))
		io_monitor_print(IO_ERROR, "failed to write\n");

	if (ret != len) {
		io_monitor_print(IO_ERROR, "want write: %d, real write: %d\n",
			len, ret);
		return -1;
	}
	return ret;
}

static int lseek_log_file(struct file *filp, loff_t offset, int whence)
{
	int ret;
	mm_segment_t oldfs;

	if (filp == NULL)
		return -1;
	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_llseek(filp, offset, whence);
	set_fs(oldfs);
	if (unlikely(ret < 0))
		io_monitor_print(IO_ERROR, "failed to lseek\n");

	return ret;
}

static int read_log_file(struct file *filp, unsigned char *buf, int size)
{
	mm_segment_t oldfs;
	struct iovec vec[1];
	int ret;

	if (filp == NULL || buf == NULL)
		return -1;

	vec[0].iov_base = buf;
	vec[0].iov_len = size;

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_readv(filp, (struct iovec *)&vec, 1, &filp->f_pos);

	set_fs(oldfs);

	return ret;
}

static int check_log_file(unsigned char *buf)
{
	struct io_monitor_file_header *h = GET_FILE_HEAD(buf);
	struct io_monitor_file_end *e = GET_FILE_END(buf);

	if (buf == NULL)
		return -1;
	if (h->magic != MAGIC_HEAD|| e->magic != MAGIC_END) {
		io_monitor_print(IO_ERROR, "head magic:%x,end magic:%x\n",
				h->magic, e->magic);
		io_monitor_print(IO_ERROR, "head offset:%x, end offset:%lx\n",
				GET_HEAD_OFFSET, GET_END_OFFSET);
		return -1;
	}

	io_monitor_print(IO_DEBUG, "head magic:%x, end magic:%x\n", h->magic,
			e->magic);

	return 0;
}

static int failed_imonitor_upload(void)
{

	int ret = 0;
	struct imonitor_eventobj *obj = imonitor_create_eventobj(IO_BLK_DMD_CODE);

	if (!obj) {
		io_monitor_print(IO_ERROR, "obj failed. %d\n", IO_BLK_DMD_CODE);
		return -1;
	}
	ret = ret | imonitor_set_param(obj, E914009000_HOST_TOTAL_READ_INT,
		0xffffffff);
	ret = ret | imonitor_set_param(obj, E914009000_HOST_TOTAL_WRITE_INT,
		0xffffffff);

	if (!ret)
		imonitor_send_event(obj);
	imonitor_destroy_eventobj(obj);

	return 0;
}

static int truncate_log_file(unsigned char *name)
{
	struct path file_path;
	int err;

	if (name == NULL)
		return -1;
	io_monitor_print(IO_DEBUG, "truncate log file: %s\n", name);
	err = kern_path(name, LOOKUP_FOLLOW, &file_path);
	if (err) {
		io_monitor_print(IO_ERROR, "path get failed!: %d\n", err);
		return -1;
	}
	err = vfs_truncate(&file_path, 0);
	if (err < 0) {
		io_monitor_print(IO_ERROR, "vfs truncate failed!: %d\n", err);
		return -1;
	}
	failed_imonitor_upload();

	return 0;
}

static int io_monitor_write_header(struct file *filp,
	struct io_monitor_ctl *ctl)
{
	struct io_monitor_file_header *h = GET_FILE_HEAD(ctl->io_buf);
	int ret;

	if (filp == NULL || ctl == NULL)
		return -1;

	h->magic = MAGIC_HEAD;
	h->file_size = 0;
	ret = lseek_log_file(filp, GET_HEAD_OFFSET, SEEK_SET);
	if (ret < 0)
		return ret;

	return write_log_file(filp, (unsigned char *)h,
			sizeof(struct io_monitor_file_header));
}

static int io_monitor_write_end(struct file *filp, struct io_monitor_ctl *ctl)
{
	struct io_monitor_file_end *e = GET_FILE_END(ctl->io_buf);
	int ret;

	if (filp == NULL || ctl == NULL)
		return -1;

	e->magic = MAGIC_END;
	ret = lseek_log_file(filp, GET_END_OFFSET, SEEK_SET);
	if (ret < 0)
		return ret;

	return write_log_file(filp, (unsigned char *)e,
		sizeof(struct io_monitor_file_end));
}

static void io_monitor_init_timer(struct io_monitor_ctl *ctl)
{
	unsigned long now = jiffies;
	int i;

	for (i = 0; i < IO_MONITOR_CNT; i++) {
		struct io_module_template *mngt = ctl->log_module[i];

		if (mngt)
			mngt->expires = now +
				msecs_to_jiffies(mngt->base_interval);
	}
}
static void io_monitor_update_timer(struct io_monitor_ctl *ctl)
{
	int i;
	int sort[IO_MONITOR_CNT] = {[0 ... (IO_MONITOR_CNT - 1)] = -1};
	int wait_index = 0;
	struct io_module_template *wait = NULL;

	/*update wait list*/
	for (i = 0; i < IO_MONITOR_CNT; i++) {
		struct io_module_template *mngt = ctl->log_module[i];

		if (mngt) {
			if (wait == NULL)
				wait = mngt;
				if ((wait->base_interval == mngt->base_interval)
					|| (wait->expires == mngt->expires)) {
					sort[wait_index] = mngt->mod_id;
					wait_index++;
					io_monitor_print(IO_DEBUG,
							"mod: %d add wait\n",
							mngt->mod_id);
				continue;
			}

			if (time_before(mngt->expires, wait->expires)) {
				io_monitor_print(IO_DEBUG,
						"wait[%d] ex:%ld, mngt[%d] ex: %ld.\n",
						wait->mod_id, wait->expires,
						mngt->mod_id, mngt->expires);
				wait = mngt;
				wait_index = 0;
				sort[wait_index] = mngt->mod_id;
				wait_index++;
			}
		}
	}

	/*BUG_ON(wait_index == 0);*/

	if (wait_index > 0) {
		struct io_module_template *mngt = NULL;

		for (i = 0; i < wait_index; i++) {
			mngt = ctl->log_module[sort[i]];
			io_monitor_print(IO_DEBUG, "wait list add node: %d\n",
				mngt->mod_id);
			if (mngt)
				list_add_tail(&mngt->wait_node,
					&ctl->wait_list);
		}
		/*choose the first one*/
		mngt = ctl->log_module[sort[0]];
		io_monitor_print(IO_DEBUG, "mod timer use module: %d\n",
				mngt->mod_id);
		mod_timer(&ctl->file_timer, mngt->expires);
	}
}

static void io_monitor_timer_fn(unsigned long arg)
{
	struct io_monitor_ctl *ctl = (struct io_monitor_ctl *)arg;
	struct io_module_template *mod = NULL, *m;
	unsigned long flags;
	unsigned long now = jiffies;
	struct timespec curr_time = current_kernel_time();

	io_monitor_print(IO_DEBUG, "curr sec: %ld, nsec: %ld\n",
			curr_time.tv_sec, curr_time.tv_nsec);

	io_monitor_print(IO_DEBUG, "timer fn\n");
	if (list_empty(&ctl->wait_list)) {
		io_monitor_print(IO_DEBUG, "wait list is empty!\n");
		io_monitor_init_timer(ctl);
		io_monitor_update_timer(ctl);
		return;
	}

	list_for_each_entry_safe(mod, m, &ctl->wait_list, wait_node) {
		list_del_init(&mod->wait_node);
		/*update expires*/
		mod->expires = now + msecs_to_jiffies(mod->base_interval);
		io_monitor_print(IO_DEBUG, "add mod %d upload, expires: %ld\n",
				mod->mod_id, mod->expires);
		spin_lock_irqsave(&ctl->ctl_spinlock, flags);
		list_add_tail(&mod->upload_node, &ctl->upload_list);
		spin_unlock_irqrestore(&ctl->ctl_spinlock, flags);
	}

	io_monitor_update_timer(ctl);
	wake_up_process(ctl->io_work_thread);
}

static int io_monitor_read_file(struct io_monitor_ctl *ctl)
{
	unsigned char name[64];
	int ret, len;

	if (ctl == NULL)
		return -1;

	sprintf(name, IO_MONITOR_DIR"%s", "io_monitor.txt");
	ctl->io_file = open_log_file(name);
	if (unlikely(!ctl->io_file)) {
		io_monitor_print(IO_ERROR, "upload file open failed!\n");
		return -1;
	}
	len = read_log_file(ctl->io_file, ctl->io_buf, IO_MONITOR_BUFF_SIZE);
	/*create*/
	filp_close(ctl->io_file, NULL);
	if (len == 0) {
		io_monitor_print(IO_DEBUG, "first read file!\n");
		return 0;
	}
	io_monitor_print(IO_DEBUG, "read file size: %d\n", len);
	ret = check_log_file(ctl->io_buf);
	if (ret < 0) {
		io_monitor_print(IO_ERROR, "log file error!\n");
		return -1;
	}

	return ret;
}

static int io_monitor_disk_sb(uint64_t *total_space, uint64_t *free_space)
{
	char *data = "/data";
	struct kstatfs st;
	struct path partition_path;
	int err;

	if (total_space == NULL || free_space == NULL)
		return -1;
	err = kern_path(data, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&partition_path);
	if (err) {
		io_monitor_print(IO_ERROR, "kernel path get failed!\n");
		return -1;
	}
	err = vfs_statfs(&partition_path, &st);
	if (err) {
		io_monitor_print(IO_ERROR, "vfs statfs failed!\n");
		return -1;
	}
	io_monitor_print(IO_DEBUG, "f_blocks:%llu, f_bfree:%llu, f_bsize:%ld\n",
			st.f_blocks, st.f_bfree, st.f_bsize);
	*total_space = (uint64_t)st.f_blocks * st.f_bsize;
	*free_space = (uint64_t)st.f_bfree * st.f_bsize;

	return 0;
}

static int io_monitor_sd_exist(void)
{
	char *sd = "dev/block/mmcblk1";
	struct path partition_path;
	int err;

	err = kern_path(sd, LOOKUP_FOLLOW, &partition_path);
	if (err)
		return -1;

	return 0;
}

static int module_upload_timeout(struct io_module_template *mod)
{
	struct timespec curr_time = current_kernel_time();
	long last_sec = mod->upload_time_sec;
	long now_sec = curr_time.tv_sec;

	if ((now_sec - last_sec) > IO_MONITOR_UPLOAD_INTER) {
		mod->upload_time_sec = curr_time.tv_sec;
		return 1;
	}

	return 0;
}

static int io_monitor_upload(struct io_monitor_ctl *ctl,
		struct io_module_template *mod)
{
	unsigned char *mod_buf;
	unsigned char name[64];
	int ret = -1;

	sprintf(name, IO_MONITOR_DIR"%s", "io_monitor.txt");

	if (ctl == NULL || mod == NULL)
		return ret;

	if (!ctl->in_memory) {
		if (io_monitor_read_file(ctl) < 0) {
			ctl->io_file = NULL;
			goto failed;
		}
		ctl->in_memory = 1;
	}

	mod_buf = GET_FILE_MOD_OFFSET(ctl->io_buf, mod->mod_id);
	if (mod->ops.log_record != NULL)
		ret = mod->ops.log_record(mod_buf, IO_MONITOR_MOD_SIZE);

	if (ret < 0)
		goto failed;

	BUG_ON(ret >= IO_MONITOR_MOD_SIZE);

	ctl->io_file = open_log_file(name);
	if (!ctl->io_file) {
		io_monitor_print(IO_ERROR, "open file error!\n");
		return -1;
	}

	if (io_monitor_write_header(ctl->io_file, ctl) < 0) {
		io_monitor_print(IO_ERROR, "write file header error!\n");
		goto failed;
	}

	if (lseek_log_file(ctl->io_file,
		GET_MOD_OFFSET(mod->mod_id), SEEK_SET) < 0) {
		io_monitor_print(IO_ERROR, "lseek error on mod_id:%d\n",
			mod->mod_id);
		goto failed;
	}
	if (write_log_file(ctl->io_file, mod_buf, ret) < 0) {
		io_monitor_print(IO_ERROR, "write log erro. mod_id: %d!\n",
			mod->mod_id);
		goto failed;
	}

	if (io_monitor_write_end(ctl->io_file, ctl) < 0) {
		io_monitor_print(IO_ERROR, "write file end error!\n");
		goto failed;
	}

	filp_close(ctl->io_file, NULL);

	if (mod->ops.log_upload != NULL) {
		io_monitor_print(IO_DEBUG, "mod[%d]direct upload\n",
			mod->mod_id);
		mod->ops.log_upload(mod, mod_buf);
	}

	 if (mod->ops.log_set_param && module_upload_timeout(mod)) {
		int ret1;
		struct imonitor_eventobj *obj1 = NULL;

		obj1 = imonitor_create_eventobj(mod->event_id);
		if (!obj1) {
			io_monitor_print(IO_ERROR, "create obj failed. %d\n",
				mod->event_id);
			return 0;
		}
		ret1 = mod->ops.log_set_param(obj1, mod_buf);
		io_monitor_print(IO_DEBUG, "mod[%d]log set param.\n",
			mod->mod_id);

		if (!ret1) {
			io_monitor_print(IO_DEBUG, "imonitor send event.\n");
			imonitor_send_event(obj1);
		}
		imonitor_destroy_eventobj(obj1);

		io_monitor_print(IO_DEBUG, "mod[%d]upload iomonitor.\n",
			mod->mod_id);
	}
	return 0;

failed:
	if (ctl->io_file)
		filp_close(ctl->io_file, NULL);
	truncate_log_file(name);

	/*BUG_ON(1);*/
	return -1;
}

static void io_work_del_list(struct io_monitor_ctl *ctl)
{
	unsigned long flags;
	struct list_head *l, *m;

	spin_lock_irqsave(&ctl->ctl_spinlock, flags);
	if (!list_empty(&ctl->upload_list)) {
		io_monitor_print(IO_DEBUG, "io work del upload list\n");
		list_for_each_safe(l, m, &ctl->upload_list)
			list_del_init(l);
	}

	if (!list_empty(&ctl->wait_list)) {
		io_monitor_print(IO_DEBUG, "io work del wait list\n");
		list_for_each_safe(l, m, &ctl->wait_list)
			list_del_init(l);
	}

	spin_unlock_irqrestore(&ctl->ctl_spinlock, flags);
}

static int io_work_thread_fn(void *data)
{
	struct io_monitor_ctl *ctl = (struct io_monitor_ctl *)data;
	unsigned long flags;
	unsigned int stop = 0;

	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		if (stop || (ctl == NULL)) {
			io_monitor_print(IO_ERROR, "io monitor thread exit!\n");
			break;
		}
		if (list_empty(&ctl->upload_list))
			schedule();
		__set_current_state(TASK_RUNNING);

		io_monitor_print(IO_DEBUG, "io working!\n");
		spin_lock_irqsave(&ctl->ctl_spinlock, flags);
		while (!list_empty(&ctl->upload_list)) {
			struct io_module_template *mod;

			mod = list_entry(ctl->upload_list.next,
					struct io_module_template, upload_node);
			list_del_init(&mod->upload_node);
			io_monitor_print(IO_DEBUG, "del mod %d\n", mod->mod_id);
			spin_unlock_irqrestore(&ctl->ctl_spinlock, flags);
			if (io_monitor_upload(ctl, mod) < 0) {
				del_timer_sync(&ctl->file_timer);
				io_work_del_list(ctl);
				/*break don't spin unlock*/
				stop = 1;
				io_monitor_print(IO_ERROR, "io monitor thread exit!\n");
				return 0;
			}
			spin_lock_irqsave(&ctl->ctl_spinlock, flags);
		}
		spin_unlock_irqrestore(&ctl->ctl_spinlock, flags);
	}

	return 0;
}

static struct io_monitor_ctl *io_monitor_init(void)
{
	struct io_monitor_ctl *ctl = NULL;

	ctl = kzalloc(sizeof(struct io_monitor_ctl), GFP_KERNEL);
	if (ctl) {
		INIT_LIST_HEAD(&ctl->upload_list);
		INIT_LIST_HEAD(&ctl->wait_list);
		spin_lock_init(&ctl->ctl_spinlock);
		setup_timer(&ctl->file_timer, io_monitor_timer_fn,
			(unsigned long)ctl);

		ctl->all_equal = 0;
		ctl->in_memory = 0;
		ctl->reserved = 0;

		ctl->io_buf = kzalloc(IO_MONITOR_BUFF_SIZE, GFP_KERNEL);
		if (ctl->io_buf == NULL) {
			kfree(ctl);
			return NULL;
		}
		ctl->io_file = NULL;

		ctl->io_work_thread = kthread_run(io_work_thread_fn, ctl,
			"io-monitor");
		if (IS_ERR(ctl->io_work_thread)) {
			kfree(ctl);
			io_monitor_print(IO_ERROR, "io work thread error!\n");
			return NULL;
		}
	}
	return ctl;
}

int io_monitor_mod_register(int mod_id, struct io_module_template *mngt)
{
	struct timespec curr_time = current_kernel_time();

	io_monitor_print(IO_DEBUG, "mod id %d register.\n", mod_id);
	/*init only once*/
	if (mngt == NULL)
		return -1;
	if (monitor_ctl == NULL) {
		monitor_ctl = io_monitor_init();
		if (monitor_ctl == NULL) {
			io_monitor_print(IO_ERROR, "monitor not init.!\n");
			return -1;
		}
	}

	if (mod_id >= IO_MONITOR_CNT)
		return -1;

	INIT_LIST_HEAD(&mngt->upload_node);
	INIT_LIST_HEAD(&mngt->wait_node);
	mngt->upload_time_sec = curr_time.tv_sec;
	monitor_ctl->log_module[mod_id] = mngt;
	atomic_inc(&monitor_ctl->reg_cnt);

	if (atomic_read(&monitor_ctl->reg_cnt) == IO_MONITOR_CNT) {
		io_monitor_print(IO_DEBUG, "update timer.\n");
		io_monitor_init_timer(monitor_ctl);
		io_monitor_update_timer(monitor_ctl);
	}

	return 0;
}
EXPORT_SYMBOL(io_monitor_mod_register);

#define MAX_HD_DEVICE   (1)
struct io_monitor_load_data {
	unsigned long  last_ios[2];
	unsigned long  ios[2]; /*read and write*/

	unsigned long last_sector[2];
	unsigned long  sector[2];

	unsigned long last_upload[2];
	unsigned long total_sector[2];

	unsigned long last_ticks[2];
	unsigned long ticks[2];

	unsigned int  flight;

	unsigned char name[32];
	unsigned char *private;
};

struct io_monitor_load {
	/*io stats*/
	unsigned long load_timestamp;
	unsigned int  period;
	unsigned int  init;
	struct io_monitor_load_data io_data[MAX_HD_DEVICE];
};

static struct io_monitor_load io_load_this  = {
	.init = 0,
	.load_timestamp = 0,
	.period = (5 * HZ),
};

static int io_monitor_load_init(void)
{
	int i;

	for (i = 0; i < MAX_HD_DEVICE; i++) {
		struct io_monitor_load_data *data = &io_load_this.io_data[i];

		memset(data, 0, sizeof(struct io_monitor_load_data));
	}
	io_load_this.init = 1;

	return 0;
}

static int my_value_is_valid(struct hd_struct *p)
{
	return ((strncmp((part_to_disk(p))->disk_name, "sdd", 3) == 0)
		|| (strncmp((part_to_disk(p))->disk_name, "mmcblk0", 7) == 0));
}

static int my_value_from_key(struct hd_struct *p)
{
	unsigned char *key = (unsigned char *)p;
	int i = 0;

	if (p == NULL)
		return MAX_HD_DEVICE;
	for (i = 0; i < MAX_HD_DEVICE; i++) {
		struct io_monitor_load_data *data = &io_load_this.io_data[i];

		if (data->private == key)
			return i;

		if (data->private == NULL && my_value_is_valid(p))
			break;
	}
	/*all slot used, but private is not valid??*/
	if ((i >= MAX_HD_DEVICE) && my_value_is_valid(p))
		i = 0;

	if (i < MAX_HD_DEVICE) {
		struct io_monitor_load_data *data = &io_load_this.io_data[i];

		int len = (strlen((part_to_disk(p))->disk_name) > 31) ? 31 :
			strlen((part_to_disk(p))->disk_name);

		io_monitor_print(IO_DEBUG, "device %s in key-value.\n",
				(part_to_disk(p))->disk_name);
		strncpy(data->name, (part_to_disk(p))->disk_name, len);
		data->name[len] = '\0';
		data->private = key;
	}

	return i;
}

void io_monitor_perf(struct hd_struct *p)
{
	unsigned long now = jiffies;
	unsigned long last;
	unsigned long read_sect, write_sect, tmp_sect;
	unsigned long read_ios, write_ios, tmp_ios;
	unsigned long read_tick, write_tick, tmp_tick;
	unsigned int index;
	struct io_monitor_load_data *data = NULL;

	if (p == NULL)
		return;

	if (io_load_this.init == 0)
		return;

	last = io_load_this.load_timestamp;
	if (io_load_this.load_timestamp == 0) {
		io_load_this.load_timestamp = now;
		last = now;
	}
	if (time_before(now, io_load_this.period + last))
		return;

	if (cmpxchg(&io_load_this.load_timestamp, last, now) !=
			last)
		return;
	/*only one thread can enter here*/
	index = my_value_from_key(p);
	if (index >= MAX_HD_DEVICE)
		return;

	data = &io_load_this.io_data[index];
	/*sector*/
	tmp_sect = part_stat_read(p, sectors[READ]);
	data->total_sector[READ] = tmp_sect;
	read_sect = tmp_sect - data->last_sector[READ];
	data->last_sector[READ] = tmp_sect;
	data->sector[READ] = read_sect;

	tmp_sect = part_stat_read(p, sectors[WRITE]);
	data->total_sector[WRITE] = tmp_sect;
	write_sect = tmp_sect - data->last_sector[WRITE];
	data->last_sector[WRITE] = tmp_sect;
	data->sector[WRITE] = write_sect;

	io_monitor_print(IO_DEBUG, "%s:total_sec[R]: %ld, total_sec[W]: %ld\n",
			(part_to_disk(p))->disk_name, data->total_sector[READ],
			data->total_sector[WRITE]);

	io_monitor_print(IO_DEBUG, "%s:sector[R]: %ld, sector[W]: %ld\n",
			(part_to_disk(p))->disk_name, data->sector[READ],
			data->sector[WRITE]);
	/*iops*/
	tmp_ios = part_stat_read(p, ios[READ]);
	read_ios = tmp_ios - data->last_ios[READ];
	data->last_ios[READ] = tmp_ios;
	data->ios[READ] = read_ios;

	tmp_ios = part_stat_read(p, ios[WRITE]);
	write_ios = tmp_ios - data->last_ios[WRITE];
	data->last_ios[WRITE] = tmp_ios;
	data->ios[WRITE] = write_ios;

	io_monitor_print(IO_DEBUG, "%s:ios[R]: %ld, ios[W]: %ld\n",
			(part_to_disk(p))->disk_name, data->ios[READ],
			data->ios[WRITE]);
	/*tick*/
	tmp_tick = part_stat_read(p, ticks[READ]);
	read_tick = tmp_tick - data->last_ticks[READ];
	data->last_ticks[READ] = tmp_tick;
	data->ticks[READ] = jiffies_to_msecs(read_tick);

	tmp_tick = part_stat_read(p, ticks[WRITE]);
	write_tick = tmp_tick - data->last_ticks[WRITE];
	data->last_ticks[WRITE] = tmp_tick;
	data->ticks[WRITE] = jiffies_to_msecs(write_tick);

	io_monitor_print(IO_DEBUG, "%s:tick[R]: %ld, tick[W]: %ld\n",
			(part_to_disk(p))->disk_name, data->ticks[READ],
			data->ticks[WRITE]);

	data->flight = part_in_flight(p);
	io_monitor_print(IO_DEBUG, "%s:flight %u\n",
			(part_to_disk(p))->disk_name, data->flight);
}
EXPORT_SYMBOL(io_monitor_perf);

enum {
	IO_MONITOR_DELAY_4K = 0,
	IO_MONITOR_DELAY_512K,
	IO_MONITOR_DELAY_NUM,
};

static int delay_range[][5] = {
	[IO_MONITOR_DELAY_4K] = {0, 1, 10, 50, 100},/*ms*/
	[IO_MONITOR_DELAY_512K] = {1, 10, 50, 100, 500},/*ms*/
};

struct io_monitor_delay_data {
	unsigned long stage_one[2];
	unsigned long stage_two[2];
	unsigned long stage_thr[2];
	unsigned long stage_fou[2];
	unsigned long stage_fiv[2];
	unsigned int  max_delay[2];
	unsigned long cnt[2];
	unsigned long total_delay[2];
};

static int delay_keys[][14] = {
	[IO_MONITOR_DELAY_4K] = {
		E914009000_READ_DELAY_4K_FIVE_INT,
		E914009000_WRITE_DELAY_4K_FIVE_INT,
		E914009000_READ_DELAY_4K_FOUR_INT,
		E914009000_WRITE_DELAY_4K_FOUR_INT,
		E914009000_READ_DELAY_4K_THREE_INT,
		E914009000_WRITE_DELAY_4K_THREE_INT,
		E914009000_READ_DELAY_4K_TWO_INT,
		E914009000_WRITE_DELAY_4K_TWO_INT,
		E914009000_READ_DELAY_4K_ONE_INT,
		E914009000_WRITE_DELAY_4K_ONE_INT,
		E914009000_READ_DELAY_4K_AVERAGE_INT,
		E914009000_WRITE_DELAY_4K_AVERAGE_INT,
		E914009000_READ_DELAY_MIN_INT,
		E914009000_WRITE_DELAY_MIN_INT,
		/*TODO*/
	},
	[IO_MONITOR_DELAY_512K] = {
		E914009000_READ_DELAY_512K_FIVE_INT,
		E914009000_WRITE_DELAY_512K_FIVE_INT,
		E914009000_READ_DELAY_512K_FOUR_INT,
		E914009000_WRITE_DELAY_512K_FOUR_INT,
		E914009000_READ_DELAY_512K_THREE_INT,
		E914009000_WRITE_DELAY_512K_THREE_INT,
		E914009000_READ_DELAY_512K_TWO_INT,
		E914009000_WRITE_DELAY_512K_TWO_INT,
		E914009000_READ_DELAY_512K_ONE_INT,
		E914009000_WRITE_DELAY_512K_ONE_INT,
		E914009000_READ_DELAY_512K_AVERAGE_INT,
		E914009000_WRITE_DELAY_512K_AVERAGE_INT,
		E914009000_READ_DELAY_MAX_INT,
		E914009000_WRITE_DELAY_MAX_INT,
		/*TODO*/
	},
};

struct io_monitor_delay {
	unsigned int init;
	struct io_monitor_delay_data __percpu *io_data[IO_MONITOR_DELAY_NUM];
	struct io_monitor_delay_data last_io_data[IO_MONITOR_DELAY_NUM];
};

static struct io_monitor_delay io_delay_this = {
	.init = 0,
};

static void io_monitor_delay_init(void)
{
	int i, j;

	for (j = 0; j < IO_MONITOR_DELAY_NUM; j++) {
		io_delay_this.io_data[j] = alloc_percpu(
			struct io_monitor_delay_data);
		for_each_possible_cpu(i) {
			struct io_monitor_delay_data *data;

			data = per_cpu_ptr(io_delay_this.io_data[j], i);
			memset(data, 0, sizeof(struct io_monitor_delay_data));
		}
		memset(&io_delay_this.last_io_data[j], 0,
			sizeof(struct io_monitor_delay_data));
	}
	io_delay_this.init = 1;
}

static int is_io_monitor_latency(unsigned int len)
{
	if (len == (4 * 1024))
		return IO_MONITOR_DELAY_4K;

	if (len == (512 * 1024))
		return IO_MONITOR_DELAY_512K;

	return IO_MONITOR_DELAY_NUM;
}

void io_monitor_latency(struct request *req, unsigned int len)
{
	unsigned long duration = jiffies - req->start_time;
	unsigned long latency = jiffies_to_msecs(duration);
	int index, rw;
	struct io_monitor_delay_data *data = NULL;

	if (req == NULL)
		return;
	if (io_delay_this.init == 0)
		return;

	index = is_io_monitor_latency(len);
	if (index >= IO_MONITOR_DELAY_NUM)
		return;
	rw = rq_data_dir(req);
	data = per_cpu_ptr(io_delay_this.io_data[index], get_cpu());
	data->cnt[rw]++;
	data->total_delay[rw] += latency;
	/*<=1ms don't need record*/
	if (latency < 1 && index == IO_MONITOR_DELAY_512K) {
		put_cpu();
		return;
	}
	if (latency >= delay_range[index][4])
		data->stage_fiv[rw]++;
	else if (latency >= delay_range[index][3])
		data->stage_fou[rw]++;
	else if (latency >= delay_range[index][2])
		data->stage_thr[rw]++;
	else if (latency >= delay_range[index][1])
		data->stage_two[rw]++;
	else if (latency >= delay_range[index][0])
		data->stage_one[rw]++;

	data->max_delay[rw] = (data->max_delay[rw] <= latency) ? latency :
		data->max_delay[rw];

	put_cpu();
}
EXPORT_SYMBOL(io_monitor_latency);

#define io_delay_stat_read(ptr, field) \
	({      \
		typeof((ptr)->field) res = 0;  \
		unsigned int _cpu; \
		for_each_possible_cpu(_cpu) \
			res += per_cpu_ptr(ptr, _cpu)->field; \
		res;    \
	 })

#define io_delay_stat_max(ptr, field) \
	({      \
		typeof((ptr)->field) res = 0;  \
		unsigned int _cpu; \
		for_each_possible_cpu(_cpu) \
			if (res <= per_cpu_ptr(ptr, _cpu)->field) \
				res = per_cpu_ptr(ptr, _cpu)->field; \
		res;    \
	})
#define io_monitor_max(a, b) ((a) >= (b) ? (a) : (b))

struct io_delay_file_header {
	unsigned int magic;
	unsigned int sdcard;
	unsigned long version;
	unsigned long total_space;
	unsigned long free_space;
};
struct io_delay_file_stat {
	unsigned long total_one[2];
	unsigned long total_two[2];
	unsigned long total_thr[2];
	unsigned long total_fou[2];
	unsigned long total_fiv[2];
	unsigned int max_delay[2];
	unsigned long cnt[2];
	unsigned long total_delay[2];
};

struct io_load_file_header {
	unsigned int magic;
	unsigned int index;
	unsigned long version;
	unsigned long total_sector[2];
};
struct io_load_file_stat {
	unsigned long  ios[2]; /*read and write*/
	unsigned long  sector[2];
	unsigned long ticks[2];
	unsigned int  flight;
};

#define IO_LOAD_FILE_CNT    10

static void io_monitor_file_print(unsigned char *d, unsigned char *l)
{
	if (d) {
		struct io_delay_file_header *header;
		struct io_delay_file_stat *delay;

		header = (struct io_delay_file_header *)d;
		delay = (struct io_delay_file_stat *)(d +
				sizeof(struct io_delay_file_header));
		io_monitor_print(IO_DEBUG, "read stage[%ld,%ld,%ld,%ld,%ld]\n",
			delay->total_one[READ], delay->total_two[READ],
			delay->total_thr[READ], delay->total_fou[READ],
			delay->total_fiv[READ]);
		io_monitor_print(IO_DEBUG, "write stage[%ld,%ld,%ld,%ld,%ld]\n",
			delay->total_one[WRITE], delay->total_two[WRITE],
			delay->total_thr[WRITE], delay->total_fou[WRITE],
			delay->total_fiv[WRITE]);
		io_monitor_print(IO_DEBUG, "read delay max[%u]\n",
			delay->max_delay[READ]);
		io_monitor_print(IO_DEBUG, "write delay max[%u]\n",
			delay->max_delay[WRITE]);
		io_monitor_print(IO_DEBUG, "read cnt: %ld, %ld, %ld\n",
				delay->cnt[READ], delay->total_delay[READ],
				delay->total_delay[READ] / delay->cnt[READ]);
		io_monitor_print(IO_DEBUG, "write cnt: %ld, %ld, %ld\n",
				delay->cnt[WRITE], delay->total_delay[WRITE],
				delay->total_delay[WRITE] / delay->cnt[WRITE]);
		io_monitor_print(IO_DEBUG, "sdcard: %d, %ld, %ld\n",
				header->sdcard, header->total_space / (GB),
				header->free_space / (GB));
	}

	if (l) {
		int j;
		struct io_load_file_header *header;
		struct io_load_file_stat *h;
		struct io_load_file_stat *m;
		int index;

		header = (struct io_load_file_header *)l;
		h = (struct io_load_file_stat *)(l +
				sizeof(struct io_load_file_header));
		index = header->index;

		for (j = 0; j < IO_LOAD_FILE_CNT; j++) {
			m = h + index;
			io_monitor_print(IO_DEBUG,
					"index: %d,ios[%ld,%ld],sector[%ld,%ld], ticks[%ld, %ld]\n",
					index, m->ios[READ], m->ios[WRITE],
					m->sector[READ], m->sector[WRITE],
					m->ticks[READ], m->ticks[WRITE]);
			index = (index == 0) ? (IO_LOAD_FILE_CNT - 1) :
				(index - 1);
		}
		io_monitor_print(IO_DEBUG, "total_sector[%ld, %ld]\n",
				header->total_sector[READ],
				header->total_sector[WRITE]);
	}
}

#define IO_DELAY_FILE_SIZE (sizeof(struct io_delay_file_stat) +	\
		sizeof(struct io_delay_file_header))
#define IO_LOAD_FILE_SIZE (sizeof(struct io_load_file_header) +	\
		IO_LOAD_FILE_CNT * sizeof(struct io_load_file_stat))

static int write_io_delay_file(unsigned char *buf,
		struct io_monitor_delay_data *stat,
		struct io_monitor_delay_data *last_data)
{
	struct io_delay_file_header *header;
	struct io_delay_file_stat *delay_file_stat;
	unsigned long total_space, free_space;

	header = (struct io_delay_file_header *)buf;
	delay_file_stat = (struct io_delay_file_stat *)(buf +
			sizeof(struct io_delay_file_header));

	if (!header->version) {
		io_monitor_print(IO_DEBUG, "first io delay upload.\n");
		header->magic = MAGIC_BLK;
	}

	if (header->magic != MAGIC_BLK) {
		io_monitor_print(IO_ERROR, "io delay file error!\n");
		return -1;
	}
	header->sdcard = 0;
	if (!io_monitor_disk_sb((uint64_t *)&total_space,
		(uint64_t *)&free_space)) {
		header->total_space = total_space;
		header->free_space = free_space;
	}
	if (!io_monitor_sd_exist())
		header->sdcard = 1;

	delay_file_stat->total_one[READ]  += (stat->stage_one[READ] -
		last_data->stage_one[READ]);
	delay_file_stat->total_one[WRITE] += (stat->stage_one[WRITE] -
		last_data->stage_one[WRITE]);
	delay_file_stat->total_two[READ]  += (stat->stage_two[READ] -
		last_data->stage_two[READ]);
	delay_file_stat->total_two[WRITE] += (stat->stage_two[WRITE] -
		last_data->stage_two[WRITE]);
	delay_file_stat->total_thr[READ]  += (stat->stage_thr[READ] -
		last_data->stage_thr[READ]);
	delay_file_stat->total_thr[WRITE] += (stat->stage_thr[WRITE] -
		last_data->stage_thr[WRITE]);
	delay_file_stat->total_fou[READ]  += (stat->stage_fou[READ] -
		last_data->stage_fou[READ]);
	delay_file_stat->total_fou[WRITE] += (stat->stage_fou[WRITE] -
		last_data->stage_fou[WRITE]);
	delay_file_stat->total_fiv[READ]  += (stat->stage_fiv[READ] -
		last_data->stage_fiv[READ]);
	delay_file_stat->total_fiv[WRITE] += (stat->stage_fiv[WRITE] -
		last_data->stage_fiv[WRITE]);

	delay_file_stat->max_delay[READ]  = io_monitor_max(
		delay_file_stat->max_delay[READ], stat->max_delay[READ]);
	delay_file_stat->max_delay[WRITE] = io_monitor_max(
		delay_file_stat->max_delay[WRITE], stat->max_delay[WRITE]);
	delay_file_stat->cnt[READ]  += (stat->cnt[READ] -
		last_data->cnt[READ]);
	delay_file_stat->cnt[WRITE] += (stat->cnt[WRITE] -
		last_data->cnt[WRITE]);
	delay_file_stat->total_delay[READ]  += (stat->total_delay[READ] -
		last_data->total_delay[READ]);
	delay_file_stat->total_delay[WRITE] += (stat->total_delay[WRITE] -
		last_data->total_delay[WRITE]);
	/**/
	header->version++;
	return 0;
}

static int write_io_load_file(unsigned char *buf,
		struct io_monitor_load_data *load_data)
{
	struct io_load_file_header *header;
	struct io_load_file_stat *load_file_stat;

	header = (struct io_load_file_header *)buf;
	load_file_stat = (struct io_load_file_stat *)(buf +
			sizeof(struct io_load_file_header));
	if (buf == NULL)
		return -1;
	if (!header->version) {
		io_monitor_print(IO_DEBUG, "first io load upload.\n");
		header->magic = MAGCI_IO;
		header->index = 0;
	}

	if (header->magic != MAGCI_IO) {
		io_monitor_print(IO_ERROR, "io load file error! %x\n",
			header->magic);
		return -1;
	}
	/*write header*/
	header->total_sector[READ] += (load_data->total_sector[READ] -
		load_data->last_upload[READ]);
	header->total_sector[WRITE] += (load_data->total_sector[WRITE] -
		load_data->last_upload[WRITE]);
	/*write load*/
	if (header->index > IO_LOAD_FILE_CNT) {
		io_monitor_print(IO_ERROR, "io load index error!\n");
		header->index = 0;
	}
	load_file_stat = load_file_stat + header->index;
	io_monitor_print(IO_DEBUG, "load index: %d, addr:%p\n",
		header->index, load_file_stat);

	load_file_stat->ios[READ] = load_data->ios[READ];
	load_file_stat->ios[WRITE] = load_data->ios[WRITE];
	load_file_stat->sector[READ] = load_data->sector[READ];
	load_file_stat->sector[WRITE] = load_data->sector[WRITE];
	load_file_stat->ticks[READ] = load_data->ticks[READ];
	load_file_stat->ticks[WRITE] = load_data->ticks[WRITE];
	load_file_stat->flight = load_data->flight;
	header->index++;
	if (header->index == IO_LOAD_FILE_CNT)
		header->index = 0;

	header->version++;

	return 0;
}

static int io_monitor_blk_set_param(struct imonitor_eventobj *obj,
		unsigned char *buff)
{
	int ret = 0, i;
	struct io_delay_file_header *header;

	header = (struct io_delay_file_header *)buff;
	if (obj == NULL)
		return -1;

	ret = ret | imonitor_set_param(obj,
		E914009000_SD_CARD_EXIST_SMALLINT, header->sdcard);
	ret = ret | imonitor_set_param(obj,
		E914009000_DATA_PART_FREE_SPACE_INT,
			header->free_space / (GB));
	/*4k delay info*/
	io_monitor_print(IO_DEBUG, "io monitor blk set param.\n");
	for (i = 0; i < IO_MONITOR_DELAY_NUM; i++) {
		struct io_delay_file_stat *delay_file_stat;
		unsigned long r_aver_delay, w_aver_delay;

		delay_file_stat = (struct io_delay_file_stat *)(buff +
				sizeof(struct io_delay_file_header));

		r_aver_delay = delay_file_stat->total_delay[READ] /
			delay_file_stat->cnt[READ];
		w_aver_delay = delay_file_stat->total_delay[WRITE] /
			delay_file_stat->cnt[WRITE];
		ret = ret | imonitor_set_param(obj, delay_keys[i][0],
			delay_file_stat->total_fiv[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][1],
			delay_file_stat->total_fiv[WRITE]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][2],
			delay_file_stat->total_fou[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][3],
			delay_file_stat->total_fou[WRITE]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][4],
			delay_file_stat->total_thr[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][5],
			delay_file_stat->total_thr[WRITE]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][6],
			delay_file_stat->total_two[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][7],
			delay_file_stat->total_two[WRITE]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][8],
			delay_file_stat->total_one[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][9],
			delay_file_stat->total_one[WRITE]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][10],
			r_aver_delay);
		ret = ret | imonitor_set_param(obj, delay_keys[i][11],
			w_aver_delay);
		ret = ret | imonitor_set_param(obj, delay_keys[i][12],
			delay_file_stat->max_delay[READ]);
		ret = ret | imonitor_set_param(obj, delay_keys[i][13],
			delay_file_stat->max_delay[WRITE]);
		buff += IO_DELAY_FILE_SIZE;
	}

	return ret;
}

static int io_monitor_upload_load(struct io_module_template *mod,
		unsigned char *buff)
{
	int ret = 0;
	struct imonitor_eventobj *obj = imonitor_create_eventobj(mod->event_id);
	struct io_load_file_header *header = (struct io_load_file_header *)buff;
	struct io_load_file_stat *load_file_stat;
	int index = 0;

	load_file_stat = (struct io_load_file_stat *)(buff +
			sizeof(struct io_load_file_header));
	if (!obj) {
		io_monitor_print(IO_ERROR, "imonitor create obj failed. %d\n",
			mod->event_id);
		return -1;
	}
	if (mod == NULL || buff == NULL)
	{
		imonitor_destroy_eventobj(obj);
		return -1;
	}
	index = header->index;

	ret = ret | imonitor_set_param(obj, E914009000_HOST_TOTAL_READ_INT,
			(header->total_sector[READ] * 512) / MB);
	ret = ret | imonitor_set_param(obj, E914009000_HOST_TOTAL_WRITE_INT,
			(header->total_sector[WRITE] * 512) / MB);

	index = (index == 0) ? (IO_LOAD_FILE_CNT - 1) :
		(index - 1);
	io_monitor_print(IO_DEBUG, "upload imonitor index[%d]\n", index);

	load_file_stat = load_file_stat + index;
	ret = ret | imonitor_set_param(obj, E914009000_READ_RUN_IO_NUM_INT,
		load_file_stat->ios[READ]);
	ret = ret | imonitor_set_param(obj, E914009000_WRITE_RUN_IO_NUM_INT,
		load_file_stat->ios[WRITE]);
	ret = ret | imonitor_set_param(obj, E914009000_READ_RUN_IO_SECTOR_INT,
		load_file_stat->sector[READ]);
	ret = ret | imonitor_set_param(obj, E914009000_WRITE_RUN_IO_SECTOR_INT,
		load_file_stat->sector[WRITE]);
	ret = ret | imonitor_set_param(obj, E914009000_READ_RUN_IO_TICKS_INT,
		load_file_stat->ticks[READ]);
	ret = ret | imonitor_set_param(obj, E914009000_WRITE_RUN_IO_TICKS_INT,
		load_file_stat->ticks[WRITE]);
	ret = ret | imonitor_set_param(obj, E914009000_READ_RUN_IO_FLIGHT_INT,
		load_file_stat->flight);

	if (!ret)
		imonitor_send_event(obj);
	imonitor_destroy_eventobj(obj);

	return 0;
}

static int io_monitor_blk_show(unsigned char *buff, int len)
{
	int ret = 0, i;

	if (buff == NULL)
		return -1;
	for (i = 0; i < IO_MONITOR_DELAY_NUM; i++) {
		struct io_monitor_delay_data stat;
		struct io_monitor_delay_data *last_data =
			&io_delay_this.last_io_data[i];
		/*from load stat get the total sector.*/
		stat.stage_one[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_one[READ]);
		stat.stage_one[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_one[WRITE]);
		stat.stage_two[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_two[READ]);
		stat.stage_two[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_two[WRITE]);
		stat.stage_thr[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_thr[READ]);
		stat.stage_thr[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_thr[WRITE]);
		stat.stage_fou[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fou[READ]);
		stat.stage_fou[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fou[WRITE]);
		stat.stage_fiv[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fiv[READ]);
		stat.stage_fiv[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fiv[WRITE]);
		stat.max_delay[READ]  = io_delay_stat_max(
			io_delay_this.io_data[i], max_delay[READ]);
		stat.max_delay[WRITE] = io_delay_stat_max(
			io_delay_this.io_data[i], max_delay[WRITE]);
		stat.cnt[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], cnt[READ]);
		stat.cnt[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], cnt[WRITE]);
		stat.total_delay[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], total_delay[READ]);
		stat.total_delay[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], total_delay[WRITE]);
		/*write delay file*/
		if (write_io_delay_file(buff, &stat, last_data) < 0)
			return -1;

		io_monitor_print(IO_DEBUG, "get file delay stat:\n");
		io_monitor_file_print(buff, NULL);
		/*update latest data*/
		*last_data = stat;

		buff += IO_DELAY_FILE_SIZE;
		ret += IO_DELAY_FILE_SIZE;
	}
	io_monitor_print(IO_DEBUG, "return legnth: %d\n", ret);

	return ret;
}

#ifdef CONFIG_HUAWEI_IO_TRACING
static int load_report_to_iotrace(unsigned char *buff, int len)
{
	struct io_monitor_load_data *load_data = &io_load_this.io_data[0];
	struct load_to_iotrace {
		unsigned long total_sector[2];
		unsigned long  ios[2]; /*read and write*/
		unsigned long  sector[2];
		unsigned long ticks[2];
		unsigned int  flight;
	};
	struct load_to_iotrace *load;
	int ret = sizeof(struct load_to_iotrace);

	if (ret > len)
		return 0;
	load = (struct load_to_iotrace *)buff;
	load->total_sector[READ] += load_data->total_sector[READ];
	load->total_sector[WRITE] += load_data->total_sector[WRITE];
	load->ios[READ] = load_data->ios[READ];
	load->ios[WRITE] = load_data->ios[WRITE];
	load->sector[READ] = load_data->sector[READ];
	load->sector[WRITE] = load_data->sector[WRITE];
	load->ticks[READ] = load_data->ticks[READ];
	load->ticks[WRITE] = load_data->ticks[WRITE];
	load->flight = load_data->flight;

	return ret;
}

static int blk_report_to_iotrace(unsigned char *buff, int len)
{
	int ret = 0, i;

	if ((sizeof(struct io_monitor_delay_data) *
			IO_MONITOR_DELAY_NUM) > len)
		return 0;

	for (i = 0; i < IO_MONITOR_DELAY_NUM; i++) {
		struct io_monitor_delay_data *dest =
			(struct io_monitor_delay_data *)buff;
		struct io_monitor_delay_data stat;
		/*from load stat get the total sector.*/
		stat.stage_one[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_one[READ]);
		stat.stage_one[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_one[WRITE]);
		stat.stage_two[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_two[READ]);
		stat.stage_two[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_two[WRITE]);
		stat.stage_thr[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_thr[READ]);
		stat.stage_thr[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_thr[WRITE]);
		stat.stage_fou[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fou[READ]);
		stat.stage_fou[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fou[WRITE]);
		stat.stage_fiv[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fiv[READ]);
		stat.stage_fiv[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], stage_fiv[WRITE]);
		stat.max_delay[READ]  = io_delay_stat_max(
			io_delay_this.io_data[i], max_delay[READ]);
		stat.max_delay[WRITE] = io_delay_stat_max(
			io_delay_this.io_data[i], max_delay[WRITE]);
		stat.cnt[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], cnt[READ]);
		stat.cnt[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], cnt[WRITE]);
		stat.total_delay[READ]  = io_delay_stat_read(
			io_delay_this.io_data[i], total_delay[READ]);
		stat.total_delay[WRITE] = io_delay_stat_read(
			io_delay_this.io_data[i], total_delay[WRITE]);
		/*update latest data*/
		*dest = stat;

		buff += sizeof(struct io_monitor_delay_data);
		ret += sizeof(struct io_monitor_delay_data);
	}
	io_monitor_print(IO_DEBUG, "return legnth: %d\n", ret);

	return ret;
}

int io_monitor_report_to_iotrace(unsigned char *buff, int len)
{
	int ret = 0, length = 0;

	if (buff == NULL)
		return ret;

	ret = vfs_report_to_iotrace(buff + length, len - length);
	if (ret == 0)
		return ret;
	length += ret;

	ret = load_report_to_iotrace(buff + length, len - length);
	if (ret == 0)
		return ret;
	length += ret;

	ret = blk_report_to_iotrace(buff + length, len - length);
	if (ret == 0)
		return ret;
	length += ret;

#ifdef CONFIG_HUAWEI_UFS_HEALTH_INFO
	ret = dev_report_to_iotrace(buff + length, len - length);
	if (ret == 0)
		return ret;
	length += ret;
#endif
	return length;
}
EXPORT_SYMBOL(io_monitor_report_to_iotrace);
#endif

static struct io_module_template ioblk = {
	.mod_id = IO_MONITOR_BLK,
	.event_id = IO_BLK_DMD_CODE,
	.base_interval = IO_BLK_UPLOAD_INTER, /*ms*/
	.ops = {
		.log_record = io_monitor_blk_show,
		.log_set_param = io_monitor_blk_set_param,
		.log_upload = NULL,
	},
};

static int io_monitor_load_show(unsigned char *buff, int len)
{
	int ret = 0;
	struct io_monitor_load_data *load_data = &io_load_this.io_data[0];

	if (buff == NULL)
		return -1;
	if (write_io_load_file(buff, load_data) < 0)
		return -1;
	io_monitor_print(IO_DEBUG, "get file load stat:\n");
	io_monitor_file_print(NULL, buff);
	load_data->last_upload[READ] = load_data->total_sector[READ];
	load_data->last_upload[WRITE] = load_data->total_sector[WRITE];

	ret += IO_LOAD_FILE_SIZE;

	return ret;
}

static struct io_module_template ioload = {
	.mod_id = IO_MONITOR_LOAD,
	.event_id = IO_BLK_DMD_CODE,
	.base_interval = IO_BLK_UPLOAD_INTER, /*ms*/
	.ops = {
		.log_record = io_monitor_load_show,
		.log_set_param = NULL,
		.log_upload = io_monitor_upload_load,
	},
};

static int __init io_monitor_blk_init(void)
{
	io_monitor_mod_register(IO_MONITOR_BLK, &ioblk);
	io_monitor_mod_register(IO_MONITOR_LOAD, &ioload);
	io_monitor_delay_init();
	io_monitor_load_init();

	return 0;
}
/*lint -restore*/
/*lint -restore*/
/*lint -e528 -esym(528,*)*/
subsys_initcall(io_monitor_blk_init);
/*lint -e528 +esym(528,*)*/
#endif
