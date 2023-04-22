/*
 * =============================================================================
 *    Description:  cmdq code in core level
 *
 *        Created:  17/03/2016
 *
 *         Author:
 *   Organization:  HISILICON
 *
 * =============================================================================
 */
#include <linux/dma-mapping.h>
#include <linux/mmc/core.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/ioprio.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <trace/events/mmc.h>

/*move from core.c*/
#define CMDQ_RETRY 32

#ifdef CONFIG_HW_MMC_MAINTENANCE_CMD
extern void record_mmc_cmdq_cmd(struct mmc_request *mrq);
#endif

extern int cmdq_clear_task(struct mmc_host *mmc, u32 task, bool entire);
extern int sdhci_cmdq_discard_task(struct mmc_host *mmc, u32 tag, bool entire);

static unsigned long cmdq_read_blocks = 0;
static unsigned long cmdq_write_blocks = 0;

extern u64 rwlog_enable_flag;

int mmc_start_cmdq_request(struct mmc_host *host,
				   struct mmc_request *mrq)
{
	int ret;
	if (mrq->data) {
		pr_debug("%s:     blksz %d blocks %d flags %08x tsac %lu ms nsac %d\n",
			mmc_hostname(host), mrq->data->blksz,
			mrq->data->blocks, mrq->data->flags,
			mrq->data->timeout_ns / NSEC_PER_MSEC,
			mrq->data->timeout_clks);
	}


	if(1 == rwlog_enable_flag)
	{
		if (mrq->data) {
			if (mrq->data->flags & MMC_DATA_READ) {
				cmdq_read_blocks += mrq->data->blocks;
				printk("%s-lifetime:     blk_addr %d blksz %d blocks %d flags %08x tsac %lu ms nsac %d (r blocks %ld)\n",
				mmc_hostname(host), mrq->cmdq_req->blk_addr, mrq->data->blksz,
				mrq->data->blocks, mrq->data->flags,
				mrq->data->timeout_ns / NSEC_PER_MSEC,
				mrq->data->timeout_clks, cmdq_read_blocks);
			}
			if (mrq->data->flags & MMC_DATA_WRITE) {
				cmdq_write_blocks += mrq->data->blocks;
				printk("%s-lifetime:     blk_addr %d blksz %d blocks %d flags %08x tsac %lu ms nsac %d (w blocks %ld)\n",
				mmc_hostname(host), mrq->cmdq_req->blk_addr, mrq->data->blksz,
				mrq->data->blocks, mrq->data->flags,
				mrq->data->timeout_ns / NSEC_PER_MSEC,
				mrq->data->timeout_clks, cmdq_write_blocks);
			}
		}
		if (mrq->cmdq_req && mrq->cmdq_req->cmdq_req_flags & DCMD) {
			if (mrq->cmd->opcode == MMC_SWITCH
				|| mrq->cmd->opcode == MMC_ERASE_GROUP_START
				|| mrq->cmd->opcode == MMC_ERASE_GROUP_END
				|| mrq->cmd->opcode == MMC_ERASE) {
				printk("%s-lifetime:     cmd=%d,index=%d,arg=%x \n",
				mmc_hostname(host), (int)mrq->cmd->opcode, host->index, mrq->cmd->arg);
			}
		}
	}


	mrq->cmd->error = 0;
	mrq->cmd->retries = CMDQ_RETRY;
	mrq->cmd->mrq = mrq;
	if (mrq->data) {
		BUG_ON(mrq->data->blksz > host->max_blk_size);
		BUG_ON(mrq->data->blocks > host->max_blk_count);
		BUG_ON(mrq->data->blocks * mrq->data->blksz >
			host->max_req_size);

		mrq->cmd->data = mrq->data;
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
	}


#ifdef CONFIG_HW_MMC_MAINTENANCE_CMD
	record_mmc_cmdq_cmd(mrq);
#endif

	ret = host->cmdq_ops->request(host, mrq);
	return ret;
}
EXPORT_SYMBOL(mmc_start_cmdq_request);

/**
 *	mmc_cmdq_post_req - post process of a completed request
 *	@host: host instance
 *	@mrq: the request to be processed
 *	@err: non-zero is error, success otherwise
 */
