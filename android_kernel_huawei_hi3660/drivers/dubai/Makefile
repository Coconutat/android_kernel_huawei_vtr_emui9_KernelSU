#
# Makefile for DUBAI.
#

ifneq ($(findstring $(TARGET_BOARD_PLATFORM), kirin980 kirin970 kirin710),)
CFLAGS_dubai_stats.o += -DSENSORHUB_DUBAI_INTERFACE
endif

obj-$(CONFIG_HUAWEI_DUBAI) += \
	dubai.o \
	dubai_gpu.o \
	dubai_stats.o \
	dubai_process_cputime.o \
	buffered_log_sender.o
