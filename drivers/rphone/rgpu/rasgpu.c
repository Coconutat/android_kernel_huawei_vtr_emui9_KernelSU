#include "../rasbase/rasbase.h"
#include "../rasbase/rasprobe.h"
#include "../rasbase/rasproc.h"
#include <linux/kernel.h>
#include <linux/plist.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/miscdevice.h>

#include <drivers/staging/android/ion/ion.h>
#include <drivers/staging/android/ion/ion_priv.h>
#define MSEC(time) (time*HZ/1000)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
#include <drivers/staging/android/sync.h>
/*ion fault,use func replace*/
struct ion_device {
	struct miscdevice dev;
	struct rb_root buffers;
	struct mutex buffer_lock;
	struct rw_semaphore client_lock;
	struct rw_semaphore heap_lock;
	struct plist_head heaps;
	long (*custom_ioctl)(struct ion_client *client, unsigned int cmd,
			     unsigned long arg);
	struct rb_root clients;
	struct dentry *debug_root;
	struct dentry *heaps_debug_root;
	struct dentry *clients_debug_root;
};
/*workqueue is for fence timeout fault*/
static struct task_struct *fence_timeout_tsk;
struct wq_head_list {
	struct list_head stlist;
	rwlock_t rwk;
};
struct wq_work_item {
	struct list_head stlist;
	struct sync_fence *fence;
	long timeout;
	unsigned long expire;
};
struct wq_head_list wq_list;
/*for fence timeout worker memory*/
static struct kmem_cache *fence_wk_cache;
#endif
typedef struct ion_device *(*get_iondev_fun)(void);
static get_iondev_fun get_iondev;

struct ion_replace_item {
	struct ion_heap *heap;
	int (*orig_alloc)(struct ion_heap *heap,
	struct ion_buffer *buffer, unsigned long len,
	unsigned long align, unsigned long flags);
	struct list_head stlist;
};
struct ion_heaps_head {
	struct list_head stlist;
	rwlock_t rwk;
};
/*for ion heap memory*/
static struct kmem_cache *ion_heap_cache;
struct ion_heaps_head ion_heaps;
/*ion fault end*/

/*workqueue is for fence timeout fault*/

/*for fence timeout worker memory*/
enum fault_type {
	FAULT_NONE = 0,
	FAULT_FENCE_TIMEOUT,
	FAULT_GPU_FAULT,
	FAULT_SOFT_RESET,
	FAULT_HARD_RESET,
	FAULT_CHIPSET_HUNG,
	FAULT_ION_ALLOC
};
struct fault_ops {
	const char *name;
	enum fault_type fault;
};
static const struct fault_ops fault_ops_list[] = {
{.name = "soft_reset", .fault = FAULT_SOFT_RESET,},
{.name = "hard_reset", .fault = FAULT_HARD_RESET,},
{.name = "fence_timeout", .fault = FAULT_FENCE_TIMEOUT},
{.name = "gpu_fault", .fault = FAULT_GPU_FAULT},
{.name = "chipset_hung", .fault = FAULT_CHIPSET_HUNG},
{.name = "ion_alloc", .fault = FAULT_ION_ALLOC},
};
#define NAME_LEN 128
struct fault_impl {
	pid_t pid;
	enum fault_type fault;
	/* fault input para*/
	char name[NAME_LEN];
	unsigned long jiff;
	int times;
	int delay;
	unsigned int space;
	unsigned int chipset_irq;
};

#define FAULT_MAX 32
struct fault_list {
	rwlock_t rwk;
	struct fault_impl impl[FAULT_MAX];
};

/*record the faults which was injected.*/
static struct fault_list fault_injected;
static struct fault_ops *get_fault_ops(enum fault_type type)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops_list); i++) {
		if (fault_ops_list[i].fault == type)
			return (struct fault_ops *)&fault_ops_list[i];
	}
	return NULL;
}
static const char *type2name(enum fault_type type)
{
	struct fault_ops *ops = get_fault_ops(type);

	return (ops) ? ops->name : "fault_unknown";
}
static enum fault_type name2type(const char *name)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops_list); i++) {
		if (strcmp(name, fault_ops_list[i].name) == 0)
			return fault_ops_list[i].fault;
	}
	return FAULT_NONE;
}
struct fault_impl *fault_include(enum fault_type type)
{
	int i = 0;
	struct fault_impl *fault = NULL;

	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		if (fault_injected.impl[i].fault == type) {
			fault = &fault_injected.impl[i];
			break;
		}
	}
	return fault;
}

