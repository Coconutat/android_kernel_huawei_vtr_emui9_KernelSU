/*
 * Synopsys DesignWare Multimedia Card Interface driver
 *  (Based on NXP driver for lpc 31xx)
 *
 * Copyright (C) 2009 NXP Semiconductors
 * Copyright (C) 2009, 2010 Imagination Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wswitch"

#include <linux/blkdev.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/bitops.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/mmc/slot-gpio.h>

#include "dw_mmc.h"

#ifdef CONFIG_SD_TIMEOUT_RESET
#define VOLT_HOLD_CLK_08V 0x1
#define VOLT_TO_1S 0x1
#endif

#define DW_MCI_SEND_STATUS	1
#define DW_MCI_RECV_STATUS	2
#define DW_MCI_DMA_THRESHOLD	4
#define IDMAC_INT_CLR		(SDMMC_IDMAC_INT_AI | SDMMC_IDMAC_INT_NI | \
				 SDMMC_IDMAC_INT_CES | SDMMC_IDMAC_INT_DU | \
				 SDMMC_IDMAC_INT_FBE | SDMMC_IDMAC_INT_RI | \
				 SDMMC_IDMAC_INT_TI)

#define DESC_RING_BUF_SZ	PAGE_SIZE

struct idmac_desc {
	u32		des0;	/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_OWN	BIT(31)

	u32		des1;	/* Buffer sizes */
#define IDMAC_SET_BUFFER1_SIZE(d, s) \
	((d)->des1 = ((d)->des1 & 0x03ffe000) | ((s) & 0x1fff))

	u32		des2;	/* buffer 1 physical address */

	u32		des3;	/* buffer 2 physical address */
};

/*64bit DMA descriptor struct*/
struct idmac_desc_64bit {
	u32		des0;	/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_OWN	BIT(31)

	u32		des1;	/* reserved */
	u32		des2;	/* Buffer sizes */
#define IDMAC_64ADDR_SET_BUFFER1_SIZE(d, s) \
	((d)->des2 = ((d)->des2 & cpu_to_le32(0x03ffe000)) | \
	 ((cpu_to_le32(s)) & cpu_to_le32(0x1fff)))

	u32		des3;	/* reserved */

	u32		des4;	/* buffer 1 low 32bit address point */

	u32		des5;    /* buffer 1 upper 32bit  address point */

	u32		des6;	/* buffer 2 low 32bit address point */

	u32		des7;    /* buffer 2 upper 32bit  address point */
};

/* Each descriptor can transfer up to 4KB of data in chained mode */
#define DW_MCI_DESC_DATA_LENGTH	0x1000

static bool dw_mci_ctrl_reset(struct dw_mci *host, u32 reset);
static int dw_mci_card_busy(struct mmc_host *mmc);
//static int dw_mci_get_cd(struct mmc_host *mmc);
struct mmc_host *g_mmc_for_mmctrace[4] = {NULL};

extern void gic_reg_dump(void);


extern void dw_mci_reg_dump(struct dw_mci *host);
#ifdef CONFIG_MMC_HISI_TRACE
extern void dw_mci_reg_dump_fortrace(struct mmc_host *mmc);
#endif
extern void dw_mci_set_timeout(struct dw_mci *host);
extern bool dw_mci_stop_abort_cmd(struct mmc_command *cmd);
extern bool dw_mci_wait_reset(struct device *dev, struct dw_mci *host,unsigned int reset_val);
extern void dw_mci_ciu_reset(struct device *dev, struct dw_mci *host);
extern bool dw_mci_fifo_reset(struct device *dev, struct dw_mci *host);
extern u32 dw_mci_prep_stop(struct dw_mci *host, struct mmc_command *cmd);
extern bool dw_mci_wait_data_busy(struct dw_mci *host, struct mmc_request *mrq);
extern void dw_mci_set_cd(struct dw_mci *host);
extern int dw_mci_start_signal_voltage_switch(struct mmc_host *mmc,struct mmc_ios *ios);
extern void dw_mci_slowdown_clk(struct mmc_host *mmc, int timing);
extern void dw_mci_timeout_timer(unsigned long data);
extern void dw_mci_work_routine_card(struct work_struct *work);
extern bool mci_wait_reset(struct device *dev, struct dw_mci *host);
static int mci_send_cmd(struct dw_mci_slot *slot, u32 cmd, u32 arg);


static bool dw_mci_ctrl_reset(struct dw_mci *host, u32 reset)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);
	u32 ctrl;

	ctrl = mci_readl(host, CTRL);
	ctrl |= reset;
	mci_writel(host, CTRL, ctrl);

	/* wait till resets clear */
	do {
		ctrl = mci_readl(host, CTRL);
		if (!(ctrl & reset))
			return true;
	} while (time_before(jiffies, timeout));

	dev_err(host->dev,
		"Timeout resetting block (ctrl reset %#x)\n",
		ctrl & reset);

	return false;
}


#if defined(CONFIG_DEBUG_FS)
static int dw_mci_req_show(struct seq_file *s, void *v)
{
	struct dw_mci_slot *slot = s->private;
	struct mmc_request *mrq;
	struct mmc_command *cmd;
	struct mmc_command *stop;
	struct mmc_data	*data;

	/* Make sure we get a consistent snapshot */
	spin_lock_bh(&slot->host->lock);
	mrq = slot->mrq;

	if (mrq) {
		cmd = mrq->cmd;
		data = mrq->data;
		stop = mrq->stop;

		if (cmd)
			seq_printf(s,
				   "CMD%u(0x%x) flg %x rsp %x %x %x %x err %d\n",
				   cmd->opcode, cmd->arg, cmd->flags,
				   cmd->resp[0], cmd->resp[1], cmd->resp[2],
				   cmd->resp[2], cmd->error);
		if (data)
			seq_printf(s, "DATA %u / %u * %u flg %x err %d\n",
				   data->bytes_xfered, data->blocks,
				   data->blksz, data->flags, data->error);
		if (stop)
			seq_printf(s,
				   "CMD%u(0x%x) flg %x rsp %x %x %x %x err %d\n",
				   stop->opcode, stop->arg, stop->flags,
				   stop->resp[0], stop->resp[1], stop->resp[2],
				   stop->resp[2], stop->error);
	}

	spin_unlock_bh(&slot->host->lock);

	return 0;
}

static int dw_mci_req_open(struct inode *inode, struct file *file)
{
	return single_open(file, dw_mci_req_show, inode->i_private);
}

static const struct file_operations dw_mci_req_fops = {
	.owner		= THIS_MODULE,
	.open		= dw_mci_req_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int dw_mci_regs_show(struct seq_file *s, void *v)
{
	seq_printf(s, "STATUS:\t0x%08x\n", SDMMC_STATUS);
	seq_printf(s, "RINTSTS:\t0x%08x\n", SDMMC_RINTSTS);
	seq_printf(s, "CMD:\t0x%08x\n", SDMMC_CMD);
	seq_printf(s, "CTRL:\t0x%08x\n", SDMMC_CTRL);
	seq_printf(s, "INTMASK:\t0x%08x\n", SDMMC_INTMASK);
	seq_printf(s, "CLKENA:\t0x%08x\n", SDMMC_CLKENA);

	return 0;
}

static int dw_mci_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dw_mci_regs_show, inode->i_private);
}

static const struct file_operations dw_mci_regs_fops = {
	.owner		= THIS_MODULE,
	.open		= dw_mci_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int mmc_caps_opt_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = host->caps;

	return 0;
}

static int mmc_caps_opt_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	host->caps = val;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(mmc_caps_fops, mmc_caps_opt_get, mmc_caps_opt_set,
	"%llu\n");

static int mmc_caps2_opt_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = host->caps2;

	return 0;
}

static int mmc_caps2_opt_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	host->caps2 = val;

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(mmc_caps2_fops, mmc_caps2_opt_get, mmc_caps2_opt_set,
	"%llu\n");

static void dw_mci_init_debugfs(struct dw_mci_slot *slot)
{
	struct mmc_host	*mmc = slot->mmc;
	struct dw_mci *host = slot->host;
	struct dentry *root;
	struct dentry *node;

	root = mmc->debugfs_root;
	if (!root)
		return;

	node = debugfs_create_file("regs", S_IRUSR, root, host,
				   &dw_mci_regs_fops);
	if (!node)
		goto err;

	node = debugfs_create_file("req", S_IRUSR, root, slot,
				   &dw_mci_req_fops);
	if (!node)
		goto err;

	node = debugfs_create_u32("state", S_IRUSR, root, (u32 *)&host->state);
	if (!node)
		goto err;

	node = debugfs_create_x32("pending_events", S_IRUSR, root,
				  (u32 *)&host->pending_events);
	if (!node)
		goto err;

	node = debugfs_create_x32("completed_events", S_IRUSR, root,
				  (u32 *)&host->completed_events);
	if (!node)
		goto err;

	if (!debugfs_create_file("caps", S_IRUSR | S_IWUSR, root, mmc,
			&mmc_caps_fops))
		goto err;

	if (!debugfs_create_file("caps2", S_IRUSR | S_IWUSR, root, mmc,
			&mmc_caps2_fops))
		goto err;

	return;

err:
	dev_err(&mmc->class_dev, "failed to initialize debugfs for slot\n");
}
#endif /* defined(CONFIG_DEBUG_FS) */
static u32 dw_mci_prepare_command(struct mmc_host *mmc, struct mmc_command *cmd)
{
	struct mmc_data	*data;
	struct dw_mci_slot *slot = mmc_priv(mmc);
	u32 cmdr;

	cmd->error = -EINPROGRESS;/*lint !e570*/
	cmdr = cmd->opcode;

	if (cmdr == SD_SWITCH_VOLTAGE)
		cmdr |= SDMMC_CMD_VOLT_SWITCH;
	if (cmdr == MMC_STOP_TRANSMISSION)
		cmdr |= SDMMC_CMD_STOP;
	else
		cmdr |= SDMMC_CMD_PRV_DAT_WAIT;

	if (cmd->flags & MMC_RSP_PRESENT) {
		/* We expect a response, so set this bit */
		cmdr |= SDMMC_CMD_RESP_EXP;
		if (cmd->flags & MMC_RSP_136)
			cmdr |= SDMMC_CMD_RESP_LONG;
	}

	if (cmd->flags & MMC_RSP_CRC)
		cmdr |= SDMMC_CMD_RESP_CRC;

	data = cmd->data;
	if (data) {
		cmdr |= SDMMC_CMD_DAT_EXP;
		if (data->flags & MMC_DATA_WRITE)
			cmdr |= SDMMC_CMD_DAT_WR;
	}

	if (!test_bit(DW_MMC_CARD_NO_USE_HOLD, &slot->flags))
		cmdr |= SDMMC_CMD_USE_HOLD_REG;

	return cmdr;
}

static void dw_mci_start_command(struct dw_mci *host,
				 struct mmc_command *cmd, u32 cmd_flags)
{
	host->cmd = cmd;
	dev_vdbg(host->dev,
		 "start command: ARGR=0x%08x CMDR=0x%08x\n",
		 cmd->arg, cmd_flags);

	if (cmd->opcode == SD_SWITCH_VOLTAGE) {
		int loop_count;
		u32 temp;

		temp = mci_readl(host, INTMASK);
		temp &= ~SDMMC_INT_RESP_ERR;
		mci_writel(host, INTMASK, temp);
		cmd_flags |= SDMMC_CMD_VOLT_SWITCH;

		/* disable clock low power */
		mci_writel(host, CLKENA, (0x1<<0));
		mci_writel(host, CMD , SDMMC_CMD_ONLY_CLK | SDMMC_CMD_VOLT_SWITCH);
		loop_count = 0x100000;
		do {
			if (!(mci_readl(host, CMD) & SDMMC_CMD_START))
				break;
			loop_count--;
		} while (loop_count);
		if (!loop_count)
			dev_err(host->dev, "disable clk low power failed in volt_switch\n");

	}
	mci_writel(host, CMDARG, cmd->arg);
	wmb(); /* drain writebuffer */

	mci_writel(host, CMD, cmd_flags | SDMMC_CMD_START);
}

static void send_stop_cmd(struct dw_mci *host, struct mmc_data *data)
{

	dw_mci_start_command(host, data->stop, host->stop_cmdr);

}

/* DMA interface functions */
void dw_mci_stop_dma(struct dw_mci *host)
{
	if (host->using_dma) {
		host->dma_ops->stop(host);
		host->dma_ops->cleanup(host);
		host->dma_ops->reset(host);
	} else {
		/* Data transfer was stopped by the interrupt handler */
		set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
	}
}

static int dw_mci_get_dma_dir(struct mmc_data *data)
{
	if (data->flags & MMC_DATA_WRITE)
		return DMA_TO_DEVICE;
	else
		return DMA_FROM_DEVICE;
}


static int mci_send_cmd(struct dw_mci_slot *slot, u32 cmd, u32 arg)
{
	struct dw_mci *host = slot->host;
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	unsigned int cmd_status = 0;

	mci_writel(host, CMDARG, arg);
	wmb();
	mci_writel(host, CMD, SDMMC_CMD_START | cmd);
	while (time_before(jiffies, timeout)) {
		cmd_status = mci_readl(host, CMD);
		if (!(cmd_status & SDMMC_CMD_START))
			return 0;
	}

	if(!dw_mci_wait_reset(host->dev, host, SDMMC_CTRL_RESET))
		return 1;

	timeout = jiffies + msecs_to_jiffies(100);
	mci_writel(host, CMD, SDMMC_CMD_START | cmd);
	while (time_before(jiffies, timeout)) {
		cmd_status = mci_readl(host, CMD);
		if (!(cmd_status & SDMMC_CMD_START))
			return 0;
	}

	dev_err(&slot->mmc->class_dev,
		"Timeout sending command (cmd %#x arg %#x status %#x)\n",
		cmd, arg, cmd_status);
	return 1;
}


static void dw_mci_dma_cleanup(struct dw_mci *host)
{
	struct mmc_data *data = host->data;

	if (data)
		if (!data->host_cookie)
			dma_unmap_sg(host->dev,
				     data->sg,
				     data->sg_len,
				     dw_mci_get_dma_dir(data));
}

static void dw_mci_idmac_stop_dma(struct dw_mci *host)
{
	u32 temp;

	/* Disable and reset the IDMAC interface */
	temp = mci_readl(host, CTRL);
	temp &= ~SDMMC_CTRL_USE_IDMAC;
	mci_writel(host, CTRL, temp);

	/* reset the IDMAC interface */
	dw_mci_wait_reset(host->dev, host, SDMMC_CTRL_DMA_RESET);

	/* Stop the IDMAC running */
	temp = mci_readl(host, BMOD);
	temp &= ~(SDMMC_IDMAC_ENABLE | SDMMC_IDMAC_FB);
	mci_writel(host, BMOD, temp);
}

void dw_mci_idmac_reset(struct dw_mci *host)
{
	u32 temp;

	temp = mci_readl(host, BMOD);
	/* Software reset of DMA */
	temp |= SDMMC_IDMAC_SWRESET;
	mci_writel(host, BMOD, temp);
}

static void dw_mci_dmac_complete_dma(void *arg)
{
	struct dw_mci *host = arg;
	struct mmc_data *data = host->data;

	dev_vdbg(host->dev, "DMA complete\n");
	if ((1 == host->use_dma) &&
	    data && (data->flags & MMC_DATA_READ))
		/* Invalidate cache after read */
		dma_sync_sg_for_cpu(mmc_dev(host->cur_slot->mmc),
				    data->sg,
				    data->sg_len,
				    DMA_FROM_DEVICE);

	if ((host->use_dma == TRANS_MODE_EDMAC) &&
	    data && (data->flags & MMC_DATA_READ))
		/* Invalidate cache after read */
		dma_sync_sg_for_cpu(mmc_dev(host->cur_slot->mmc),
				    data->sg,
				    data->sg_len,
				    DMA_FROM_DEVICE);

	host->dma_ops->cleanup(host);

	/*
	 * If the card was removed, data will be NULL. No point in trying to
	 * send the stop command or waiting for NBUSY in this case.
	 */
	if (data) {
		set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
		tasklet_schedule(&host->tasklet);
	}
}

