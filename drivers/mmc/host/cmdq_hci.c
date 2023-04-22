/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/leds.h>
#include <linux/platform_device.h>

#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/mmc/cmdq_hci.h>
#include <linux/pm_runtime.h>
#include "sdhci.h"
#ifdef CONFIG_HISI_DEBUG_FS
#include "hisi_mmc_debug.h"
#define CMDQ_DEBUG
#endif
#ifdef CONFIG_HUAWEI_EMMC_DSM
extern void sdhci_dsm_handle(struct sdhci_host *host, struct mmc_request *mrq);
extern void sdhci_dsm_set_host_status(struct sdhci_host *host, u32 error_bits);
#endif

/* 1 sec FIXME: optimize it */
#define HALT_TIMEOUT_MS 1000
#define CLEAR_TIMEOUT_MS 1000

#define CMDQ_TASK_TIMEOUT_MS 29000

#define DRV_NAME "cmdq-host"
#define INT2BOOL(x)  ((x)? 1:0)

extern int sdhci_get_cmd_err(u32 intmask);
extern int sdhci_get_data_err(u32 intmask);

void cmdq_dump_task_history(struct cmdq_host *cq_host)
{
	struct mmc_host *mmc = cq_host->mmc;
	struct mmc_cmdq_task_info *cmdq_task_info = mmc->cmdq_task_info;
	unsigned long active_reqs = mmc->cmdq_ctx.active_reqs;
	u32 tag;

	pr_err(DRV_NAME ": current time:0x%lld\n", ktime_to_ns(ktime_get()));
	for_each_set_bit(tag, &active_reqs, cq_host->num_slots) {
		pr_info(DRV_NAME ": req:0x%pK, tag:%d, cmd flag:0x%llx,"
			"issue time:%lld, start time:%lld, end time:%lld\n",
			cmdq_task_info[tag].req, cmdq_task_info[tag].req->tag,
			cmdq_task_info[tag].req->cmd_flags,
			ktime_to_ns(cmdq_task_info[tag].issue_time),
			ktime_to_ns(cmdq_task_info[tag].start_dbr_time),
			ktime_to_ns(cmdq_task_info[tag].end_dbr_time));
	}
	pr_err(DRV_NAME ": claimed:%d, claimer:%s, claim_cnt:%d\n",
		mmc->claimed,
		mmc->claimer ? mmc->claimer->comm : "NULL",
		mmc->claim_cnt);
}

void cmdq_dumpregs(struct cmdq_host *cq_host)
{
	struct mmc_host *mmc = cq_host->mmc;
	int ret = 0;

	if (cq_host->ops->dump_vendor_regs)
		ret = cq_host->ops->dump_vendor_regs(mmc);

	if (ret) {
		pr_info(DRV_NAME ": Maybe it is not a right time to dump(%s)\n",
			mmc_hostname(mmc));
		return;
	}

	pr_info(DRV_NAME ": ========== REGISTER DUMP (%s)==========\n", mmc_hostname(mmc));

	pr_info(DRV_NAME ": Version: 0x%08x | Caps:  0x%08x\n",
			cmdq_readl(cq_host, CQVER),
			cmdq_readl(cq_host, CQCAP));
	pr_info(DRV_NAME ": Queing config: 0x%08x | Queue Ctrl:  0x%08x\n",
			cmdq_readl(cq_host, CQCFG),
			cmdq_readl(cq_host, CQCTL));
	pr_info(DRV_NAME ": Int stat: 0x%08x | Int enab:  0x%08x\n",
			cmdq_readl(cq_host, CQIS),
			cmdq_readl(cq_host, CQISTE));
	pr_info(DRV_NAME ": Int sig: 0x%08x | Int Coal:  0x%08x\n",
			cmdq_readl(cq_host, CQISGE),
			cmdq_readl(cq_host, CQIC));
	pr_info(DRV_NAME ": TDL base: 0x%08x | TDL up32:  0x%08x\n",
			cmdq_readl(cq_host, CQTDLBA),
			cmdq_readl(cq_host, CQTDLBAU));
	pr_info(DRV_NAME ": Doorbell: 0x%08x | Comp Notif:  0x%08x\n",
			cmdq_readl(cq_host, CQTDBR),
			cmdq_readl(cq_host, CQTCN));
	pr_info(DRV_NAME ": Dev queue: 0x%08x | Dev Pend:  0x%08x\n",
			cmdq_readl(cq_host, CQDQS),
			cmdq_readl(cq_host, CQDPT));
	pr_info(DRV_NAME ": Task clr: 0x%08x | Send stat 1:  0x%08x\n",
			cmdq_readl(cq_host, CQTCLR),
			cmdq_readl(cq_host, CQSSC1));
	pr_info(DRV_NAME ": Send stat 2: 0x%08x | DCMD resp:  0x%08x\n",
			cmdq_readl(cq_host, CQSSC2),
			cmdq_readl(cq_host, CQCRDCT));
	pr_info(DRV_NAME ": Resp err mask: 0x%08x | Task err:  0x%08x\n",
			cmdq_readl(cq_host, CQRMEM),
			cmdq_readl(cq_host, CQTERRI));
	pr_info(DRV_NAME ": Resp idx 0x%08x | Resp arg:  0x%08x\n",
			cmdq_readl(cq_host, CQCRI),
			cmdq_readl(cq_host, CQCRA));
	pr_info(DRV_NAME ": ===========================================\n");

}

void cmdq_dumpstate(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	cmdq_dumpregs(cq_host);
	cmdq_dump_task_history(cq_host);
}

#ifdef CONFIG_PM
static int cmdq_runtime_pm_get(struct mmc_host *mmc)
{
	return pm_runtime_get_sync(mmc->parent);
}

static int cmdq_runtime_pm_put(struct mmc_host *mmc)
{
	pm_runtime_mark_last_busy(mmc->parent);
	return pm_runtime_put_autosuspend(mmc->parent);
}
#else
static int cmdq_runtime_pm_get(struct mmc_host *mmc)
{
	return 0;
}

static int cmdq_runtime_pm_put(struct mmc_host *mmc)
{
	return 0;
}
#endif

static inline struct mmc_request *get_req_by_tag(struct cmdq_host *cq_host,
                                          unsigned int tag)
{
        return cq_host->mrq_slot[tag];
}

static inline u8 *get_desc(struct cmdq_host *cq_host, u8 tag)
{
	return cq_host->desc_base + (tag * cq_host->slot_sz);
}

static inline u8 *get_link_desc(struct cmdq_host *cq_host, u8 tag)
{
	u8 *desc = get_desc(cq_host, tag);

	return desc + cq_host->task_desc_len;
}


static inline dma_addr_t get_trans_desc_dma(struct cmdq_host *cq_host, u8 tag)
{
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	return cq_host->trans_desc_dma_base +
		(u32)(cq_host->mmc->max_segs * 2 * tag *cq_host->trans_desc_len);
#else
	return cq_host->trans_desc_dma_base +
		(u32)(cq_host->mmc->max_segs * tag *cq_host->trans_desc_len);
#endif
}

