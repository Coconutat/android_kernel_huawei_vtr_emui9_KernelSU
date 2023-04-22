#gcov
ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
GCOV_PROFILE_icc_debug.o := n
endif

EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
obj-$(CONFIG_ICC_BALONG)       += icc_core.o  icc_linux.o  icc_debug.o
obj-$(CONFIG_CA_ICC)           += ca_icc.o
obj-$(CONFIG_ENABLE_TEST_CODE) += icc_test.o

subdir-ccflags-y := -Werror