static int dw_mci_idmac_init(struct dw_mci *host)
{
	if(SDMMC_32_BIT_DMA == host->dma_64bit_address) {
		struct idmac_desc *p;
		unsigned int i;

		/* Number of descriptors in the ring buffer */
		host->ring_size = host->desc_sz * DESC_RING_BUF_SZ / sizeof(struct idmac_desc);

		/* Forward link the descriptor list */
		for (i = 0, p = host->sg_cpu; i < host->ring_size - 1; i++, p++)
		    p->des3 = host->sg_dma + (sizeof(struct idmac_desc) * (i + 1));

		/* Set the last descriptor as the end-of-ring descriptor */
		p->des3 = host->sg_dma;
		p->des0 = IDMAC_DES0_ER;

		mci_writel(host, BMOD, SDMMC_IDMAC_SWRESET);

		/* Mask out interrupts - get Tx & Rx complete only */
#ifdef CONFIG_MMC_DW_IDMAC
		mci_writel(host, IDSTS, IDMAC_INT_CLR);
		mci_writel(host, IDINTEN, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI |
			SDMMC_IDMAC_INT_TI);
#endif
		/* Set the descriptor base address */
		mci_writel(host, DBADDR, host->sg_dma);
		return 0;
	} else {
		struct idmac_desc_64bit *p;
		unsigned int i;

		/* Number of descriptors in the ring buffer */
		host->ring_size = host->desc_sz * DESC_RING_BUF_SZ / sizeof(struct idmac_desc_64bit);

		/* Forward link the descriptor list */

		for (i = 0, p = host->sg_cpu; i < host->ring_size - 1; i++, p++) {
			p->des6 = (host->sg_dma + (sizeof(struct idmac_desc_64bit) * (i + 1)))& 0xffffffff;
			p->des7 = (host->sg_dma + (sizeof(struct idmac_desc_64bit) * (i + 1)))>> 32;

			p->des1 = 0;
			p->des2 = 0;
			p->des3 = 0;
		}
		/* Set the last descriptor as the end-of-ring descriptor */
		p->des6 = host->sg_dma & 0xffffffff;
		p->des7 = (u64)host->sg_dma >> 32;
		p->des0 = IDMAC_DES0_ER;

		mci_writel(host, BMOD, SDMMC_IDMAC_SWRESET);
		/* Mask out interrupts - get Tx & Rx complete only */
		mci_writel(host, IDSTS64, IDMAC_INT_CLR);
		mci_writel(host, IDINTEN64, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI | SDMMC_IDMAC_INT_TI);
		/* Set the descriptor base address */
		mci_writel(host, DBADDRL, host->sg_dma & 0xffffffff);
		mci_writel(host, DBADDRU,(u64)host->sg_dma >> 32);
		return 0;
	}
}
#ifdef CONFIG_HISI_MMC
static void dw_mci_translate_sglist(struct dw_mci *host, struct mmc_data *data,
				    unsigned int sg_len)
{
	unsigned int desc_len;
	if(SDMMC_32_BIT_DMA == host->dma_64bit_address) {
		unsigned int i;
		struct idmac_desc *desc = host->sg_cpu;

		if (!sg_len)
			return;

		for (i = 0; i < sg_len; i++, desc++) {
			unsigned int length = sg_dma_len(&data->sg[i]);
			u32 mem_addr = sg_dma_address(&data->sg[i]);

			/* Set the OWN bit and disable interrupts for this descriptor */
			desc->des0 = IDMAC_DES0_OWN | IDMAC_DES0_DIC | IDMAC_DES0_CH;
			/*优化修改，防止内存不初始化*/
			if(desc->des0 & IDMAC_DES0_CH) {
				desc->des1 = 0;
			}

			/*优化修改，防止内存不初始化*/
			if(desc->des0 & IDMAC_DES0_CH) {
				desc->des1 = 0;
			}

			/* Buffer length */
			IDMAC_SET_BUFFER1_SIZE(desc, length);

			/* Physical address to DMA to/from */
			desc->des2 = mem_addr;
		}

		/* Set first descriptor */
		desc = host->sg_cpu;
		desc->des0 |= IDMAC_DES0_FD;

		/* Set last descriptor */
		desc = host->sg_cpu + (i - 1) * sizeof(struct idmac_desc);
		desc->des0 &= ~(IDMAC_DES0_CH | IDMAC_DES0_DIC);
		desc->des0 |= IDMAC_DES0_LD;

		wmb();
	} else {
		unsigned int i;

		struct idmac_desc_64bit *desc_first, *desc_last, *desc;

		desc_first = desc_last = desc = host->sg_cpu;
		if (!sg_len)
			return;
		for (i = 0; i < sg_len; i++) {
			unsigned int length = sg_dma_len(&data->sg[i]);

			u64 mem_addr = sg_dma_address(&data->sg[i]);

			for ( ; length ; desc++) {/*lint !e440*/
				desc_len = (length <= DW_MCI_DESC_DATA_LENGTH) ?
					   length : DW_MCI_DESC_DATA_LENGTH;

				length -= desc_len;

				/* Set the OWN bit and disable interrupts for this descriptor */
				desc->des0 = IDMAC_DES0_OWN | IDMAC_DES0_DIC | IDMAC_DES0_CH;

				/*优化修改，防止内存不初始化*/
				if(desc->des0 & IDMAC_DES0_CH) {
				        desc->des2 = 0;
				}

				/*优化修改，防止内存不初始化*/
				if(desc->des0 & IDMAC_DES0_CH) {
					desc->des2 = 0;
				}

				/* Buffer length */
				IDMAC_64ADDR_SET_BUFFER1_SIZE(desc, desc_len);

				/* Physical address to DMA to/from */
				desc->des4 = mem_addr & 0xffffffff;
				desc->des5 = mem_addr >> 32;

				/* Update physical address for the next desc */
				mem_addr += desc_len;

				/* Save pointer to the last descriptor */
				desc_last = desc;
			}
		}

		/* Set first descriptor */
		desc_first->des0 |= IDMAC_DES0_FD;

		/* Set last descriptor */
		desc_last->des0 &= ~(IDMAC_DES0_CH | IDMAC_DES0_DIC);
		desc_last->des0 |= IDMAC_DES0_LD;
		wmb(); /* drain writebuffer */
	}

}
#else
static inline int dw_mci_prepare_desc64(struct dw_mci *host,
					 struct mmc_data *data,
					 unsigned int sg_len)
{
	unsigned int desc_len;
	struct idmac_desc_64bit *desc_first, *desc_last, *desc;
	unsigned long timeout;
	unsigned int i;

	desc_first = desc_last = desc = host->sg_cpu;

	if (!sg_len)
		return -EINVAL;
	for (i = 0; i < sg_len; i++) {
		unsigned int length = sg_dma_len(&data->sg[i]);

		u64 mem_addr = sg_dma_address(&data->sg[i]);

		for ( ; length ; desc++) {/*lint !e440*/
			desc_len = (length <= DW_MCI_DESC_DATA_LENGTH) ?
				   length : DW_MCI_DESC_DATA_LENGTH;

			length -= desc_len;

			/*
			 * Wait for the former clear OWN bit operation
			 * of IDMAC to make sure that this descriptor
			 * isn't still owned by IDMAC as IDMAC's write
			 * ops and CPU's read ops are asynchronous.
			 */
			timeout = jiffies + msecs_to_jiffies(100);
			while (readl(&desc->des0) & IDMAC_DES0_OWN) {
				if (time_after(jiffies, timeout))
					goto err_own_bit;
				udelay(10);
			}

			/*
			 * Set the OWN bit and disable interrupts
			 * for this descriptor
			 */
			desc->des0 = IDMAC_DES0_OWN | IDMAC_DES0_DIC |
						IDMAC_DES0_CH;
			
			/*优化修改，防止内存不初始化*/	
			if(desc->des0 & IDMAC_DES0_CH) {
                                desc->des1 = 0;
                        }

			/* Buffer length */
			IDMAC_64ADDR_SET_BUFFER1_SIZE(desc, desc_len);

			/* Physical address to DMA to/from */
			desc->des4 = mem_addr & 0xffffffff;
			desc->des5 = mem_addr >> 32;

			/* Update physical address for the next desc */
			mem_addr += desc_len;

			/* Save pointer to the last descriptor */
			desc_last = desc;
		}
	}

	/* Set first descriptor */
	desc_first->des0 |= IDMAC_DES0_FD;

	/* Set last descriptor */
	desc_last->des0 &= ~(IDMAC_DES0_CH | IDMAC_DES0_DIC);
	desc_last->des0 |= IDMAC_DES0_LD;

	return 0;
err_own_bit:
	/* restore the descriptor chain as it's polluted */
	dev_dbg(host->dev, "desciptor is still owned by IDMAC.\n");
	memset(host->sg_cpu, 0, DESC_RING_BUF_SZ);
	dw_mci_idmac_init(host);
	return -EINVAL;
}


static inline int dw_mci_prepare_desc32(struct dw_mci *host,
					 struct mmc_data *data,
					 unsigned int sg_len)
{
	unsigned int desc_len;
	struct idmac_desc *desc_first, *desc_last, *desc;
	unsigned long timeout;
	unsigned int i;

	desc_first = desc_last = desc = host->sg_cpu;

	if(!sg_len)
		return -EINVAL;
	for (i = 0; i < sg_len; i++) {
		unsigned int length = sg_dma_len(&data->sg[i]);

		u32 mem_addr = sg_dma_address(&data->sg[i]);

		for ( ; length ; desc++) {/*lint !e440*/
			desc_len = (length <= DW_MCI_DESC_DATA_LENGTH) ?
				   length : DW_MCI_DESC_DATA_LENGTH;

			length -= desc_len;

			/*
			 * Wait for the former clear OWN bit operation
			 * of IDMAC to make sure that this descriptor
			 * isn't still owned by IDMAC as IDMAC's write
			 * ops and CPU's read ops are asynchronous.
			 */
			timeout = jiffies + msecs_to_jiffies(100);
			while (readl(&desc->des0) &
			       cpu_to_le32(IDMAC_DES0_OWN)) {
				if (time_after(jiffies, timeout))
					goto err_own_bit;
				udelay(10);
			}

			/*
			 * Set the OWN bit and disable interrupts
			 * for this descriptor
			 */
			desc->des0 = cpu_to_le32(IDMAC_DES0_OWN |
						 IDMAC_DES0_DIC |
						 IDMAC_DES0_CH);
			
			/*优化修改，防止内存不初始化*/
			if(desc->des0 & IDMAC_DES0_CH) {
				desc->des1 = 0;
			}

			/* Buffer length */
			IDMAC_SET_BUFFER1_SIZE(desc, desc_len);

			/* Physical address to DMA to/from */
			desc->des2 = cpu_to_le32(mem_addr);

			/* Update physical address for the next desc */
			mem_addr += desc_len;

			/* Save pointer to the last descriptor */
			desc_last = desc;
		}
	}

	/* Set first descriptor */
	desc_first->des0 |= cpu_to_le32(IDMAC_DES0_FD);

	/* Set last descriptor */
	desc_last->des0 &= cpu_to_le32(~(IDMAC_DES0_CH |
				       IDMAC_DES0_DIC));
	desc_last->des0 |= cpu_to_le32(IDMAC_DES0_LD);

	return 0;
err_own_bit:
	/* restore the descriptor chain as it's polluted */
	dev_dbg(host->dev, "desciptor is still owned by IDMAC.\n");
	memset(host->sg_cpu, 0, DESC_RING_BUF_SZ);
	dw_mci_idmac_init(host);
	return -EINVAL;
}
#endif
static int dw_mci_idmac_start_dma(struct dw_mci *host, unsigned int sg_len)
{
	u32 temp;
#ifdef CONFIG_HISI_MMC
	dw_mci_translate_sglist(host, host->data, sg_len);
#else
	int ret;

	if (host->dma_64bit_address == SDMMC_64_BIT_DMA)
		ret = dw_mci_prepare_desc64(host, host->data, sg_len);
	else
		ret = dw_mci_prepare_desc32(host, host->data, sg_len);

	if (ret)
		goto out;

	/* drain writebuffer */
	wmb();
#endif

	/* Make sure to reset DMA in case we did PIO before this */
	dw_mci_ctrl_reset(host, SDMMC_CTRL_DMA_RESET);
	dw_mci_idmac_reset(host);

	/* Select IDMAC interface */
	temp = mci_readl(host, CTRL);
	temp |= SDMMC_CTRL_USE_IDMAC;
	mci_writel(host, CTRL, temp);

	/* drain writebuffer */
	wmb();

	/* Enable the IDMAC */
	temp = mci_readl(host, BMOD);
	temp |= SDMMC_IDMAC_ENABLE | SDMMC_IDMAC_FB;
	mci_writel(host, BMOD, temp);

	/* Start it running */
	mci_writel(host, PLDMND, 1);

#ifdef CONFIG_HISI_MMC
	return 0;
#else
out:
	return ret;
#endif
}

static const struct dw_mci_dma_ops dw_mci_idmac_ops = {
	.init = dw_mci_idmac_init,
	.start = dw_mci_idmac_start_dma,
	.stop = dw_mci_idmac_stop_dma,
	.reset = dw_mci_idmac_reset,
	.complete = dw_mci_dmac_complete_dma,
	.cleanup = dw_mci_dma_cleanup,
};

static void dw_mci_edmac_stop_dma(struct dw_mci *host)
{
	dmaengine_terminate_async(host->dms->ch);
}

static int dw_mci_edmac_start_dma(struct dw_mci *host,
					    unsigned int sg_len)
{
	struct dma_slave_config cfg;
	struct dma_async_tx_descriptor *desc = NULL;
	struct scatterlist *sgl = host->data->sg;
	const u32 mszs[] = {1, 4, 8, 16, 32, 64, 128, 256};
	u32 sg_elems = host->data->sg_len;
	u32 fifoth_val;
	u32 fifo_offset = host->fifo_reg - host->regs;
	int ret = 0;

	/* Set external dma config: burst size, burst width */
	cfg.dst_addr = host->phy_regs + fifo_offset;
	cfg.src_addr = cfg.dst_addr;
	cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	/* Match burst msize with external dma config */
	fifoth_val = mci_readl(host, FIFOTH);
	cfg.dst_maxburst = mszs[(fifoth_val >> 28) & 0x7];
	cfg.src_maxburst = cfg.dst_maxburst;

	if (host->data->flags & MMC_DATA_WRITE)
		cfg.direction = DMA_MEM_TO_DEV;
	else
		cfg.direction = DMA_DEV_TO_MEM;

	ret = dmaengine_slave_config(host->dms->ch, &cfg);
	if (ret) {
		dev_err(host->dev, "Failed to config edmac.\n");
		return -EBUSY;
	}

	desc = dmaengine_prep_slave_sg(host->dms->ch, sgl,
				       sg_len, cfg.direction,
				       DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc) {
		dev_err(host->dev, "Can't prepare slave sg.\n");
		return -EBUSY;
	}

	/* Set dw_mci_dmac_complete_dma as callback */
	desc->callback = dw_mci_dmac_complete_dma;
	desc->callback_param = (void *)host;
	dmaengine_submit(desc);

	/* Flush cache before write */
	if (host->data->flags & MMC_DATA_WRITE)
		dma_sync_sg_for_device(mmc_dev(host->cur_slot->mmc), sgl,
				       sg_elems, DMA_TO_DEVICE);

	dma_async_issue_pending(host->dms->ch);

	return 0;
}

static int dw_mci_edmac_init(struct dw_mci *host)
{
	/* Request external dma channel */
	host->dms = kzalloc(sizeof(struct dw_mci_dma_slave), GFP_KERNEL);
	if (!host->dms)
		return -ENOMEM;

	host->dms->ch = dma_request_slave_channel(host->dev, "rx-tx");
	if (!host->dms->ch) {
		dev_err(host->dev, "Failed to get external DMA channel.\n");
		kfree(host->dms);
		host->dms = NULL;
		return -ENXIO;
	}

	return 0;
}

static void dw_mci_edmac_exit(struct dw_mci *host)
{
	if (host->dms) {
		if (host->dms->ch) {
			dma_release_channel(host->dms->ch);
			host->dms->ch = NULL;
		}
		kfree(host->dms);
		host->dms = NULL;
	}
}