static inline u8 *get_trans_desc(struct cmdq_host *cq_host, u8 tag)
{
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	return cq_host->trans_desc_base +
		(u32)(cq_host->trans_desc_len * cq_host->mmc->max_segs * 2 * tag);
#else
	return cq_host->trans_desc_base +
		(u32)(cq_host->trans_desc_len * cq_host->mmc->max_segs * tag);
#endif
}

static void setup_trans_desc(struct cmdq_host *cq_host, u8 tag)
{
	u64 *link_temp;
	dma_addr_t trans_temp;

	link_temp = (u64 *)get_link_desc(cq_host, tag);
	trans_temp = get_trans_desc_dma(cq_host, tag);

	memset(link_temp, 0, cq_host->link_desc_len);
	if (cq_host->link_desc_len > 8)
		*(link_temp + 1) &= 0;

	*link_temp = VALID(1) | ACT(0x6) | END(0);

	*link_temp |= DAT_ADDR_LO((u64) lower_32_bits(trans_temp));

	if (cq_host->dma64) {
		*(link_temp + 1) = DAT_ADDR_HI(upper_32_bits(trans_temp));
	}
}

static void cmdq_clear_set_irqs(struct cmdq_host *cq_host, u32 clear, u32 set)
{
	u32 ier;

	ier = cmdq_readl(cq_host, CQISTE);
	ier &= ~clear;
	ier |= set;
	cmdq_writel(cq_host, ier, CQISTE);
	cmdq_writel(cq_host, ier, CQISGE);
	/* ensure the writes are done */
	mb();
}

static void cmdq_dump_adma_mem(struct cmdq_host *cq_host)
{
	struct mmc_host *mmc = cq_host->mmc;
	dma_addr_t desc_dma;
	u32 tag = 0;
	unsigned long data_active_reqs =
		mmc->cmdq_ctx.data_active_reqs;
	unsigned long desc_size =
		(cq_host->mmc->max_segs * cq_host->trans_desc_len);

	for_each_set_bit(tag, &data_active_reqs, cq_host->num_slots) {
		desc_dma = get_trans_desc_dma(cq_host, tag);
		pr_err("%s: %s: tag = %d, trans_desc(virt) = 0x%pK\n",
				mmc_hostname(mmc), __func__, tag,
				get_trans_desc(cq_host, tag));
		print_hex_dump(KERN_ERR, "cmdq-adma:", DUMP_PREFIX_ADDRESS,
				32, 8, get_trans_desc(cq_host, tag),
				(desc_size), false);
	}
}


/**
 * The allocated descriptor table for task, link & transfer descritors
 * looks like:
 * |----------|
 * |task desc |  |->|----------|
 * |----------|  |  |trans desc|
 * |link desc-|->|  |----------|
 * |----------|          .
 *      .                .
 *  no. of slots      max-segs
 *      .           |----------|
 * |----------|
 * The idea here is to create the [task+trans] table and mark & point the
 * link desc to the transfer desc table on a per slot basis.
 */

#define CMDQ_TASK_DESC_LEN_64		8
#define CMDQ_TASK_DESC_LEN_128	16
#define CMDQ_TRANS_DESC_LEN_64		8
#define CMDQ_TRANS_DESC_LEN_128		16
#define CMDQ_TRANS_DESC_LEN_128_FIX	12
static int cmdq_host_alloc_tdl(struct cmdq_host *cq_host, int noalloc)
{

	size_t desc_size;
	size_t data_size;
	unsigned int i = 0;

	/* task descriptor can be 64/128 bit irrespective of arch */
	if (cq_host->caps & CMDQ_TASK_DESC_SZ_128) {
		cmdq_writel(cq_host, cmdq_readl(cq_host, CQCFG) | CQ_TASK_DESC_SZ, CQCFG);
		cq_host->task_desc_len = CMDQ_TASK_DESC_LEN_128;
	} else {
		cq_host->task_desc_len = CMDQ_TASK_DESC_LEN_64;
	}

	/*
	 * 96 bits length of transfer desc instead of 128 bits which means
	 * ADMA would expect next valid descriptor at the 96th bit
	 * or 128th bit
	 */
	if (cq_host->dma64) {
		if (cq_host->quirks & CMDQ_QUIRK_SHORT_TXFR_DESC_SZ)
			cq_host->trans_desc_len = CMDQ_TRANS_DESC_LEN_128_FIX;
		else
			cq_host->trans_desc_len = CMDQ_TRANS_DESC_LEN_128;
		cq_host->link_desc_len = CMDQ_TRANS_DESC_LEN_128;
	} else {
		cq_host->trans_desc_len = CMDQ_TRANS_DESC_LEN_64;
		cq_host->link_desc_len = CMDQ_TRANS_DESC_LEN_64;
	}

	/* total size of a slot: 1 task & 1 transfer (link) */
	cq_host->slot_sz = cq_host->task_desc_len + cq_host->link_desc_len;

	desc_size = (long)(unsigned)((cq_host->slot_sz) * cq_host->num_slots);

	/* FIXME: consider allocating smaller chunks iteratively */
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	/* synopsys controller has dma 128M limit, may result the descriptors > max_segs(128)*/
	data_size = (long)(unsigned)(cq_host->trans_desc_len * (cq_host->mmc->max_segs * 2) * (cq_host->num_slots - 1));
#else
	data_size = (long)(unsigned)(cq_host->trans_desc_len * cq_host->mmc->max_segs * (cq_host->num_slots - 1));
#endif
	/*
	 * allocate a dma-mapped chunk of memory for the descriptors
	 * allocate a dma-mapped chunk of memory for link descriptors
	 * setup each link-desc memory offset per slot-number to
	 * the descriptor table.
	 */
	if (!noalloc) {
		cq_host->desc_base = dmam_alloc_coherent(mmc_dev(cq_host->mmc), desc_size, &cq_host->desc_dma_base, GFP_KERNEL);
		cq_host->trans_desc_base = dmam_alloc_coherent(mmc_dev(cq_host->mmc), data_size, &cq_host->trans_desc_dma_base, GFP_KERNEL);
		cq_host->mmc->cmdq_task_info= devm_kzalloc(mmc_dev(cq_host->mmc),
			sizeof(*cq_host->mmc->cmdq_task_info) *cq_host->num_slots, GFP_KERNEL);
	}
	if (!cq_host->desc_base || !cq_host->trans_desc_base)
		return -ENOMEM;

	for (i = 0; i < (cq_host->num_slots - 1); i++)
		setup_trans_desc(cq_host, i);

	return 0;
}

static int cmdq_mmc_invalid(struct mmc_host *mmc)
{
	if (!mmc->card || !mmc_card_mmc(mmc->card) || !mmc_card_cmdq(mmc->card))
		return -EINVAL;

	return 0;
}

