ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif


obj-$(CONFIG_MEM_BALONG) 	+= mem_balong.o
obj-$(CONFIG_ENABLE_TEST_CODE) += mem_balong_test.o

subdir-ccflags-y := -Werror
