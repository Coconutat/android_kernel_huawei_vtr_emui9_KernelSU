#ifndef KASLR_H
#define KASLR_H

#ifndef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE
#define kstack_randomize_init()
#else
extern unsigned int kstack_offset;
extern void kstack_randomize_init(void);
#endif

#ifndef CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE_STRONG
#define kti_randomize_init()
#define set_init_thread_info(x)
#else
extern unsigned long kti_offset;
extern void kti_randomize_init(void);
extern void set_init_thread_info(unsigned long addr);
#endif

#ifndef CONFIG_HUAWEI_KERNEL_STACK_NX
#define set_init_stack_nx(x)
#define set_task_stack_nx(x)
#else
extern void __init set_init_stack_nx(struct thread_info *ti);
extern void set_task_stack_nx(struct thread_info *ti);
#endif

#ifndef CONFIG_HUAWEI_KERNEL_STACK_NX
#define set_init_stack_nx(x)
#else
extern void __init set_init_stack_nx(struct thread_info *ti);
#endif

#ifndef CONFIG_HUAWEI_KERNEL_MODULE_RANDOMIZE
#define get_module_load_offset()
#else
extern unsigned long get_module_load_offset(void);
#endif

#endif