static int cmdq_enable(struct mmc_host *mmc)
{
	int err = 0;
	u32 cqcfg;
	bool dcmd_enable;
	struct cmdq_host *cq_host = mmc_cmdq_private(mmc);

	cmdq_runtime_pm_get(mmc);

	if (!cq_host || cmdq_mmc_invalid(mmc)) {
		pr_err("%s: cmdq_enable fail wrong input para\n", __func__);
		err = -EINVAL;
		goto out;
	}

	if (cq_host->enabled)
		pr_err("WRONG reenabled cmdq %s:line %u\n", __func__, __LINE__);
	/*  goto out; */

	/* TODO: if the legacy MMC host controller is in idle state */

	if (cq_host->ops->enter)
		cq_host->ops->enter(mmc);

	cqcfg = cmdq_readl(cq_host, CQCFG);
	if (cqcfg & 0x1) {
		pr_err("%s: %s: cq_host is already enabled\n", mmc_hostname(mmc), __func__);
		WARN_ON(1);
		goto out;
	}

	if (!cq_host->desc_base || !cq_host->trans_desc_base) {
		err = cmdq_host_alloc_tdl(cq_host, cq_host->enabled);
		if (err)
			goto out;
	}

	pr_debug("****** %s:line %u  desc_dma_base is 0x%x  desc_base is %pK trans_desc_base is %pK\n", __func__, __LINE__, (u32) cq_host->desc_dma_base, cq_host->desc_base, cq_host->trans_desc_base);

	if (cq_host->quirks & CMDQ_QUIRK_NO_DCMD)
		dcmd_enable = false;
	else
		dcmd_enable = true;

	cqcfg = ((cq_host->dma64 ? CQ_TASK_DESC_SZ : 0) | (dcmd_enable ? CQ_DCMD : 0));

	cmdq_writel(cq_host, cqcfg, CQCFG);
	cmdq_writel(cq_host, 0x10100, CQSSC1);
	cq_host->cqcfg = cqcfg;

	/* want to write this address in any enable scario. */
	cmdq_writel(cq_host, lower_32_bits(cq_host->desc_dma_base), CQTDLBA);
	cmdq_writel(cq_host, upper_32_bits(cq_host->desc_dma_base), CQTDLBAU);

	/* leave send queue status timer configs to reset values */

	/* configure interrupt coalescing */
	/* mmc_cq_host_intr_aggr(host->cq_host, CQIC_DEFAULT_ICCTH,
	   CQIC_DEFAULT_ICTOVAL); */

	/* leave CQRMEM to reset value */

	/*
	 * disable all vendor interrupts
	 * enable CMDQ interrupts
	 * enable the vendor error interrupts
	 */
	if (cq_host->ops->clear_set_irqs)
		cq_host->ops->clear_set_irqs(mmc, true);

	cmdq_clear_set_irqs(cq_host, 0x0, CQ_INT_ALL);

	/* cq_host would use this rca to address the card */
	cmdq_writel(cq_host, mmc->card->rca, CQSSC2);

	cq_host->rca = mmc->card->rca;

	/* ensure the writes are done before enabling CQE */
	mb();

	/* enable CQ_HOST */
	cmdq_writel(cq_host, cmdq_readl(cq_host, CQCFG) | CQ_ENABLE, CQCFG);

	cq_host->enabled = true;

out:
	cmdq_runtime_pm_put(mmc);
	return err;
}

static int cmdq_disable(struct mmc_host *mmc, bool soft)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	u32 reg = 0;
	u32 i = 0;
	unsigned long timeout = (8 * 1000 * 1000);

	cmdq_runtime_pm_get(mmc);

	do {
		reg = cmdq_readl(cq_host, CQTDBR);
		reg |= cmdq_readl(cq_host, CQTCN);
		reg |= cmdq_readl(cq_host, CQDPT);

		for (i = 0; i < cq_host->num_slots; i++) {
			if (cq_host->mrq_slot[i])
				reg |= (u32) 1 << i;
		}

		if (timeout == 0) {
			pr_err("%s: wait cmdq complete reqs timeout !\n", __func__);
			return -ETIMEDOUT;
		}
		timeout--;
		udelay(1);
	} while (reg);

	if (soft) {
		cmdq_writel(cq_host, cmdq_readl(cq_host, CQCFG) & ~(CQ_ENABLE), CQCFG);
	} else {
		/* FIXME: free the allocated descriptors */
	}
	cmdq_clear_set_irqs(cq_host, 0x0, 0x0);

	if (cq_host->ops->exit)
		cq_host->ops->exit(mmc);

	if (cq_host->ops->clear_set_irqs)
		cq_host->ops->clear_set_irqs(mmc, false);

	cq_host->enabled = false;

	pr_debug("%s: done.\n", __func__);

	cmdq_runtime_pm_put(mmc);

	return 0;
}

/*disable cmdq imediatly,no need to wait req finish*/
void cmdq_disable_immediatly(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	unsigned long comp_status;
#endif

#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	/*first clear TCN;then clear all interrupt*/
	comp_status = cmdq_readl(cq_host, CQTCN);
	if (comp_status)
		cmdq_writel(cq_host, comp_status, CQTCN);
	cmdq_writel(cq_host, 0xFFFFFFFF, CQIS);

	cmdq_clear_set_irqs(cq_host, 0x0, 0x0);

	if (cq_host->ops->clear_set_irqs)
		cq_host->ops->clear_set_irqs(mmc, false);

	if (cq_host->ops->exit)
		cq_host->ops->exit(mmc);
#endif

	cmdq_writel(cq_host, cmdq_readl(cq_host, CQCFG) & ~(CQ_ENABLE), CQCFG);

	cq_host->enabled = false;

	pr_debug("%s: done.\n", __func__);

	return;
}

static void cmdq_set_halt_irq(struct cmdq_host *cq_host, bool enable)
{
	u32 ier;

	ier = cmdq_readl(cq_host, CQISTE);
	if (enable) {
		cmdq_writel(cq_host, ier | HALT, CQISTE);
		cmdq_writel(cq_host, ier | HALT, CQISGE);
	} else {
		cmdq_writel(cq_host, ier & ~HALT, CQISTE);
		cmdq_writel(cq_host, ier & ~HALT, CQISGE);
	}
}

/* cmdq_halt_poll - Halting CQE using polling method.
 * @mmc: struct mmc_host
 * @halt: bool halt
 * This is used mainly from interrupt context to halt/unhalt
 * CQE engine.
 */
static int cmdq_halt_poll(struct mmc_host *mmc, bool halt)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	int retries = 100;

	if (!halt) {
		if (cq_host->ops->set_data_timeout)
			cq_host->ops->set_data_timeout(mmc, 0xE);
		if (cq_host->ops->clear_set_irqs)
			cq_host->ops->clear_set_irqs(mmc, true);
		cmdq_writel(cq_host, cmdq_readl(cq_host, CQCTL) & ~HALT,
			    CQCTL);
		return 0;
	}

	cmdq_set_halt_irq(cq_host, false);
	cmdq_writel(cq_host, cmdq_readl(cq_host, CQCTL) | HALT, CQCTL);
	while (retries) {
		if (!(cmdq_readl(cq_host, CQCTL) & HALT)) {
			udelay(5);
			retries--;
			continue;
		} else {
			/* halt done: re-enable legacy interrupts */
			if (cq_host->ops->clear_set_irqs)
				cq_host->ops->clear_set_irqs(mmc, false);
			mmc_host_set_halt(mmc);
			break;
		}
	}
	cmdq_set_halt_irq(cq_host, true);
	return retries ? 0 : -ETIMEDOUT;
}