static const struct dw_mci_dma_ops dw_mci_edmac_ops = {
	.init = dw_mci_edmac_init,
	.exit = dw_mci_edmac_exit,
	.start = dw_mci_edmac_start_dma,
	.stop = dw_mci_edmac_stop_dma,
	.complete = dw_mci_dmac_complete_dma,
	.cleanup = dw_mci_dma_cleanup,
};

static int dw_mci_pre_dma_transfer(struct dw_mci *host,
				   struct mmc_data *data,
				   bool next)
{
	struct scatterlist *sg;
	unsigned int i, sg_len;

	if (!next && data->host_cookie)
		return data->host_cookie;

	/*
	 * We don't do DMA on "complex" transfers, i.e. with
	 * non-word-aligned buffers or lengths. Also, we don't bother
	 * with all the DMA setup overhead for short transfers.
	 */
	if (data->blocks * data->blksz < DW_MCI_DMA_THRESHOLD)
		return -EINVAL;

	if (data->blksz & 3)
		return -EINVAL;

	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3)
			return -EINVAL;
	}

	sg_len = dma_map_sg(host->dev,
			    data->sg,
			    data->sg_len,
			    dw_mci_get_dma_dir(data));
	if (sg_len == 0)
		return -EINVAL;

	if (next)
		data->host_cookie = sg_len;

	return sg_len;
}

static void dw_mci_pre_req(struct mmc_host *mmc,
			   struct mmc_request *mrq,
			   bool is_first_req)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (!slot->host->use_dma || !data)
		return;

	if (data->host_cookie) {
		data->host_cookie = 0;
		return;
	}

	if (dw_mci_pre_dma_transfer(slot->host, mrq->data, 1) < 0)
		data->host_cookie = 0;
}

static void dw_mci_post_req(struct mmc_host *mmc,
			    struct mmc_request *mrq,
			    int err)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (!slot->host->use_dma || !data)
		return;

	if (data->host_cookie)
		dma_unmap_sg(slot->host->dev,
			     data->sg,
			     data->sg_len,
			     dw_mci_get_dma_dir(data));
	data->host_cookie = 0;
}

static void dw_mci_adjust_fifoth(struct dw_mci *host, struct mmc_data *data)
{
	unsigned int blksz = data->blksz;
	const u32 mszs[] = { 1, 4, 8, 16, 32, 64, 128, 256 };
	u32 fifo_width = 1 << host->data_shift;
	u32 blksz_depth = blksz / fifo_width, fifoth_val;
	u32 msize = 0, rx_wmark = 1, tx_wmark, tx_wmark_invers;
	int idx = ARRAY_SIZE(mszs) - 1;

	/* pio should ship this scenario */
	if (!host->use_dma)
		return;

	tx_wmark = (host->fifo_depth) / 2;
	tx_wmark_invers = host->fifo_depth - tx_wmark;

	/*
	 * MSIZE is '1',
	 * if blksz is not a multiple of the FIFO width
	 */
	if (blksz % fifo_width)
		goto done;

	do {
		if (!((blksz_depth % mszs[idx]) ||
		      (tx_wmark_invers % mszs[idx]))) {
			msize = idx;
			rx_wmark = mszs[idx] - 1;
			break;
		}
	} while (--idx > 0);
	/*
	 * If idx is '0', it won't be tried
	 * Thus, initial values are uesed
	 */
done:
	fifoth_val = SDMMC_SET_FIFOTH(msize, rx_wmark, tx_wmark);
	mci_writel(host, FIFOTH, fifoth_val);
}

static void dw_mci_ctrl_thld(struct dw_mci *host, struct mmc_data *data)
{
	unsigned int blksz = data->blksz;
	u32 blksz_depth, fifo_depth;
	u16 thld_size;
	u8 enable;

	/*
	 * CDTHRCTL doesn't exist prior to 240A (in fact that register offset is
	 * in the FIFO region, so we really shouldn't access it).
	 */
	if (host->verid < DW_MMC_240A ||
		(host->verid < DW_MMC_280A && data->flags & MMC_DATA_WRITE))
		return;

	/*
	 * Card write Threshold is introduced since 2.80a
	 * It's used when HS400 mode is enabled.
	 */
	if (data->flags & MMC_DATA_WRITE &&
		!(host->timing != MMC_TIMING_MMC_HS400))
		return;

	if (data->flags & MMC_DATA_WRITE)
		enable = SDMMC_CARD_WR_THR_EN;
	else
		enable = SDMMC_CARD_RD_THR_EN;


	if (host->cur_slot->mmc->ios.timing != MMC_TIMING_MMC_HS200 &&
	    host->cur_slot->mmc->ios.timing != MMC_TIMING_UHS_SDR104)
		goto disable;

	blksz_depth = blksz / (1 << host->data_shift);/*lint !e573*/
	fifo_depth = host->fifo_depth;

	if (blksz_depth > fifo_depth)
		goto disable;

	/*
	 * If (blksz_depth) >= (fifo_depth >> 1), should be 'thld_size <= blksz'
	 * If (blksz_depth) <  (fifo_depth >> 1), should be thld_size = blksz
	 * Currently just choose blksz.
	 */
	thld_size = blksz;
	mci_writel(host, CDTHRCTL, SDMMC_SET_THLD(thld_size, enable));
	return;

disable:
	mci_writel(host, CDTHRCTL, 0);
}

static int dw_mci_submit_data_dma(struct dw_mci *host, struct mmc_data *data)
{
	int sg_len;
	u32 temp;

	host->using_dma = 0;

	/* If we don't have a channel, we can't do DMA */
	if (!host->use_dma)
		return -ENODEV;

	sg_len = dw_mci_pre_dma_transfer(host, data, 0);
	if (sg_len < 0) {
		host->dma_ops->stop(host);
		return sg_len;
	}

	host->using_dma = 1;

	if (host->use_dma == TRANS_MODE_IDMAC)
		dev_vdbg(host->dev,
			 "sd sg_cpu: %#lx sg_dma: %#lx sg_len: %d\n",
			 (unsigned long)host->sg_cpu,
			 (unsigned long)host->sg_dma,
			 sg_len);

	/*
	 * Decide the MSIZE and RX/TX Watermark.
	 * If current block size is same with previous size,
	 * no need to update fifoth.
	 */
	if (host->prev_blksz != data->blksz)
		dw_mci_adjust_fifoth(host, data);

	/* Disable RX/TX IRQs, let DMA handle it */
	temp = mci_readl(host, INTMASK);
	temp  &= ~(SDMMC_INT_RXDR | SDMMC_INT_TXDR);
	mci_writel(host, INTMASK, temp);

	if (host->dma_ops->start(host, sg_len)) {
		host->dma_ops->stop(host);
		/* We can't do DMA, try PIO for this one */
		dev_dbg(host->dev,
			"%s: fall back to PIO mode for current transfer\n",
			__func__);
		return -ENODEV;
	}

	return 0;
}

static void dw_mci_submit_data(struct dw_mci *host, struct mmc_data *data)
{

	int flags = SG_MITER_ATOMIC;
	u32 temp;

	data->error = -EINPROGRESS;/*lint !e570*/

	WARN_ON(host->data);
	host->sg = NULL;
	host->data = data;

	if (data->flags & MMC_DATA_READ)
		host->dir_status = DW_MCI_RECV_STATUS;
	else
		host->dir_status = DW_MCI_SEND_STATUS;

	dw_mci_ctrl_thld(host, data);

	if (dw_mci_submit_data_dma(host, data)) {
		if (host->data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;

		sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
		host->sg = data->sg;
		host->part_buf_start = 0;
		host->part_buf_count = 0;

		mci_writel(host, RINTSTS, SDMMC_INT_TXDR | SDMMC_INT_RXDR);
		temp = mci_readl(host, INTMASK);
		temp |= SDMMC_INT_TXDR | SDMMC_INT_RXDR;
		mci_writel(host, INTMASK, temp);

		temp = mci_readl(host, CTRL);
		temp &= ~SDMMC_CTRL_DMA_ENABLE;
		mci_writel(host, CTRL, temp);

		/*
		 * Use the initial fifoth_val for PIO mode.
		 * If next issued data may be transfered by DMA mode,
		 * prev_blksz should be invalidated.
		 */
		mci_writel(host, FIFOTH, host->fifoth_val);
		host->prev_blksz = 0;
	} else {
		/*
		 * Keep the current block size.
		 * It will be used to decide whether to update
		 * fifoth register next time.
		 */
		host->prev_blksz = data->blksz;
	}
}
static void dw_mci_setup_bus(struct dw_mci_slot *slot, bool force_clkinit)
{
	struct dw_mci *host = slot->host;
	const struct dw_mci_drv_data *drv_data = slot->host->drv_data;
	u32 div;
	u32 clk_en_a;
	u32 ret = 0;
	u32 retry_times = 1;
	u32 i = 0;
	u32 id = (u32)(slot->id);

	if (slot->clock != host->current_speed || force_clkinit) {
		div = host->bus_hz / slot->clock;
		if (host->bus_hz % slot->clock && host->bus_hz > slot->clock)
			/*
			 * move the + 1 after the divide to prevent
			 * over-clocking the card.
			 */
			div += 1;

		div = (host->bus_hz != slot->clock) ? DIV_ROUND_UP(div, 2) : 0;

		dev_info(&slot->mmc->class_dev,
			 "Bus speed (slot %d) = %dHz (slot req %dHz, actual %dHZ"
			 " div = %d)\n", slot->id, host->bus_hz, slot->clock,
			 div ? ((host->bus_hz / div) >> 1) : host->bus_hz, div);

retry:
		/* disable clock */
		mci_writel(host, CLKENA, 0);
		mci_writel(host, CLKSRC, 0);

		/* inform CIU */
		ret = mci_send_cmd(slot,
			     SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);
		if (ret && retry_times)
			goto reset;

		/* set clock to desired speed */
		mci_writel(host, CLKDIV, div);

		/* inform CIU */
		ret = mci_send_cmd(slot,
			     SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);
		if (ret && retry_times)
			goto reset;

		/* enable clock; only low power if no SDIO */
		clk_en_a = SDMMC_CLKEN_ENABLE << id;
		if (!(mci_readl(host, INTMASK) & SDMMC_INT_SDIO(slot->id))
			&& (host->hw_mmc_id != DWMMC_SD_ID)
			&& mmc_host_wifi_support_lowpwr(slot->mmc))
			clk_en_a |= SDMMC_CLKEN_LOW_PWR << id;
		mci_writel(host, CLKENA, clk_en_a);

		/* inform CIU */
		ret = mci_send_cmd(slot,
			     SDMMC_CMD_UPD_CLK | SDMMC_CMD_PRV_DAT_WAIT, 0);
		if (ret && retry_times)
			goto reset;

		host->current_speed = slot->clock;
reset:
		if (ret && retry_times) {
			udelay(100);
			for (i = 0; i < 1000; i++) {
				if (host->cmd_status & SDMMC_INT_HLE)
					mdelay(1);
				else
					break;
			}

			dev_err(host->dev, "need to reset ip\n");
			if (drv_data && drv_data->work_fail_reset)
				drv_data->work_fail_reset(host);
			retry_times = 0;
			goto retry;
		}
	}

	/* Set the current slot bus width */
	mci_writel(host, CTYPE, (slot->ctype << id));
}

static void __dw_mci_start_request(struct dw_mci *host,
				   struct dw_mci_slot *slot,
				   struct mmc_command *cmd)
{
	struct mmc_request *mrq;
	struct mmc_data	*data;
	u32 cmdflags;

	mrq = slot->mrq;
	host->stop_snd = false;
	if (host->pdata->select_slot)
		host->pdata->select_slot(slot->id);

	/* Slot specific timing and width adjustment */
	dw_mci_setup_bus(slot, 0);

	if (host->flags & DWMMC_IN_TUNING){
		mod_timer(&host->timer, jiffies + msecs_to_jiffies(100));
	} else {
		if(DWMMC_EMMC_ID == host->hw_mmc_id) {
			/*add to 20s timeout for some write slow emmc*/
			mod_timer(&host->timer, jiffies + msecs_to_jiffies(20000));

		} else if ((host->hw_mmc_id == DWMMC_SD_ID) && (slot->mmc->card)) {
			if (mmc_card_suspended(slot->mmc->card))
				mod_timer(&host->timer, jiffies + msecs_to_jiffies(2000));
#ifdef CONFIG_SD_TIMEOUT_RESET
			else if ((VOLT_HOLD_CLK_08V == host->volt_hold_clk_sd) &&
				(VOLT_TO_1S == host->set_sd_data_tras_timeout))
				mod_timer(&host->timer, jiffies + msecs_to_jiffies(1000));
#endif
			else
				mod_timer(&host->timer, jiffies + msecs_to_jiffies(10000));
		} else if (host->hw_mmc_id == DWMMC_SDIO_ID) {
#ifdef CONFIG_SD_TIMEOUT_RESET
			if ((VOLT_HOLD_CLK_08V == host->volt_hold_clk_sdio) &&
				(VOLT_TO_1S == host->set_sdio_data_tras_timeout))
				mod_timer(&host->timer, jiffies + msecs_to_jiffies(1000));
			else
#endif
				mod_timer(&host->timer, jiffies + msecs_to_jiffies(5000));
		} else
			mod_timer(&host->timer, jiffies + msecs_to_jiffies(10000));
	}

	host->cur_slot = slot;
	host->mrq = mrq;

	host->pending_events = 0;
	host->completed_events = 0;
	host->data_status = 0;
	host->cmd_status = 0;
	host->dir_status = 0;

	data = cmd->data;
	if (data) {
		dw_mci_set_timeout(host);
		mci_writel(host, BYTCNT, data->blksz*data->blocks);
		mci_writel(host, BLKSIZ, data->blksz);
	}

	cmdflags = dw_mci_prepare_command(slot->mmc, cmd);

	/* this is the first command, send the initialization clock */
	if (test_and_clear_bit(DW_MMC_CARD_NEED_INIT, &slot->flags))
		cmdflags |= SDMMC_CMD_INIT;

	if (data) {
		dw_mci_submit_data(host, data);
		wmb(); /* drain writebuffer */
	}

	dw_mci_start_command(host, cmd, cmdflags);

	if (mrq->stop)
		host->stop_cmdr = dw_mci_prepare_command(slot->mmc, mrq->stop);
	else {
		if (data)
			host->stop_cmdr = dw_mci_prep_stop(host, cmd);
	}
}

static void dw_mci_start_request(struct dw_mci *host,
				 struct dw_mci_slot *slot)
{
	struct mmc_request *mrq = slot->mrq;
	struct mmc_command *cmd;

	cmd = mrq->sbc ? mrq->sbc : mrq->cmd;
	__dw_mci_start_request(host, slot, cmd);
}

/* must be called with host->lock held */
static void dw_mci_queue_request(struct dw_mci *host, struct dw_mci_slot *slot,
				 struct mmc_request *mrq)
{
	dev_vdbg(&slot->mmc->class_dev, "queue request: state=%d\n",
		 host->state);

	slot->mrq = mrq;

	if (host->state == STATE_IDLE) {
		host->state = STATE_SENDING_CMD;
		dw_mci_start_request(host, slot);
	} else {
		list_add_tail(&slot->queue_node, &host->queue);
	}
}

static void dw_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;

	WARN_ON(slot->mrq);


	/*
	 * The check for card presence and queueing of the request must be
	 * atomic, otherwise the card could be removed in between and the
	 * request wouldn't fail until another card was inserted.
	 */

	if (!test_bit(DW_MMC_CARD_PRESENT, &slot->flags)) {
		mrq->cmd->error = -ENOMEDIUM;/*lint !e570*/
		mmc_request_done(mmc, mrq);
		return;
	}

	pm_runtime_get_sync(mmc_dev(mmc));

	if (!dw_mci_stop_abort_cmd(mrq->cmd)) {
		if (!dw_mci_wait_data_busy(host, mrq)) {
			dev_err(&mmc->class_dev, "wait data busy timeout !\n");
			mrq->cmd->error = -ENOTRECOVERABLE;/*lint !e570*/
			mmc_request_done(mmc, mrq);
			pm_runtime_mark_last_busy(mmc_dev(mmc));
			pm_runtime_put_autosuspend(mmc_dev(mmc));
			return;
		}
	}

	spin_lock_bh(&host->lock);

	dw_mci_queue_request(host, slot, mrq);

	spin_unlock_bh(&host->lock);
}

