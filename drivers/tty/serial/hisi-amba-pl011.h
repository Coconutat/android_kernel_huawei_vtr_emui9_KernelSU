/*
 * Hisilicon Amba-pl011.h.
 *
 * Copyright (c) 2016 HUAWEI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef HISI_AMBA_PL011_H
#define HISI_AMBA_PL011_H

#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/of_address.h>
#include <linux/ioport.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <linux/clk.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>

#define UART_NR			14

enum {
	REG_DR,
	REG_ST_DMAWM,
	REG_ST_TIMEOUT,
	REG_FR,
	REG_LCRH_RX,
	REG_LCRH_TX,
	REG_IBRD,
	REG_FBRD,
	REG_CR,
	REG_IFLS,
	REG_IMSC,
	REG_RIS,
	REG_MIS,
	REG_ICR,
	REG_DMACR,
	REG_ST_XFCR,
	REG_ST_XON1,
	REG_ST_XON2,
	REG_ST_XOFF1,
	REG_ST_XOFF2,
	REG_ST_ITCR,
	REG_ST_ITIP,
	REG_ST_ABCR,
	REG_ST_ABIMSC,

	/* The size of the array - must be last */
	REG_ARRAY_SIZE,
};

static u16 pl011_std_offsets[REG_ARRAY_SIZE] = {
	[REG_DR] = UART01x_DR,
	[REG_FR] = UART01x_FR,
	[REG_LCRH_RX] = UART011_LCRH,
	[REG_LCRH_TX] = UART011_LCRH,
	[REG_IBRD] = UART011_IBRD,
	[REG_FBRD] = UART011_FBRD,
	[REG_CR] = UART011_CR,
	[REG_IFLS] = UART011_IFLS,
	[REG_IMSC] = UART011_IMSC,
	[REG_RIS] = UART011_RIS,
	[REG_MIS] = UART011_MIS,
	[REG_ICR] = UART011_ICR,
	[REG_DMACR] = UART011_DMACR,
};

struct vendor_data {
	const u16		*reg_offset;
	unsigned int		ifls;
	unsigned int		fr_busy;
	unsigned int		fr_dsr;
	unsigned int		fr_cts;
	unsigned int		fr_ri;
	bool			access_32b;
	bool			oversampling;
	bool			dma_threshold;
	bool			cts_event_workaround;
	bool			always_enabled;
	bool			fixed_options;
	unsigned int (*get_fifosize)(struct amba_device *dev);
};

unsigned int get_fifosize_arm(struct amba_device *dev);

static struct vendor_data vendor_arm = {
	.reg_offset		= pl011_std_offsets,
	.ifls			= UART011_IFLS_RX2_8|UART011_IFLS_TX4_8 | (4<<6),
	.fr_busy		= UART01x_FR_BUSY,
	.fr_dsr			= UART01x_FR_DSR,
	.fr_cts			= UART01x_FR_CTS,
	.fr_ri			= UART011_FR_RI,
	.oversampling		= false,
	.dma_threshold		= false,
	.cts_event_workaround	= false,
	.always_enabled		= false,
	.fixed_options		= false,
	.get_fifosize		= get_fifosize_arm,
};

struct pl011_sgbuf {
	struct scatterlist sg;
	char *buf;
};

struct pl011_dmarx_data {
	struct dma_chan		*chan;
	struct completion	complete;
	bool			use_buf_b;
	struct pl011_sgbuf	sgbuf_a;
	struct pl011_sgbuf	sgbuf_b;
	dma_cookie_t		cookie;
	bool			running;
	struct timer_list	timer;
	unsigned int last_residue;
	unsigned long last_jiffies;
	bool auto_poll_rate;
	unsigned int poll_rate;
	unsigned int poll_timeout;
};

struct pl011_dmatx_data {
	struct dma_chan		*chan;
	struct scatterlist	sg;
	char			*buf;
	bool			queued;
};