/**
 * cmdq_trylock_hostlock - try to claim the lock of cq host;
 * @cq_host:	cmdq host;
 * @flags:  store the flags for restore;
 * try to claim the cmdq host lock;
 */
static int cmdq_trylock_hostlock(struct cmdq_host *cq_host,unsigned long* flags)
{
	int locked = 0;
	unsigned int trycount = 100000;
	do{
		locked = spin_trylock_irqsave(&cq_host->cmdq_lock, *flags);/*lint !e666*/
		if(locked)
			break;
		udelay(10);
	}while(--trycount>0);
	return locked;
}

/**
 * cmdq_halt_irq_safe - halt the cqe even if the irq system
 * does not work;
 * @cq_host:	cmdq host;
 * note:need to claim lock of cq-host before invoke this func;
 */
static int cmdq_halt_irq_safe(struct cmdq_host *cq_host)
{
	unsigned long timeout = (1 * 1000);
	if (cmdq_readl(cq_host, CQCTL) & HALT) {
		pr_warn("%s: CQE already HALT\n", __func__);
		return 0;
	}
	cq_host->irq_safe_flag = 1;
	cmdq_writel(cq_host, cmdq_readl(cq_host, CQCTL) | HALT, CQCTL);
	do {
		if (cmdq_readl(cq_host, CQIS) & CQIS_HAC) {
			cmdq_writel(cq_host, CQIS_HAC, CQIS);
			break;
		}
		mdelay(1);
	} while(--timeout > 0);
	cq_host->irq_safe_flag = 0;
	if (timeout)
		return 0;
	else {
		pr_err("%s cmdq halt timeout\n", __func__);
		cmdq_dumpregs(cq_host);
		return -ETIMEDOUT;
	}

}

/**
 * cmdq_dbr_clear_and_halt - wait for doorbell clear and halt cqe;
 * @mmc:	mmc host;
 * wait for doorbell clear and halt cqe;
 */
int cmdq_dbr_clear_and_halt(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	unsigned long flags;
	u32 reg = 0;
	int ret;
	unsigned long timeout = (2 * 1000);

	/*Get hostlock timeout, the abnormal context may have the locker*/
	if(!cmdq_trylock_hostlock(cq_host,&flags)) {
		pr_err("%s, can't get the hostlock!\n", __func__);
		return -EIO;
	}

	/*begin to wait doorbell clear*/
	do {
		reg = cmdq_readl(cq_host, CQTDBR);
		reg |= cmdq_readl(cq_host, CQDPT);

		if (timeout == 0) {
			pr_err("%s: wait doorbell clear timeout !\n", __func__);
			spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
			return -ETIMEDOUT;
		}
		timeout--;
		mdelay(1);
	} while (reg);
	/*halt cqe*/
	ret = cmdq_halt_irq_safe(cq_host);
	spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);

	return ret;
}

static int cmdq_restore_irqs(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);

	if (!cq_host->enabled)
		return 0;

	if (cq_host->ops->clear_set_irqs)
		cq_host->ops->clear_set_irqs(cq_host->mmc, true);
	cmdq_clear_set_irqs(cq_host, 0x0, CQ_INT_ALL);

	return 0;
}

static void cmdq_prep_task_desc(struct mmc_request *mrq, u64 *data, bool intr, bool qbr)
{
	struct mmc_cmdq_req *cmdq_req = mrq->cmdq_req;
	u32 req_flags = cmdq_req->cmdq_req_flags;

	pr_debug("%s: %s: data-tag: 0x%08x - dir: %d - prio: %d - cnt: 0x%08x -	addr: 0x%x\n", mmc_hostname(mrq->host), __func__,
			!!(req_flags & DAT_TAG), !!(req_flags & DIR), !!(req_flags & PRIO), cmdq_req->data.blocks, mrq->cmdq_req->blk_addr);
#ifdef CMDQ_FIX_CLEAR_QBRTASK
	qbr = false;
#endif
	*data = VALID(1) | END(1) | INT(INT2BOOL(intr)) | ACT(0x5) | FORCED_PROG(INT2BOOL((req_flags & FORCED_PRG)))
		| CONTEXT(mrq->cmdq_req->ctx_id) | DATA_TAG(INT2BOOL((req_flags & DAT_TAG)))
		| DATA_DIR(INT2BOOL((req_flags & DIR))) | PRIORITY(INT2BOOL((req_flags & PRIO))) | QBAR(INT2BOOL(qbr)) | REL_WRITE(INT2BOOL((req_flags & REL_WR)))
		| BLK_COUNT(mrq->cmdq_req->data.blocks) | BLK_ADDR((u64)mrq->cmdq_req->blk_addr);
}