static void dw_mci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	const struct dw_mci_drv_data *drv_data = slot->host->drv_data;
	u32 regs;
	u32 id = (u32)(slot->id);

	pm_runtime_get_sync(mmc_dev(mmc));

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_4:
		slot->ctype = SDMMC_CTYPE_4BIT;
		break;
	case MMC_BUS_WIDTH_8:
		slot->ctype = SDMMC_CTYPE_8BIT;
		break;
	default:
		/* set default 1 bit mode */
		slot->ctype = SDMMC_CTYPE_1BIT;
	}

	regs = mci_readl(slot->host, UHS_REG);

	/* DDR mode set */
	if (ios->timing == MMC_TIMING_UHS_DDR50)
		regs |= ((0x1 << id) << 16);
	else
		regs &= ~((0x1 << id) << 16);/*lint !e502*/

	if (slot->host->pdata->caps &
				(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
				 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
				 MMC_CAP_UHS_DDR50))
		regs |= (0x1 << id);
	mci_writel(slot->host, UHS_REG, regs);

	if (ios->clock) {
		/*
		 * Use mirror of ios->clock to prevent race with mmc
		 * core ios update when finding the minimum.
		 */
		slot->clock = ios->clock;
	}

	if (drv_data && drv_data->set_ios)
		drv_data->set_ios(slot->host, ios);

	mmc->f_min = DIV_ROUND_UP(slot->host->bus_hz, 510);
	mmc->f_max = slot->host->bus_hz;/*上面设置的bus_hz*/

	/* Slot specific timing and width adjustment */
	dw_mci_setup_bus(slot, false);

	switch (ios->power_mode) {
	case MMC_POWER_UP:
		set_bit(DW_MMC_CARD_NEED_INIT, &slot->flags);
		/* Power up slot */
		if (slot->host->pdata->setpower)
			slot->host->pdata->setpower(slot->id, mmc->ocr_avail);
		regs = mci_readl(slot->host, PWREN);
		regs |= (1 << id);
		mci_writel(slot->host, PWREN, regs);
		break;
	case MMC_POWER_OFF:
		/* Power down slot */
		if (slot->host->pdata->setpower)
			slot->host->pdata->setpower(slot->id, 0);
		regs = mci_readl(slot->host, PWREN);
		regs &= ~(1 << id);/*lint !e502*/
		mci_writel(slot->host, PWREN, regs);
		break;
	default:
		break;
	}

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

}
static int dw_mci_get_ro(struct mmc_host *mmc)
{
	int read_only;
	struct dw_mci_slot *slot = mmc_priv(mmc);
	int gpio_ro = mmc_gpio_get_ro(mmc);
	u32 id = (u32)(slot->id);

	/* Use platform get_ro function, else try on board write protect */
	if (gpio_ro >= 0)
		read_only = gpio_ro;
	else
		read_only =
			mci_readl(slot->host, WRTPRT) & (1 << id) ? 1 : 0;

	dev_dbg(&mmc->class_dev, "card is %s\n",
		read_only ? "read-only" : "read-write");

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return read_only;
}

int dw_mci_get_cd(struct mmc_host *mmc)
{
	int present;
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci_board *brd = slot->host->pdata;
	struct dw_mci *host = slot->host;
	u32 id = (u32)(slot->id);
	int gpio_cd = mmc_gpio_get_cd(mmc);
	pm_runtime_get_sync(mmc_dev(mmc));
	/* Use platform get_cd function, else try onboard card detect */
	if (brd->quirks & DW_MCI_QUIRK_BROKEN_CARD_DETECTION)
		present = 1;
	else if (brd->get_cd)
		present = !brd->get_cd(slot->host, slot->id);
	else if (host->hw_mmc_id == DWMMC_SDIO_ID)
		present = mmc->sdio_present;
	else
		present = (mci_readl(slot->host, CDETECT) & (1 << id))
			== 0 ? 1 : 0;
	if (present)
		dev_err(&mmc->class_dev, "card is present\n");
	else
		dev_err(&mmc->class_dev, "card is not present\n");

	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return present;
}

static void dw_mci_hw_reset(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	int reset;
	u32 id = (u32)(slot->id);

	if (host->use_dma == TRANS_MODE_IDMAC)
		dw_mci_idmac_reset(host);

	if (!dw_mci_ctrl_reset(host, SDMMC_CTRL_DMA_RESET |
				     SDMMC_CTRL_FIFO_RESET))
		return;

	/*
	 * According to eMMC spec, card reset procedure:
	 * tRstW >= 1us:   RST_n pulse width
	 * tRSCA >= 200us: RST_n to Command time
	 * tRSTH >= 1us:   RST_n high period
	 */
	reset = mci_readl(host, RST_N);
	reset &= ~(SDMMC_RST_HWACTIVE << id);/*lint !e502*/
	mci_writel(host, RST_N, reset);
	usleep_range(1, 2);
	reset |= SDMMC_RST_HWACTIVE << id;
	mci_writel(host, RST_N, reset);
	usleep_range(200, 300);
}

/*
 * Disable lower power mode.
 *
 * Low power mode will stop the card clock when idle.  According to the
 * description of the CLKENA register we should disable low power mode
 * for SDIO cards if we need SDIO interrupts to work.
 *
 * This function is fast if low power mode is already disabled.
 */
static void dw_mci_disable_low_power(struct dw_mci_slot *slot)
{
	struct dw_mci *host = slot->host;
	u32 clk_en_a;
	u32 id = (u32)(slot->id);

	const u32 clken_low_pwr = SDMMC_CLKEN_LOW_PWR << id;

	clk_en_a = mci_readl(host, CLKENA);

	if (clk_en_a & clken_low_pwr) {
		mci_writel(host, CLKENA, clk_en_a & ~clken_low_pwr);
		(void)mci_send_cmd(slot, SDMMC_CMD_UPD_CLK |
			     SDMMC_CMD_PRV_DAT_WAIT, 0);
	}
}

static void dw_mci_enable_sdio_irq(struct mmc_host *mmc, int enb)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	u32 int_mask;

	/* Enable/disable Slot Specific SDIO interrupt */
	int_mask = mci_readl(host, INTMASK);
	if (enb) {
		/*
		 * Turn off low power mode if it was enabled.  This is a bit of
		 * a heavy operation and we disable / enable IRQs a lot, so
		 * we'll leave low power mode disabled and it will get
		 * re-enabled again in dw_mci_setup_bus().
		 */
		dw_mci_disable_low_power(slot);

		mci_writel(host, INTMASK,
			   (int_mask | SDMMC_INT_SDIO(slot->id)));
	} else {
		mci_writel(host, INTMASK,
			   (int_mask & ~SDMMC_INT_SDIO(slot->id)));
	}
}

static int dw_mci_execute_tuning(struct mmc_host *mmc,u32 opcode)
{
    struct dw_mci_slot *slot = mmc_priv(mmc);
    struct dw_mci *host = slot->host;
    const struct dw_mci_drv_data *drv_data = host->drv_data;
    struct dw_mci_tuning_data tuning_data;
    int err = -EINVAL;

    if(drv_data && drv_data->execute_tuning)
        err = drv_data->execute_tuning(slot,opcode,&tuning_data);

    return err;
}

/*Function we transfer in core to ensure we need retuning*/
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
static int dw_mci_need_retuning(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;

	return host->clk_change;
}
#endif

static int dw_mci_downshift(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	bool downshift_res = 0;

	downshift_res = host->downshift;

	/*Ensure downshift only once*/
	host->downshift = false;
	return downshift_res;
}


#ifdef CONFIG_MMC_PASSWORDS
static int dw_mci_sd_lock_reset(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	const struct dw_mci_drv_data *drv_data = slot->host->drv_data;
	u32 present = 0;

	pm_runtime_get_sync(mmc_dev(mmc));
	present = dw_mci_get_cd(mmc);
	if (present == 1) {
		 if (drv_data && drv_data->work_fail_reset)
			drv_data->work_fail_reset(host);
	}
	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return 0;
}
#endif

static int dw_mci_card_busy(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	u32 present_state;

	pm_runtime_get_sync(mmc_dev(mmc));
	/* Check whether DAT[3:0] is 0000 */
	present_state = mci_readl(host, STATUS);
	pm_runtime_mark_last_busy(mmc_dev(mmc));
	pm_runtime_put_autosuspend(mmc_dev(mmc));

	return (present_state & SDMMC_STATUS_BUSY);

}
/*lint -save -e485 -e570*/
static const struct mmc_host_ops dw_mci_ops = {
	.request		= dw_mci_request,
	.pre_req		= dw_mci_pre_req,
	.post_req		= dw_mci_post_req,
	.set_ios		= dw_mci_set_ios,
	.get_ro			= dw_mci_get_ro,
	.get_cd			= dw_mci_get_cd,
	.hw_reset               = dw_mci_hw_reset,
	.enable_sdio_irq	= dw_mci_enable_sdio_irq,
	.execute_tuning		= dw_mci_execute_tuning,
	.slowdown_clk		= dw_mci_slowdown_clk,
	.card_busy		= dw_mci_card_busy,
	.enable_sdio_irq	= dw_mci_enable_sdio_irq,
	.start_signal_voltage_switch = dw_mci_start_signal_voltage_switch,
	.execute_tuning		 = dw_mci_execute_tuning,
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	.need_retuning                       = dw_mci_need_retuning,
#endif
	.downshift           = dw_mci_downshift,
#ifdef CONFIG_MMC_PASSWORDS
	.sd_lock_reset		= dw_mci_sd_lock_reset,
#endif
};

#ifdef CONFIG_SD_SDIO_CRC_RETUNING
extern void dw_mci_hs_change_timing_config(struct dw_mci *host);
#endif

void dw_mci_retuning_flag_set(struct dw_mci *host,int timing)
{
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	if ((host->retuning_flag == SD_RETUNING_ON) && (MMC_TIMING_UHS_SDR104 == timing) && (0 == host->need_clk_change)) {
		dev_err(host->dev, "SD clk change to ppll3\n");
		host->clk_change = true;
		dw_mci_hs_change_timing_config(host);
		host->tuning_current_sample = -1;
		host->need_clk_change = 1;
	}
#endif
	/*Downshit when tuning move fail*/
	if ((DWMMC_SD_ID == host->hw_mmc_id) && ((MMC_TIMING_UHS_SDR104 == timing) || (MMC_TIMING_UHS_SDR50 == timing))) {
		host->downshift = true;
	}

}

void dw_mci_request_end(struct dw_mci *host, struct mmc_request *mrq)
	__releases(&host->lock)
	__acquires(&host->lock)
{
	struct dw_mci_slot *slot;
	struct mmc_host	*prev_mmc = host->cur_slot->mmc;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int timing = prev_mmc->ios.timing;
	WARN_ON(host->cmd || host->data);

	del_timer(&host->timer);
	if (host->cur_slot->mrq == NULL || host->mrq == NULL)
		return;

	host->cur_slot->mrq = NULL;
	host->mrq = NULL;

	if (drv_data->tuning_move) {

	/*SOC1005*/
	if ((host->flags & DWMMC_TUNING_DONE) && mrq && mrq->cmd &&
		((mrq->cmd->error ) ||
		(mrq->cmd->data && ((mrq->cmd->data->error) ||
		(mrq->cmd->data->stop && mrq->cmd->data->stop->error))))){
			/* if SD get hardware timeout, No tuning move! */
			if (host->sd_hw_timeout == 1) {
				host->sd_hw_timeout = 0;
				goto out;
			}

			dev_dbg(host->dev, "move tuning del_sel, start=%d, cmd=%d, arg=0x%x\n",
					host->tuning_move_start, mrq->cmd->opcode, mrq->cmd->arg);
			/* req error, need move del_sel */
			if (host->tuning_move_start != -1) {
				if (drv_data->tuning_move(host, timing, host->tuning_move_start)) {
					host->tuning_move_start = 0;
					mrq->cmd->retries++;

					if (mrq->cmd->data && mrq->cmd->data->error) {
						mrq->cmd->data->error = 0;
					}
					if (mrq->cmd->data && mrq->cmd->data->stop && mrq->cmd->data->stop->error) {
						mrq->cmd->data->stop->error = 0;
					}
					if (!mrq->cmd->error) {
						mrq->cmd->error = -EILSEQ;
					}
				} else {
					dw_mci_retuning_flag_set(host,timing);
					host->tuning_move_start = -1;
				}
			}
		} else {
			host->tuning_move_start = 1;
		}
	}

out:
	if (!list_empty(&host->queue)) {
		slot = list_entry(host->queue.next,
				  struct dw_mci_slot, queue_node);
		list_del(&slot->queue_node);
		dev_vdbg(host->dev, "list not empty: %s is next\n",
			 mmc_hostname(slot->mmc));
		host->state = STATE_SENDING_CMD;
		dw_mci_start_request(host, slot);
	} else {
		dev_vdbg(host->dev, "list empty\n");
		host->state = STATE_IDLE;
	}

	spin_unlock(&host->lock);/*lint !e455*/
	mmc_request_done(prev_mmc, mrq);
	spin_lock(&host->lock);

	pm_runtime_mark_last_busy(mmc_dev(prev_mmc));
	pm_runtime_put_autosuspend(mmc_dev(prev_mmc));
}/*lint !e454*/

void dw_mci_print_error(struct dw_mci *host, struct mmc_command *cmd)
{
	if((host->hw_mmc_id == DWMMC_SD_ID) && ((cmd->opcode != SD_IO_RW_EXTENDED)
	 && (cmd->opcode != SD_IO_SEND_OP_COND) && (cmd->opcode != SD_IO_RW_DIRECT))) {
		dev_err(host->dev, "CMD %d(arg=0x%x) REP time out\n", cmd->opcode, cmd->arg);
	} else if ((host->hw_mmc_id == DWMMC_SDIO_ID) || (host->hw_mmc_id == DWMMC_EMMC_ID)) {
		dev_err(host->dev, "CMD %d(arg=0x%x) REP time out\n", cmd->opcode, cmd->arg);
                if ((0x5 == cmd->opcode) && (0x0 == cmd->arg))
			dw_mci_reg_dump(host);
	}
}

static void dw_mci_command_complete(struct dw_mci *host, struct mmc_command *cmd)
{
	u32 status = host->cmd_status;
	struct dw_mci_slot *slot = host->cur_slot;

	host->cmd_status = 0;

	/* Read the response from the card (up to 16 bytes) */
	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			cmd->resp[3] = mci_readl(host, RESP0);
			cmd->resp[2] = mci_readl(host, RESP1);
			cmd->resp[1] = mci_readl(host, RESP2);
			cmd->resp[0] = mci_readl(host, RESP3);
		} else {
			cmd->resp[0] = mci_readl(host, RESP0);
			cmd->resp[1] = 0;
			cmd->resp[2] = 0;
			cmd->resp[3] = 0;
		}
	}

	if (status & SDMMC_INT_RTO) {
		if ((host->hw_mmc_id == DWMMC_SD_ID) &&
		    (cmd->opcode == MMC_SEND_TUNING_BLOCK))
			udelay(1000);

		if (!(slot && slot->sdio_wakelog_switch))
		{
                                dw_mci_print_error(host,cmd);
		}
		cmd->error = -ETIMEDOUT;
	} else if ((cmd->flags & MMC_RSP_CRC) && (status & SDMMC_INT_RCRC)) {
		if ((host->hw_mmc_id == DWMMC_SD_ID) &&
		    (cmd->opcode == MMC_SEND_TUNING_BLOCK))
			udelay(1000);

		dev_err(host->dev, "CMD %d(arg=0x%x) REP CRC error\n",
			cmd->opcode, cmd->arg);
		cmd->error = -EILSEQ;
	} else if (status & SDMMC_INT_RESP_ERR) {
		if ((host->hw_mmc_id == DWMMC_SD_ID) &&
		    (cmd->opcode == MMC_SEND_TUNING_BLOCK))
			udelay(1000);

		dev_err(host->dev, "CMD %d(arg=0x%x) REP error\n", cmd->opcode,
			cmd->arg);
		cmd->error = -EIO;
	} else
		cmd->error = 0;

}
/*lint -restore*/

