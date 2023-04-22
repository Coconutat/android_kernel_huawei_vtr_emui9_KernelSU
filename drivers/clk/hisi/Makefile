ifeq ($(CONFIG_HISI_CLK_KIRIN),y)
obj-$(CONFIG_HISI_CLK)                  += clk-kirin-common.o
obj-$(CONFIG_HISI_CLK)                  += hisi-clkgate.o
obj-$(CONFIG_HISI_CLK)                  += hisi-kirin-ppll.o
obj-$(CONFIG_HISI_CLK_DEBUG)		+= clk_unit_test.o
else
ifeq ($(CONFIG_HISI_CLK_HI3660),y)
obj-$(CONFIG_HISI_CLK)			+= clk-kirin.o
else
obj-$(CONFIG_HISI_CLK)			+= clk-hi3xxx.o
endif
endif
obj-$(CONFIG_HISI_CLK_MAILBOX_SUPPORT)	+= hisi-clk-mailbox.o

ifeq ($(CONFIG_HISI_CLK_KIRIN),y)
obj-$(CONFIG_HISI_CLK)                  += clk_dvfs.o
else
ifeq ($(CONFIG_HISI_CLK_HI3660),y)
obj-$(CONFIG_HISI_CLK)			+= clk_peri_dvfs.o
endif
endif
obj-$(CONFIG_HISI_CLK_PM_MONITOR)	+= hisi_clk_pm_monitor.o

ifeq ($(CONFIG_HISI_CLK_KIRIN),y)
obj-$(CONFIG_HISI_PERIDVFS)  += dvfs/
else
obj-$(CONFIG_HISI_PERIDVFS)  += peri_dvfs/
endif