static int cmdq_dma_map(struct mmc_host *host, struct mmc_request *mrq)
{
	int sg_count;
	struct mmc_data *data = mrq->data;

	if (!data)
		return -EINVAL;

	sg_count = dma_map_sg(mmc_dev(host), data->sg, data->sg_len, (data->flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
	if (!sg_count)
		return -ENOMEM;

	return sg_count;
}

static void _cmdq_set_tran_desc(u8 *desc, dma_addr_t addr, int len,
				bool end, bool is_dma64)
{
	__le32 *attr = (__le32 __force *)desc;

	*attr = (VALID(1) |
		 END(end ? 1 : 0) |
		 INT(0) |
		 ACT(0x4) |
		 DAT_LENGTH(len));

	if (is_dma64) {
		__le64 *dataddr = (__le64 __force *)(desc + 4);

		dataddr[0] = cpu_to_le64(addr);
	} else {
		__le32 *dataddr = (__le32 __force *)(desc + 4);

		dataddr[0] = cpu_to_le32(addr);
	}
}

#define SYNOPSYS_DMA_LIMIT	0x8000000
static void cmdq_set_tran_desc(struct cmdq_host *cq_host, u8 **desc,
			dma_addr_t addr, int len, bool end, u32 blksz)
{
	u8 *tran_desc = *desc;
	int tran_len = len;
	dma_addr_t tran_addr = addr;

#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	int first_desc_len;

	if ((tran_addr % SYNOPSYS_DMA_LIMIT + tran_len > SYNOPSYS_DMA_LIMIT)
		&& ((tran_addr + (unsigned int)tran_len) % SYNOPSYS_DMA_LIMIT  >= blksz)) {

		first_desc_len = (SYNOPSYS_DMA_LIMIT - tran_addr % SYNOPSYS_DMA_LIMIT);
		_cmdq_set_tran_desc(tran_desc, tran_addr,first_desc_len, false, cq_host->dma64);

		pr_err("%s: 1th, tran_desc:0x%pK, tran_addr:0x%pK, tran_len:%d\n", __func__, *desc, (void*)tran_addr, first_desc_len);
		*desc = *desc + cq_host->trans_desc_len;
		tran_desc = *desc;
		tran_len -= first_desc_len;
		tran_addr += first_desc_len;
		pr_err("%s: 2th, tran_desc:0x%pK, tran_addr:0x%pK, tran_len:%d\n", __func__, *desc, (void*)tran_addr, tran_len);
	}
#endif
	_cmdq_set_tran_desc(tran_desc, tran_addr, tran_len, end, cq_host->dma64);
}

static int cmdq_prep_tran_desc(struct mmc_request *mrq, struct cmdq_host *cq_host, int tag)
{
	struct mmc_data *data = mrq->data;
	int i, sg_count, len;
	bool end = false;
	u8 *trans_desc = NULL;
	dma_addr_t addr;
	u8 *desc;
	struct scatterlist *sg;

	mrq->data->bytes_xfered = 0;
	sg_count = cmdq_dma_map(mrq->host, mrq);
	if (sg_count < 0) {
		pr_err("%s: %s: unable to map sg lists, %d\n", mmc_hostname(mrq->host), __func__, sg_count);
		return sg_count;
	}

	trans_desc = get_trans_desc(cq_host, tag);
	desc = trans_desc;
	memset(desc, 0, cq_host->trans_desc_len * cq_host->mmc->max_segs);

	for_each_sg(data->sg, sg, sg_count, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);

		if ((i + 1) == sg_count)
			end = true;

		cmdq_set_tran_desc(cq_host, &desc, addr, len, end, data->blksz);
		desc += cq_host->trans_desc_len;

		mrq->data->bytes_xfered += len;
	}

	pr_debug("%s: req: 0x%pK tag: %d calc-link_des: 0x%pK sg-cnt: %d\n", __func__, mrq->req, tag, trans_desc, sg_count);

	return 0;
}

static void cmdq_prep_dcmd_desc(struct mmc_host *mmc, struct mmc_request *mrq)
{
	u64 *task_desc = NULL;
	u64 data = 0;
	u8 resp_type;
	u8 *desc;
	/*__le64 *dataddr;*/
	u32 *dataddr;
	struct cmdq_host *cq_host = mmc_cmdq_private(mmc);

	if (!(mrq->cmd->flags & MMC_RSP_PRESENT)) {
		resp_type = RES_TYPE_NO_RES;
	} else if ((mrq->cmd->flags & MMC_RSP_R1B) == MMC_RSP_R1B) {
		resp_type =(cq_host->quirks & CMDQ_QUIRK_CHECK_BUSY) ? RES_TYPE_R145 : RES_TYPE_R1B;
	} else if (((mrq->cmd->flags & MMC_RSP_R1) == MMC_RSP_R1)
				|| ((mrq->cmd->flags & MMC_RSP_R4) == MMC_RSP_R4)
				|| ((mrq->cmd->flags & MMC_RSP_R5) == MMC_RSP_R5)) {
			resp_type = RES_TYPE_R145;
	} else {
		pr_err("%s: weird response: 0x%x\n", mmc_hostname(mmc), mrq->cmd->flags);
		BUG_ON(1);
	}

	pr_debug("%s: DCMD->opcode: %d, arg: 0x%x, resp_type = %d\n", __func__, mrq->cmd->opcode, mrq->cmd->arg, resp_type);

	task_desc = (u64 *)get_desc(cq_host, cq_host->dcmd_slot);
	memset(task_desc, 0, sizeof(*task_desc));
	data |= (VALID(1) | END(1) | INT(1) | QBAR(1) | ACT(0x5) | CMD_INDEX(mrq->cmd->opcode) | CMD_TIMING(0) | RESP_TYPE(resp_type));/* [false alarm]: resp_type can be set*/
	*task_desc |= data;
	desc = (u8 *) task_desc;

	/*dataddr = (__le64 __force *)(desc + 4);*/
	/*dataddr[0] = cpu_to_le64((u64)mrq->cmd->arg);*/

	dataddr = (u32 *) (desc + 4);
	dataddr[0] = (u32) mrq->cmd->arg;
	wmb();

	if (cq_host->ops->set_data_timeout)
		cq_host->ops->set_data_timeout(mmc, 0xe);
}


/* arasan controller has a bug: if previous request is R1b, we should check
  * whether the data line is busy before the current request.
  * Attention:this function is only called by cmdq_request.
  */
static int cmdq_check_busy_before_request(struct mmc_host *mmc)
{
	int busy_status;
	u32 req_status = 0;
	u32 waitbusy_timeout;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	unsigned long flags;

	spin_lock_irqsave(&cq_host->cmdq_lock, flags);
	if (true == cq_host->check_busy) {
		while (1) {
			req_status = cmdq_readl(cq_host, CQTDBR);
			if (0 == (req_status & ((u32)1 << 31)))
				break;
		}

		if (cq_host->ops->card_busy) {
			/*
			 *When the card is dirty, read and write performance decreases,
			 *causing the lock interrupt time longer,
			 *resulting in cpu0 ipc 20ms timeout,
			 *Hifi cut off,
			 *so no lock interrupt, when the loop test card is busy.
			 *
			 * */
			spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
			/*some dcmd such as secure trim will cost long time
			 *set the timeout time to 10s,the same with the timeout
			 *set in emmc5.0;
			 */
			waitbusy_timeout = 10000000;
			while (1) {
				/* During error handling, after reset controller and not
				  *dis-reset yet , the register cannot be access, if do it,
				  *noc will occur. When err occur, halt or diasble will be set*/
				if (mmc_host_halt(mmc) || !cq_host->enabled) {
					pr_err("%s: Can not access controller during reset\n", __func__);
					return -EHOSTDOWN;
				}
				busy_status = cq_host->ops->card_busy(mmc);
				if (!busy_status)
					break;
				if (0 == waitbusy_timeout) {
					pr_err("%s: wait card busy timeout\n", __func__);
					break;
				}
				waitbusy_timeout--;
				udelay(1);
			}
			spin_lock_irqsave(&cq_host->cmdq_lock, flags);
		} else {
			pr_warn("%s: no card_busy ops.\n", __func__);
		}

		cq_host->check_busy = false;
	}

	spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);

	return 0;
}