static void dw_mci_set_drto(struct dw_mci *host)
{
	unsigned int drto_clks;
	unsigned int drto_ms;

	drto_clks = mci_readl(host, TMOUT) >> 8;
	drto_ms = DIV_ROUND_UP(drto_clks, host->bus_hz / 1000);

	/* add a bit spare time */
	drto_ms += 10;

	mod_timer(&host->dto_timer, jiffies + msecs_to_jiffies(drto_ms));
}

static int pro_state_send_stop(struct dw_mci *host)
{
	if (!test_and_clear_bit(EVENT_CMD_COMPLETE,
				&host->pending_events))
		return 1;

	if (host->mrq->cmd->error &&
			host->mrq->data) {
		dw_mci_stop_dma(host);
		sg_miter_stop(&host->sg_miter);
		host->sg = NULL;
		dw_mci_fifo_reset(host->dev, host);
		dw_mci_ciu_reset(host->dev, host);
	}

	host->cmd = NULL;
	host->data = NULL;

	if (host->mrq->stop)
		dw_mci_command_complete(host, host->mrq->stop);
	else {
		host->cmd_status = 0;
	}
	dw_mci_request_end(host, host->mrq);

	return 0;
}
/*lint -save -e613 -e570*/
static int pro_state_send_data(struct dw_mci *host, struct mmc_data *data, enum dw_mci_state state)
{
	int ret = 1;

	if (test_and_clear_bit(EVENT_DATA_ERROR,
			       &host->pending_events)) {
		set_bit(EVENT_XFER_COMPLETE,
				&host->pending_events);
		if (data && data->stop)
			send_stop_cmd(host, data);
		else {
			dw_mci_start_command(host,
					&host->stop,
					host->stop_cmdr);
			host->stop_snd = true;
		}
		/* To avoid fifo full condition */
		dw_mci_fifo_reset(host->dev, host);
		state = STATE_DATA_ERROR;
		return ret;
	}

	if (!test_and_clear_bit(EVENT_XFER_COMPLETE,
				&host->pending_events)) {
		/*
		 * If all data-related interrupts don't come
		 * within the given time in reading data state.
		 */
		if ((host->quirks & DW_MCI_QUIRK_BROKEN_DTO) &&
		    (host->dir_status == DW_MCI_RECV_STATUS))
			dw_mci_set_drto(host);
		return ret;
	}
	return 0;
}

#ifdef CONFIG_SD_TIMEOUT_RESET
static void dw_mci_work_volt_mmc_func(struct work_struct *work)
{
	struct dw_mci *host = container_of(work, struct dw_mci, work_volt_mmc);

	if (host->hw_mmc_id == DWMMC_SD_ID) {
		/*When sd data transmission error,volt peri volt to 0.8v in libra*/
		if (clk_prepare_enable(host->volt_hold_sd_clk))
			dev_err(host->dev, "volt_hold_sd_clk clk_prepare_enable failed\n");
		else
			dev_err(host->dev, "volt_hold_sd_clk to 0.8v\n");

	} else if (host->hw_mmc_id == DWMMC_SDIO_ID) {
		/*When sdio data transmission error,volt peri volt to 0.8v in libra*/
		if (clk_prepare_enable(host->volt_hold_sdio_clk))
			dev_err(host->dev, "volt_hold_sdio_clk clk_prepare_enable failed\n");
		else
			dev_err(host->dev, "volt_hold_sdio_clk to 0.8v\n");
	}
}

static void dw_mci_set_peri_08v(struct dw_mci *host)
{
	/*When sdio data transmission error,volt peri volt to 0.8v in libra*/
	if ((DWMMC_SDIO_ID == host->hw_mmc_id) && !IS_ERR(host->volt_hold_sdio_clk) &&
		(VOLT_HOLD_CLK_08V != host->volt_hold_clk_sdio) &&
		(MMC_TIMING_UHS_SDR104 == host->cur_slot->mmc->ios.timing)) {
		host->volt_hold_clk_sdio = VOLT_HOLD_CLK_08V;
		host->set_sdio_data_tras_timeout = VOLT_TO_1S;
		queue_work(host->card_workqueue, &host->work_volt_mmc);/*dw_mci_work_volt_mmc_func*/
	}
}
#endif

static void dw_mci_tasklet_func(unsigned long priv)
{
	struct dw_mci *host = (struct dw_mci *)priv;
	struct mmc_data	*data;
	struct mmc_command *cmd;
	enum dw_mci_state state;
	enum dw_mci_state prev_state;
	u32 status;
	int ret = 0;

	spin_lock(&host->lock);

	state = host->state;
	data = host->data;

	if (host->cmd_status & SDMMC_INT_HLE) {
		clear_bit(EVENT_CMD_COMPLETE, &host->pending_events);
		dev_err(host->dev, "hardware locked write error\n");
		dw_mci_reg_dump(host);
		host->cmd_status &= ~SDMMC_INT_HLE;
		goto unlock;
	}

	do {
		prev_state = state;

		switch (state) {
		case STATE_IDLE:
			break;

		case STATE_SENDING_CMD:
			if (NULL == host->cmd) {
				printk(KERN_ERR"%s: The command currently being send to the card is NULL!\n",
						__func__);
				break;
			}
			if (!test_and_clear_bit(EVENT_CMD_COMPLETE,
						&host->pending_events))
				break;
			cmd = host->cmd;
			host->cmd = NULL;
			set_bit(EVENT_CMD_COMPLETE, &host->completed_events);
			dw_mci_command_complete(host, cmd);
			if (cmd == host->mrq->sbc && !cmd->error) {
				prev_state = state = STATE_SENDING_CMD;
				__dw_mci_start_request(host, host->cur_slot,
						       host->mrq->cmd);
				goto unlock;
			}

			if (data && cmd->error &&
					cmd != data->stop) {
				if (host->mrq->data->stop)
					send_stop_cmd(host, host->mrq->data);
				else {
					dw_mci_start_command(host, &host->stop,
							host->stop_cmdr);
					host->stop_snd = true;
				}
				/* To avoid fifo full condition */
				dw_mci_fifo_reset(host->dev, host);
				state = STATE_SENDING_STOP;
				break;
			}

			if (!host->mrq->data || cmd->error) {
				dw_mci_request_end(host, host->mrq);
				goto unlock;
			}

			prev_state = state = STATE_SENDING_DATA;
			/* fall through */

		case STATE_SENDING_DATA:
			ret = pro_state_send_data(host, data, state);
			if (1 == ret)
				break;

			set_bit(EVENT_XFER_COMPLETE, &host->completed_events);
			prev_state = state = STATE_DATA_BUSY;
			/* fall through */

		case STATE_DATA_BUSY:
			if (!test_and_clear_bit(EVENT_DATA_COMPLETE,
						&host->pending_events)) {
				/*
				 * If data error interrupt comes but data over
				 * interrupt doesn't come within the given time.
				 * in reading data state.
				 */
				if ((host->quirks & DW_MCI_QUIRK_BROKEN_DTO) &&
				    (host->dir_status == DW_MCI_RECV_STATUS))
					dw_mci_set_drto(host);
				break;
			}

			set_bit(EVENT_DATA_COMPLETE, &host->completed_events);
			status = host->data_status;

			if (!data) {
				pr_err("%s data ponit is NULL\n",__func__);
				break;
			}
			if (status & DW_MCI_DATA_ERROR_FLAGS) {
				if (status & SDMMC_INT_DRTO) {
					dev_err(host->dev,
						"data timeout error\n");
					data->error = -ETIMEDOUT;
				} else if (status & SDMMC_INT_DCRC) {
					if (!(host->flags & DWMMC_IN_TUNING)) {
						dev_err(host->dev,
							"data CRC error\n");
						dw_mci_reg_dump(host);

#ifdef CONFIG_SD_TIMEOUT_RESET
						dw_mci_set_peri_08v(host);
#endif
					}
					data->error = -EILSEQ;
				} else if (status & SDMMC_INT_EBE) {
					if (host->dir_status ==
							DW_MCI_SEND_STATUS) {
						/*
						 * No data CRC status was returned.
						 * The number of bytes transferred will
						 * be exaggerated in PIO mode.
						 */
						data->bytes_xfered = 0;
						data->error = -ETIMEDOUT;
						dev_err(host->dev,
							"Write no CRC\n");
					} else {
						data->error = -EIO;
						dev_err(host->dev,
							"End bit error\n");
					}

				} else if (status & SDMMC_INT_SBE) {
					dev_err(host->dev,
						"Start bit error "
						"(status=%08x)\n",
						status);
					data->error = -EIO;
				} else {
					dev_err(host->dev,
						"data FIFO error "
						"(status=%08x)\n",
						status);
					data->error = -EIO;
				}
				/*
				 * After an error, there may be data lingering
				 * in the FIFO, so reset it - doing so
				 * generates a block interrupt, hence setting
				 * the scatter-gather pointer to NULL.
				 */
				sg_miter_stop(&host->sg_miter);
				host->sg = NULL;
				dw_mci_fifo_reset(host->dev, host);
			} else {
				data->bytes_xfered = data->blocks * data->blksz;
				data->error = 0;
			}

			host->data = NULL;

			if (!data->stop && !host->stop_snd) {
				dw_mci_request_end(host, host->mrq);
				goto unlock;
			}

			if (host->mrq->sbc && !data->error) {
				if (data->stop)
					data->stop->error = 0;
				dw_mci_request_end(host, host->mrq);
				goto unlock;
			}

			prev_state = state = STATE_SENDING_STOP;
			if (!data->error)
				send_stop_cmd(host, data);

			if (test_and_clear_bit(EVENT_DATA_ERROR,
						&host->pending_events)) {
				if (data->stop)
					send_stop_cmd(host, data);
				else {
					dw_mci_start_command(host,
							&host->stop,
							host->stop_cmdr);
					host->stop_snd = true;
				}
			}
			/* fall through */

		case STATE_SENDING_STOP:
			ret = pro_state_send_stop(host);
			if (1 == ret)
				break;

			goto unlock;

		case STATE_DATA_ERROR:
			if (!test_and_clear_bit(EVENT_XFER_COMPLETE,
						&host->pending_events))
				break;

			dw_mci_stop_dma(host);
			set_bit(EVENT_XFER_COMPLETE, &host->completed_events);

			state = STATE_DATA_BUSY;
			break;
		}
	} while (state != prev_state);

	host->state = state;
unlock:
	spin_unlock(&host->lock);

}
/*lint -restore*/
/* push final bytes to part_buf, only use during push */
static void dw_mci_set_part_bytes(struct dw_mci *host, void *buf, int cnt)
{
	memcpy((void *)&host->part_buf, buf, cnt);
	host->part_buf_count = cnt;
}

/* append bytes to part_buf, only use during push */
static int dw_mci_push_part_bytes(struct dw_mci *host, void *buf, int cnt)
{
	cnt = min(cnt, (1 << host->data_shift) - host->part_buf_count);
	memcpy((void *)&host->part_buf + host->part_buf_count, buf, cnt);
	host->part_buf_count += cnt;
	return cnt;
}

/* pull first bytes from part_buf, only use during pull */
static int dw_mci_pull_part_bytes(struct dw_mci *host, void *buf, int cnt)
{
	cnt = min_t(int, cnt, host->part_buf_count);
	if (cnt) {
		memcpy(buf, (void *)&host->part_buf + host->part_buf_start,
		       cnt);
		host->part_buf_count -= cnt;
		host->part_buf_start += cnt;
	}
	return cnt;
}

/* pull final bytes from the part_buf, assuming it's just been filled */
static void dw_mci_pull_final_bytes(struct dw_mci *host, void *buf, int cnt)
{
	memcpy(buf, &host->part_buf, cnt);
	host->part_buf_start = cnt;
	host->part_buf_count = (1 << host->data_shift) - cnt;
}

static void dw_mci_push_data16(struct dw_mci *host, void *buf, int cnt)
{
	struct mmc_data *data = host->data;
	int init_cnt = cnt;

	/* try and push anything in the part_buf */
	if (unlikely(host->part_buf_count)) {
		int len = dw_mci_push_part_bytes(host, buf, cnt);

		buf += len;
		cnt -= len;
		if (host->part_buf_count == 2) {
			mci_fifo_writew(host->fifo_reg, host->part_buf16);
			host->part_buf_count = 0;
		}
	}
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x1)) {
		while (cnt >= 2) {
			u16 aligned_buf[64];
			int len = min(cnt & -2, (int)sizeof(aligned_buf));
			int items = len >> 1;
			int i;
			/* memcpy from input buffer into aligned buffer */
			memcpy(aligned_buf, buf, len);
			buf += len;
			cnt -= len;
			/* push data from aligned buffer into fifo */
			for (i = 0; i < items; ++i)
				mci_fifo_writew(host->fifo_reg, aligned_buf[i]);
		}
	} else
#endif
	{
		u16 *pdata = buf;

		for (; cnt >= 2; cnt -= 2)
			mci_fifo_writew(host->fifo_reg, *pdata++);
		buf = pdata;
	}
	/* put anything remaining in the part_buf */
	if (cnt) {
		dw_mci_set_part_bytes(host, buf, cnt);
		 /* Push data if we have reached the expected data length */
		if ((data->bytes_xfered + init_cnt) ==
		    (data->blksz * data->blocks))
			mci_fifo_writew(host->fifo_reg, host->part_buf16);
	}
}

static void dw_mci_pull_data16(struct dw_mci *host, void *buf, int cnt)
{
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x1)) {
		while (cnt >= 2) {
			/* pull data from fifo into aligned buffer */
			u16 aligned_buf[64] = {0};
			int len = min(cnt & -2, (int)sizeof(aligned_buf));
			int items = len >> 1;
			int i;

			for (i = 0; i < items; ++i)
				aligned_buf[i] = mci_fifo_readw(host->fifo_reg);
			/* memcpy from aligned buffer into output buffer */
			memcpy(buf, aligned_buf, len);
			buf += len;
			cnt -= len;
		}
	} else
#endif
	{
		u16 *pdata = buf;

		for (; cnt >= 2; cnt -= 2)
			*pdata++ = mci_fifo_readw(host->fifo_reg);
		buf = pdata;
	}
	if (cnt) {
		host->part_buf16 = mci_fifo_readw(host->fifo_reg);
		dw_mci_pull_final_bytes(host, buf, cnt);
	}
}

static void dw_mci_push_data32(struct dw_mci *host, void *buf, int cnt)
{
	struct mmc_data *data = host->data;
	int init_cnt = cnt;

	/* try and push anything in the part_buf */
	if (unlikely(host->part_buf_count)) {
		int len = dw_mci_push_part_bytes(host, buf, cnt);

		buf += len;
		cnt -= len;
		if (host->part_buf_count == 4) {
			mci_fifo_writel(host->fifo_reg,	host->part_buf32);
			host->part_buf_count = 0;
		}
	}
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x3)) {
		while (cnt >= 4) {
			u32 aligned_buf[32];
			int len = min(cnt & -4, (int)sizeof(aligned_buf));
			int items = len >> 2;
			int i;
			/* memcpy from input buffer into aligned buffer */
			memcpy(aligned_buf, buf, len);
			buf += len;
			cnt -= len;
			/* push data from aligned buffer into fifo */
			for (i = 0; i < items; ++i)
				mci_fifo_writel(host->fifo_reg,	aligned_buf[i]);
		}
	} else
#endif
	{
		u32 *pdata = buf;

		for (; cnt >= 4; cnt -= 4)
			mci_fifo_writel(host->fifo_reg, *pdata++);
		buf = pdata;
	}
	/* put anything remaining in the part_buf */
	if (cnt) {
		dw_mci_set_part_bytes(host, buf, cnt);
		 /* Push data if we have reached the expected data length */
		if ((data->bytes_xfered + init_cnt) ==
		    (data->blksz * data->blocks))
			mci_fifo_writel(host->fifo_reg, host->part_buf32);
	}
}