int is_fault(struct fault_impl *fault)
{
	unsigned long jiff = jiffies;

	/*1. check time to fault*/
	if (fault->space > 0) {
		if (fault->jiff  == 0 ||
			(jiff - fault->jiff) > msecs_to_jiffies(fault->space))
			fault->jiff = jiff;
		else
			return 0;
	}
	/*2. check times is match*/
	if (fault->times == -1)
		return 1;

	if (fault->times > 0) {
		fault->times--;
		return 1;
	}
	return 0;
}
int set_args(struct fault_impl *fault, char *prm)
{
	if (!prm)
		return 0;
	rasbase_set_func(fault, fault, prm, name2type);
	rasbase_set(fault, times, prm);
	rasbase_set(fault, space, prm);
	rasbase_set(fault, delay, prm);
	rasbase_set(fault, chipset_irq, prm);
	rasbase_cset(fault, name, prm);
	return 0;
}

int fit_set_args(struct fault_impl *fault, int args, char *argv[])
{
	int i;

	/*init fault args*/
	fault->times = -1;
	for (i = 0; i < args; i++)
		ras_retn_iferr(set_args(fault, argv[i]));
	ras_retn_if((fault->fault == FAULT_NONE), -EINVAL);
	/*mali gpu reset allow 500ms, default 600ms*/
	if (fault->fault == FAULT_HARD_RESET && fault->delay == 0)
		fault->delay = 600;
	/*fence timeout 10s*/
	if (fault->fault == FAULT_FENCE_TIMEOUT && fault->delay == 0)
		fault->delay = 10000;
	return 0;
}
/*ion heaps allocate*/
int ion_heap_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
	unsigned long len, unsigned long align, unsigned long flags)
{
	struct ion_replace_item *heap_item, *next;
	struct fault_impl *fault;
	int find = 0;

	/*find heap*/
	read_lock(&ion_heaps.rwk);
	list_for_each_entry_safe(heap_item, next, &(ion_heaps.stlist), stlist) {
		if (heap_item->heap != heap)
			continue;
		find = 1;
		break;
	}
	read_unlock(&ion_heaps.rwk);
	if (!find)
		return -EINVAL;
	/*orig_alloc*/
	fault = fault_include(FAULT_ION_ALLOC);
	if (!fault || !is_fault(fault))
		return heap_item->orig_alloc(heap, buffer, len, align, flags);
	return -ENOMEM;
}

void restore_ion_allocate(void)
{
	struct ion_device *dev;
	struct ion_replace_item *heap_item, *next;

	if (!get_iondev)
		return;
	dev = get_iondev();
	if (!dev)
		return;

	down_write(&(dev->heap_lock));
	write_lock(&ion_heaps.rwk);
	list_for_each_entry_safe(heap_item, next, &(ion_heaps.stlist), stlist) {
		heap_item->heap->ops->allocate = heap_item->orig_alloc;
		list_del_init(&heap_item->stlist);
		kmem_cache_free(ion_heap_cache, heap_item);
	}
	write_unlock(&ion_heaps.rwk);
	up_write(&dev->heap_lock);
}

/*repleace ion heaps func*/
int replace_ion_func(void)
{
	struct ion_device *dev;
	struct ion_heap *heap;
	struct ion_replace_item *heap_item;

	if (!get_iondev)
		return 0;
	dev = get_iondev();
	if (!dev)
		return 0;

	down_read(&dev->heap_lock);
	plist_for_each_entry(heap, &dev->heaps, node) {
		/*repleace ion heap allocate func*/
		/*1.if ops is NULL ,or allocate is NULL no replace*/
		if (!heap->ops || !heap->ops->allocate)
			continue;
		/*2.if is already replace, no replace*/
		if (heap->ops->allocate == ion_heap_allocate)
			continue;
		/*3. replace*/
		heap_item = kmem_cache_alloc(ion_heap_cache, GFP_ATOMIC);
		if (!heap_item)
			break;
		heap_item->heap = heap;
		heap_item->orig_alloc = heap->ops->allocate;
		INIT_LIST_HEAD(&heap_item->stlist);
		write_lock(&ion_heaps.rwk);
		list_add_tail(&heap_item->stlist, &ion_heaps.stlist);
		write_unlock(&ion_heaps.rwk);
		heap->ops->allocate = ion_heap_allocate;
	}
	up_read(&dev->heap_lock);
	return 1;
}

/*Convert arguments to faults, then inject and inject them.*/
static struct fault_impl *args2fault(int args, char *argv[])
{
	int i = 0;
	struct fault_impl *fault = NULL, fault_tmp;

	/*1. check the commands*/
	memset(&fault_tmp, 0, sizeof(struct fault_impl));
	if (fit_set_args(&fault_tmp, args, argv))
		return NULL;

