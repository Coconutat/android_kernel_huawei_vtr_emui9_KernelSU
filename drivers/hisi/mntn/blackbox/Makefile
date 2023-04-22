ccflags-y += -Wall -Werror
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
EXTRA_CFLAGS += -Wno-error=date-time

obj-$(CONFIG_HISI_BB) += rdr_core.o
obj-$(CONFIG_HISI_BB) += rdr_common.o
obj-$(CONFIG_HISI_BB) += rdr_exception_core.o
obj-$(CONFIG_HISI_BB) += rdr_module_core.o
obj-$(CONFIG_HISI_BB) += rdr_dump_core.o
obj-$(CONFIG_HISI_BB) += rdr_logmonitor_core.o
obj-$(CONFIG_HISI_BB) += rdr_field_core.o
#obj-$(CONFIG_HISI_BB) += rdr_bootcheck.o

obj-$(CONFIG_HISI_BB_DEBUG) += rdr_debug.o
obj-$(CONFIG_HISI_BB_DEBUG) += rdr_demo.o
obj-$(CONFIG_HISI_BB) += platform_ap/
obj-$(CONFIG_HISI_BB) += diaginfo/
obj-$(CONFIG_HISI_LPMCU_BB) += platform_lpm3/

obj-$(CONFIG_HISI_HIFI_BB) += platform_hifi/

obj-$(CONFIG_HISI_DFX_CORE) += rdr_dfx_core.o
obj-$(CONFIG_HISI_BB) += rdr_cleartext.o
obj-$(CONFIG_HISI_BB) += rdr_exception_trace.o