static void dw_mci_pull_data32(struct dw_mci *host, void *buf, int cnt)
{
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x3)) {
		while (cnt >= 4) {
			/* pull data from fifo into aligned buffer */
			u32 aligned_buf[32] = {0};
			int len = min(cnt & -4, (int)sizeof(aligned_buf));
			int items = len >> 2;
			int i;

			for (i = 0; i < items; ++i)
				aligned_buf[i] = mci_fifo_readl(host->fifo_reg);
			/* memcpy from aligned buffer into output buffer */
			memcpy(buf, aligned_buf, len);
			buf += len;
			cnt -= len;
		}
	} else
#endif
	{
		u32 *pdata = buf;

		for (; cnt >= 4; cnt -= 4)
			*pdata++ = mci_fifo_readl(host->fifo_reg);
		buf = pdata;
	}
	if (cnt) {
		host->part_buf32 = mci_fifo_readl(host->fifo_reg);
		dw_mci_pull_final_bytes(host, buf, cnt);
	}
}

static void dw_mci_push_data64(struct dw_mci *host, void *buf, int cnt)
{
	struct mmc_data *data = host->data;
	int init_cnt = cnt;

	/* try and push anything in the part_buf */
	if (unlikely(host->part_buf_count)) {
		int len = dw_mci_push_part_bytes(host, buf, cnt);

		buf += len;
		cnt -= len;

		if (host->part_buf_count == 8) {
			mci_fifo_writeq(host->fifo_reg,	host->part_buf);
			host->part_buf_count = 0;
		}
	}
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x7)) {
		while (cnt >= 8) {
			u64 aligned_buf[16] = {0};
			int len = min(cnt & -8, (int)sizeof(aligned_buf));
			int items = len >> 3;
			int i;
			/* memcpy from input buffer into aligned buffer */
			memcpy(aligned_buf, buf, len);
			buf += len;
			cnt -= len;
			/* push data from aligned buffer into fifo */
			for (i = 0; i < items; ++i)
				mci_fifo_writeq(host->fifo_reg,	aligned_buf[i]);
		}
	} else
#endif
	{
		u64 *pdata = buf;

		for (; cnt >= 8; cnt -= 8)
			mci_fifo_writeq(host->fifo_reg, *pdata++);
		buf = pdata;
	}
	/* put anything remaining in the part_buf */
	if (cnt) {
		dw_mci_set_part_bytes(host, buf, cnt);
		/* Push data if we have reached the expected data length */
		if ((data->bytes_xfered + init_cnt) ==
		    (data->blksz * data->blocks))
			mci_fifo_writeq(host->fifo_reg, host->part_buf);
	}
}

static void dw_mci_pull_data64(struct dw_mci *host, void *buf, int cnt)
{
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (unlikely((unsigned long)buf & 0x7)) {
		while (cnt >= 8) {
			/* pull data from fifo into aligned buffer */
			u64 aligned_buf[16];
			int len = min(cnt & -8, (int)sizeof(aligned_buf));
			int items = len >> 3;
			int i;

			for (i = 0; i < items; ++i)
				aligned_buf[i] = mci_fifo_readq(host->fifo_reg);

			/* memcpy from aligned buffer into output buffer */
			memcpy(buf, aligned_buf, len);
			buf += len;
			cnt -= len;
		}
	} else
#endif
	{
		u64 *pdata = buf;

		for (; cnt >= 8; cnt -= 8)
			*pdata++ = mci_fifo_readq(host->fifo_reg);
		buf = pdata;
	}
	if (cnt) {
		host->part_buf = mci_fifo_readq(host->fifo_reg);
		dw_mci_pull_final_bytes(host, buf, cnt);
	}
}

static void dw_mci_pull_data(struct dw_mci *host, void *buf, int cnt)
{
	int len;

	/* get remaining partial bytes */
	len = dw_mci_pull_part_bytes(host, buf, cnt);
	if (unlikely(len == cnt))
		return;
	buf += len;
	cnt -= len;

	/* get the rest of the data */
	host->pull_data(host, buf, cnt);
}

static void dw_mci_read_data_pio(struct dw_mci *host, bool dto)
{
	struct sg_mapping_iter *sg_miter = &host->sg_miter;
	void *buf;
	unsigned int offset;
	struct mmc_data	*data = host->data;
	int shift = host->data_shift;
	u32 status;
	unsigned int len;
	unsigned int remain, fcnt;

	do {
		if (!sg_miter_next(sg_miter))
			goto done;

		host->sg = sg_miter->piter.sg;
		buf = sg_miter->addr;
		remain = sg_miter->length;
		offset = 0;

		do {
			fcnt = (SDMMC_GET_FCNT(mci_readl(host, STATUS))
					<< shift) + host->part_buf_count;
			len = min(remain, fcnt);
			if (!len)
				break;
			dw_mci_pull_data(host, (void *)(buf + offset), len);
			data->bytes_xfered += len;
			offset += len;
			remain -= len;
		} while (remain);

		sg_miter->consumed = offset;
		status = mci_readl(host, MINTSTS);
		mci_writel(host, RINTSTS, SDMMC_INT_RXDR);
	/* if the RXDR is ready read again */
	} while ((status & SDMMC_INT_RXDR) ||
		 (dto && SDMMC_GET_FCNT(mci_readl(host, STATUS))));

	if (!remain) {
		if (!sg_miter_next(sg_miter))
			goto done;
		sg_miter->consumed = 0;
	}
	sg_miter_stop(sg_miter);
	return;

done:
	sg_miter_stop(sg_miter);
	host->sg = NULL;
	smp_wmb(); /* drain writebuffer */
	set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
}

static void dw_mci_write_data_pio(struct dw_mci *host)
{
	struct sg_mapping_iter *sg_miter = &host->sg_miter;
	void *buf;
	unsigned int offset;
	struct mmc_data	*data = host->data;
	int shift = host->data_shift;
	u32 status;
	unsigned int len;
	unsigned int fifo_depth = host->fifo_depth;
	unsigned int remain, fcnt;

	do {
		if (!sg_miter_next(sg_miter))
			goto done;

		host->sg = sg_miter->piter.sg;
		buf = sg_miter->addr;
		remain = sg_miter->length;
		offset = 0;

		do {
			fcnt = ((fifo_depth -
				 SDMMC_GET_FCNT(mci_readl(host, STATUS)))
					<< shift) - host->part_buf_count;
			len = min(remain, fcnt);
			if (!len)
				break;
			host->push_data(host, (void *)(buf + offset), len);
			data->bytes_xfered += len;
			offset += len;
			remain -= len;
		} while (remain);

		sg_miter->consumed = offset;
		status = mci_readl(host, MINTSTS);
		mci_writel(host, RINTSTS, SDMMC_INT_TXDR);
	} while (status & SDMMC_INT_TXDR); /* if TXDR write again */

	if (!remain) {
		if (!sg_miter_next(sg_miter))
			goto done;
		sg_miter->consumed = 0;
	}
	sg_miter_stop(sg_miter);
	return;

done:
	sg_miter_stop(sg_miter);
	host->sg = NULL;
	smp_wmb(); /* drain writebuffer */
	set_bit(EVENT_XFER_COMPLETE, &host->pending_events);
}

static void dw_mci_cmd_interrupt(struct dw_mci *host, u32 status)
{
	if (!host->cmd_status)
		host->cmd_status = status;

	smp_wmb(); /* drain writebuffer */

	set_bit(EVENT_CMD_COMPLETE, &host->pending_events);
	tasklet_schedule(&host->tasklet);/*dw_mci_tasklet_func*/
}