static int cmdq_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	int err;
	u64 data = 0;
	u64 *task_desc = NULL;
	u32 tag = mrq->cmdq_req->tag;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	unsigned long flags;

	if (!cq_host->enabled) {
		pr_err("%s cq_host->enable not true.\n", __func__);
		err = -EHOSTDOWN;
		goto out;
	}

	if (cq_host->quirks & CMDQ_QUIRK_CHECK_BUSY) {
		err = cmdq_check_busy_before_request(mmc);
		if (err)
			goto out;
	}

	spin_lock_irqsave(&cq_host->cmdq_lock, flags);
	if (mmc_host_halt(mmc) || !cq_host->enabled) {
		spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
		pr_err("%s: Can not access controller during reset\n", __func__);
		err = -EHOSTDOWN;
		goto out;
	}

	if (mrq->cmdq_req->cmdq_req_flags & DCMD) {
		pr_debug("%s: DCMD mrq\n", __func__);

		cmdq_prep_dcmd_desc(mmc, mrq);
		if (cq_host->mrq_slot[31])
			BUG_ON(1);

		cq_host->mrq_slot[31] = mrq;

		if (true == cq_host->fix_qbr)
			BUG_ON(cmdq_readl(cq_host, CQTDBR));

		mmc->cmdq_task_info[tag].start_dbr_time = ktime_get();
		cmdq_writel(cq_host, (u32)1 << 31, CQTDBR);

		if (cq_host->quirks & CMDQ_QUIRK_CHECK_BUSY)
			cq_host->check_busy = true;

		spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
		return 0;
	}

	task_desc = (u64 *)get_desc(cq_host, tag);

	cmdq_prep_task_desc(mrq, &data, 1, (mrq->cmdq_req->cmdq_req_flags & QBR));
	*task_desc = cpu_to_le64(data);
	if (!*task_desc)
		pr_emerg("%s:%d cmdq task data is null!!!!\n", __func__, __LINE__);

	err = cmdq_prep_tran_desc(mrq, cq_host, tag);
	if (err) {
		pr_err("%s: %s: failed to setup tx desc: %d\n", mmc_hostname(mmc), __func__, err);
		BUG_ON(1);
	}
	wmb();

	BUG_ON(cmdq_readl(cq_host, CQTDBR) & (1 << tag));

	if (cq_host->mrq_slot[tag])
		BUG_ON(1);

	if (cq_host->ops->set_tranfer_params)
		cq_host->ops->set_tranfer_params(mmc);

	if (cq_host->ops->set_data_timeout)
		cq_host->ops->set_data_timeout(mmc, CMDQ_DATA_TIMOUT);

	cq_host->mrq_slot[tag] = mrq;

	if (true == cq_host->fix_qbr) {
		if (0 == cmdq_readl(cq_host, CQTDBR)) {
			mmc->cmdq_task_info[tag].start_dbr_time = ktime_get();
			cmdq_writel(cq_host, (u32) 1 << tag, CQTDBR);
		}
	} else {
			mmc->cmdq_task_info[tag].start_dbr_time = ktime_get();
			cmdq_writel(cq_host, (u32) 1 << tag, CQTDBR);
	}

	spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);

out:
	return err;
}

static int cmdq_finish_data(struct mmc_host *mmc, unsigned int tag)
{
	struct mmc_request *mrq;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);

	mrq = cq_host->mrq_slot[tag];
	if (NULL == mrq) {
	/* synopsys controller may report TCC interrupt after a busy request is done,
	 * it print too many, so shield it */
#ifndef CONFIG_MMC_SDHCI_DWC_MSHC
		pr_err("%s: mrq_slot %d is NULL in data finishing!!!\n", mmc_hostname(mmc), tag);
#endif
		return -1;
	}

	cq_host->mrq_slot[tag] = NULL;
	cq_host->ops->tuning_move(mmc, TUNING_CLK, TUNING_FLAG_CLEAR_COUNT);

	/*TODO: error handle*/
	mrq->done(mrq);

	return 0;
}


u32 sdhci_cmdq_readl(struct cmdq_host *host, int reg)
{
	return cmdq_readl(host, CQTDBR);
}

static void cmdq_ring_dbl_in_irq(struct mmc_host *mmc)
{
	u32 reg_val = 0;
	u32 tmp_qctcn = 0;
	unsigned long tag = 0;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);

	if (true == cq_host->fix_qbr) {
		spin_lock(&cq_host->cmdq_lock);
		if (0 == cmdq_readl(cq_host, CQTDBR)) {
			reg_val = 0;
			tmp_qctcn = cmdq_readl(cq_host, CQTCN);
			for (tag = 0; tag < cq_host->num_slots - 1; tag++) {
				if (cq_host->mrq_slot[tag] && (!(tmp_qctcn & (1 << tag)))) {
					reg_val |= 1 << tag;
					mmc->cmdq_task_info[tag].start_dbr_time = ktime_get();
				}
			}
			cmdq_writel(cq_host, reg_val, CQTDBR);
		}
		spin_unlock(&cq_host->cmdq_lock);
	}
	return;
}

struct mmc_request* get_first_req(struct cmdq_host *cq_host, u32 *tag)
{
	int i;
	for (i = 0; i < cq_host->num_slots; i++) {/*lint !e574 */
	      if(cq_host->mrq_slot[i]) {
			*tag = i;
		      return cq_host->mrq_slot[i];
	      }
	}

	return NULL;
}

struct mmc_request* get_mrq_by_tag(struct cmdq_host *cq_host, u32 *tag)
{
	struct mmc_request *mrq = NULL;

	if (*tag < cq_host->num_slots)
		mrq = get_req_by_tag(cq_host, *tag);
	if (!mrq) {
		mrq = get_first_req(cq_host, tag);
		if (!mrq) {
			pr_err("%s: cannot get avalible mrq\n", __func__);
			BUG_ON(1);
		}
	}

	return mrq;
}


#ifdef CONFIG_EMMC_FAULT_INJECT
int cmdq_interrupt_errors_handle(struct mmc_host *mmc, u32 intmask,
					u32 status, int err, bool do_inj)
#else
int cmdq_interrupt_errors_handle(struct mmc_host *mmc, u32 intmask,
					u32 status, int err)
#endif
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	struct mmc_request *mrq;
	u32 dbr_set = 0;
	unsigned long tag = 0xFFFF;
	u32 err_info = 0;
	int poll_ret = 0;

	spin_lock(&cq_host->cmdq_lock);

	pr_err("%s: cmd queue err, intmask = 0x%x\n", __func__, intmask);
	cmdq_dumpregs(cq_host);

	/* prase error information */
	err_info = cmdq_readl(cq_host, CQTERRI);
	dbr_set = cmdq_readl(cq_host, CQTDBR);

	/*
	 * Need to halt CQE in case of error in interrupt context itself
	 * otherwise CQE may proceed with sending CMD to device even if
	 * CQE/card is in error state.
	 * CMDQ error handling will make sure that it is unhalted after
	 * handling all the errors.
	 */
	poll_ret = cmdq_halt_poll(mmc, true);
	if (poll_ret)
		pr_err("%s: %s: halt failed ret=%d\n",
				mmc_hostname(mmc), __func__, poll_ret);

#ifdef CONFIG_EMMC_FAULT_INJECT
	if (do_inj) {
		mmcdbg_cmdq_inj_fill_errinfo(cq_host, &err_info);
		pr_err("err_info is 0x%x\n", err_info);
	}