	/*2. manage the faults*/
	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		if (fault == NULL && fault_injected.impl[i].fault == FAULT_NONE)
			fault = &fault_injected.impl[i];
		/*fault already exist*/
		if (fault_injected.impl[i].fault == fault_tmp.fault) {
			fault = &fault_injected.impl[i];
			break;
		}
	}
	/*3. inject*/
	if (NULL == fault)
		return NULL;
	memcpy(fault, &fault_tmp, sizeof(struct fault_impl));
	if (fault->fault == FAULT_ION_ALLOC && !replace_ion_func())
		return NULL;
	return fault;
}

static int rasprobe_handler(kbase_mmu_hw_do_operation)
	(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct fault_impl *fault = fault_include(FAULT_SOFT_RESET);

	if (!fault || !is_fault(fault))
		return 0;
	rasprobe_seturn(regs, 1);
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
static int rasprobe_handler_entry(fence_check_cb_func)
	(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct RasRegs *rr = NULL;
	struct fence_cb *f_cb = NULL;
	struct sync_fence *fence;
	struct sync_fence_cb *check;
	struct wq_work_item *fence_wk;
	struct fault_impl *fault = fault_include(FAULT_FENCE_TIMEOUT);

	rasprobe_entry(ri, regs);
	rr = (struct RasRegs *)ri->data;
	f_cb = (struct fence_cb *)rr->args[1];
	check = container_of(f_cb, struct sync_fence_cb, cb);
	fence = check->fence;
	if (!fault || !is_fault(fault))
		return 0;

	if (strlen(fault->name) && strcasecmp(fault->name, fence->name))
		return 0;
	fence_wk = kmem_cache_alloc(fence_wk_cache, GFP_ATOMIC);
	if (!fence_wk)
		return 0;

	fence_wk->fence = fence;
	atomic_inc(&fence->status);
	INIT_LIST_HEAD(&fence_wk->stlist);
	fence_wk->timeout = fault->delay;
	if (fault->delay >= 0)
		fence_wk->expire = msecs_to_jiffies(100 + fault->delay) + jiffies;
	write_lock(&wq_list.rwk);
	list_add_tail(&fence_wk->stlist, &wq_list.stlist);
	write_unlock(&wq_list.rwk);
	return 0;
}
void check_del_work(int ischeck)
{
	struct wq_work_item *wk_item, *next;

	write_lock(&wq_list.rwk);
	list_for_each_entry_safe(wk_item, next, &(wq_list.stlist), stlist) {
		if (ischeck &&
		(wk_item->timeout < 0 || time_after(wk_item->expire, jiffies)))
			continue;
		atomic_dec(&(wk_item->fence->status));
		list_del_init(&wk_item->stlist);
		kmem_cache_free(fence_wk_cache, wk_item);
	}
	write_unlock(&wq_list.rwk);
}

static int fit_fence_timeout_thread(void *data)
{
	while (!kthread_should_stop()) {
		check_del_work(1);
		ras_sleep(500);
	}
	return 0;
}
#endif
#include <linux/delay.h>
void delay_block(unsigned long long ms)
{
	while (ms) {
		if (ms >= 2) {
			udelay(2000);
			ms -= 2;
		} else {
			udelay(ms * 1000);
			ms = 0;
		}
	}
}

/*kbase_pm_reset_do_normal--->gpu reset --->wait irq
 *-->kbase_gpu_irq_handler -->kbase_reg_read get gpu status
 *--->kbase_pm_reset_do_normal wait timeout,check status
 */
static int rasprobe_handler(kbase_pm_wait_for_reset)
	(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct fault_impl *fault = fault_include(FAULT_HARD_RESET);

	if (!fault || !is_fault(fault))
		return 0;
	delay_block(fault->delay);
	return 0;
}

static int rasprobe_handler_entry(kbase_gpu_complete_hw)
	(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct RasRegs *rr = NULL;
	struct fault_impl *fault = fault_include(FAULT_GPU_FAULT);

	if (!fault || !is_fault(fault))
		return 0;
	rasprobe_entry(ri, regs);
	rr = (struct RasRegs *)ri->data;
	/* 0x1 mali event code, BASE_JD_EVENT_DONE,*/
	if ((long)rr->args[2] == 0x1)
		/*mali event code, BASE_JD_EVENT_TERMINATED=0x4;*/
		rasprobe_setarg(regs, 2, 0x4);
	return 0;
}

static int rasprobe_handler_entry(hisi_powerkey_handler)
	(struct rasprobe_instance *ri, struct pt_regs *regs)
{
	struct RasRegs *rr = NULL;
	struct fault_impl *fault = fault_include(FAULT_CHIPSET_HUNG);

	rasprobe_entry(ri, regs);
	rr = (struct RasRegs *)ri->data;
	if (!fault)
		return 0;
	if (fault->chipset_irq != -1
		&& ((long)rr->args[0] != fault->chipset_irq))
		return 0;
	if (!is_fault(fault))
		return 0;
	/* chipeset irq */
	rasprobe_setarg(regs, 0, 0);/*set 0 to ignore irq*/
	return 0;
}

rasprobe_define(kbase_pm_wait_for_reset);/*hard_reset*/
rasprobe_define(kbase_mmu_hw_do_operation);/*soft_reset*/
rasprobe_entry_define(kbase_gpu_complete_hw);/*gpu_fault*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
rasprobe_entry_define(fence_check_cb_func);/*fence_timeout*/
#endif
rasprobe_entry_define(hisi_powerkey_handler);/*chipset_hang*/

static struct rasprobe *probes[] = {
	&rasprobe_name(kbase_pm_wait_for_reset),
	&rasprobe_name(kbase_mmu_hw_do_operation),
	&rasprobe_name(kbase_gpu_complete_hw),
	#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
	&rasprobe_name(fence_check_cb_func),
	#endif
	&rasprobe_name(hisi_powerkey_handler)
};

static int cmd_main(void *data, int argc, char *args[])
{
	ras_retn_if(0 == argc, -EINVAL);
	ras_retn_if(NULL == args2fault(argc, args), -EINVAL);
	return 0;
}
static int proc_ops_show(rGPU) (struct seq_file *m, void *v)
{
	int i = 0;
	struct fault_impl *impl = NULL;

	for (i = 0; i < ARRAY_SIZE(fault_injected.impl); i++) {
		impl = &fault_injected.impl[i];
		if (impl->fault == FAULT_NONE)
			continue;
		seq_printf(m, "%2d\t%s\t time=%d\t space=%u\n",
			i, type2name(impl->fault), impl->times, impl->space);
	}
	return 0;
}
static int proc_ops_open(rGPU) (struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rGPU), NULL);
}
static ssize_t proc_ops_write(rGPU) (struct file *filp,
		const char __user *bff, size_t count,
		loff_t *data) {
	char buf_cmd[256];

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	memset(buf_cmd, 0, sizeof(buf_cmd));
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd, count, cmd_main, NULL));
	return count;
}