static irqreturn_t dw_mci_interrupt(int irq, void *dev_id)
{
	struct dw_mci *host = dev_id;
	u32 pending;
	u32 i;
	u32 temp;

	pending = mci_readl(host, MINTSTS); /* read-only mask reg */

	if (pending) {
		
		/*
                 * DTO fix - version 2.10a and below, and only if internal DMA
                 * is configured.
                 */
                if (host->quirks & DW_MCI_QUIRK_IDMAC_DTO) {
                        if (!pending &&
                            ((mci_readl(host, STATUS) >> 17) & 0x1fff))
                                pending |= SDMMC_INT_DATA_OVER;
                }

		if (pending & SDMMC_INT_CMD_DONE) {
			u32 cmd = mci_readl(host, CMD) & 0x3f;
			if (cmd == SD_SWITCH_VOLTAGE &&
				!(mci_readl(host, STATUS) & SDMMC_STATUS_BUSY)) {
					pending |= SDMMC_INT_RTO;
			}
		}

		if (pending & SDMMC_INT_HLE) {
			mci_writel(host, RINTSTS, SDMMC_INT_HLE);
			host->cmd_status = pending;
			tasklet_schedule(&host->tasklet);/*dw_mci_tasklet_func*/
		}

		if (pending & DW_MCI_CMD_ERROR_FLAGS) {
			mci_writel(host, RINTSTS, DW_MCI_CMD_ERROR_FLAGS);
			host->cmd_status = pending;
			smp_wmb(); /* drain writebuffer */
			set_bit(EVENT_CMD_COMPLETE, &host->pending_events);
		}

		if (pending & SDMMC_INT_VOLT_SWITCH) {
			u32 cmd = mci_readl(host, CMD) & 0x3f;
			if (cmd == SD_SWITCH_VOLTAGE) {
				mci_writel(host, RINTSTS, SDMMC_INT_VOLT_SWITCH);

				dw_mci_cmd_interrupt(host, pending);

				mci_writel(host, RINTSTS, SDMMC_INT_RESP_ERR);
				temp = mci_readl(host, INTMASK);
				temp |= SDMMC_INT_RESP_ERR;
				mci_writel(host, INTMASK, temp);
			}
		}

		if (pending & DW_MCI_DATA_ERROR_FLAGS) {
			/* if there is an error report DATA_ERROR */
			mci_writel(host, RINTSTS, DW_MCI_DATA_ERROR_FLAGS);
			host->data_status = pending;
			smp_wmb(); /* drain writebuffer */
			set_bit(EVENT_DATA_ERROR, &host->pending_events);
			tasklet_schedule(&host->tasklet);/*dw_mci_tasklet_func*/
		}

		if (pending & SDMMC_INT_DATA_OVER) {
			if (host->quirks & DW_MCI_QUIRK_BROKEN_DTO)
				del_timer(&host->dto_timer);

			mci_writel(host, RINTSTS, SDMMC_INT_DATA_OVER);
			if (!host->data_status)
				host->data_status = pending;
			smp_wmb(); /* drain writebuffer */
			if (host->dir_status == DW_MCI_RECV_STATUS) {
				if (host->sg != NULL)
					dw_mci_read_data_pio(host, true);
			}
			set_bit(EVENT_DATA_COMPLETE, &host->pending_events);
			tasklet_schedule(&host->tasklet);/*dw_mci_tasklet_func*/
		}

		if (pending & SDMMC_INT_RXDR) {
			mci_writel(host, RINTSTS, SDMMC_INT_RXDR);
			if (host->dir_status == DW_MCI_RECV_STATUS && host->sg)
				dw_mci_read_data_pio(host, false);
		}

		if (pending & SDMMC_INT_TXDR) {
			mci_writel(host, RINTSTS, SDMMC_INT_TXDR);
			if (host->dir_status == DW_MCI_SEND_STATUS && host->sg)
				dw_mci_write_data_pio(host);
		}

		if (pending & SDMMC_INT_CMD_DONE) {
			mci_writel(host, RINTSTS, SDMMC_INT_CMD_DONE);
			dw_mci_cmd_interrupt(host, pending);
		}

		if (pending & SDMMC_INT_CD) {
			mci_writel(host, RINTSTS, SDMMC_INT_CD);
			queue_work(host->card_workqueue, &host->card_work);/*dw_mci_work_routine_card*/
		}

		/* Handle SDIO Interrupts */
		for (i = 0; i < host->num_slots; i++) {
			struct dw_mci_slot *slot = host->slot[i];
			if (pending & SDMMC_INT_SDIO(i)) {
				mci_writel(host, RINTSTS, SDMMC_INT_SDIO(i));
				mmc_signal_sdio_irq(slot->mmc);
			}
		}

	}

	if (host->use_dma != TRANS_MODE_IDMAC)
		return IRQ_HANDLED;


#ifdef CONFIG_MMC_DW_IDMAC
	if(SDMMC_32_BIT_DMA == host->dma_64bit_address) {
		/* Handle 32 bit DMA interrupts */
		pending = mci_readl(host, IDSTS);
		if (pending & (SDMMC_IDMAC_INT_TI | SDMMC_IDMAC_INT_RI)) {
			mci_writel(host, IDSTS, SDMMC_IDMAC_INT_TI | SDMMC_IDMAC_INT_RI);
			mci_writel(host, IDSTS, SDMMC_IDMAC_INT_NI);
			host->dma_ops->complete(host);
		}
	} else {
		pending = mci_readl(host, IDSTS64);
		if (pending & (SDMMC_IDMAC_INT_TI | SDMMC_IDMAC_INT_RI)) {
			mci_writel(host, IDSTS64, SDMMC_IDMAC_INT_TI |
							SDMMC_IDMAC_INT_RI);
			mci_writel(host, IDSTS64, SDMMC_IDMAC_INT_NI);
			host->dma_ops->complete((void *)host);
		}
	}
#endif

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
/* given a slot, find out the device node representing that slot */
static struct device_node *dw_mci_of_find_slot_node(struct dw_mci_slot *slot)
{
        struct device *dev = slot->mmc->parent;
        struct device_node *np;
        const __be32 *addr;
        int len;

        if (!dev || !dev->of_node)
                return NULL;

        for_each_child_of_node(dev->of_node, np) {
                addr = of_get_property(np, "reg", &len);
                if (!addr || (len < sizeof(int)))/*lint !e574*/
                        continue;
                if (be32_to_cpup(addr) == slot->id)
                        return np;
        }
        return NULL;
}

static void dw_mci_slot_of_parse(struct dw_mci_slot *slot)
{
	struct device_node *np = dw_mci_of_find_slot_node(slot);

	if (!np)
		return;

	if (of_property_read_bool(np, "disable-wp")) {
		slot->mmc->caps2 |= MMC_CAP2_NO_WRITE_PROTECT;
		dev_warn(slot->mmc->parent,
			"Slot quirk 'disable-wp' is deprecated\n");
	}
}
/* find out bus-width for a given slot */
static u32 dw_mci_of_get_bus_wd(struct dw_mci_slot *slot)
{
        struct device *dev = slot->mmc->parent;
        struct device_node *np = dw_mci_of_find_slot_node(slot);
        u32 bus_wd = 1;

        if (!np)
                return 1;

        if (of_property_read_u32(np, "bus-width", &bus_wd))
                dev_warn(dev, "bus-width property not found, assuming width"
                               " as 1\n");
        return bus_wd;
}

/* find the write protect gpio for a given slot; or -1 if none specified */
static int dw_mci_of_get_wp_gpio(struct dw_mci_slot *slot)
{
        struct device_node *np = dw_mci_of_find_slot_node(slot);
        struct device *dev = slot->mmc->parent;
        int gpio;

        if (!np)
                return -EINVAL;

        gpio = of_get_named_gpio(np, "wp-gpios", 0);

        /* Having a missing entry is valid; return silently */
        if (!gpio_is_valid(gpio))
                return -EINVAL;

        if (devm_gpio_request(dev, gpio, "dw-mci-wp")) {
                dev_warn(dev, "gpio [%d] request failed\n", gpio);
                return -EINVAL;
        }

        return gpio;
}
#else /* CONFIG_OF */
static struct device_node *dw_mci_of_find_slot_node(struct dw_mci_slot *slot)
{
        return NULL;
}

tatic u32 dw_mci_of_get_bus_wd(struct dw_mci_slot *slot)
{
        return 1;
}
static int dw_mci_of_get_wp_gpio(struct dw_mci_slot *slot)
{
        return -EINVAL;
}
#endif /* CONFIG_OF */

static int dw_mci_init_slot(struct dw_mci *host, unsigned int id)
{
	struct mmc_host *mmc;
	struct dw_mci_slot *slot;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int ctrl_id, ret;
	u8 bus_width;

	mmc = mmc_alloc_host(sizeof(struct dw_mci_slot), host->dev);
	if (!mmc)
		return -ENOMEM;

	g_mmc_for_mmctrace[mmc->index] = mmc;
	printk(KERN_ERR "$$$$$$$$$$$$$$$$$$$$ %pK, %d \n", g_mmc_for_mmctrace[mmc->index], mmc->index );

	slot = mmc_priv(mmc);
	slot->id = id;
	slot->mmc = mmc;
	slot->host = host;
	host->slot[id] = slot;

	mmc->ops = &dw_mci_ops;
	mmc->f_min = DIV_ROUND_UP(host->bus_hz, 510);
	mmc->f_max = host->bus_hz;

	mmc->sdio_present = 0;

	if (host->pdata->get_ocr)
		mmc->ocr_avail = host->pdata->get_ocr(id);
	else
		mmc->ocr_avail = MMC_VDD_28_29;

	/*
	 * Start with slot power disabled, it will be enabled when a card
	 * is detected.
	 */
	if (host->pdata->setpower)
		host->pdata->setpower(id, 0);


	if (host->pdata->caps)
		mmc->caps = host->pdata->caps;

	if (host->pdata->pm_caps) {
		mmc->pm_caps = host->pdata->pm_caps;
		mmc->pm_flags = mmc->pm_caps;
	}

	if (host->dev->of_node) {
		ctrl_id = of_alias_get_id(host->dev->of_node, "mshc");
		if (ctrl_id < 0)
			ctrl_id = 0;
	} else {
		ctrl_id = to_platform_device(host->dev)->id;
	}

	if (drv_data && drv_data->caps)
		mmc->caps |= drv_data->caps[ctrl_id];

	if (host->pdata->caps2)
		mmc->caps2 = host->pdata->caps2;

	dw_mci_slot_of_parse(slot);
	if (host->pdata->get_bus_wd)
		bus_width = host->pdata->get_bus_wd(slot->id);
	else if (host->dev->of_node)
		bus_width = dw_mci_of_get_bus_wd(slot);
	else
		bus_width = 1;
	/*lint -save -e616*/
	switch (bus_width) {
	case 8:
		mmc->caps |= MMC_CAP_8_BIT_DATA;
	case 4:
		mmc->caps |= MMC_CAP_4_BIT_DATA;
	}
	/*lint -restore*/
	if (host->pdata->quirks & DW_MCI_QUIRK_HIGHSPEED)
		mmc->caps |= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED;


	/* Useful defaults if platform data is unset. */
	if (host->use_dma == TRANS_MODE_IDMAC) {
		mmc->max_segs = host->ring_size;
		mmc->max_blk_size = 65535;
		mmc->max_seg_size = 0x1000;
		mmc->max_req_size = mmc->max_seg_size * host->ring_size;
		mmc->max_blk_count = mmc->max_req_size / 512;
	} else if (host->use_dma == TRANS_MODE_EDMAC) {
		mmc->max_segs = 64;
		mmc->max_blk_size = 65535;
		mmc->max_blk_count = 65535;
		mmc->max_req_size =
				mmc->max_blk_size * mmc->max_blk_count;
		mmc->max_seg_size = mmc->max_req_size;
	} else {
		/* TRANS_MODE_PIO */
		mmc->max_segs = 64;
		mmc->max_blk_size = 65535; /* BLKSIZ is 16 bits */
		mmc->max_blk_count = 512;
		mmc->max_req_size = mmc->max_blk_size *
				    mmc->max_blk_count;
		mmc->max_seg_size = mmc->max_req_size;
	}

	host->pinctrl = devm_pinctrl_get(mmc_dev(mmc));
	if (IS_ERR(host->pinctrl)) {
		dev_warn(host->dev, "could not get pinctrl\n");
		host->pinctrl = NULL;
	}

	host->pins_default = pinctrl_lookup_state(host->pinctrl,
			PINCTRL_STATE_DEFAULT);

	/* enable pins to be muxed in and configured */
	if (IS_ERR(host->pins_default)) {
		dev_warn(host->dev, "could not get default pinstate\n");
		host->pins_default = NULL;
	}

	host->pins_idle = pinctrl_lookup_state(host->pinctrl,
			PINCTRL_STATE_IDLE);
	if (IS_ERR(host->pins_idle)) {
		dev_warn(host->dev, "could not get default pinstate\n");
		host->pins_idle = NULL;
	}

	/*
	 * comment only for balong
    	 * sd:vmmc :ldo10 卡侧电压；
       	 * vqmmc:ldo7  I/O 上拉；
       	 * buck2/ldo5 硬件确认不需要控制，包括低功耗模式下；

    	 * mmc:vmmc:ldo19 vcc 常供3V；
         * vqmmc:ldo5 vccq 单板长供，软件不设置
	*/
	host->vmmc = devm_regulator_get(mmc_dev(mmc), "vmmc");
	if (IS_ERR(host->vmmc)) {
		pr_info("%s: no vmmc regulator found\n", mmc_hostname(mmc));
		host->vmmc = NULL;
	}/* else {
		ret = regulator_enable(host->vmmc);
		if (ret) {
			dev_err(host->dev,
				"failed to enable regulator: %d\n", ret);
			goto err_setup_bus;
		}
	}*/

	host->vqmmc = devm_regulator_get(mmc_dev(mmc), "vqmmc");
	if (IS_ERR(host->vqmmc)) {
		pr_warning("%s: no vqmmc regulator found\n", mmc_hostname(mmc));
		host->vqmmc = NULL;
	} /*else {
		ret = regulator_enable(host->vqmmc);
		if (ret) {
			dev_err(host->dev,
				"failed to enable regulator vqmmc: %d\n", ret);
			goto err_setup_bus;
		}
	}*/

	if(1 == host->sd_vmmcmosen_switch)
	{
		host->vmmcmosen = devm_regulator_get(mmc_dev(mmc), "vmmcmosen");
		if (IS_ERR(host->vmmcmosen)) {
			pr_warning("%s: no vmmcmosen regulator found\n", mmc_hostname(mmc));
			host->vmmcmosen = NULL;
		}
	}
	else
	{
		host->vmmcmosen = NULL;
	}

	if (drv_data && drv_data->cd_detect_init)
		drv_data->cd_detect_init(host);

	if (dw_mci_get_cd(mmc))
		set_bit(DW_MMC_CARD_PRESENT, &slot->flags);
	else
		clear_bit(DW_MMC_CARD_PRESENT, &slot->flags);
	slot->wp_gpio = dw_mci_of_get_wp_gpio(slot);

	ret = mmc_add_host(mmc);
	if (ret)
		goto err_setup_bus;

#if defined(CONFIG_DEBUG_FS)
	dw_mci_init_debugfs(slot);
#endif

	/* Card initially undetected */
	slot->last_detect_state = 0;

	return 0;

err_setup_bus:
	mmc_free_host(mmc);
	return -EINVAL;
}

static void dw_mci_cleanup_slot(struct dw_mci_slot *slot, unsigned int id)
{
	/* Shutdown detect IRQ */
	if (slot->host->pdata->exit)
		slot->host->pdata->exit(id);

	/* Debugfs stuff is cleaned up by mmc core */
	mmc_remove_host(slot->mmc);
	slot->host->slot[id] = NULL;
	mmc_free_host(slot->mmc);
}

static void dw_mci_init_dma(struct dw_mci *host)
{
	int addr_config = 0;
	struct device *dev = host->dev;

	/*
	* Check tansfer mode from HCON[17:16]
	* Clear the ambiguous description of dw_mmc databook:
	* 2b'00: No DMA Interface -> Actually means using Internal DMA block
	* 2b'01: DesignWare DMA Interface -> Synopsys DW-DMA block
	* 2b'10: Generic DMA Interface -> non-Synopsys generic DMA block
	* 2b'11: Non DW DMA Interface -> pio only
	* Compared to DesignWare DMA Interface, Generic DMA Interface has a
	* simpler request/acknowledge handshake mechanism and both of them
	* are regarded as external dma master for dw_mmc.
	*/
	host->use_dma = SDMMC_GET_TRANS_MODE(mci_readl(host, HCON));
	if (host->use_dma == DMA_INTERFACE_IDMA) {
		host->use_dma = TRANS_MODE_IDMAC;
	} else if (host->use_dma == DMA_INTERFACE_DWDMA ||
		   host->use_dma == DMA_INTERFACE_GDMA) {
		host->use_dma = TRANS_MODE_EDMAC;
	} else {
		goto no_dma;
	}

	/* Determine which DMA interface to use */
	if (host->use_dma == TRANS_MODE_IDMAC) {
		/*
		* Check ADDR_CONFIG bit in HCON to find
		* IDMAC address bus width
		*/
		addr_config = SDMMC_GET_ADDR_CONFIG(mci_readl(host, HCON));

		if (1 == addr_config) {
			/* host supports IDMAC in 64-bit address mode */
			host->desc_sz = DW_MCI_DESC_SZ_64BIT;
			host->dma_64bit_address = SDMMC_64_BIT_DMA;
			dev_info(host->dev, "IDMAC supports 64-bit address mode.\n");
			host->dma_mask = DMA_BIT_MASK(64);/*lint !e598 !e648*/
			host->dev->dma_mask = &host->dma_mask;
		} else {
			/* host supports IDMAC in 32-bit address mode */
			host->desc_sz = DW_MCI_DESC_SZ;
			host->dma_64bit_address = SDMMC_32_BIT_DMA;
			dev_info(host->dev, "IDMAC supports 32-bit address mode.\n");
			host->dma_mask = DMA_BIT_MASK(32);
			host->dev->dma_mask = &host->dma_mask;
		}


		/* Alloc memory for sg translation */
		host->sg_cpu = dmam_alloc_coherent(host->dev, host->desc_sz * DESC_RING_BUF_SZ,
						   &host->sg_dma, GFP_KERNEL);
		if (!host->sg_cpu) {
			dev_err(host->dev,
				"%s: could not alloc DMA memory\n",
				__func__);
			goto no_dma;
		}

		host->dma_ops = &dw_mci_idmac_ops;
		dev_info(host->dev, "Using internal DMA controller.\n");
	} else {
		/* TRANS_MODE_EDMAC: check dma bindings again */
		if ((device_property_read_string_array(dev, "dma-names",
						       NULL, 0) < 0) ||
		    !device_property_present(dev, "dmas")) {
			goto no_dma;
		}
		host->dma_ops = &dw_mci_edmac_ops;
		dev_info(host->dev, "Using external DMA controller.\n");
	}

	if (host->dma_ops->init && host->dma_ops->start &&
	    host->dma_ops->stop && host->dma_ops->cleanup) {
		if (host->dma_ops->init(host)) {
			dev_err(host->dev, "%s: Unable to initialize DMA Controller.\n",
				__func__);
			goto no_dma;
		}
	} else {
		dev_err(host->dev, "DMA initialization not found.\n");
		goto no_dma;
	}

	return;

no_dma:
	dev_info(host->dev, "Using PIO mode.\n");
	host->use_dma = TRANS_MODE_PIO;
}

static void dw_mci_dto_timer(unsigned long arg)
{
	struct dw_mci *host = (struct dw_mci *)arg;

	switch (host->state) {
	case STATE_SENDING_DATA:
	case STATE_DATA_BUSY:
		/*
		 * If DTO interrupt does NOT come in sending data state,
		 * we should notify the driver to terminate current transfer
		 * and report a data timeout to the core.
		 */
		host->data_status = SDMMC_INT_DRTO;
		set_bit(EVENT_DATA_ERROR, &host->pending_events);
		set_bit(EVENT_DATA_COMPLETE, &host->pending_events);
		tasklet_schedule(&host->tasklet);
		break;
	default:
		break;
	}
}


#ifdef CONFIG_OF
static struct dw_mci_of_quirks {
        char *quirk;
        int id;
} of_quirks[] = {
        {
                .quirk  = "supports-highspeed",
                .id     = DW_MCI_QUIRK_HIGHSPEED,
        }, {
                .quirk  = "broken-cd",
                .id     = DW_MCI_QUIRK_BROKEN_CARD_DETECTION,
        },
};

static void dw_mci_parse_dt_extend(struct dw_mci_board *pdata, struct device_node *np)
{
	if (of_find_property(np, "non-removable", NULL))
		pdata->caps |= MMC_CAP_NONREMOVABLE;
}
static struct dw_mci_board *dw_mci_parse_dt(struct dw_mci *host)
{
	struct dw_mci_board *pdata;
	struct device *dev = host->dev;
	struct device_node *np = dev->of_node;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int idx, ret;
	u32 factory_mode = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "could not allocate memory for pdata\n");
		return ERR_PTR(-ENOMEM);
	}

	/* find reset controller when exist */
	pdata->rstc = devm_reset_control_get_optional(dev, "reset");
	if (IS_ERR(pdata->rstc)) {
		if (PTR_ERR(pdata->rstc) == -EPROBE_DEFER)
			return ERR_PTR(-EPROBE_DEFER);/*lint !e429*/
	}

	/* find out number of slots supported */
	if (of_property_read_u32(dev->of_node, "num-slots",
				&pdata->num_slots)) {
		dev_info(dev,
			 "num-slots property not found, assuming 1 slot is available\n");
		pdata->num_slots = 1;
	}

	/* get quirks */
	for (idx = 0; idx < ARRAY_SIZE(of_quirks); idx++)/*lint !e574*/
		if (of_get_property(np, of_quirks[idx].quirk, NULL))
			pdata->quirks |= of_quirks[idx].id;

	if (of_property_read_u32(np, "fifo-depth", &pdata->fifo_depth))
		dev_info(dev,
			 "fifo-depth property not found, using value of FIFOTH register as default\n");

	if (of_property_read_u32(np, "card-detect-delay", &pdata->detect_delay_ms))
		dev_info(dev, "card-detect-delay property not found, "
				"using default value\n");

	if (drv_data && drv_data->parse_dt) {
		ret = drv_data->parse_dt(host);
		if (ret)
			return ERR_PTR(ret);/*lint !e429*/
	}

	if (of_find_property(np, "keep-power-in-suspend", NULL))
		pdata->pm_caps |= MMC_PM_KEEP_POWER;

	if (of_find_property(np, "keep-power-ignore-pm-notify", NULL))
		pdata->pm_caps |= MMC_PM_IGNORE_PM_NOTIFY;

	if (of_find_property(np, "enable-sdio-wakeup", NULL))
		pdata->pm_caps |= MMC_PM_WAKE_SDIO_IRQ;

	if (of_find_property(np, "caps2-mmc-packed-command", NULL))
		pdata->caps2 |= MMC_CAP2_PACKED_CMD;

	if (of_find_property(np, "caps2-mmc-hs200-1_8v", NULL))
		pdata->caps2 |= MMC_CAP2_HS200_1_8V_SDR;

	if (of_find_property(np, "caps2-mmc-hs200-1_2v", NULL))
		pdata->caps2 |= MMC_CAP2_HS200_1_2V_SDR;

	if (of_find_property(np, "caps2-mmc-cache-ctrl", NULL))
	{
		dev_info(dev, "caps2-mmc-cache-ctrl is set in dts.\n");
#ifdef CONFIG_HISI_CMDLINE_PARSE
		factory_mode = runmode_is_factory();
#endif
		if(!factory_mode) {
			dev_info(dev, "normal mode cache ctrl on\n");
			pdata->caps2 |= MMC_CAP2_CACHE_CTRL;
		} else {
			dev_info(dev, "factory mode cache ctrl off\n");
		}
	}

	if (of_find_property(np, "caps2-wifi-support-cmd11", NULL)
		&& of_property_count_u8_elems(np, "caps2-wifi-support-cmd11") <= 0) {
		dev_info(dev, "dw_mci:%d %s find wifi support cmd11 in dts\n",
									host->hw_mmc_id,  __func__);
		pdata->caps2 |= MMC_CAP2_SUPPORT_WIFI_CMD11;
	}

	if (of_find_property(np, "caps-wifi-no-lowpwr", NULL)
		&& of_property_count_u8_elems(np, "caps-wifi-no-lowpwr") <= 0) {
		dev_info(dev, "dw_mci:%d %s find wifi support no_lowpwr in dts\n",
									host->hw_mmc_id,  __func__);
		pdata->caps2 |= MMC_CAP2_WIFI_NO_LOWPWR;
	}

	if (of_find_property(np, "caps2-support-wifi", NULL)) {
		pr_info("dw_mci:%d %s find caps2-support-wifi in dts\n", host->hw_mmc_id,  __func__);
		pdata->caps2 |= MMC_CAP2_SUPPORT_WIFI;
	}

	if (of_find_property(np, "caps2-support-via-modem", NULL)) {
		pr_info("dw_mci:%d %s find caps2-support-via-modem in dts\n", host->hw_mmc_id,  __func__);
		pdata->caps2 |= MMC_CAP2_SUPPORT_VIA_MODEM;
	}

	if (of_find_property(np, "full-pwr-cycle", NULL))
		pdata->caps2 |= MMC_CAP2_FULL_PWR_CYCLE;

	if (of_find_property(np, "caps2-mmc-ddr50-notify", NULL))
	{
	    if(drv_data && drv_data->caps)
		drv_data->caps[0] |= MMC_CAP_1_8V_DDR|MMC_CAP_UHS_DDR50;
	}

	dw_mci_parse_dt_extend(pdata, np);

	return pdata;
}

