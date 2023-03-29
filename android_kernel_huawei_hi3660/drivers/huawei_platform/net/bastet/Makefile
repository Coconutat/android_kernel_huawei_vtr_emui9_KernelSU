#
# Makefile for the Bastet.
#
EXTRA_CFLAGS += -Inet/netfilter

obj-$(CONFIG_HUAWEI_BASTET) += \
	bastet_sync_seq.o \
	bastet_traffic_flow.o \
	bastet_dev.o \
	bastet_utils.o \
	bastet_channel.o \
	bastet_reconn.o \
	bastet_partner.o \
	bastet_fastgrab.o \
	bastet_filter.o \
	bastet_hbm.o \

obj-$(CONFIG_HUAWEI_BASTET_COMM_NEW) += bastet_comm.o

ifneq ($(OBB_SEPARATE),true)
ifneq ($(CFG_HISI_MINI_AP),true)
EXTRA_CFLAGS += -DCONFIG_HUAWEI_BASTET_COMM
endif
endif
