

#include <linux/audit.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/random.h>
#include <asm/page.h>
#include <chipset_common/hiview_selinux/hiview_selinux.h>

#define CREATE_TRACE_POINTS
#include <trace/events/hiview_selinux.h>
#include <log/log_usertype/log-usertype.h>

#define PATH_LEN		128
#define COMM_LEN 		64
#define MAX_WORKER		256
#define TMP_TOTAL_COUNT	 	10000

static struct workqueue_struct *s_wq = NULL;

static unsigned int usertype = 0;
static unsigned int max_count = 500;
static atomic_t s_c_queued;
static atomic_t s_c_finished;
static int s_c_old = 0;
static u32 droped_worker = 0;
static unsigned int temp_worker = 0;
static u64 real_begin = 0, real_end = 0;
static unsigned long wait_time = 1000000000;
static unsigned int randnum = 0;

/*
 * We only need this data after we have decided to send an audit message.
 */
struct selinux_audit_data {
	u32 ssid;
	u32 tsid;
	u16 tclass;
	u32 requested;
	u32 audited;
	u32 denied;
	int result;
};

typedef struct _tag_hiview_fs_work
{
    struct work_struct          work;

    u64                         timestamp;
    ino_t 			ino;
    char			*fullpath;
    char 			path[PATH_LEN];
    char 			comm[COMM_LEN];
    struct selinux_audit_data   audit;
    struct task_struct          *task;
    struct dentry               *dentry;
    struct inode                *inode;
} hiview_fs_work_t;

static inline hiview_fs_work_t* create_work_item(struct common_audit_data *cad)
{
	hiview_fs_work_t *w = NULL;
	struct dentry *d = NULL;
	struct inode *i = NULL;

	switch (cad->type) {
	case LSM_AUDIT_DATA_PATH:
		d = cad->u.path.dentry;
		break;

	case LSM_AUDIT_DATA_INODE:
		i = cad->u.inode;
		break;

	case LSM_AUDIT_DATA_DENTRY:
		d = cad->u.dentry;
		break;

	case LSM_AUDIT_DATA_IOCTL_OP:
		d = cad->u.op->path.dentry;
		break;

	default:  //  only care about filesystem related actions
		goto end;
	}

	w = kzalloc(sizeof(hiview_fs_work_t), GFP_KERNEL);
	if (!w) {
		printk(KERN_WARNING "%s(%d): kzalloc failed!\n", __func__, __LINE__);
		goto end;
	}

	w->timestamp = local_clock();
	w->audit = *cad->selinux_audit_data;
	w->task = current->group_leader;
	w->dentry = d;
	w->inode = i;

	get_task_struct(w->task);

	if (d) {
		dget(d);
		w->inode = d_backing_inode(w->dentry);
		if (w->inode)
			igrab(w->inode);
	}
	if (i) {
		igrab(i);
		w->dentry = d_find_alias(w->inode);
	}

	if (w->dentry && w->inode) {
		w->fullpath = dentry_path_raw(w->dentry, w->path, PATH_LEN);
		if (w->dentry->d_inode)
			w->ino = w->dentry->d_inode->i_ino;
	}

	if (w->dentry)
		dput(w->dentry);
	if (w->inode)
		iput(w->inode);

end:
	return w;
}

static inline void destroy_work_item(hiview_fs_work_t *w)
{
	put_task_struct(w->task);

	kfree(w);
}

static inline void hiview_log_out(hiview_fs_work_t *w)
{
	if (IS_ERR(w->fullpath) || (w->fullpath == NULL))
		return;

	get_cmdline(w->task, w->comm, COMM_LEN);

	trace_hiview_log_out(task_tgid_nr(w->task), w->ino, w->audit.requested, w->audit.denied, w->audit.tclass,
			     w->fullpath, w->comm, droped_worker);
}

static void hiview_fs_do(struct work_struct *ws)
{
    hiview_fs_work_t *w;

	if (ws) {
		w = container_of(ws, hiview_fs_work_t, work);
		hiview_log_out(w);
		atomic_inc(&s_c_finished);
		destroy_work_item(w);
	}
}

static void hiview_fs_do_work(struct work_struct *ws)
{
#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
    struct timespec64 begin, end;
    int queued;
    int finished;

    getnstimeofday64(&begin);
#endif

    hiview_fs_do(ws);

#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
    getnstimeofday64(&end);
    begin = timespec64_sub(end, begin);

    queued = atomic_read(&s_c_queued);
    finished = atomic_read(&s_c_finished);

    printk(KERN_WARNING "%s takes %ldns (wait=%d q=%d f=%d)!\n",
            __func__, (long)begin.tv_sec * NSEC_PER_SEC + begin.tv_nsec,
            queued - finished, queued, finished);
#endif
}

static int __init hiview_fs_init(void)
{
    atomic_set(&s_c_queued, 0);
    atomic_set(&s_c_finished, 0);
    real_begin = local_clock();

    s_wq = alloc_workqueue("hiviewsa", 0, 1);
    if (s_wq == NULL) {
        return EFAULT;
    }
    return 0;
}

late_initcall(hiview_fs_init)

static inline int hiview_fs(struct common_audit_data *cad)
{
	hiview_fs_work_t *w;
	int ret = 1;

	if ((atomic_read(&s_c_queued) - s_c_old) > max_count) {
		real_end = local_clock();
		if (real_end - real_begin > wait_time) {
			real_begin = real_end;
			s_c_old = atomic_read(&s_c_queued);
		} else {
			droped_worker++;
#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
			printk(KERN_DEBUG "%s queue larger than excepted, droped:%d, queued = %d!\n",
					__func__, droped_worker, atomic_read(&s_c_queued));
#endif
			goto out;
		}
	}

	if (atomic_read(&s_c_queued) - atomic_read(&s_c_finished) < MAX_WORKER) {
		w = create_work_item(cad);

		if (w != NULL) {
			INIT_WORK(&w->work, hiview_fs_do_work);
			queue_work(s_wq, &w->work);
			atomic_inc(&s_c_queued);
		}
	} else {
		droped_worker++;
#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
		printk(KERN_WARNING "%s wait_queue larger than 256, droped:%d!\n",
				__func__, droped_worker);
#endif
	}
out:
	return ret;
}

int hw_hiview_selinux_avc_audit(struct common_audit_data *cad)
{
	int ret = 1;
#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
	struct timespec64 begin, end;
	getnstimeofday64(&begin);
#endif

	switch (cad->type) {
	case LSM_AUDIT_DATA_PATH:
	case LSM_AUDIT_DATA_INODE:
	case LSM_AUDIT_DATA_DENTRY:
	case LSM_AUDIT_DATA_IOCTL_OP:
		break;

	default:  //  only care about filesystem related actions
		goto out;
	}

	if (usertype == 0)
		usertype = get_logusertype_flag();

	if (usertype == COMMERCIAL_USER) {
		temp_worker++;
		if (randnum == 0)
			randnum = get_random_int() % TMP_TOTAL_COUNT;

		if (temp_worker ==  randnum) {
			ret = hiview_fs(cad);
		} else {
			droped_worker++;
			if (temp_worker >= TMP_TOTAL_COUNT) {
				randnum = 0;
				temp_worker = 0;
			}
		}
	} else if (usertype == BETA_USER) {
		ret = hiview_fs(cad);
	}

out:
#ifdef CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE
	getnstimeofday64(&end);
	begin = timespec64_sub(end, begin);
	printk(KERN_WARNING "%s takes %ldns!\n", __func__,
		(long)begin.tv_sec * NSEC_PER_SEC + begin.tv_nsec);
#endif
	return ret;
}
