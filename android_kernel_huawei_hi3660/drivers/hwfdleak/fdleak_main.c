/**********************************************************
 * Filename:    fdleak_main.c
 *
 * Discription: ioctl and leak watchpoint report implementaion for fdleak
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: huangyu(00381502)
 *
**********************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/ioctls.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/stacktrace.h>
#include <chipset_common/hwlogger/hw_logger.h>
#include <chipset_common/hwfdleak/fdleak.h>
#include <log/log_usertype/log-usertype.h>

//#define FDLEAK_DEBUG
#define fdleak_err(format, ...)  do {printk(KERN_ERR "[fdleak]%s %d: " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define fdleak_info(format, ...)  do {printk(KERN_DEBUG "[fdleak]%s %d: " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)

typedef struct{
    stack_item items[MAX_STACK_TRACE_COUNT];
    int hit_cnt[MAX_STACK_TRACE_COUNT];
    int diff_cnt;
}stack_pid_item;
typedef struct {
    fdleak_wp_id id;
    char *name;
    int probe_cnt;
    int pid_cnt;
    int pids[MAX_FDLEAK_PID_NUM];
    int is_32bit[MAX_FDLEAK_PID_NUM];
    stack_pid_item* list[MAX_FDLEAK_PID_NUM];
}fdleak_wp_item;
static fdleak_wp_item fdleak_table[] =
{
    {FDLEAK_WP_EVENTFD,     "eventfd",   MAX_EVENTPOLL_PROBE_CNT, 0, {0}, {0}, {0}},
    {FDLEAK_WP_EVENTPOLL,   "eventpoll", MAX_EVENTFD_PROBE_CNT,   0, {0}, {0}, {0}},
    {FDLEAK_WP_DMABUF,      "dmabuf",    MAX_DMABUF_PROBE_CNT,    0, {0}, {0}, {0}},
    {FDLEAK_WP_SYNCFENCE,   "syncfence", MAX_SYNCFENCE_PROBE_CNT, 0, {0}, {0}, {0}},
    {FDLEAK_WP_SOCKET,      "socket",    MAX_SOCKET_PROBE_CNT,    0, {0}, {0}, {0}},
    {FDLEAK_WP_PIPE,        "pipe",      MAX_PIPE_PROBE_CNT,      0, {0}, {0}, {0}},
    {FDLEAK_WP_ASHMEM,      "ashmem",    MAX_ASHMEM_PROBE_CNT,    0, {0}, {0}, {0}},
};

struct stack_frame_user64 {
    const void __user    *fp;
    unsigned long long    lr;
};
struct stack_frame_user32 {
    unsigned int    fp;
    unsigned int    sp;
    unsigned int    lr;
    unsigned int    pc;
};
static unsigned long long stack_entries[FDLEAK_MAX_STACK_TRACE_DEPTH] = {0};
static unsigned int usertype = 0;
static int tgid_hiview = 0;

DEFINE_MUTEX(mutex);

static int fdleak_check_parameter(void __user *argp, fdleak_op *op)
{
    if (!argp || !op) {
        fdleak_err("invalid ioctl parameter\n");
        return -1;
    }
    if (copy_from_user((void*)op, argp, sizeof(*op))) {
        fdleak_err("can not copy parameter data from user space\n");
        return -1;
    }
    if (op->magic != FDLEAK_MAGIC) {
        fdleak_err("invalid magic number\n");
        return -1;
    }
    if (op->pid <= 0 ) {
        fdleak_err("invalid pid \n");
        return -1;
    }

    return 0;
}

static int fdleak_find_pid(int pid, int wpid)
{
    int idx_pid = 0;

    for (idx_pid = 0; idx_pid < fdleak_table[wpid].pid_cnt; idx_pid++) {
        if (fdleak_table[wpid].pids[idx_pid] == pid)
            break;
    }
    if (idx_pid == fdleak_table[wpid].pid_cnt) {
        return -1;
    }

    return idx_pid;
}

static int fdleak_insert_pid(int pid, int wpid)
{
    int idx_pid = -1;
    void *stack = NULL;

    if (wpid < FDLEAK_WP_MIN || wpid >= FDLEAK_WP_NUM_MAX)
        return -1;
    if (fdleak_table[wpid].pid_cnt >= MAX_FDLEAK_PID_NUM) {
        fdleak_err("the leak watchpoint %s has reach to maximun pid count\n", fdleak_table[wpid].name);
        return -1;
    }

    idx_pid = fdleak_find_pid(pid, wpid);
    if (idx_pid >= 0) {
        fdleak_err("the leak watchpoint %s is aleady enabled for pid=%d, do nothing\n", fdleak_table[wpid].name, pid);
    } else {
        int size = fdleak_table[wpid].probe_cnt * sizeof(stack_pid_item);
        stack = vmalloc(size);
        if (!stack) {
            fdleak_err("the leak watchpoint %s insert for pid=%d failed due to vmalloc failed\n", fdleak_table[wpid].name, pid);
            return -1;
        }
        memset(stack, 0, size);
        fdleak_table[wpid].pids[fdleak_table[wpid].pid_cnt] = pid;
        fdleak_table[wpid].list[fdleak_table[wpid].pid_cnt] = stack;
        fdleak_table[wpid].pid_cnt++;
        fdleak_info("the leak watchpoint %s for pid=%d insert success\n", fdleak_table[wpid].name, pid);
    }

    return 0;
}

static int fdleak_remove_pid(int pid, int wpid)
{
    int idx_pid = 0;

    if (wpid < FDLEAK_WP_MIN || wpid >= FDLEAK_WP_NUM_MAX)
        return -1;
    if (fdleak_table[wpid].pid_cnt <= 0) {
        fdleak_err("the leak watchpoint %s for pid %d has been disabled\n", fdleak_table[wpid].name, pid);
        return -1;
    }

    idx_pid = fdleak_find_pid(pid, wpid);
    if (idx_pid < 0) {
        fdleak_err("the leak watchpoint %s is aleady removed for pid=%d, do nothing\n", fdleak_table[wpid].name, pid);
        return -1;
    }

    if (fdleak_table[wpid].list[idx_pid]) {
        vfree(fdleak_table[wpid].list[idx_pid]);
        fdleak_table[wpid].list[idx_pid] = NULL;
    }
    if (idx_pid < fdleak_table[wpid].pid_cnt - 1)
    {
        int copy_count = fdleak_table[wpid].pid_cnt - idx_pid - 1;
        memmove(&fdleak_table[wpid].pids[idx_pid], &fdleak_table[wpid].pids[idx_pid + 1], copy_count * sizeof(fdleak_table[wpid].pids[idx_pid]));
        memmove(&fdleak_table[wpid].is_32bit[idx_pid], &fdleak_table[wpid].is_32bit[idx_pid + 1], copy_count * sizeof(fdleak_table[wpid].is_32bit[idx_pid]));
        memmove(&fdleak_table[wpid].list[idx_pid], &fdleak_table[wpid].list[idx_pid + 1], copy_count * sizeof(fdleak_table[wpid].list[idx_pid]));
    }
    fdleak_table[wpid].pid_cnt--;
    fdleak_info("the leak watchpoint %s removed for pid=%d successfully\n", fdleak_table[wpid].name, pid);
    return 0;
}

static void fdleak_cleanup(void)
{
    int index_pid = 0;
    int wpid = 0;

    for (wpid = FDLEAK_WP_MIN; wpid < FDLEAK_WP_NUM_MAX; wpid++) {
        fdleak_table[wpid].pid_cnt = 0;
        memset(fdleak_table[wpid].pids, 0, sizeof(fdleak_table[wpid].pids));
        memset(fdleak_table[wpid].is_32bit, 0, sizeof(fdleak_table[wpid].is_32bit));
        for (index_pid = 0; index_pid < MAX_FDLEAK_PID_NUM; index_pid++) {
            if (fdleak_table[wpid].list[index_pid]) {
                vfree(fdleak_table[wpid].list[index_pid]);
                fdleak_table[wpid].list[index_pid] = NULL;
            }
        }
    }
    fdleak_err("hiview crash, cleanup kernel fdleak data\n");
}

static int fdleak_process_onoff(fdleak_op *op, bool enable)
{
    int wpid = 0;

    if (!op)
        return -1;

    for (wpid = FDLEAK_WP_MIN; wpid < FDLEAK_WP_NUM_MAX; wpid++) {
        if (op->wp_mask & (1 << wpid)) {
            if (fdleak_table[wpid].probe_cnt <= 0) {
                fdleak_err("the leak watchpoint %s is not supported to be probed currently\n", fdleak_table[wpid].name);
                continue;
            }
            mutex_lock(&mutex);
            if (enable) {
                //check if pid of hiview changed or not
                if (current->real_parent && tgid_hiview != current->real_parent->tgid) {
                    fdleak_err("old hiview tgid=%d, current hiview tgid=%d", tgid_hiview, current->real_parent->tgid);
                    if (tgid_hiview) {
                        fdleak_cleanup();
                    }
                    tgid_hiview = current->real_parent->tgid;
                }
                fdleak_insert_pid(op->pid, wpid);
            }
            else {
                fdleak_remove_pid(op->pid, wpid);
            }
            mutex_unlock(&mutex);
        }
    }
    return 0;
}

static int fdleak_get_stack(void __user *arg)
{
    int i = 0;
    int idx_pid = -1;
    int probe_id = 0;
    fdleak_stackinfo *info = NULL;

    if (!arg) {
        fdleak_err("invalid ioctl parameter for FDLEAK_GET_STACK\n");
        return -1;
    }

    info = vmalloc(sizeof(*info));
    if (!info) {
        fdleak_err("vmalloc memory for FDLEAK_GET_STACK ioctl failed\n");
        return -1;
    }
    memset(info, 0, sizeof(*info));
    if (copy_from_user((void*)info, arg, sizeof(*info))) {
        fdleak_err("can not copy data from user space for FDLEAK_GET_STACK\n");
        goto err;
    }
    if (info->magic != FDLEAK_MAGIC) {
        fdleak_err("invalid magic number for FDLEAK_GET_STACK\n");
        goto err;
    }
    if (info->pid <= 0) {
        fdleak_err("invalid pid\n");
        goto err;
    }
    if (info->wpid < FDLEAK_WP_MIN || info->wpid >= FDLEAK_WP_NUM_MAX) {
        fdleak_err("invalid leak watchpoint id\n");
        goto err;
    }
    if (fdleak_table[info->wpid].probe_cnt <= 0) {
        fdleak_err("no probes for this leak watchpoint %s\n", fdleak_table[info->wpid].name);
        goto err;
    }

    mutex_lock(&mutex);
    idx_pid = fdleak_find_pid(info->pid, info->wpid);
    if (idx_pid < 0) {
        fdleak_err("the leak watchpoint %s for pid=%d is not enable, return nothing to userspace\n", fdleak_table[info->wpid].name, info->pid);
        mutex_unlock(&mutex);
        goto err;
    }

    info->is_32bit = fdleak_table[info->wpid].is_32bit[idx_pid];
    info->probe_cnt = fdleak_table[info->wpid].probe_cnt;
    strncpy(info->wp_name, fdleak_table[info->wpid].name, sizeof(info->wp_name));

    for (probe_id = 0 ; probe_id < info->probe_cnt; probe_id++) {
        info->diff_cnt[probe_id] = fdleak_table[info->wpid].list[idx_pid][probe_id].diff_cnt;
        memcpy(info->hit_cnt[probe_id], fdleak_table[info->wpid].list[idx_pid][probe_id].hit_cnt, sizeof(info->hit_cnt[probe_id]));
        for (i = 0; i < info->diff_cnt[probe_id]; i++) {
            memcpy(&info->list[probe_id][i], &fdleak_table[info->wpid].list[idx_pid][probe_id].items[i], sizeof(info->list[probe_id][i]));
        }
    }
    mutex_unlock(&mutex);

    if (copy_to_user(arg, info, sizeof(*info))) {
        fdleak_err("can not copy data to user space for FDLEAK_GET_STACK\n");
        goto err;
    }

    vfree(info);
    return 0;

err:
    vfree(info);
    return -1;
}

long fdleak_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = FDLEAK_CMD_INVALID;
    fdleak_op op = {0};

    if(cmd < FDLEAK_ENABLE_WATCH || cmd >= FDLEAK_CMD_MAX)
        return ret;

    switch (cmd) {
        case FDLEAK_ENABLE_WATCH:
            memset(&op, 0, sizeof(op));
            if (fdleak_check_parameter((void*)arg, &op)) {
                return -EINVAL;
            }
            ret = fdleak_process_onoff(&op, true);
            break;

        case FDLEAK_DISABLE_WATCH:
            memset(&op, 0, sizeof(op));
            if (fdleak_check_parameter((void*)arg, &op)) {
                return -EINVAL;
            }
            ret = fdleak_process_onoff(&op, false);
            break;

        case FDLEAK_GET_STACK:
            ret = fdleak_get_stack((void*)arg);
            break;

        default:
            break;
    }

    return ret;
}
EXPORT_SYMBOL(fdleak_ioctl);

/*****the following session are used for save user stack when specific syscall happen*******/
static void save_stack_trace32(struct task_struct *task, struct stack_trace *trace)
{
    const struct pt_regs *regs = task_pt_regs(task);
    const void __user *fp = NULL;
    unsigned long addr = 0;

    fp = (const void __user *)regs->regs[11];
    if (trace->nr_entries < trace->max_entries)
        trace->entries[trace->nr_entries++] = regs->pc;

    while (trace->nr_entries < trace->max_entries) {
        struct stack_frame_user32 frame;

        memset(&frame, 0, sizeof(frame));
        addr = (unsigned long)fp;
        if (!access_process_vm(task, addr, (void *)&frame, sizeof(frame), 0))
            break;
        if ((unsigned long)fp < regs->sp)
            break;
        if (frame.lr)
            trace->entries[trace->nr_entries++] = frame.lr;
        if ((unsigned long)fp == frame.fp)
            break;
        fp = (const void __user *)frame.fp;
    }

    if (trace->nr_entries < trace->max_entries)
        trace->entries[trace->nr_entries++] = ULONG_MAX;
}