#endif

	if (err_info & CQTERRI_RES_ERR) {
		tag = CQTERRI_RES_TASK(err_info);
		pr_err("%s: CMD err tag: %lu\n", __func__, tag);

		mrq = get_mrq_by_tag(cq_host,(u32 *)&tag);
		/* CMD44/45/46/47 will not have a valid cmd */
		if (mrq->cmd)
			mrq->cmd->error = err;
		else
			mrq->data->error = err;
	} else if (err_info & CQTERRI_DAT_ERR) {
		tag = CQTERRI_DAT_TASK(err_info);
		pr_err("%s: Dat err  tag: %lu\n", __func__, tag);
		mrq = get_mrq_by_tag(cq_host,(u32 *)&tag);
		mrq->data->error = err;
	} else {
#ifdef CONFIG_EMMC_FAULT_INJECT
		if (do_inj) {
			mmcdbg_cmdq_inj_fake_dbl(cq_host, &dbr_set);
			pr_err("fake dbl is 0x%x\n", dbr_set);
		}
#endif
		if (!dbr_set) {
			pr_err("%s: spurious/force error interrupt, err_info = 0x%x!!!\n",
					mmc_hostname(mmc), err_info);
			cmdq_halt_poll(mmc, false);
			mmc_host_clr_halt(mmc);
			spin_unlock(&cq_host->cmdq_lock);
			return IRQ_HANDLED;
		}
		tag = ffs(dbr_set) - 1;
		mrq = get_mrq_by_tag(cq_host,(u32 *)&tag);
		if (mrq->data)
			mrq->data->error = err;
		else
			mrq->cmd->error = err;
		/*
		 * Get ADMA descriptor memory in case of ADMA
		 * error for debug.
		 */
		if (err == -EIO)
			cmdq_dump_adma_mem(cq_host);
	}

	/*
	 * If CQE halt fails then, disable CQE
	 * from processing any further requests
	 */
	if (poll_ret)
		cmdq_disable_immediatly(mmc);

	if (status & CQIS_RED) {
		if(mrq->cmdq_req)
			mrq->cmdq_req->resp_err = true;
		pr_err("%s: RED error %d !!!\n", mmc_hostname(mmc), status);
	}

#ifdef CONFIG_HUAWEI_EMMC_DSM
	sdhci_dsm_set_host_status(mmc_priv(mmc), intmask & SDHCI_INT_ERROR_MASK);
	sdhci_dsm_handle(mmc_priv(mmc), mrq);
#endif
	cmdq_finish_data(mmc, tag);
	spin_unlock(&cq_host->cmdq_lock);

	return 0;
}

irqreturn_t cmdq_irq(struct mmc_host *mmc, u32 intmask)
{
	u32 status;
	unsigned long tag = 0;
	unsigned long comp_status = 0;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	u32 reg_val = 0;
	int err = 0;
	int ret = 0;
#ifdef CONFIG_EMMC_FAULT_INJECT
	bool do_inj = false;
#endif

	if (intmask & SDHCI_INT_CMD_MASK)
		err = sdhci_get_cmd_err(intmask);
	else if (intmask & SDHCI_INT_DATA_MASK)
		err = sdhci_get_data_err(intmask);
	else if (intmask != SDHCI_INT_CMDQ) {
		pr_err("%s: not expect cmdq irq, intmask = 0x%x\n", __func__, intmask);
		cmdq_dumpregs(cq_host);
	}

	status = cmdq_readl(cq_host, CQIS);

#ifdef CONFIG_EMMC_FAULT_INJECT
	do_inj = mmcdbg_error_inject_dispatcher(mmc,
			ERR_INJECT_CMDQ_INTR, 0, (u32*)&err, false);
	if (do_inj) {
		mmcdbg_cmdq_inj_fill_status(cq_host, &status);
	}
#endif

	if (!status && !err) {
		pr_err("%s: no irq\n", __func__);
		return IRQ_NONE;
	}

/*for sys controller, need to clear tcn first, so move it from below*/
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	if (status & CQIS_TCC) {
		/* read QCTCN and complete the request */
		comp_status = cmdq_readl(cq_host, CQTCN);
		reg_val = cmdq_readl(cq_host, CQTDBR);
		comp_status &= ~(unsigned long)reg_val;
		cmdq_writel(cq_host, comp_status, CQTCN);
	}
#endif

	cmdq_writel(cq_host, status, CQIS);

	if ((status & CQIS_RED) || err) {
		pr_err("%s: interrupt of errors detected. status = 0x%x, err = 0x%x\n", __func__, status, err);
#ifdef CONFIG_EMMC_FAULT_INJECT
		ret = cmdq_interrupt_errors_handle(mmc, intmask, status, err, do_inj);
#else
		ret = cmdq_interrupt_errors_handle(mmc, intmask, status, err);
#endif
		if(ret)
			return IRQ_HANDLED;
	}

	if (status & CQIS_HAC) {
		/* halt is completed, wakeup waiting thread */
		pr_err("%s: cmd queue halt completed. status = 0x%x\n", __func__, status);
		if (!cq_host->irq_safe_flag)
			complete(&cq_host->halt_comp);
	}

	if (status & CQIS_TCL) {
		/* task is cleared, wakeup waiting thread */
		pr_err("%s: cmd queue task clear. status = 0x%x\n", __func__, status);
		complete(&cq_host->clear_comp);
	}

	if (status & CQIS_TERR) {
		pr_err("%s: cmd queue task error (invalid task descriptor) %d !!!\n", mmc_hostname(mmc), status);
		cmdq_dumpregs(cq_host);
		BUG_ON(1);
	}

	if (status & CQIS_TCC) {
#ifndef CONFIG_MMC_SDHCI_DWC_MSHC
		/* read QCTCN and complete the request */
		comp_status = cmdq_readl(cq_host, CQTCN);
		reg_val = cmdq_readl(cq_host, CQTDBR);
		comp_status &= ~(unsigned long)reg_val;
		cmdq_writel(cq_host, comp_status, CQTCN);
#endif
		for_each_set_bit(tag, &comp_status, cq_host->num_slots) {
			/* complete the corresponding mrq */
			mmc->cmdq_task_info[tag].end_dbr_time = ktime_get();
			cmdq_finish_data(mmc, tag);
		}
		cmdq_ring_dbl_in_irq(mmc);
	}

	return IRQ_HANDLED;
}

EXPORT_SYMBOL(cmdq_irq);

/* May sleep */
static int cmdq_halt(struct mmc_host *mmc, bool halt)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	u32 val;
	unsigned long flags;

	if (halt) {
		spin_lock_irqsave(&cq_host->cmdq_lock, flags);
		if (cmdq_readl(cq_host, CQCTL) & HALT) {
			spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
			pr_warn("%s: CQE already HALT\n", mmc_hostname(mmc));
			return 0;
		}
		cmdq_writel(cq_host, cmdq_readl(cq_host, CQCTL) | HALT, CQCTL);
		spin_unlock_irqrestore(&cq_host->cmdq_lock, flags);
		val = wait_for_completion_timeout(&cq_host->halt_comp, msecs_to_jiffies(HALT_TIMEOUT_MS));
		if (val)
			cq_host->ops->clear_set_irqs(mmc,	false);
		return val ? 0 : -ETIMEDOUT;
	} else {
		if (cq_host->ops->set_data_timeout)
			cq_host->ops->set_data_timeout(mmc, 0xE);
		if (cq_host->ops->clear_set_irqs)
			cq_host->ops->clear_set_irqs(mmc, true);
		cmdq_writel(cq_host, cmdq_readl(cq_host, CQCTL) & ~HALT, CQCTL);
	}

	return 0;
}
int cmdq_clear_task(struct mmc_host *mmc, u32 task, bool entire)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);
	u32 value, ret;
	unsigned long tag, comp_status;

	if (!task) {
		pr_err("%s: task is null\n", __func__);
		return 0;
	}

	if (0 == (HALT & cmdq_readl(cq_host, CQCTL))) {
		pr_err("CQE is not in HALT, cannot clear task\n");
		return -1;
	}

	value = cmdq_readl(cq_host, CQTDBR);
	if (value == task || entire) {
		/* clean all task */
		cmdq_writel(cq_host, CLEAR_ALL_TASKS | cmdq_readl(cq_host, CQCTL), CQCTL);
		ret = wait_for_completion_timeout(&cq_host->clear_comp, msecs_to_jiffies(CLEAR_TIMEOUT_MS));
		return ret ? 0 : -ETIMEDOUT;
	} else {
		/* clean task one by one */
		comp_status = task;
		for_each_set_bit(tag, &comp_status, cq_host->num_slots) {
			cmdq_writel(cq_host, 1 << tag, CQTCLR);
			ret = wait_for_completion_timeout(&cq_host->clear_comp, msecs_to_jiffies(CLEAR_TIMEOUT_MS));
			if (!ret)
				return -ETIMEDOUT;
		}
		return 0;
	}
}
EXPORT_SYMBOL(cmdq_clear_task);

