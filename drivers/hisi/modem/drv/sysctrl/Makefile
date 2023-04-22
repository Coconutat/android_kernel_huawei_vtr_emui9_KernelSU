ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

obj-y += dt_sysctrl.o
obj-$(CONFIG_SYSCTRL_TEST) += sc_interface_test.o

subdir-ccflags-y := -Werror