static void save_stack_trace64(struct task_struct *task, struct stack_trace *trace)
{
    const struct pt_regs *regs = task_pt_regs(task);
    const void __user *fp = NULL;
    unsigned long addr = 0;
    
    fp = (const void __user *)regs->regs[29];
    if (trace->nr_entries < trace->max_entries)
        trace->entries[trace->nr_entries++] = regs->pc;

    while (trace->nr_entries < trace->max_entries) {
        struct stack_frame_user64 frame;

        memset(&frame, 0, sizeof(frame));
        addr = (unsigned long)fp;
        if (!access_process_vm(task, addr, (void *)&frame, sizeof(frame), 0))
            break;
        if ((unsigned long)fp < regs->sp)
            break;
        if (frame.lr)
            trace->entries[trace->nr_entries++] = frame.lr;
        if (fp == frame.fp)
            break;
        fp = frame.fp;
    }

    if (trace->nr_entries < trace->max_entries)
        trace->entries[trace->nr_entries++] = ULONG_MAX;
}

static bool fdleak_compare_stack(unsigned long long *stack1, unsigned long long *stack2)
{
    int i = 0;

    for (i = 0; (i < FDLEAK_MAX_STACK_TRACE_DEPTH) && (stack1[i] == stack2[i]); i++);

    return (i == FDLEAK_MAX_STACK_TRACE_DEPTH) ? true : false;
}