#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
#define PL011_UART_TX_WORK
static int console_uart_name_is_ttyAMA;
extern int get_console_index(void);
extern int get_console_name(char *name, int name_buf_len);
#define PL011_TX_LOCAL_BUF_SIZE	96
#define UART_TX_BUF_SIZE_DEF	0x20000	/* 256k */
#define UART_TX_CPUON_DEF		3		/* default cpu id */
#define UART_TX_CPUON_NOTSET	(-1)
struct uart_tx_unit {
	unsigned int			tx_valid; /* enable or not */
	int						tx_cpuid;   /* -1:not set */
	int						max_cpus;
	struct kfifo			tx_fifo;
	spinlock_t				tx_lock_in;
	unsigned long			tx_got;
	unsigned long			tx_queued;
	unsigned long			tx_out;
	unsigned long			tx_sent;
	unsigned long			tx_buf_times;
	unsigned long			tx_buf_max; /* fifo required size max */
	unsigned long			tx_buf_over_cnt;
	unsigned long			tx_buf_over_size;
	unsigned long			tx_uart_fifo_full;
	unsigned long			tx_uart_fifo_full_lost;
	unsigned long			tx_uart_tasklet_run;
	wait_queue_head_t		waitqueue;
	atomic_t			 	tx_sig;
	struct task_struct		*thread;
};
#endif

 struct uart_amba_port {
	struct uart_port	port;
	const u16		*reg_offset;
	struct clk		*clk;
	/* Two optional pin states - default & sleep */
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_idle;
	const struct vendor_data *vendor;
	unsigned int		dmacr;		/* dma control reg */
	unsigned int		im;		/* interrupt mask */
	unsigned int		old_status;
	unsigned int		fifosize;	/* vendor-specific */
	unsigned int		old_cr;		/* state during shutdown */
	bool			autorts;
	unsigned int		fixed_baud;	/* vendor-set fixed baud rate */
	unsigned int		tx_irq_seen;	/* 0=none, 1=1, 2=2 or more */
	char			type[12];
#ifdef CONFIG_DMA_ENGINE
	/* DMA stuff */
	bool			using_tx_dma;
	bool			using_rx_dma;
	struct pl011_dmarx_data dmarx;
	struct pl011_dmatx_data	dmatx;
	bool			dma_probed;
#endif
	int			pm_op_in_progress;
#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
	bool reset;
	void __iomem *reset_reg_base;
	u32 reset_enable_off;
	u32 reset_disable_off;
	u32 reset_status_off;
	u32 reset_bit;
	u32 console_fbrd;
	u32 console_ibrd;
	u32 console_lcrh_rx;
	u32 console_lcrh_tx;
	struct uart_tx_unit tx_unit;
#endif
#ifdef CONFIG_HISI_AMBA_PL011
	bool			rx_dma_disabled;
#endif
};
static struct uart_amba_port *amba_ports[UART_NR];

unsigned int pl011_reg_to_offset(const struct uart_amba_port *uap, unsigned int reg);
unsigned int pl011_read(const struct uart_amba_port *uap, unsigned int reg);
void pl011_write(unsigned int val, const struct uart_amba_port *uap, unsigned int reg);
bool pl011_split_lcrh(const struct uart_amba_port *uap);

void uart_chip_reset_endisable(struct uart_amba_port *dev, unsigned int enable);
void hisi_pl011_console_write(struct console *co, const char *s, unsigned int count);
#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
int pl011_tx_work_init(struct uart_amba_port *uap, unsigned int aurt_tx_buf_size, int cpuid);
int pl011_tx_work_uninit(struct uart_amba_port *uap);
void pl011_console_write_tx(struct console *co, const char *s, unsigned int count);
void pl011_console_putchar(struct uart_port *port, int ch);
#endif
void hisi_pl011_probe_console_enable(struct amba_device *dev, struct uart_amba_port *uap, char* amba_console_name);
int hisi_pl011_probe_reset_func_enable(struct amba_device *dev, struct uart_amba_port *uap);
int hisi_pl011_probe_get_clk_freq(struct amba_device *dev, struct uart_amba_port *uap, int i);
void pl011_pm(struct uart_port *port, unsigned int state, unsigned int oldstate);
#ifdef CONFIG_PM_SLEEP
int hisi_pl011_suspend(struct device *dev);
int hisi_pl011_resume(struct device *dev);
#endif
int hisi_pl011_pinctrl(struct amba_device *dev, struct uart_amba_port *uap);
void hisi_pl011_disable_ms(struct uart_port *port);
#endif
