ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

obj-$(CONFIG_SEC_CALL) += sec_call.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver

subdir-ccflags-y := -Werror