void mmc_cmdq_post_req(struct mmc_host *host, struct mmc_request *mrq, int err)
{
	if (host->cmdq_ops->post_req)
		host->cmdq_ops->post_req(host, mrq, err);
}
EXPORT_SYMBOL(mmc_cmdq_post_req);

/**
 *	mmc_cmdq_halt - halt/un-halt the command queue engine
 *	@host: host instance
 *	@halt: true - halt, un-halt otherwise
 *
 *	Host halts the command queue engine. It should complete
 *	the ongoing transfer and release the SD bus.
 *	All legacy SD commands can be sent upon successful
 *	completion of this function.
 *	Returns 0 on success, negative otherwise
 */
int mmc_cmdq_halt(struct mmc_host *host, bool halt)
{
	int err = 0;

	if ((halt && mmc_host_halt(host)) ||
		(!halt && !mmc_host_halt(host))) {
	    pr_debug("%s: %s: CQE is already %s\n", mmc_hostname(host),
	    				__func__, halt ? "halted" : "un-halted");
		return 0;
	}

	if (host->cmdq_ops->halt) {
		err = host->cmdq_ops->halt(host, halt);
		if (!err && halt)
			mmc_host_set_halt(host);
		else if (!err && !halt) {
			mmc_host_clr_halt(host);
			wake_up(&host->cmdq_ctx.wait);
		}
	}
	return err;
}
EXPORT_SYMBOL(mmc_cmdq_halt);

int mmc_cmdq_start_req(struct mmc_host *host, struct mmc_cmdq_req *cmdq_req)
{
	struct mmc_request *mrq = &cmdq_req->mrq;
	int ret;

	mrq->host = host;
	if (mmc_card_removed(host->card)) {
		mrq->cmd->error = -ENOMEDIUM;
		return -ENOMEDIUM;
	}
	ret = mmc_start_cmdq_request(host, mrq);
	return ret;
}
EXPORT_SYMBOL(mmc_cmdq_start_req);


/*move from mmc_ops.c*/
static inline void mmc_prepare_switch(struct mmc_command *cmd, u8 index,
				      u8 value, u8 set, unsigned int tout_ms,
				      bool use_busy_signal)
{
	cmd->opcode = MMC_SWITCH;
	cmd->arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		  (index << 16) |
		  (value << 8) |
		  set;
	cmd->flags = MMC_CMD_AC;
	cmd->busy_timeout = tout_ms;
	if (use_busy_signal)
		cmd->flags |= MMC_RSP_SPI_R1B | MMC_RSP_R1B;
	else
		cmd->flags |= MMC_RSP_SPI_R1 | MMC_RSP_R1;
}

int __mmc_switch_cmdq_mode(struct mmc_command *cmd, u8 set, u8 index, u8 value,
			   unsigned int timeout_ms, bool use_busy_signal,
			   bool ignore_timeout)
{
	mmc_prepare_switch(cmd, index, value, set, timeout_ms, use_busy_signal);
	return 0;
}
EXPORT_SYMBOL(__mmc_switch_cmdq_mode);


int mmc_cmdq_hw_reset(struct mmc_host *host)
{
	if (host->cmdq_ops->reset)
		host->cmdq_ops->reset(host);
	return mmc_hw_reset(host);
}

int mmc_discard_queue(struct mmc_host *host, u32 tasks)
{
	struct mmc_command cmd = {0};

	cmd.opcode = MMC_CMDQ_TASK_MGMT;
	if (tasks) {
		cmd.arg = DISCARD_TASK;
		cmd.arg |= (tasks << 16);
	} else {
		cmd.arg = DISCARD_QUEUE;
	}

	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;

	return mmc_wait_for_cmd(host, &cmd, 0);
}
EXPORT_SYMBOL(mmc_discard_queue);

/**
 *	mmc_cmdq_discard_card_queue - discard the task[s] in the device
 *	@host: host instance
 *	@tasks: mask of tasks to be knocked off
 *		0: remove all queued tasks
 */
int mmc_cmdq_discard_queue(struct mmc_host *host, u32 tasks)
{
	int ret;

	ret = host->cmdq_ops->discard_task(host, tasks, true);
	if (ret)
		pr_err("%s: discard queue fail\n", __func__);

	return ret;
}
EXPORT_SYMBOL(mmc_cmdq_discard_queue);