static void cmdq_post_req(struct mmc_host *mmc, struct mmc_request *mrq, int err)
{
	struct mmc_data *data = mrq->data;

	if (data) {
		data->error = err;
		dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, (data->flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		if (err)
			data->bytes_xfered = 0;
		else
			data->bytes_xfered = blk_rq_bytes(mrq->req);
	}
}

int cmdq_is_reset(struct mmc_host *host)
{
	struct cmdq_host *cq_host = mmc_cmdq_private(host);

	return cq_host->reset_flag;
}



static void cmdq_work_resend(struct work_struct *work)
{
}

static void cmdq_timeout_timer(unsigned long param)
{
	return;
}

/* clear the mrq_slot, it may result bug on when process request after reset.
    call this func while doing reset */
void cmdq_clear_mrq_slot(struct cmdq_host *cq_host)
{
	u32 tag;

	for (tag = 0; tag < cq_host->num_slots; tag++)
		cq_host->mrq_slot[tag] = NULL;
}

int cmdq_discard_task(struct mmc_host *mmc, u32 task, bool entire)
{
	int ret;
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);

	ret = cmdq_clear_task( mmc, task,  entire);
	if (ret) {
		pr_err("%s: clear task fail, ret = %d\n", __func__, ret);
		return ret;
	}

	ret = cq_host->ops->discard_task(mmc, task, entire);
	if (ret)
		pr_err("%s: discard fail, ret = %d\n", __func__, ret);

	return ret;
}

void cmdq_reset(struct mmc_host *mmc)
{
	struct cmdq_host *cq_host = (struct cmdq_host *)mmc_cmdq_private(mmc);

	cmdq_clear_mrq_slot(cq_host);
}

static const struct mmc_cmdq_host_ops cmdq_host_ops = {
	.enable = cmdq_enable,
	.disable = cmdq_disable,
	.restore_irqs = cmdq_restore_irqs,
	.request = cmdq_request,
	.halt = cmdq_halt,
	.post_req = cmdq_post_req,
	.disable_immediatly = cmdq_disable_immediatly,
	.clear_and_halt = cmdq_dbr_clear_and_halt,
	.dumpstate = cmdq_dumpstate,
	.reset = cmdq_reset,
	.discard_task = cmdq_discard_task,
};


static void cmdq_populate_dt(struct platform_device *pdev, struct cmdq_host *cq_host)
{
	struct device_node *np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "can not find device node\n");
		return;
	}

	if (of_get_property(np, "cmdq_check_busy", NULL)) {
		cq_host->quirks |= CMDQ_QUIRK_CHECK_BUSY;
		cq_host->quirks |= CMDQ_QUIRK_EMPTY_BEFORE_DCMD;
		pr_info("cmdq check busy enable\n");
	}
}


struct cmdq_host *cmdq_pltfm_init(struct platform_device *pdev, void __iomem *cmda_ioaddr)
{
	struct cmdq_host *cq_host;
	struct resource *cmdq_memres = NULL;

	if (cmda_ioaddr) {
		cq_host = kzalloc(sizeof(*cq_host), GFP_KERNEL);
		if (!cq_host) {
			dev_err(&pdev->dev, "allocate memory for CMDQ fail\n");
			return ERR_PTR(-ENOMEM);
		}

		cq_host->mmio = cmda_ioaddr;
	} else {
		/* check and setup CMDQ interface */
		cmdq_memres = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cmdq_mem");
		if (!cmdq_memres) {
			dev_err(&pdev->dev, "CMDQ not supported\n");
			return ERR_PTR(-EINVAL);
		}
		cq_host = kzalloc(sizeof(*cq_host), GFP_KERNEL);
		if (!cq_host) {
			dev_err(&pdev->dev, "failed to allocate memory for CMDQ\n");
			return ERR_PTR(-ENOMEM);
		}

		cq_host->mmio = devm_ioremap(&pdev->dev, cmdq_memres->start, resource_size(cmdq_memres));

		if (!cq_host->mmio) {
			dev_err(&pdev->dev, "failed to remap cmdq regs\n");
			kfree(cq_host);
			return ERR_PTR(-EBUSY);
		}

		dev_dbg(&pdev->dev, "CMDQ ioremap: done\n");
	}

	cmdq_populate_dt(pdev, cq_host);
#ifdef CONFIG_EMMC_FAULT_INJECT
	cq_host->inject_para = kzalloc(sizeof(struct cmdq_inject_para), GFP_KERNEL);
	if (!cq_host->inject_para) {
		dev_err(&pdev->dev, "failed to alloc mem for inject_para\n");
	}
#endif
	return cq_host;
}

EXPORT_SYMBOL(cmdq_pltfm_init);

int cmdq_init(struct cmdq_host *cq_host, struct mmc_host *mmc, bool dma64)
{
	unsigned int i;

	cq_host->dma64 = dma64;
	cq_host->mmc = mmc;

	/* should be parsed */
	cq_host->num_slots = 32;
	cq_host->dcmd_slot = 31;

	mmc->cmdq_slots = cq_host->num_slots;
	mmc->cmdq_ops = &cmdq_host_ops;

	cq_host->mrq_slot = kzalloc(sizeof(cq_host->mrq_slot) * cq_host->num_slots, GFP_KERNEL);
	if (!cq_host->mrq_slot)
		return -ENOMEM;
	cq_host->timer = kzalloc(sizeof(struct timer_list) * cq_host->num_slots, GFP_KERNEL);

	if (!cq_host->timer) {
		kfree(cq_host->mrq_slot);
		return -ENOMEM;
	}

	mmc->cmdq_private = cq_host;
	cq_host->check_busy = false;
	cq_host->cmd13_err_count = 0;
	cq_host->err_handle = false;

	spin_lock_init(&cq_host->cmdq_lock);

	init_completion(&cq_host->halt_comp);
	init_completion(&cq_host->clear_comp);

	cq_host->wq_resend = create_singlethread_workqueue("cmdq_wq_resend");
	INIT_WORK(&cq_host->work_resend, cmdq_work_resend);

	for (i = 0; i < cq_host->num_slots; i++) {
		setup_timer(&cq_host->timer[i], cmdq_timeout_timer, (unsigned long)cq_host);
	}

	pr_err("%s: done.\n", __func__);
	return 0;
}

EXPORT_SYMBOL(cmdq_init);
