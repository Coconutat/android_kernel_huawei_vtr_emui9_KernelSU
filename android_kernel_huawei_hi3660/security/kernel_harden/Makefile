# Makefile for the Huawei Kernel Harden.
#

obj-$(CONFIG_HUAWEI_KERNEL_STACK_NX) += kernel_harden/hw_stack_nx.o
obj-$(CONFIG_HUAWEI_KERNEL_MODULE_RANDOMIZE) += kernel_harden/hw_kaslr_module.o
obj-$(CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE) += kernel_harden/hw_kaslr_stack.o
obj-$(CONFIG_HUAWEI_KERNEL_STACK_RANDOMIZE_STRONG) += kernel_harden/hw_kaslr_stack_strong.o
obj-$(CONFIG_HUAWEI_HIDESYMS) += kernel_harden/hw_litehide_symbols.o
obj-$(CONFIG_HUAWEI_KASLR_DEBUGFS) += kernel_harden/hw_kaslr_debugfs.o
obj-$(CONFIG_GCC_PLUGIN_RANDSTRUCT_DEBUGFS) += kernel_harden/hw_struct_rand_debugfs.o
obj-$(CONFIG_HUAWEI_FREELIST_RANDOM_DEBUGFS) += kernel_harden/hw_freelist_random_debugfs.o
obj-$(CONFIG_HUAWEI_DOUBLE_FREE_CHECK_DEBUGFS) += kernel_harden/hw_double_free_debugfs.o
obj-$(CONFIG_HUAWEI_HARDEN_DOUBLE_FREE_CHECK_DEBUGFS) += kernel_harden/hw_harden_double_free_debugfs.o
obj-y += kernel_harden/hw_ptrace_log.o

