
#ifndef __RDR_HISI_AP_HOOK_H__
#define __RDR_HISI_AP_HOOK_H__

#include <linux/thread_info.h>
#include <linux/hisi/rdr_types.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 76))
#include <linux/sched.h>
#endif

#define MEM_ALLOC 1
#define MEM_FREE  0

typedef u64 (*arch_timer_func_ptr)(void);

extern arch_timer_func_ptr g_arch_timer_func_ptr;

typedef enum{
    HK_IRQ = 0,
    HK_TASK,
    HK_CPUIDLE,
    HK_WORKER,
    HK_MEM_ALLOCATOR,
    HK_ION_ALLOCATOR,
    HK_TIME,
    HK_PERCPU_TAG, /*percpu的轨迹加在HK_PERCPU_TAG以上*/
    HK_CPU_ONOFF = HK_PERCPU_TAG,
    HK_SYSCALL,
    HK_HUNGTASK,
    HK_TASKLET,
    HK_DIAGINFO,
    HK_MAX
}
hook_type;

typedef struct
{
    u64 clock;
    u64 jiff;
    u32 irq;
    u8 dir;
}
irq_info;

typedef struct
{
    u64 clock;
    u64 stack;
    u32 pid;
    char comm[TASK_COMM_LEN];
}
task_info;

typedef struct
{
    u64 clock;
    u8 dir;
}
cpuidle_info;

typedef struct
{
    u64 clock;
    u8 cpu;
    u8 on;
}
cpu_onoff_info;

typedef struct
{
    u64 clock;
    u32 syscall;
    u8 cpu;
    u8 dir;
}
syscall_info;

typedef struct
{
    u64 clock;
    u32 timeout;
    u32 pid;
    char comm[TASK_COMM_LEN];
}
hung_task_info;

typedef struct
{
    u64 clock;
    u64 action;
    u8 cpu;
    u8 dir;
}
tasklet_info;

typedef struct
{
    u64 clock;
    u64 action;
    u8 dir;
}
worker_info;

typedef struct
{
    u64 clock;
    u32 pid;
    char comm[TASK_COMM_LEN];
    u64 caller;
    u8 operation;
    u64 va_addr;
    u64 phy_addr;
    u64 size;
}
mem_allocator_info;

typedef struct
{
    u64 clock;
    u32 pid;
    char comm[TASK_COMM_LEN];
    u8 operation;
    u64 va_addr;
    u64 phy_addr;
    u32 size;
}
ion_allocator_info;

typedef struct
{
    u64 clock;
    u64 action;
    u8 dir;
}
time_info;

typedef struct
{
    unsigned char * buffer_addr;
    unsigned char * percpu_addr[NR_CPUS];
    unsigned int    percpu_length[NR_CPUS];
    unsigned int    buffer_size;
}
percpu_buffer_info;

int register_arch_timer_func_ptr(arch_timer_func_ptr func);
int irq_buffer_init(percpu_buffer_info* buffer_info, unsigned char* addr, unsigned int size);
int task_buffer_init(percpu_buffer_info* buffer_info, unsigned char* addr, unsigned int size);
int cpuidle_buffer_init(percpu_buffer_info* buffer_info, unsigned char* addr, unsigned int size);
int cpu_onoff_buffer_init(unsigned char **addr, unsigned int size);
int syscall_buffer_init(unsigned char **addr, unsigned int size);
int hung_task_buffer_init(unsigned char **addr, unsigned int size);
int worker_buffer_init(percpu_buffer_info* buffer_info, unsigned char* addr, unsigned int size);
int tasklet_buffer_init(unsigned char **addr, unsigned int size);
int diaginfo_buffer_init(unsigned char **addr, unsigned int size);
int mem_alloc_buffer_init(percpu_buffer_info *buffer_info, unsigned char* addr, unsigned int size);
int ion_alloc_buffer_init(percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int percpu_buffer_init(percpu_buffer_info *buffer_info, u32 ratio[][8],
		       u32 cpu_num, u32 fieldcnt, const char *keys, u32 gap);
int time_buffer_init(percpu_buffer_info *buffer_info, unsigned char *addr,
		       unsigned int size);

#ifdef CONFIG_HISI_BB
void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec);
void task_switch_hook(const void *pre_task, void *next_task);
void cpuidle_stat_hook (u32 dir);
void cpu_on_off_hook(u32 cpu, u32 on);
void syscall_hook(u32 syscall_num, u32 dir);
void hung_task_hook(void *tsk, u32 timeout);
void tasklet_hook(u64 address, u32 dir);
void worker_hook(u64 address, u32 dir);
#ifdef CONFIG_HISI_MEM_TRACE
void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order);
void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size);
void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size);
void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle);
void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size);
#else
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order){}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size){}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size){}
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle){}
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size){}
#endif
#else
static inline void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec){}
static inline void task_switch_hook(const void *pre_task, void *next_task){}
static inline void cpuidle_stat_hook (u32 dir){}
static inline void cpu_on_off_hook(u32 cpu, u32 on){}
static inline void syscall_hook(u32 syscall_num, u32 dir){}
static inline void hung_task_hook(void *tsk, u32 timeout){}
static inline u32 get_current_last_irq(unsigned int cpu){}
static inline void tasklet_hook(u64 address, u32 dir){}
static inline void worker_hook(u64 address, u32 dir){}
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order){}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size){}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size){}
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle){}
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size){}
#endif

#ifdef CONFIG_HISI_TIME_HOOK
void time_hook(u64 address, u32 dir);
#else
static inline void time_hook(u64 address, u32 dir){}
#endif

void hisi_ap_defopen_hook_install(void);
int hisi_ap_hook_install(hook_type hk);
int hisi_ap_hook_uninstall(hook_type hk);
 
#endif