#else /* CONFIG_OF */
static struct dw_mci_board *dw_mci_parse_dt(struct dw_mci *host)
{
	return ERR_PTR(-EINVAL);
}
#endif /* CONFIG_OF */

static void dw_mci_enable_cd(struct dw_mci *host)
{
	struct dw_mci_board *brd = host->pdata;
	u32 temp;

	 /* No need for CD if broken card detection */
	if (brd->quirks & DW_MCI_QUIRK_BROKEN_CARD_DETECTION)
                return;

	temp = mci_readl(host, INTMASK);
	temp  |= SDMMC_INT_CD;
	mci_writel(host, INTMASK, temp);
}

int dw_mci_probe(struct dw_mci *host)
{
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int width, i, ret = 0;
	u32 fifo_size;
	int init_slots = 0;

	if (!host->pdata) {
		host->pdata = dw_mci_parse_dt(host);
		if (PTR_ERR(host->pdata) == -EPROBE_DEFER) {
			return -EPROBE_DEFER;
		} else if (IS_ERR(host->pdata)) {
			dev_err(host->dev, "platform data not available\n");
			return -EINVAL;
		}
	}
	if (!host->pdata->select_slot && host->pdata->num_slots < 1) {

		dev_err(host->dev,
			"Platform data must supply select_slot function\n");
		return -ENODEV;
	}

	host->biu_clk = devm_clk_get(host->dev, "biu");
	if (IS_ERR(host->biu_clk)) {
		dev_dbg(host->dev, "biu clock not available\n");
	} else {
		ret = clk_prepare_enable(host->biu_clk);
		if (ret) {
			dev_err(host->dev, "failed to enable biu clock\n");
			return ret;
		}
	}

	host->ciu_clk = devm_clk_get(host->dev, "ciu");
	if (IS_ERR(host->ciu_clk)) {
		dev_dbg(host->dev, "ciu clock not available\n");
	} else {
		ret = clk_prepare_enable(host->ciu_clk);
		if (ret) {
			dev_err(host->dev, "failed to enable ciu clock\n");
			goto err_clk_biu;
		}
	}

#ifdef CONFIG_SD_TIMEOUT_RESET
	host->volt_hold_sd_clk = devm_clk_get(host->dev, "volt_hold");
	if (IS_ERR(host->volt_hold_sd_clk)) {
		dev_dbg(host->dev, "volt_hold_sd_clk clock not available\n");
	}

	host->volt_hold_sdio_clk = devm_clk_get(host->dev, "volt_hold_sdio");
	if (IS_ERR(host->volt_hold_sdio_clk)) {
		dev_dbg(host->dev, "volt_hold_sdio_clk clock not available\n");
	}
#endif

	if (IS_ERR(host->ciu_clk))
		host->bus_hz = host->pdata->bus_hz;
	else
		host->bus_hz = clk_get_rate(host->ciu_clk);

	host->parent_clk = clk_get_parent(host->ciu_clk);
	if (IS_ERR_OR_NULL(host->parent_clk)) {
		dev_err(host->dev, "failed to get ciu clock parent\n");
		goto err_clk_ciu;
	}

	if (!IS_ERR(host->pdata->rstc)) {
		reset_control_assert(host->pdata->rstc);
		usleep_range(10, 50);
		reset_control_deassert(host->pdata->rstc);
	}

	if (drv_data && drv_data->setup_clock) {
		ret = drv_data->setup_clock(host);
		if (ret) {
			dev_err(host->dev,
				"implementation specific clock setup failed\n");
			goto err_clk_ciu;
		}
	}

	if (!host->bus_hz) {
		dev_err(host->dev,
			"Platform data must supply bus speed\n");
		ret = -ENODEV;
		goto err_clk_ciu;
	}

	host->quirks = host->pdata->quirks;
	host->sd_reinit = 0;
	host->sd_hw_timeout = 0;
	
	if (host->quirks & DW_MCI_QUIRK_BROKEN_DTO)
                setup_timer(&host->dto_timer,
                            dw_mci_dto_timer, (unsigned long)host);

	spin_lock_init(&host->lock);
	INIT_LIST_HEAD(&host->queue);

	/*
	 * Get the host data width - this assumes that HCON has been set with
	 * the correct values.
	 */
	i = SDMMC_GET_HDATA_WIDTH(mci_readl(host, HCON));
	if (!i) {
		host->push_data = dw_mci_push_data16;
		host->pull_data = dw_mci_pull_data16;
		width = 16;
		host->data_shift = 1;
	} else if (i == 2) {
		host->push_data = dw_mci_push_data64;
		host->pull_data = dw_mci_pull_data64;
		width = 64;
		host->data_shift = 3;
	} else {
		/* Check for a reserved value, and warn if it is */
		WARN((i != 1),
		     "HCON reports a reserved host data width!\n"
		     "Defaulting to 32-bit access.\n");
		host->push_data = dw_mci_push_data32;
		host->pull_data = dw_mci_pull_data32;
		width = 32;
		host->data_shift = 2;
	}

	/* Reset all blocks */
	if (!mci_wait_reset(host->dev, host)){
		ret = -ENODEV;
		goto err_clk_ciu;
	}

	host->dma_ops = host->pdata->dma_ops;
	dw_mci_init_dma(host);

	/* Clear the interrupts for the host controller */
	mci_writel(host, RINTSTS, 0xFFFFFFFF);

	/* disable all mmc interrupt first */
	mci_writel(host, INTMASK, 0);

	/* Put in max timeout */
	mci_writel(host, TMOUT, 0xFFFFFFFF);

	/*
	 * FIFO threshold settings  RxMark  = fifo_size / 2 - 1,
	 *                          Tx Mark = fifo_size / 2 DMA Size = 8
	 */
	if (!host->pdata->fifo_depth) {
		/*
		 * Power-on value of RX_WMark is FIFO_DEPTH-1, but this may
		 * have been overwritten by the bootloader, just like we're
		 * about to do, so if you know the value for your hardware, you
		 * should put it in the platform data.
		 */
		fifo_size = mci_readl(host, FIFOTH);
		fifo_size = 1 + ((fifo_size >> 16) & 0xfff);
	} else {
		fifo_size = host->pdata->fifo_depth;
	}
	host->fifo_depth = fifo_size;
	host->fifoth_val =
		SDMMC_SET_FIFOTH(0x2, fifo_size / 2 - 1, fifo_size / 2);
	mci_writel(host, FIFOTH, host->fifoth_val);

	/* disable clock to CIU */
	mci_writel(host, CLKENA, 0);
	mci_writel(host, CLKSRC, 0);

	/*
	 * In 2.40a spec, Data offset is changed.
	 * Need to check the version-id and set data-offset for DATA register.
	 */
	host->verid = SDMMC_GET_VERID(mci_readl(host, VERID));
	dev_info(host->dev, "Version ID is %04x\n", host->verid);

	if (host->verid < DW_MMC_240A)
		host->fifo_reg = host->regs + DATA_OFFSET;
	else
		host->fifo_reg = host->regs + DATA_240A_OFFSET;

	tasklet_init(&host->tasklet, dw_mci_tasklet_func, (unsigned long)host);

	host->card_workqueue = alloc_workqueue("dw-mci-card/%d",
			WQ_MEM_RECLAIM, 1, host->hw_mmc_id);

	if (!host->card_workqueue)
		goto err_dmaunmap;

	INIT_WORK(&host->card_work, dw_mci_work_routine_card);

#ifdef CONFIG_SD_TIMEOUT_RESET
	INIT_WORK(&host->work_volt_mmc, dw_mci_work_volt_mmc_func);
#endif

	setup_timer(&host->timer, dw_mci_timeout_timer, (unsigned long)host);

	ret = devm_request_irq(host->dev, host->irq, dw_mci_interrupt,
			       host->irq_flags, "dw-mci", host);
	if (ret)
		goto err_workqueue;

	if (host->pdata->num_slots)
		host->num_slots = host->pdata->num_slots;
	else
		host->num_slots = 1;

	if (host->num_slots < 1 ||
	    host->num_slots > SDMMC_GET_SLOT_NUM(mci_readl(host, HCON))) {
		dev_err(host->dev,
			"Platform data must supply correct num_slots.\n");
		ret = -ENODEV;
		goto err_clk_ciu;
	}

	/*
	 * Enable interrupts for command done, data over, data empty, card det,
	 * receive ready and error such as transmit, receive timeout, crc error
	 */
#ifdef CONFIG_MMC_DW_IDMAC
	if(SDMMC_32_BIT_DMA == host->dma_64bit_address)
		mci_writel(host, IDSTS, IDMAC_INT_CLR); //32 bit dma interrupt clear
	else
		mci_writel(host, IDSTS64, IDMAC_INT_CLR); //64 bit dma interrupt clear
#endif
	mci_writel(host, INTMASK, SDMMC_INT_CMD_DONE | SDMMC_INT_DATA_OVER |
		   SDMMC_INT_TXDR | SDMMC_INT_RXDR |
		   DW_MCI_ERROR_FLAGS);
	/* Enable mci interrupt */
	mci_writel(host, CTRL, SDMMC_CTRL_INT_ENABLE);

	dev_info(host->dev,
		 "DW MMC controller at irq %d,%d bit host data width,%u deep fifo\n",
		 host->irq, width, fifo_size);

	/*每个IP可以支持多张卡*/
	/* We need at least one slot to succeed */
	for (i = 0; i < host->num_slots; i++) {/*lint !e574*/
		ret = dw_mci_init_slot(host, i);
		if (ret)
			dev_dbg(host->dev, "slot %d init failed\n", i);
		else
			init_slots++;
	}

	/* Now that slots are all setup, we can enable card detect */
	dw_mci_enable_cd(host);

	if (init_slots) {
		dev_info(host->dev, "%d slots initialized\n", init_slots);
	} else {
		dev_dbg(host->dev,
			"attempted to initialize %d slots, but failed on all\n",
			host->num_slots);
		goto err_workqueue;
	}

	/* Now that slots are all setup, we can enable card detect */
	dw_mci_enable_cd(host);
	if (host->quirks & DW_MCI_QUIRK_IDMAC_DTO)
                dev_info(host->dev, "Internal DMAC interrupt fix enabled.\n");

	return 0;

err_workqueue:
	destroy_workqueue(host->card_workqueue);

err_dmaunmap:
	if (host->use_dma && host->dma_ops->exit)
		host->dma_ops->exit(host);

	if (!IS_ERR(host->pdata->rstc))
		reset_control_assert(host->pdata->rstc);

	if(host->vmmc) {
		ret = regulator_disable(host->vmmc);
		if (ret)
			dev_warn(host->dev, "regulator_disable failed ret = %d \n",ret);
	}

err_clk_ciu:
	if (!IS_ERR(host->ciu_clk))
		clk_disable_unprepare(host->ciu_clk);
err_clk_biu:
	if (!IS_ERR(host->biu_clk))
		clk_disable_unprepare(host->biu_clk);
	return ret;
}
EXPORT_SYMBOL(dw_mci_probe);

void dw_mci_remove(struct dw_mci *host)
{
	int i;

	for (i = 0; i < host->num_slots; i++) {/*lint !e574*/
		dev_dbg(host->dev, "remove slot %d\n", i);
		if (host->slot[i])
			dw_mci_cleanup_slot(host->slot[i], i);
	}

	mci_writel(host, RINTSTS, 0xFFFFFFFF);
	mci_writel(host, INTMASK, 0); /* disable all mmc interrupt first */

	host->flags &= ~DWMMC_IN_TUNING;
	host->flags &= ~DWMMC_TUNING_DONE;
	host->sd_reinit = 0;


	/* disable clock to CIU */
	mci_writel(host, CLKENA, 0);
	mci_writel(host, CLKSRC, 0);

	del_timer_sync(&host->timer);
	destroy_workqueue(host->card_workqueue);

	if (host->use_dma && host->dma_ops->exit)
		host->dma_ops->exit(host);

	if (host->vmmc) {
		if(regulator_disable(host->vmmc))
			dev_warn(host->dev, "regulator_disable vmmc failed\n");
	}
	if (host->vqmmc) {
		if(regulator_disable(host->vqmmc))
		    dev_warn(host->dev, "regulator_disable vqmmc failed\n");
	}

	if (!IS_ERR(host->pdata->rstc))
                reset_control_assert(host->pdata->rstc);

	clk_disable_unprepare(host->ciu_clk);
	clk_disable_unprepare(host->biu_clk);
}
EXPORT_SYMBOL(dw_mci_remove);



#ifdef CONFIG_PM_SLEEP
/*
 * TODO: we should probably disable the clock to the card in the suspend path.
 */
int dw_mci_suspend(struct dw_mci *host)
{
	if (host->use_dma && host->dma_ops->exit)
		host->dma_ops->exit(host);

	return 0;
}
EXPORT_SYMBOL(dw_mci_suspend);

int dw_mci_resume(struct dw_mci *host)
{
	int i, ret;

	if (!mci_wait_reset(host->dev, host)) {
		ret = -ENODEV;
		return ret;
	}

	if (host->use_dma && host->dma_ops->init)
		host->dma_ops->init(host);

	/* Restore the old value at FIFOTH register */
	mci_writel(host, FIFOTH, host->fifoth_val);
	host->prev_blksz = 0;

	mci_writel(host, RINTSTS, 0xFFFFFFFF);
	mci_writel(host, INTMASK, SDMMC_INT_CMD_DONE | SDMMC_INT_DATA_OVER |
		   SDMMC_INT_TXDR | SDMMC_INT_RXDR |
		   DW_MCI_ERROR_FLAGS);
	mci_writel(host, CTRL, SDMMC_CTRL_INT_ENABLE);

	for (i = 0; i < host->num_slots; i++) {/*lint !e574*/
		struct dw_mci_slot *slot = host->slot[i];

		if (!slot)
			continue;
		if(slot->mmc->ios.power_mode == MMC_POWER_OFF)
			continue;

		if (slot->mmc->pm_flags & MMC_PM_KEEP_POWER) {
			dw_mci_set_ios(slot->mmc, &slot->mmc->ios);
			dw_mci_setup_bus(slot, true);
		}
	}

	/* Now that slots are all setup, we can enable card detect */
	dw_mci_enable_cd(host);

	return 0;
}
EXPORT_SYMBOL(dw_mci_resume);
#endif /* CONFIG_PM_SLEEP */

static int __init dw_mci_init(void)
{
	pr_info("Synopsys Designware Multimedia Card Interface Driver\n");
	return 0;
}

static void __exit dw_mci_exit(void)
{
}

int mmc_blk_is_retryable(struct mmc_host *mmc)
{
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *host = slot->host;
	return host->is_reset_after_retry;
}

int sd_need_retry(struct mmc_card *card, int retry)
{
	if (mmc_card_sd(card) && mmc_blk_is_retryable(card->host)) {
		if (retry < 5)
			return 1;
	}
	return 0;
}

module_init(dw_mci_init);
module_exit(dw_mci_exit);

MODULE_DESCRIPTION("DW Multimedia Card Interface driver");
MODULE_AUTHOR("NXP Semiconductor VietNam");
MODULE_AUTHOR("Imagination Technologies Ltd");
MODULE_LICENSE("GPL v2");
#pragma GCC diagnostic pop