static int fdleak_insert_stack(fdleak_wp_id wpid, int pid_index, int probe_id, struct stack_trace *stack)
{
    int i = 0;

    if (fdleak_table[wpid].list[pid_index][probe_id].diff_cnt >= MAX_STACK_TRACE_COUNT) {
        //fdleak_err("the leak watchpoint %s probe_id(%d) has reach to max stack trace count\n", fdleak_table[wpid].name, probe_id);
        return -1;
    }
    for (i = 0; i < fdleak_table[wpid].list[pid_index][probe_id].diff_cnt; i++) {
        if (fdleak_compare_stack(fdleak_table[wpid].list[pid_index][probe_id].items[i].stack, (unsigned long long *)stack->entries))
        {
            fdleak_table[wpid].list[pid_index][probe_id].hit_cnt[i]++;
            break;
        }
    }

    if (i == fdleak_table[wpid].list[pid_index][probe_id].diff_cnt) {
        fdleak_table[wpid].list[pid_index][probe_id].hit_cnt[i] = 1;
        memcpy(fdleak_table[wpid].list[pid_index][probe_id].items[i].stack, stack->entries, sizeof(stack_entries));
        fdleak_table[wpid].list[pid_index][probe_id].diff_cnt++;
    }

    return 0;
}

int fdleak_report(fdleak_wp_id wpid, int probe_id)
{
    int ret = -1;
    int idx_pid = -1;

    if (BETA_USER != usertype) {
        if (!usertype) {
            usertype = get_logusertype_flag(); //it is not initialized, read again
            if (BETA_USER != usertype) {
                return -1;
            }
        } else {
            return -1; //it is commercial or oversea user
        }
    }
    mutex_lock(&mutex);
    idx_pid = fdleak_find_pid(current->tgid, wpid);

    if (idx_pid < 0) {
        mutex_unlock(&mutex);
        return -1;
    }
    if (probe_id >= fdleak_table[wpid].probe_cnt)
    {
        mutex_unlock(&mutex);
        return -1;
    }

    fdleak_table[wpid].is_32bit[idx_pid] = is_compat_task();
    if (fdleak_table[wpid].list[idx_pid] && current->mm) {
        struct stack_trace trace;

        memset(stack_entries, 0, sizeof(stack_entries));
        trace.nr_entries    = 0;
        trace.max_entries    = FDLEAK_MAX_STACK_TRACE_DEPTH;
        trace.entries        = (unsigned long *)stack_entries;
        trace.skip            = 0;

        if (fdleak_table[wpid].is_32bit[idx_pid])
            save_stack_trace32(current, &trace);
        else
            save_stack_trace64(current, &trace);

        ret = fdleak_insert_stack(wpid, idx_pid, probe_id, &trace);
        //fdleak_info("the leak watchpoint %s for pid=%d report stack, ret=%d\n", fdleak_table[wpid].name, current->tgid, ret);
    }
    mutex_unlock(&mutex);

    return ret;
}
EXPORT_SYMBOL(fdleak_report);

