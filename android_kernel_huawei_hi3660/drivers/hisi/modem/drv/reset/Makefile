ifeq ($(strip $(llt_gcov)),y)
GCOV_PROFILE := y
endif

obj-$(CONFIG_BALONG_MODEM_RESET)        += reset_balong.o
obj-$(CONFIG_BALONG_MODEM_RESET_TEST)   += reset_balong_test.o
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver \
                -Idrivers/tzdriver

subdir-ccflags-y := -Werror
