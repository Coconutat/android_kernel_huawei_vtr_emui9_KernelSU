#include <linux/init.h>
#include <linux/module.h>
#include <linux/async.h>
#include <linux/atomic.h>
#include <linux/blkdev.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <scsi/scsi.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_host.h>
#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include "../rasbase/rasbase.h"
#include "../rasbase/rasproc.h"
#include "drivers/scsi/ufs/ufshcd.h"
#include "drivers/scsi/sd.h"

#define FAULT_MAX 16
#define TIMEOUT_REQUEST -1
#define TIMEOUT_RESPONESE -2
#define MSEC(time) div_u64(time*HZ,1000)

#define log_err(fmt) "[ScsiError]:" fmt
#define tool_err(fmt, ...) do {\
	if (log_enable)\
		pr_warn(log_err(fmt), ##__VA_ARGS__);\
	 } while (0)

#define rasbase_setstr(ins, fld, arg) do {\
	int len = strlen(#fld"="); \
	if (0 == strncmp(#fld"=", arg, len)) { \
		char *cval = arg + len;   \
		ras_retn_if(0 == *cval, -EINVAL); \
		strcpy(ins->fld, cval);\
		return 0; \
	} \
} while (0)

typedef int (*Qcmd) (struct Scsi_Host *, struct scsi_cmnd *);
typedef void (*Qdone) (struct scsi_cmnd *);

struct fault_attribute {
	int enable;
	int percent;
	int times;
};

/*when fault to inject*/
struct fault_args {
	char path[256];		/*file path define which disk to inject */
	struct gendisk *gdisk;	/*block device sections */
	unsigned char opcode;	/*scmd->cmnd[0] scsi operation code */
	int ufshcd_state;	/* UFSHCD states */
	char ret_status;	/*scmd->result 1-7bits */
	char ret_msg;		/*scmd->result 8-15bits */
	char ret_host;		/*scmd->result 16-23bits */
	char ret_driver;	/*scmd->result 24-31bits */
	int ret_middle;		/*Midlevel queue return values. */
	unsigned int hit;
	long long delay;	/*response delay time,-1:timeout */
	struct fault_attribute attr;	/*need inject or not */
};

struct delay_work {
	struct delayed_work task;
	struct scsi_cmnd *scmd;
};

struct entry_scmd {
	struct list_head head;
	struct scsi_cmnd *scmd;
	Qdone done_src;		/*source done function */
};

struct scsi_fault {
	struct Scsi_Host *shost;
	Qcmd qcmd_src;		/*the source function of queuecommand */
	struct fault_args fts[FAULT_MAX];
};
static LIST_HEAD(scsi_list);
static int log_enable;
static unsigned int cmd_hit[0xFF];
static struct kmem_cache *delay_cache;
static struct workqueue_struct *delay_workqueue;
static struct scsi_fault s_fault;
static unsigned int workqueue_total;
/*find the gendisk though the file on the disk*/
static struct gendisk *lookup_gendisk(char *name)
{
	struct gendisk *gdisk = NULL;
	struct block_device *bdev = NULL;

	bdev = lookup_bdev(name);
	if (!IS_ERR(bdev))
		gdisk = bdev->bd_disk;
	if (gdisk)
		tool_err("gendisk:%p,%s\n", gdisk, gdisk->disk_name);
	return gdisk;
}

static struct Scsi_Host *lookup_host(struct gendisk *gdisk)
{
	struct Scsi_Host *host = NULL;
	struct scsi_device *sdev = NULL;

	if (!gdisk)
		return NULL;
	sdev = gdisk->queue->queuedata;
	host = sdev->host;
	if (!host)
		tool_err("Host NULL.\n");
	return host;
}

static int args_set(struct fault_args *fa, char *arg)
{
	if (0 == arg)
		return -1;
	rasbase_setstr(fa, path, arg);
	rasbase_set(fa, opcode, arg);
	rasbase_set(fa, ufshcd_state, arg);
	rasbase_set(fa, ret_status, arg);
	rasbase_set(fa, ret_msg, arg);
	rasbase_set(fa, ret_host, arg);
	rasbase_set(fa, ret_driver, arg);
	rasbase_set(fa, ret_middle, arg);
	rasbase_set(fa, delay, arg);
	return 0;
}

static int attr_set(struct fault_attribute *fa, char *arg)
{
	if (0 == arg)
		return -1;
	rasbase_set(fa, percent, arg);
	rasbase_set(fa, times, arg);
	return 0;
}

static void log_scmd(struct scsi_cmnd *scmd)
{
	unsigned char code = scmd->cmnd[0];

	cmd_hit[code]++;
}

static struct entry_scmd *find_entry_scsi(struct scsi_cmnd *scmd)
{
	struct entry_scmd *entry, *entry_next;

	list_for_each_entry_safe(entry, entry_next, &scsi_list, head) {
		if (entry->scmd == scmd)
			return entry;
	}
	return NULL;
}

static int del_entry_scsi(struct scsi_cmnd *scmd)
{
	struct entry_scmd *entry;

	entry = find_entry_scsi(scmd);
	if (entry) {
		scmd->scsi_done = entry->done_src;
		list_del(&entry->head);
		ras_free(entry);
		return 0;
	}
	return -1;
}

static void del_scsi_list(void)
{
	struct scsi_cmnd *scmd;
	struct entry_scmd *entry, *entry_next;

	if (list_empty(&scsi_list))
		return;

	list_for_each_entry_safe(entry, entry_next, &scsi_list, head) {
		if (!entry)
			break;
		scmd = entry->scmd;
		list_del(&entry->head);
		if (!scmd)
			continue;
		scmd->scsi_done = entry->done_src;
		ras_free(entry);
	}
}

static int should_fail_check(struct fault_args *pfa, struct scsi_cmnd *scmd)
{
	unsigned char opcode = scmd->cmnd[0];

	if (!pfa->gdisk)
		return 0;
	if (!scmd->request) {
		tool_err("Scmd->request is NULL!\n");
		return 0;
	}
	if (pfa->gdisk != scmd->request->rq_disk && scmd->request->rq_disk) {
		tool_err("Gendisk different:%p,%p\n", pfa->gdisk,
			 scmd->request->rq_disk);
		return 0;
	}
	if (!scmd->request->rq_disk)
		tool_err("Gendisk NULL,opcode:0x%x\n", opcode);
	if (pfa->opcode != 0xFF && opcode != pfa->opcode)
		return 0;
	return 1;
}

static void delay_handler(struct work_struct *w)
{
	struct delay_work *dw = (struct delay_work *)w;

	dw->scmd->scsi_done(dw->scmd);
	kmem_cache_free(delay_cache, dw);
	workqueue_total--;
}

static int fail_done_delay(struct fault_args *pfa, struct scsi_cmnd *scmd)
{
	struct delay_work *dw;

	if (pfa->delay <= 0)
		return 0;
	dw = kmem_cache_alloc(delay_cache, GFP_ATOMIC);
	if (!dw)
		return 0;
	INIT_DELAYED_WORK(&dw->task, delay_handler);
	dw->scmd = scmd;
	pfa->hit++;
	workqueue_total++;
	queue_delayed_work(delay_workqueue, &dw->task, MSEC(pfa->delay));
	return 1;
}

static void fail_done_return(struct fault_args *pfa, struct scsi_cmnd *scmd)
{
	int result;
	unsigned char v[4] = { 0 };

	v[0] = status_byte(scmd->result);
	v[1] = msg_byte(scmd->result);
	v[2] = host_byte(scmd->result);
	v[3] = driver_byte(scmd->result);
	if (pfa->ret_status)
		v[0] = pfa->ret_status;
	if (pfa->ret_msg)
		v[1] = pfa->ret_msg;
	if (pfa->ret_host)
		v[2] = pfa->ret_host;
	if (pfa->ret_driver)
		v[3] = pfa->ret_driver;
	result = (v[0] << 1) | (v[1] << 8) | (v[2] << 16) | (v[3] << 24);
	if (result != scmd->result) {
		scmd->result = result;
		pfa->hit++;
	}
}

static int should_responese_timeout(struct fault_args *pfa)
{
	if (pfa->delay == TIMEOUT_RESPONESE) {
		pfa->hit++;
		return 1;
	}
	return 0;
}

void fail_done(struct scsi_cmnd *scmd)
{
	int i;
	struct fault_args *fa;
	struct scsi_fault *sf = &s_fault;

	if (del_entry_scsi(scmd)) {
		tool_err("Scmd[%p] is missed in done\n", scmd);
		return;
	}

	for (i = 0; i < FAULT_MAX; i++) {
		fa = &sf->fts[i];
		if (!should_fail_check(fa, scmd))
			continue;
		fail_done_return(fa, scmd);	/*modify response result */
		if (should_responese_timeout(fa))	/*timeout no response */
			return;
		if (fail_done_delay(fa, scmd))
			return;
	}
	scmd->scsi_done(scmd);	/*do source scsi_done */
}

int add_entry_scsi(struct scsi_cmnd *scmd)
{
	struct entry_scmd *entry;

	if (NULL == find_entry_scsi(scmd)) {
		ras_retn_iferr(ras_malloc
			       ((void **)&entry, sizeof(struct entry_scmd)));
		entry->done_src = scmd->scsi_done;
		scmd->scsi_done = fail_done;
		entry->scmd = scmd;
		list_add(&entry->head, &scsi_list);
		return 0;
	}
	return -1;
}

static int should_modify_ufshcd_state(struct fault_args *pfa,
				      struct Scsi_Host *shost)
{
	struct ufs_hba *hba = shost_priv(shost);

	if (hba && pfa->ufshcd_state != -1) {
		hba->ufshcd_state = pfa->ufshcd_state;
		pfa->hit++;
		return 1;
	}
	return 0;
}

static int should_request_timeout(struct fault_args *pfa)
{
	if (pfa->delay == TIMEOUT_REQUEST) {
		pfa->hit++;
		return 1;
	}
	return 0;
}

static int should_scsi_middle_return(struct fault_args *pfa)
{
	if (pfa->ret_middle) {
		pfa->hit++;
		return pfa->ret_middle;
	}
	return 0;
}

static int should_fail_attr(struct fault_attribute *pattr)
{
	if (!pattr->enable)
		return 1;	/*default enable fault */

	if (pattr->percent)
		return (prandom_u32() % 100 >= pattr->percent) ? 0 : 1;

	if (pattr->times > 0) {
		pattr->times--;
		return 1;
	}
	return 0;
}

int fail_queuecommand(struct Scsi_Host *shost, struct scsi_cmnd *scmd)
{
	unsigned long flags;
	struct fault_args *fa;
	int i, ret = 0, need_bak = 0;
	struct scsi_fault *sf = &s_fault;

	if (shost != sf->shost) {
		tool_err("mixed host:%p,%p\n", shost, sf->shost);
		return shost->hostt->queuecommand(shost, scmd);
	}
	log_scmd(scmd);
	spin_lock_irqsave(shost->host_lock, flags);
	for (i = 0; i < FAULT_MAX; i++) {
		fa = &sf->fts[i];
		if (!should_fail_check(fa, scmd))
			continue;
		if (!should_fail_attr(&fa->attr))
			continue;
		if (should_request_timeout(fa)) {	/*timeout no send */
			spin_unlock_irqrestore(shost->host_lock, flags);
			return 0;
		}
		should_modify_ufshcd_state(fa, shost);
		ret = should_scsi_middle_return(fa);
		need_bak = 1;
	}
	if (need_bak)
		add_entry_scsi(scmd);	/*bakup the command */
	spin_unlock_irqrestore(shost->host_lock, flags);

	if (ret) {		/*modify return value */
		if (ret & 0x2000) {	/*send twices */
			sf->qcmd_src(shost, scmd);
			return ret & (~0x2000);
		}
		return ret;
	}
	return sf->qcmd_src(shost, scmd);
}

int fault_args_equal(struct fault_args *dest, struct fault_args *src)
{
	if (dest->gdisk != src->gdisk)
		return 0;
	if (dest->opcode != src->opcode)
		return 0;
	return 1;
}

int swap_function(struct fault_args *pa)
{
	unsigned long flags;
	struct scsi_fault *ft = &s_fault;
	struct Scsi_Host *shost = lookup_host(pa->gdisk);

	if (!shost)
		return -1;

	if (!ft->shost)
		ft->shost = shost;
	else if (ft->shost != shost) {
		tool_err("Host ununique:%p,%p", ft->shost, shost);
		return -1;
	}
	if (fail_queuecommand != shost->hostt->queuecommand) {
		ft->qcmd_src = shost->hostt->queuecommand;
		spin_lock_irqsave(shost->host_lock, flags);
		shost->hostt->queuecommand = fail_queuecommand;
		spin_unlock_irqrestore(shost->host_lock, flags);
	}
	return 0;
}

int fill_fault(struct fault_args *pa)
{
	int i, j = -1;
	struct scsi_fault *ft = &s_fault;
	struct fault_args *fa = NULL;

	for (i = 0; i < FAULT_MAX; i++) {
		fa = &ft->fts[i];
		if (j == -1 && fa->gdisk == NULL) {
			j = i;
			continue;
		}
		if (fault_args_equal(fa, pa)) {
			j = i;
			break;
		}
	}
	if (j == -1)		/*max the faults */
		return -1;
	fa = &ft->fts[j];
	memcpy(fa, pa, sizeof(struct fault_args));

	return 0;
}

int fault_inject(struct fault_args *pa)
{
	if (swap_function(pa) == 0)
		return fill_fault(pa);
	return -1;
}

void fault_restore(void)
{
	struct scsi_fault *ft = &s_fault;
	struct Scsi_Host *shost = ft->shost;
	unsigned long flags;

	if (!shost || !shost->hostt)
		return;
	ft->shost = NULL;
	spin_lock_irqsave(shost->host_lock, flags);
	shost->hostt->queuecommand = ft->qcmd_src;
	del_scsi_list();
	spin_unlock_irqrestore(shost->host_lock, flags);
}

void workqueue_restore(void)
{
	while (workqueue_total)/*wait task finished*/
		msleep(100);

	flush_workqueue(delay_workqueue);
	destroy_workqueue(delay_workqueue);
}

void enable_attr(struct fault_args *pf)
{
	/*check attr, and enable */
	if (pf->attr.percent > 0 || pf->attr.times > 0)
		pf->attr.enable = 1;
}

int cmd_main(void *data, int argc, char *args[])
{
	int i;
	struct fault_args ft_args;

	if (argc < 1)
		return -1;
	memset(&ft_args, 0, sizeof(ft_args));
	ft_args.opcode = 0xFF;	/*can't init by zero */
	ft_args.ufshcd_state = -1;

	for (i = 0; i < argc; i++) {
		ras_retn_iferr(args_set(&ft_args, args[i]));
		ras_retn_iferr(attr_set(&ft_args.attr, args[i]));
	}

	enable_attr(&ft_args);
	ft_args.gdisk = lookup_gendisk(ft_args.path);
	if (NULL == ft_args.gdisk)
		return -1;

	if (strcmp(args[0], "log") == 0) {
		log_enable = 1;
		return swap_function(&ft_args);
	}
	return fault_inject(&ft_args);
}

static int proc_ops_show(tool)(struct seq_file *m, void *v)
{
	int i;
	struct fault_args *pfa;
	struct scsi_fault *pft = &s_fault;
	int len = ARRAY_SIZE(pft->fts);

	if (!pft->shost)
		return 0;
	seq_puts(m, "SCSI CMD:\n");
	for (i = 0; i < 0xFF; i++) {
		if (cmd_hit[i] == 0)
			continue;
		seq_printf(m, "\topcode: 0x%x,\ttotal: %u\n", i, cmd_hit[i]);
	}
	seq_printf(m, "SCSI Host:%p\n", pft->shost);
	for (i = 0; i < len; i++) {
		pfa = &pft->fts[i];
		if (!pfa->gdisk)
			continue;
		seq_printf(m, "%2d\topcode=0x%x\t", i, pfa->opcode);
		if (pfa->ufshcd_state != -1)
			seq_printf(m, "ufshcd_state=%d\t", pfa->ufshcd_state);
		if (pfa->ret_status)
			seq_printf(m, "ret_status=0x%x\t", pfa->ret_status);
		if (pfa->ret_msg)
			seq_printf(m, "ret_msg=0x%x\t", pfa->ret_msg);
		if (pfa->ret_host)
			seq_printf(m, "ret_host=0x%x\t", pfa->ret_host);
		if (pfa->ret_driver)
			seq_printf(m, "ret_driver=0x%x\t", pfa->ret_driver);
		if (pfa->ret_middle)
			seq_printf(m, "ret_middle=0x%x\t", pfa->ret_middle);
		if (pfa->delay)
			seq_printf(m, "delay=%d\t", (int)pfa->delay);
		if (pfa->hit)
			seq_printf(m, "hit=%d\t", pfa->hit);

		if (pfa->attr.enable) {
			seq_puts(m, "[");
			if (pfa->attr.percent)
				seq_printf(m, "percent=%d\%%\t",
					   pfa->attr.percent);
			if (pfa->attr.times)
				seq_printf(m, "times=%d", pfa->attr.times);
			seq_puts(m, "]");
		}
		seq_puts(m, "\n");
	}
	return 0;
}

static int proc_ops_open(tool)(struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(tool), PDE_DATA(inode));
}

static ssize_t proc_ops_write(tool) (struct file *filp,
				     const char __user *bff, size_t count,
				     loff_t *data) {
	char buf_cmd[256];

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	memset(buf_cmd, 0, sizeof(buf_cmd));
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd, count, cmd_main,
				PDE_DATA(FILE_NODE(filp))));
	return count;
}

#define MODULE_NAME "rScsi"
proc_ops_define(tool);
static int scsi_init(void)
{
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	delay_workqueue = create_singlethread_workqueue("scsi_delay_wq");
	ras_retn_if(!delay_workqueue, -EINVAL);
	delay_cache =
	    kmem_cache_create("scsi_delay_wq", sizeof(struct delay_work), 0,
			      SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);
	if (!delay_cache) {
		destroy_workqueue(delay_workqueue);
		return -EINVAL;
	}
	ras_retn_iferr(proc_init(MODULE_NAME, &proc_ops_name(tool), 0));
	return 0;
}

static void scsi_exit(void)
{
	proc_exit(MODULE_NAME);
	fault_restore();
	workqueue_restore();
}

module_init(scsi_init);
module_exit(scsi_exit);
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