/**************the following is for debug only ********************/
#ifdef FDLEAK_DEBUG

#define SIZE_4K (4*1024)
#define check_break(cond) { if (cond) break; }
static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *out)
{
    fdleak_wp_id id = FDLEAK_WP_MIN;
    int probe_id = 0;
    int pid_index = 0;
    int j = 0;
    int k = 0;
    unsigned long len = 0;
    char *buf = NULL;
    buf = vmalloc(SIZE_4K);

    memset(buf, 0, SIZE_4K);
    for (id = FDLEAK_WP_MIN; id < FDLEAK_WP_NUM_MAX; id++)
    {
        len += snprintf(buf + len, SIZE_4K - len, "[%s] probe_cnt=%d pid_cnt=%d\n", fdleak_table[id].name, fdleak_table[id].probe_cnt, fdleak_table[id].pid_cnt);
        check_break(len >= SIZE_4K - 1);
        for (pid_index = 0; pid_index < fdleak_table[id].pid_cnt; pid_index++)
        {
            len += snprintf(buf + len, SIZE_4K - len , "  [pid=%d, is_32bit=%d]\n", fdleak_table[id].pids[pid_index], fdleak_table[id].is_32bit[pid_index]);
            check_break(len >= SIZE_4K - 1);
            for (probe_id = 0; probe_id < fdleak_table[id].probe_cnt; probe_id++)
            {
                len += snprintf(buf + len, SIZE_4K - len , "    [probe_id=%d, diff_cnt=%d]\n", probe_id, fdleak_table[id].list[pid_index][probe_id].diff_cnt);
                check_break(len >= SIZE_4K - 1);
                for (j = 0; j <  fdleak_table[id].list[pid_index][probe_id].diff_cnt; j++) {
                    len += snprintf(buf + len, SIZE_4K - len , "      [stack %2d : %d hits]\n", j, fdleak_table[id].list[pid_index][probe_id].hit_cnt[j]);
                    check_break(len >= SIZE_4K - 1);
                    for(k = 0; k < FDLEAK_MAX_STACK_TRACE_DEPTH && (fdleak_table[id].list[pid_index][probe_id].items[j].stack[k] != ULONG_MAX)
                                && (fdleak_table[id].list[pid_index][probe_id].items[j].stack[k]); k++) {
                        len += snprintf(buf + len, SIZE_4K - len , "        #%2d: %#llX\n", k, fdleak_table[id].list[pid_index][probe_id].items[j].stack[k]);
                        check_break(len >= SIZE_4K - 1);
                    }
                }
            }
        }
    }
    strncpy(out, buf, SIZE_4K);
    vfree(buf);

    return len;
}

static struct kobj_attribute info_attr = {
    .attr = {
         .name = "info",
         .mode = 0640,
         },
    .show = info_show,
    .store = NULL,
};
static struct attribute *attrs[] = {
    &info_attr.attr,
    NULL
};
static struct attribute_group fdleak_attr_group = {
    .attrs = attrs,
};
struct kobject *fdleak_kobj = NULL;
static int create_sysfs_fdleak(void)
{
    int ret = 0;

    while (kernel_kobj == NULL)
        msleep(1000);

    fdleak_kobj = kobject_create_and_add("fdleak", kernel_kobj);
    if (!fdleak_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(fdleak_kobj, &fdleak_attr_group);
    if (ret)
        kobject_put(fdleak_kobj);

    return ret;
}
#endif

static int __init fdleak_init(void)
{
    usertype = get_logusertype_flag();
#ifdef FDLEAK_DEBUG
    create_sysfs_fdleak();
#endif

    return 0;
}

module_init(fdleak_init);