#define MODULE_NAME "rGPU"
proc_ops_define(rGPU);

static int tool_init(void)
{
	/*1. initialize memory*/
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	memset(&fault_injected, 0, sizeof(struct fault_list));
	rwlock_init(&ion_heaps.rwk);
	INIT_LIST_HEAD(&ion_heaps.stlist);
	/*2.ion fault*/
	get_iondev = (get_iondev_fun)kallsyms_lookup_name("get_ion_device");
	ras_retn_if(!get_iondev, -EINVAL);

	/*3. initialize probes and interface*/
	ras_retn_iferr(register_rasprobes(probes, ARRAY_SIZE(probes)));
	if (proc_init(MODULE_NAME, &proc_ops_name(rGPU), &fault_injected))
		goto out_unreg;
	ion_heap_cache = kmem_cache_create("ion_replace_item",
		sizeof(struct ion_replace_item), 0,
		SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL);
	if (!ion_heap_cache)
		goto out_proc;

	#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
	rwlock_init(&wq_list.rwk);
	INIT_LIST_HEAD(&wq_list.stlist);
	fence_timeout_tsk = kthread_run(fit_fence_timeout_thread,
		NULL, "fit_fencetimeout");
	if (!fence_timeout_tsk) {
		kmem_cache_destroy(ion_heap_cache);
		goto out_proc;
	}
	fence_wk_cache = kmem_cache_create("wq_work_item",
		sizeof(struct wq_work_item), 0,
		SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL);
	if (!fence_wk_cache) {
		kmem_cache_destroy(ion_heap_cache);
		kthread_stop(fence_timeout_tsk);
		goto out_proc;
	}
	#endif
	return 0;
out_proc:
	proc_exit(MODULE_NAME);
out_unreg:
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	return -EINVAL;
}

static void tool_exit(void)
{
	/*1.destroy interface and probes*/
	proc_exit(MODULE_NAME);
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	/*2.destroy the workqueue and clean memory*/
	memset(&fault_injected, 0, sizeof(struct fault_list));
	#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
	if (fence_timeout_tsk)
		kthread_stop(fence_timeout_tsk);
	check_del_work(0);
	kmem_cache_destroy(fence_wk_cache);
	#endif
	restore_ion_allocate();
	kmem_cache_destroy(ion_heap_cache);
}
module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("GPU faults inject.");
MODULE_LICENSE("GPL");
MODULE_VERSION("V001R001C151-1.0");

