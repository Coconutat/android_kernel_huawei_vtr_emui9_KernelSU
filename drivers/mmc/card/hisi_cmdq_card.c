/*
 * =============================================================================
 *    Description:  cmdq code in card level
 *
 *        Created:  17/03/2016
 *
 *         Author:
 *   Organization:  HISILICON
 *
 * =============================================================================
 */
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/ioprio.h>
#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <trace/events/mmc.h>
#include <linux/mmc/cmdq_hci.h>
#include <linux/hisi/mmc_trace.h>
#include <linux/reboot.h>
#include <linux/hisi/rdr_pub.h>
#ifdef CONFIG_EMMC_FAULT_INJECT
#include <linux/mmc/emmc_fault_inject.h>
#endif

/*avoid to invoke mainline code,we can only use this ugly code*/
#include "mmc_hisi_card.h"
#ifdef CONFIG_HUAWEI_EMMC_DSM
void sdhci_dsm_report(struct mmc_host *host, struct mmc_request *mrq);
#endif


#ifdef CONFIG_HW_MMC_MAINTENANCE_DATA
extern void record_cmdq_rw_data(struct mmc_request *mrq);
#endif

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01

#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88

#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
extern bool hisi_mmc_is_bkops_needed(struct mmc_card *card);
#endif
extern int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md);
extern int mmc_cmdq_discard_queue(struct mmc_host *host, u32 tasks);

#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
extern int hisi_mmc_manual_bkops_config(struct request_queue *q);
#endif

static int mmc_blk_cmdq_switch(struct mmc_card *card,
			struct mmc_blk_data *md, bool enable)
{
	int ret = 0;
	bool cmdq_mode = !!mmc_card_cmdq(card);
	struct mmc_host *host = card->host;

	if (!card->ext_csd.cmdq_mode_en ||
	    (enable && md && !(md->flags & MMC_BLK_CMD_QUEUE)) ||
	    (cmdq_mode == enable))
		return 0;


	if (host->cmdq_ops) {
		if (enable) {
			ret = mmc_set_blocklen(card, MMC_CARD_CMDQ_BLK_SIZE);
			if (ret) {
				pr_err("%s: failed to set block-size to 512\n",
				       __func__);
				BUG();
			}

			ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 EXT_CSD_CMDQ_MODE, enable,
					 card->ext_csd.generic_cmd6_time);
			if (ret) {
				pr_err("cmdq mode %sable failed %d\n",
				       enable ? "en" : "dis", ret);
				goto out;
			}
			mmc_card_set_cmdq(card);

			/* enable host controller command queue engine */
			ret = host->cmdq_ops->enable(card->host);
			if (ret) {
				pr_err("failed to enable host controller cqe %d\n",
						ret);
			}

		}

		if (ret || !enable) {
			ret = host->cmdq_ops->disable(card->host, true);
			if (ret) {
				pr_err("failed to disable host controller cqe %d\n", ret);
			}
			/* disable CQ mode in card */
			ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_CMDQ_MODE, 0,
					card->ext_csd.generic_cmd6_time);
			if (ret) {
				pr_err("cmdq mode %sable failed %d\n",
					enable ? "en" : "dis", ret);
				BUG();
			}
			mmc_card_clr_cmdq(card);
		}
	} else {
		pr_err("%s: No cmdq ops defined !!!\n", __func__);
		BUG();
	}

out:

	return ret;
}

int mmc_blk_cmdq_hangup(struct mmc_card *card)
{
	struct mmc_cmdq_context_info *ctx_info;
	unsigned long timeout = (1 * 60 * 1000);
	int ret = 0;

	if (card->ext_csd.cmdq_mode_en) {
		ctx_info = &card->host->cmdq_ctx;
		set_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx_info->curr_state);
		/* wait for cmdq req handle done.*/
		while (ctx_info->active_reqs) {
			if (timeout == 0) {
				pr_err("%s: wait cmdq complete reqs timeout !\n", __func__);
			        return -ETIMEDOUT;
			}
			timeout--;
			mdelay(1);
		}
		/* disable CQ mode for ioctl */
		ret = mmc_blk_cmdq_switch(card, NULL, false);
	}
	return ret;
}
EXPORT_SYMBOL(mmc_blk_cmdq_hangup);

void mmc_blk_cmdq_restore(struct mmc_card *card)
{
	struct mmc_cmdq_context_info *ctx_info;

	if (card->ext_csd.cmdq_mode_en) {
		ctx_info = &card->host->cmdq_ctx;
		clear_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx_info->curr_state);
		wake_up(&card->host->cmdq_ctx.wait);
	}
}
EXPORT_SYMBOL(mmc_blk_cmdq_restore);


/**
 * mmc_blk_cmdq_halt - wait for dbr finished and halt cqe;
 * @card:	mmc card;
 * wait for dbr finished and halt cqe,then we can issue a
 * legacy command like flush;
 */
int mmc_blk_cmdq_halt(struct mmc_card *card)
{
	struct mmc_cmdq_context_info *ctx_info;
	int ret = 0;
	struct mmc_host *host = card->host;

	if (card->ext_csd.cmdq_mode_en && (!!mmc_card_cmdq(card))) {
		ctx_info = &card->host->cmdq_ctx;
		/*make sure blk sends no more request*/
		set_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx_info->curr_state);

		/* make sure there are no tasks transfer by cqe and
		 * cqe halt success
		 */
		ret = host->cmdq_ops->clear_and_halt(card->host);
		if (!ret)
			mmc_host_set_halt(host);
		else {
			pr_err("%s: halt fail, ret = %d\n", __func__, ret);
			clear_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx_info->curr_state);
			wake_up(&ctx_info->wait);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mmc_blk_cmdq_halt);

void mmc_blk_cmdq_dishalt(struct mmc_card *card)
{
	struct mmc_cmdq_context_info *ctx_info;
	struct mmc_host *host = card->host;

	if (card->ext_csd.cmdq_mode_en) {
		ctx_info = &card->host->cmdq_ctx;
		host->cmdq_ops->halt(host, (bool)false);
		clear_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx_info->curr_state);
		mmc_host_clr_halt(host);
		wake_up(&ctx_info->wait);
	}
}
EXPORT_SYMBOL(mmc_blk_cmdq_dishalt);

static int mmc_blk_cmdq_start_req(struct mmc_host *host,
				   struct mmc_cmdq_req *cmdq_req)
{
	struct mmc_request *mrq = &cmdq_req->mrq;

	/*cmdq_req->cmdq_req_flags |= QBR;*/

	mrq->done = mmc_blk_cmdq_req_done;
	return mmc_cmdq_start_req(host, cmdq_req);
}

/* prepare for non-data commands */
struct mmc_cmdq_req *mmc_cmdq_prep_dcmd(
		struct mmc_queue_req *mqrq, struct mmc_queue *mq)
{
	struct request *req = mqrq->req;
	struct mmc_cmdq_req *cmdq_req = &mqrq->mmc_cmdq_req;

	memset(&mqrq->mmc_cmdq_req, 0, sizeof(struct mmc_cmdq_req));

	cmdq_req->mrq.data = NULL;
	cmdq_req->cmd_flags = req->cmd_flags;
	cmdq_req->mrq.req = mqrq->req;
	req->special = mqrq;
	cmdq_req->cmdq_req_flags |= DCMD;
	cmdq_req->mrq.cmdq_req = cmdq_req;

	return &mqrq->mmc_cmdq_req;
}
EXPORT_SYMBOL(mmc_cmdq_prep_dcmd);

#define IS_RT_CLASS_REQ(x)     \
	(IOPRIO_PRIO_CLASS(req_get_ioprio(x)) == IOPRIO_CLASS_RT)

static struct mmc_cmdq_req *mmc_blk_cmdq_rw_prep(
		struct mmc_queue_req *mqrq, struct mmc_queue *mq)
{
	struct mmc_card *card = mq->card;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_rel_wr = mmc_req_rel_wr(req) && (md->flags & MMC_BLK_REL_WR);
	bool do_data_tag;
	bool read_dir = (rq_data_dir(req) == READ);
	bool prio = IS_RT_CLASS_REQ(req);
	struct mmc_cmdq_req *cmdq_rq = &mqrq->mmc_cmdq_req;
	u32 map_sg_len = 0;

	memset(&mqrq->mmc_cmdq_req, 0, sizeof(struct mmc_cmdq_req));

	cmdq_rq->tag = req->tag;
	if (read_dir) {
		cmdq_rq->cmdq_req_flags |= DIR;
		cmdq_rq->data.flags = MMC_DATA_READ;
	} else {
		cmdq_rq->data.flags = MMC_DATA_WRITE;
	}
	if (prio)
		cmdq_rq->cmdq_req_flags |= PRIO;

	if (do_rel_wr)
		cmdq_rq->cmdq_req_flags |= REL_WR;

	cmdq_rq->data.blocks = blk_rq_sectors(req);
	cmdq_rq->blk_addr = blk_rq_pos(req);
	cmdq_rq->data.blksz = MMC_CARD_CMDQ_BLK_SIZE;
	cmdq_rq->data.bytes_xfered = 0;

	mmc_set_data_timeout(&cmdq_rq->data, card);

	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((cmdq_rq->data.blocks * cmdq_rq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);
	if (do_data_tag)
		cmdq_rq->cmdq_req_flags |= DAT_TAG;
	cmdq_rq->data.sg = mqrq->sg;
	cmdq_rq->data.sg_len = mmc_queue_map_sg(mq, mqrq);
	map_sg_len = cmdq_rq->data.sg_len;

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (cmdq_rq->data.blocks > card->host->max_blk_count)
		cmdq_rq->data.blocks = card->host->max_blk_count;

	if (cmdq_rq->data.blocks != blk_rq_sectors(req)) {
		int  data_size = cmdq_rq->data.blocks << 9;
		unsigned int  i = 0;
		struct scatterlist *sg;

		for_each_sg(cmdq_rq->data.sg, sg, cmdq_rq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		cmdq_rq->data.sg_len = i;
	}

	mqrq->mmc_cmdq_req.cmd_flags = req->cmd_flags;
	mqrq->mmc_cmdq_req.mrq.req = mqrq->req;
	mqrq->mmc_cmdq_req.mrq.cmdq_req = &mqrq->mmc_cmdq_req;
	mqrq->mmc_cmdq_req.mrq.data = &mqrq->mmc_cmdq_req.data;
	/* mrq.cmd: no opcode, just for record error */
	mqrq->mmc_cmdq_req.mrq.cmd = &mqrq->mmc_cmdq_req.cmd;
	mqrq->req->special = mqrq;

	pr_debug("%s: %s: mrq: 0x%pK req: 0x%pK mqrq: 0x%pK bytes to xf: %d mmc_cmdq_req: 0x%pK card-addr: 0x%08x data_sg_len: %d map_sg_len: %d dir(r-1/w-0): %d\n",
		 mmc_hostname(card->host), __func__, &mqrq->mmc_cmdq_req.mrq,
		 mqrq->req, mqrq, (cmdq_rq->data.blocks * cmdq_rq->data.blksz),
		 cmdq_rq, cmdq_rq->blk_addr, cmdq_rq->data.sg_len, map_sg_len,
		 (cmdq_rq->cmdq_req_flags & DIR) ? 1 : 0);
/*there is no iotrace.c in linux 4.1 and the iotrace
 *use a public config.(CONFIG_TRACING) += iotrace.o
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
#else
	trace_mmc_blk_cmdq_rw_start(cmdq_rq->cmdq_req_flags, cmdq_rq->tag, cmdq_rq->blk_addr,
		(cmdq_rq->data.blocks * cmdq_rq->data.blksz));
#endif

	return &mqrq->mmc_cmdq_req;
}

static int mmc_blk_cmdq_issue_rw_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_host *host = card->host;
	struct mmc_cmdq_req *mc_rq;
	int ret = 0;

	BUG_ON((req->tag < 0) || (req->tag > card->ext_csd.cmdq_depth));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.data_active_reqs));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;

	mc_rq = mmc_blk_cmdq_rw_prep(active_mqrq, mq);


	ret = mmc_blk_cmdq_start_req(card->host, mc_rq);
	return ret;
}

void mmc_blk_cmdq_dcmd_done(struct mmc_request *mrq)
{
	complete(&mrq->cmdq_completion);
}

static int mmc_blk_cmdq_wait_for_dcmd(struct mmc_host *host,
				   struct mmc_cmdq_req *cmdq_req)
{
	struct mmc_request *mrq = &cmdq_req->mrq;
	int ret;

	init_completion(&mrq->cmdq_completion);
	mrq->done = mmc_blk_cmdq_dcmd_done;
	mrq->host = host;
	ret = mmc_start_cmdq_request(host, mrq);
	if (ret) {
		pr_err("%s: DCMD error\n", __func__);
		return ret;
	}

	cmdq_req->cmdq_req_flags |= WAIT_COMPLETE;
	wait_for_completion_io(&mrq->cmdq_completion);
	if (mrq->cmd->error) {
		pr_err("%s: DCMD %d failed with err %d\n",
			mmc_hostname(host), mrq->cmd->opcode, mrq->cmd->error);
		ret = mrq->cmd->error;
	}
	return ret;
}

static int mmc_cmdq_do_erase(struct mmc_card *card, struct mmc_queue *mq, struct request *req, unsigned int from,
			unsigned int to, unsigned int arg)
{
	unsigned int qty = 0;
	unsigned int fr, nr;
	int err;
	struct mmc_queue_req *active_mqrq;
	struct mmc_cmdq_context_info *ctx_info;
	struct mmc_cmdq_req *cmdq_req;

	fr = from;
	nr = to - from + 1;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	trace_mmc_blk_erase_start(arg, fr, nr);
#endif

	/*
	 * qty is used to calculate the erase timeout which depends on how many
	 * erase groups (or allocation units in SD terminology) are affected.
	 * We count erasing part of an erase group as one erase group.
	 * For SD, the allocation units are always a power of 2.  For MMC, the
	 * erase group size is almost certainly also power of 2, but it does not
	 * seem to insist on that in the JEDEC standard, so we fall back to
	 * division in that case.  SD may not specify an allocation unit size,
	 * in which case the timeout is based on the number of write blocks.
	 *
	 * Note that the timeout for secure trim 2 will only be correct if the
	 * number of erase groups specified is the same as the total of all
	 * preceding secure trim 1 commands.  Since the power may have been
	 * lost since the secure trim 1 commands occurred, it is generally
	 * impossible to calculate the secure trim 2 timeout correctly.
	 */
	if (card->erase_shift)
		qty += ((to >> card->erase_shift) -
			(from >> card->erase_shift)) + 1;
	else if (mmc_card_sd(card))
		qty += to - from + 1;
	else
		qty += ((to / card->erase_size) -
			(from / card->erase_size)) + 1;

	if (!mmc_card_blockaddr(card)) {
		from <<= 9;
		to <<= 9;
	}

	ctx_info = &card->host->cmdq_ctx;
	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;
	cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
	cmdq_req->cmdq_req_flags |= QBR;
	cmdq_req->mrq.cmd = &cmdq_req->cmd;
	cmdq_req->tag = req->tag;
	cmdq_req->cmd.opcode = MMC_ERASE_GROUP_START;
	cmdq_req->cmd.arg = from;
	cmdq_req->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
	if (err) {
		pr_err("mmc_erase: group start error %d.", err);
		goto out;
	}

	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;
	cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
	cmdq_req->cmdq_req_flags |= QBR;
	cmdq_req->mrq.cmd = &cmdq_req->cmd;
	cmdq_req->tag = req->tag;
	cmdq_req->cmd.opcode = MMC_ERASE_GROUP_END;
	cmdq_req->cmd.arg = to;
	cmdq_req->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
	if (err) {
		pr_err("mmc_erase: group end error %d.\n", err);
		goto out;
	}

	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;
	cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
	cmdq_req->cmdq_req_flags |= QBR;
	cmdq_req->mrq.cmd = &cmdq_req->cmd;
	cmdq_req->tag = req->tag;
	cmdq_req->cmd.opcode = MMC_ERASE;
	cmdq_req->cmd.arg = arg;
	cmdq_req->cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	cmdq_req->cmd.busy_timeout = mmc_erase_timeout(card, arg, qty);

	err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
	if (err) {
		pr_err("mmc_erase: erase error %d.\n", err);
		goto out;
	}

out:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	trace_mmc_blk_erase_end(arg, fr, nr);
#endif

	return err;
}

static int mmc_cmdq_erase(struct mmc_card *card, struct mmc_queue *mq, struct request *req,
		unsigned int from, unsigned int nr, unsigned int arg)
{
	unsigned int rem, to = from + nr;/* [false alarm]*/

	if (!(card->host->caps & MMC_CAP_ERASE) ||
	    !(card->csd.cmdclass & CCC_ERASE))
		return -EOPNOTSUPP;

	if (!card->erase_size)
		return -EOPNOTSUPP;

	if (mmc_card_sd(card) && arg != MMC_ERASE_ARG)
		return -EOPNOTSUPP;

	if ((arg & MMC_SECURE_ARGS) &&
	    !(card->ext_csd.sec_feature_support & EXT_CSD_SEC_ER_EN))
		return -EOPNOTSUPP;

	if ((arg & MMC_TRIM_ARGS) &&
	    !(card->ext_csd.sec_feature_support & EXT_CSD_SEC_GB_CL_EN))
		return -EOPNOTSUPP;

	if (arg == MMC_SECURE_ERASE_ARG) {
		if (from % card->erase_size || nr % card->erase_size)
			return -EINVAL;
	}

	if (arg == MMC_ERASE_ARG) {
		rem = from % card->erase_size;
		if (rem) {
			rem = card->erase_size - rem;
			from += rem;
			if (nr > rem)
				nr -= rem;
			else
				return 0;
		}
		rem = nr % card->erase_size;
		if (rem)
			nr -= rem;
	}

	if (nr == 0)
		return 0;

	to = from + nr;

	if (to <= from)
		return -EINVAL;

	/* 'from' and 'to' are inclusive */
	to -= 1;

	return mmc_cmdq_do_erase(card, mq, req, from, to, arg);
}

static int mmc_blk_cmdq_issue_discard_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_host *host;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_cmdq_context_info *ctx_info;
	unsigned int from, nr, arg;
	int err = 0;

	BUG_ON(!card);
	host = card->host;
	BUG_ON(!host);
	BUG_ON((req->tag < 0) || (req->tag > card->ext_csd.cmdq_depth));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

	ctx_info = &host->cmdq_ctx;

	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_discard(card))
		arg = MMC_DISCARD_ARG;
	else if (mmc_can_trim(card))
		arg = MMC_TRIM_ARG;
	else
		arg = MMC_ERASE_ARG;

	set_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);

	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		active_mqrq = &mq->mqrq_cmdq[req->tag];
		active_mqrq->req = req;
		cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
		cmdq_req->cmdq_req_flags |= QBR;
		cmdq_req->mrq.cmd = &cmdq_req->cmd;
		cmdq_req->tag = req->tag;

		err = __mmc_switch_cmdq_mode(cmdq_req->mrq.cmd, EXT_CSD_CMD_SET_NORMAL,
				INAND_CMD38_ARG_EXT_CSD,
				arg == MMC_TRIM_ARG ? INAND_CMD38_ARG_TRIM : INAND_CMD38_ARG_ERASE,
				0, true, true);
		if (err)
			goto out;
		err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
		if (err)
			goto out;
	}

	err = mmc_cmdq_erase(card, mq, req, from, nr, arg);

out:
	if (err == -EBADSLT)
		return err;

	blk_complete_request(req);
	return err;
}

static int mmc_blk_cmdq_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)
{
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_host *host;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_cmdq_context_info *ctx_info;
	unsigned int from, nr, arg;
	int err = 0;

	BUG_ON(!card);
	host = card->host;
	BUG_ON(!host);
	BUG_ON((req->tag < 0) || (req->tag > card->ext_csd.cmdq_depth));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

	ctx_info = &host->cmdq_ctx;

	if (!(mmc_can_secure_erase_trim(card))) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;

	set_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);

	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		active_mqrq = &mq->mqrq_cmdq[req->tag];
		active_mqrq->req = req;
		cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
		cmdq_req->cmdq_req_flags |= QBR;
		cmdq_req->mrq.cmd = &cmdq_req->cmd;
		cmdq_req->tag = req->tag;
		err = __mmc_switch_cmdq_mode(cmdq_req->mrq.cmd, EXT_CSD_CMD_SET_NORMAL,
				INAND_CMD38_ARG_EXT_CSD,
				arg == MMC_SECURE_TRIM1_ARG ? INAND_CMD38_ARG_SECTRIM1 : INAND_CMD38_ARG_SECERASE,
				0, true, true);
		if (err)
			goto out;
		err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
		if (err)
			goto out;
	}
	err = mmc_cmdq_erase(card, mq, req, from, nr, arg);
	if (err)
		goto out;

	if (arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			active_mqrq = &mq->mqrq_cmdq[req->tag];
			active_mqrq->req = req;
			cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
			cmdq_req->cmdq_req_flags |= QBR;
			cmdq_req->mrq.cmd = &cmdq_req->cmd;
			cmdq_req->tag = req->tag;
			err = __mmc_switch_cmdq_mode(cmdq_req->mrq.cmd, EXT_CSD_CMD_SET_NORMAL,
					INAND_CMD38_ARG_EXT_CSD,
					INAND_CMD38_ARG_SECTRIM2,
					0, true, true);
			if (err)
				goto out;
			err = mmc_blk_cmdq_wait_for_dcmd(card->host, cmdq_req);
			if (err)
				goto out;
		}
		err = mmc_cmdq_erase(card, mq, req, from, nr, MMC_SECURE_TRIM2_ARG);
		if (err)
			goto out;
	}

out:
	if (err == -EBADSLT)
		return err;

	blk_complete_request(req);

	return err ? 0 : 1;
}

/*
 * Issues a dcmd request
 * FIXME:
 *	Try to pull another request from queue and prepare it in the
 *	meantime. If its not a dcmd it can be issued as well.
 */
int mmc_blk_cmdq_issue_flush_rq(struct mmc_queue *mq, struct request *req)
{
	int err;
	struct mmc_queue_req *active_mqrq;
	struct mmc_card *card = mq->card;
	struct mmc_host *host;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_cmdq_context_info *ctx_info;

	BUG_ON(!card);
	host = card->host;
	BUG_ON(!host);
	BUG_ON((req->tag < 0) || (req->tag > card->ext_csd.cmdq_depth));
	BUG_ON(test_and_set_bit(req->tag, &host->cmdq_ctx.active_reqs));

	ctx_info = &host->cmdq_ctx;
	set_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);
	active_mqrq = &mq->mqrq_cmdq[req->tag];
	active_mqrq->req = req;

	cmdq_req = mmc_cmdq_prep_dcmd(active_mqrq, mq);
	cmdq_req->cmdq_req_flags |= QBR;
	cmdq_req->mrq.cmd = &cmdq_req->cmd;
	cmdq_req->tag = req->tag;

	err = __mmc_switch_cmdq_mode(cmdq_req->mrq.cmd, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_FLUSH_CACHE, 1,
				     MMC_FLUSH_REQ_TIMEOUT_MS, true, true);
	if (err)
		return err;

	err = mmc_blk_cmdq_start_req(card->host, cmdq_req);
	return err;
}
EXPORT_SYMBOL(mmc_blk_cmdq_issue_flush_rq);

static void mmc_blk_cmdq_reset(struct mmc_host *host, bool clear_all)
{
	int err = 0;

	if (mmc_cmdq_halt(host, true)) {
		pr_err("%s: halt failed\n", mmc_hostname(host));
		goto reset;
	}

	if (clear_all) {
		mmc_cmdq_discard_queue(host, 0);
	}

reset:
	host->cmdq_ops->disable_immediatly(host);
	err = mmc_cmdq_hw_reset(host);
	if (err == -EOPNOTSUPP) {
		pr_err("%s: not support reset\n", __func__);
		host->cmdq_ops->enable(host);
		goto out;
	}

	/*
	 * CMDQ HW reset would have already made CQE
	 * in unhalted state, but reflect the same
	 * in software state of cmdq_ctx.
	 */
	mmc_host_clr_halt(host);
	mmc_card_clr_cmdq(host->card);
out:
	return;
}

/**
 * is_cmdq_dcmd_req - Checks if tag belongs to DCMD request.
 * @q:          request_queue pointer.
 * @tag:        tag number of request to check.
 *
 * This function checks if the request with tag number "tag"
 * is a DCMD request or not based on cmdq_req_flags set.
 *
 * returns true if DCMD req, otherwise false.
 */
static int is_cmdq_dcmd_req(struct request_queue *q, int tag)
{
        struct request *req;
        struct mmc_queue_req *mq_rq;
        struct mmc_cmdq_req *cmdq_req;

        req = blk_queue_find_tag(q, tag);
        if (WARN_ON(!req))
                goto out;
        mq_rq = req->special; /* [false alarm]:WARN_ON mq_rq and req */
        if (WARN_ON(!mq_rq))
                goto out;
        cmdq_req = &(mq_rq->mmc_cmdq_req);/* [false alarm]:mmc_cmdq_req can't be NULL */
        return (cmdq_req->cmdq_req_flags & DCMD);
out:
        return -ENOENT;
}

/* While doing error handling, if a discard request is waiting, it maybe
 * block in function mmc_cmdq_wait_for_dcmd -> wait_for_completion_io,
 * So, we should do the completion if it is waiting.
 */
static void mmc_cmdq_dcmd_reset(struct request_queue *q, int tag)
{
	struct request *req;
	struct mmc_queue_req *mq_rq;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_request *mrq;

	req = blk_queue_find_tag(q, tag);
	if (WARN_ON(!req))
		return;
	mq_rq = req->special;
	if (WARN_ON(!mq_rq))
		return;
	cmdq_req = &(mq_rq->mmc_cmdq_req);
	if (cmdq_req->cmdq_req_flags & WAIT_COMPLETE) {
		mrq = &cmdq_req->mrq;
		if (mrq && !completion_done(&mrq->cmdq_completion)) {
			pr_err("%s: discard req reset done\n", __func__);
			mrq->cmd->error = -EBADSLT;
			mmc_blk_cmdq_dcmd_done(mrq);
		}
	}
}

/**
 * mmc_blk_cmdq_reset_all - Reset everything for CMDQ block request.
 * @host:       mmc_host pointer.
 * @err:        error for which reset is performed.
 *
 * This function implements reset_all functionality for
 * cmdq. It resets the controller, power cycle the card,
 * and invalidate all busy tags(requeue all request back to
 * elevator).
 */
static void mmc_blk_cmdq_reset_all(struct mmc_host *host, int err)
{
	struct mmc_request *mrq = host->err_mrq;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	struct request_queue *q;
	int itag = 0;
	int ret = 0;


	WARN_ON(!test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state));

	pr_debug("%s: %s: active_reqs = %lu \n",
			mmc_hostname(host), __func__,
			ctx_info->active_reqs );

	mmc_blk_cmdq_reset(host, false);
	if (test_bit(CMDQ_STATE_SWITCH_ERR, &ctx_info->curr_state)) {
		q = host->switch_err_req->q;
		mmc_release_host(host);
		goto invalidate_tags;
	}

	q = mrq->req->q;/* [false alarm]:req can't be NULL */
	for_each_set_bit(itag, &ctx_info->active_reqs,
			(int)(host->cmdq_slots)) {
		ret = is_cmdq_dcmd_req(q, itag);
		if (WARN_ON(ret == -ENOENT))
			continue;

		if (!ret) {
			WARN_ON(!test_and_clear_bit(itag,
					&ctx_info->data_active_reqs));
			mmc_cmdq_post_req(host, mrq, err);
		} else {
			mmc_cmdq_dcmd_reset(q, itag);
			clear_bit(CMDQ_STATE_DCMD_ACTIVE,
					&ctx_info->curr_state);
		}
		WARN_ON(!test_and_clear_bit(itag, &ctx_info->active_reqs));
		mmc_release_host(host);
	}

	/*if there is a waiting dcmd, ignore it*/
	//host->cmdq_ctx.in_recovery = true;
	//wake_up_interruptible(&host->cmdq_ctx.queue_empty_wq);

invalidate_tags:
	spin_lock_irq(q->queue_lock);
	blk_queue_invalidate_tags(q);
	spin_unlock_irq(q->queue_lock);
}


void mmc_blk_cmdq_shutdown(struct mmc_queue *mq)
{
	int err;
	struct mmc_card *card = mq->card;
	struct mmc_host *host = card->host;

	mmc_claim_host(host);
	err = mmc_cmdq_halt(host, true);
	if (err) {
		pr_err("%s: halt: failed: %d\n", __func__, err);
	}
	host->cmdq_ops->disable(host, false);

	/* disable CQ mode in card */
	if (mmc_card_cmdq(card)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_CMDQ_MODE, 0,
						card->ext_csd.generic_cmd6_time);
		if (err) {
			pr_err("%s: failed to switch card to legacy mode: %d\n",
			__func__, err);
			goto out;
		}
		mmc_card_clr_cmdq(card);
	}

	host->card->cmdq_init = false;
out:
	mmc_release_host(host);
}

/*
 * mmc_blk_cmdq_err: error handling of cmdq error requests.
 * Function should be called in context of error out request
 * which has claim_host and rpm acquired.
 * This may be called with CQ engine halted. Make sure to
 * unhalt it after error recovery.
 *
 * TODO: Currently cmdq error handler does reset_all in case
 * of any erorr. Need to optimize error handling.
 */
void mmc_blk_cmdq_err(struct mmc_queue *mq)
{
	struct mmc_host *host = mq->card->host;
	struct mmc_request *mrq = host->err_mrq;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	struct request_queue *q;
	int err = -1;

	if (test_bit(CMDQ_STATE_SWITCH_ERR, &ctx_info->curr_state))
		goto reset;

	if (WARN_ON(!mrq))
		return;

	/*if there is a waiting dcmd, ignore it*/
	host->cmdq_ctx.in_recovery = true;
	wake_up_interruptible(&host->cmdq_ctx.queue_empty_wq);

	q = mrq->req->q;/* [false alarm]:req can't be NULL */
	err = mmc_cmdq_halt(host, true);
	if (err) {
		pr_err("halt: failed: %d\n", err);
		goto reset;
	}

	/* RED error - Fatal: requires reset */
	if (mrq->cmdq_req->resp_err) {
		err = mrq->cmdq_req->resp_err;
		pr_crit("%s: Response error detected: Device in bad state\n",
				mmc_hostname(host));
		goto reset;
	}

	/*
	 * In case of software request time-out, we schedule err work only for
	 * the first error out request and handles all other request in flight
	 * here.
	 */
	if (test_bit(CMDQ_STATE_REQ_TIMED_OUT, &ctx_info->curr_state)) {
		err = -ETIMEDOUT;
	} else if (mrq->data && mrq->data->error) {
		err = mrq->data->error;
	} else if (mrq->cmd && mrq->cmd->error) {
		/* DCMD commands */
		err = mrq->cmd->error;
	}

reset:
	mmc_blk_cmdq_reset_all(host, err);
	if (mrq && mrq->cmdq_req->resp_err)
		mrq->cmdq_req->resp_err = false;
	mmc_cmdq_halt(host, false);

	host->err_mrq = NULL;
	host->cmdq_ctx.in_recovery = false;
	clear_bit(CMDQ_STATE_REQ_TIMED_OUT, &ctx_info->curr_state);
	clear_bit(CMDQ_STATE_SWITCH_ERR, &ctx_info->curr_state);
	WARN_ON(!test_and_clear_bit(CMDQ_STATE_ERR, &ctx_info->curr_state));
	wake_up(&ctx_info->wait);
}


/* invoked by block layer in softirq context */
void mmc_blk_cmdq_complete_rq(struct request *rq)
{
	struct mmc_queue_req *mq_rq = rq->special;
	struct mmc_request *mrq = &mq_rq->mmc_cmdq_req.mrq;
	struct mmc_host *host = mrq->host;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	struct mmc_cmdq_req *cmdq_req = &mq_rq->mmc_cmdq_req;
	struct mmc_queue *mq = (struct mmc_queue *)rq->q->queuedata;
	int err = 0;
	bool curr_req_clear = false;

	if (mrq->cmd && mrq->cmd->error)
		err = mrq->cmd->error;
	else if (mrq->data && mrq->data->error)
		err = mrq->data->error;

	if (err || cmdq_req->resp_err) {
		pr_err("%s: request with req: 0x%pK, tag: %d, flags: 0x%llx,"
			"curr_state:0x%lx, active reqs:0x%lx timed out\n",
			__func__, rq, rq->tag, rq->cmd_flags,
			ctx_info->curr_state, ctx_info->active_reqs);

		pr_err("%s: %s: txfr error(%d)/resp_err(%d)\n",
			mmc_hostname(mrq->host), __func__, err,
			cmdq_req->resp_err);

		if (test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state)) {
			pr_err("%s: CQ in error state, ending current req: %d\n",
				__func__, err);
		} else {
			spin_lock_bh(&ctx_info->cmdq_ctx_lock);
			set_bit(CMDQ_STATE_ERR, &ctx_info->curr_state);
			spin_unlock_bh(&ctx_info->cmdq_ctx_lock);
			BUG_ON(host->err_mrq != NULL);
			host->err_mrq = mrq;
			schedule_work(&mq->cmdq_err_work);
		}
		goto out;
	}

	/*
	 * In case of error CMDQ is expected to be either in halted
	 * or disable state so cannot receive any completion of
	 * other requests.
	 */
	spin_lock_bh(&ctx_info->cmdq_ctx_lock);
	if (test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state)) {
		spin_unlock_bh(&ctx_info->cmdq_ctx_lock);
		pr_err("%s: softirq may come from different cpu cluster, curr_state:0x%lx\n",
			__func__, ctx_info->curr_state);
		WARN_ON(1);
		return;
	}

	/* clear pending request */
	BUG_ON(!test_and_clear_bit(cmdq_req->tag, &ctx_info->active_reqs));
	curr_req_clear = true;

	if (cmdq_req->cmdq_req_flags & DCMD) {
		clear_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx_info->curr_state);
		blk_end_request_all(rq, err);
	 } else {
		BUG_ON(!test_and_clear_bit(cmdq_req->tag,
					 &ctx_info->data_active_reqs));
		mmc_cmdq_post_req(host, mrq, err);
		blk_end_request(rq, err, cmdq_req->data.bytes_xfered);
	 }

	mmc_release_host(host);
	spin_unlock_bh(&ctx_info->cmdq_ctx_lock);

out:

	spin_lock_bh(&ctx_info->cmdq_ctx_lock);
	if (!test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state)) {
		wake_up(&ctx_info->wait);
	} else {
		if (curr_req_clear)
			pr_err("%s: CMDQ_STATE_ERR bit is set after req is clear\n", __func__);
	}
	spin_unlock_bh(&ctx_info->cmdq_ctx_lock);

	if (!ctx_info->active_reqs)
		wake_up_interruptible(&host->cmdq_ctx.queue_empty_wq);

	if (blk_queue_stopped(mq->queue) && !ctx_info->active_reqs)
		complete(&mq->cmdq_shutdown_complete);

	return;
}

enum blk_eh_timer_return mmc_blk_cmdq_req_timed_out(struct request *req)
{
	struct mmc_queue *mq = req->q->queuedata;
	struct mmc_host *host = mq->card->host;
	struct mmc_queue_req *mq_rq = req->special;
	struct mmc_request *mrq;
	struct mmc_cmdq_req *cmdq_req;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;
	struct mmc_cmdq_task_info *cmdq_task_info = host->cmdq_task_info;

	BUG_ON(!host);

	pr_err("%s: request with req: 0x%pK, tag: %d, flags: 0x%llx,"
		"curr_state:0x%lx, active reqs:0x%lx timed out\n",
		__func__, req, req->tag, req->cmd_flags,
		ctx_info->curr_state, ctx_info->active_reqs);
	pr_err("%s: issue time:%lld, start time:%lld, end time:%lld\n",
		__func__,
		ktime_to_ns(cmdq_task_info[req->tag].issue_time),
		ktime_to_ns(cmdq_task_info[req->tag].start_dbr_time),
		ktime_to_ns(cmdq_task_info[req->tag].end_dbr_time));

	if (!test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state))
		host->cmdq_ops->dumpstate(host);

	/*
	 * The mmc_queue_req will be present only if the request
	 * is issued to the LLD. The request could be fetched from
	 * block layer queue but could be waiting to be issued
	 * (for e.g. clock scaling is waiting for an empty cmdq queue)
	 * Reset the timer in such cases to give LLD more time
	 */
	if (!mq_rq) {
		pr_err("%s:req 0x%pK restart timer for tag: %d, curr_state:0x%lx, active reqs:0x%lx\n",
			__func__, req, req->tag, ctx_info->curr_state, ctx_info->active_reqs);
		return BLK_EH_RESET_TIMER;
	}

	mrq = &mq_rq->mmc_cmdq_req.mrq;
	cmdq_req = &mq_rq->mmc_cmdq_req;

	BUG_ON(!mrq || !cmdq_req);

#ifdef CONFIG_HUAWEI_EMMC_DSM
	sdhci_dsm_report(host, mrq);
#endif

	if (cmdq_req->cmdq_req_flags & DCMD)
		mrq->cmd->error = -ETIMEDOUT;
	else
		mrq->data->error = -ETIMEDOUT;

	if (mrq->cmd && mrq->cmd->error) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		if (!(mrq->req->cmd_flags & REQ_FLUSH)) {
#else
		if (req_op(mrq->req) != REQ_OP_FLUSH) {
#endif
			/*
			 * Notify completion for non flush commands like
			 * discard that wait for DCMD finish.
			 */
			set_bit(CMDQ_STATE_REQ_TIMED_OUT,
					&ctx_info->curr_state);
			complete(&mrq->cmdq_completion);
			return BLK_EH_NOT_HANDLED;
		}
	}

	if (test_bit(CMDQ_STATE_REQ_TIMED_OUT, &ctx_info->curr_state) ||
		test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state))
		return BLK_EH_NOT_HANDLED;

	set_bit(CMDQ_STATE_REQ_TIMED_OUT, &ctx_info->curr_state);
	return BLK_EH_HANDLED;
}


/*
 * Complete reqs from block layer softirq context
 * Invoked in irq context
 */
void mmc_blk_cmdq_req_done(struct mmc_request *mrq)
{
	struct request *req = mrq->req;

#ifdef CONFIG_HW_MMC_MAINTENANCE_DATA
	record_cmdq_rw_data(mrq);
#endif

#ifdef CONFIG_EMMC_FAULT_INJECT
	if (mmcdbg_cq_timeout_inj(mrq, ERR_INJECT_CMDQ_TIMEOUT)) {
		return;
	}
#endif
	blk_complete_request(req);
}
EXPORT_SYMBOL(mmc_blk_cmdq_req_done);

void mmc_cmdq_task_info_init(struct mmc_card *card, struct request *req)
{
	if (card->host->cmdq_task_info) {
		card->host->cmdq_task_info[req->tag].req = req;
		card->host->cmdq_task_info[req->tag].issue_time = ktime_get();
		card->host->cmdq_task_info[req->tag].start_dbr_time.tv64 = 0UL;
		card->host->cmdq_task_info[req->tag].end_dbr_time.tv64 = 0UL;
	}
}

void mmc_cmdq_switch_error(struct mmc_card *card, struct mmc_queue *mq, struct request *req)
{
	struct mmc_host *host = card->host;
	struct mmc_cmdq_context_info *ctx_info = &host->cmdq_ctx;

	if (test_bit(CMDQ_STATE_ERR, &ctx_info->curr_state)) {
		pr_err("%s: CQ in error state, ending current req.\n",
			__func__);
		mmc_release_host(card->host);
	} else {
		pr_err("%s: start doing error handle.\n", __func__);
		host->switch_err_req = req;
		spin_lock_bh(&ctx_info->cmdq_ctx_lock);
		set_bit(CMDQ_STATE_ERR, &ctx_info->curr_state);
		set_bit(CMDQ_STATE_SWITCH_ERR, &ctx_info->curr_state);
		spin_unlock_bh(&ctx_info->cmdq_ctx_lock);
		schedule_work(&mq->cmdq_err_work);
	}
}

int mmc_blk_cmdq_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret = 0;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	unsigned int cmd_flags = req->cmd_flags;
#endif
	struct mmc_cmdq_context_info *ctx_info = &card->host->cmdq_ctx;

	mmc_claim_host(card->host);
	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		pr_err("%s: %s: partition switch failed %d\n",
				md->disk->disk_name, __func__, ret);
		blk_end_request_all(req, ret);
		goto switch_failure;
	}

	ret = mmc_blk_cmdq_switch(card, md, true);
	if (ret) {
		pr_err("%s curr part config is %u\n", __func__, card->ext_csd.part_config);

		mmc_cmdq_switch_error(card, mq, req);
		return ret;
	}

	mmc_cmdq_task_info_init(card, req);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (cmd_flags & (REQ_FLUSH | REQ_DISCARD) &&
#else
	if (((req_op(req) == REQ_OP_FLUSH) || (req_op(req) ==  REQ_OP_DISCARD)) &&
#endif
		((struct cmdq_host *)mmc_cmdq_private(card->host))->quirks
							& CMDQ_QUIRK_EMPTY_BEFORE_DCMD) {
		ret = wait_event_interruptible(ctx_info->queue_empty_wq,
		                (!ctx_info->active_reqs || ctx_info->in_recovery));

		if (ret) {
			pr_err("%s: failed while waiting for the CMDQ to be empty %s err (%d)\n",
					mmc_hostname(card->host), __func__, ret);
			BUG_ON(1);
		}

		if (ctx_info->in_recovery) {
			pr_err("%s in recovering, give up the dcmd transfer", __func__);
			ctx_info->in_recovery = false;
			spin_lock_irq(mq->queue->queue_lock);
			blk_requeue_request(mq->queue, req);
			spin_unlock_irq(mq->queue->queue_lock);
			mmc_release_host(card->host);
		    return 0;
		}
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (cmd_flags & REQ_DISCARD) {
		if (req->cmd_flags & REQ_SECURE &&
			!(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
			ret = mmc_blk_cmdq_issue_secdiscard_rq(mq, req);
		else
			ret = mmc_blk_cmdq_issue_discard_rq(mq, req);
	} else if (cmd_flags & REQ_FLUSH) {
		ret = mmc_blk_cmdq_issue_flush_rq(mq, req);
	} else {
		ret = mmc_blk_cmdq_issue_rw_rq(mq, req);
	}
#else
	if (req_op(req) == REQ_OP_DISCARD)
		ret = mmc_blk_cmdq_issue_discard_rq(mq, req);
	else if (req_op(req) == REQ_OP_SECURE_ERASE &&
			!(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
		ret = mmc_blk_cmdq_issue_secdiscard_rq(mq, req);
	else if (req_op(req) == REQ_OP_FLUSH)
		ret = mmc_blk_cmdq_issue_flush_rq(mq, req);
	else
		ret = mmc_blk_cmdq_issue_rw_rq(mq, req);

#endif
	/*EHOSTDOWN means that cq_host->enabled is false when cmdq_request;
	 *we need re-enable the cmdq feature and requeue the
	 *failed request
	 */

	return ret;

switch_failure:
	mmc_release_host(card->host);
	return ret;
}

extern int mmc_prep_request(struct request_queue *q, struct request *req);
extern void mmc_queue_setup_discard(struct request_queue *q,
				    struct mmc_card *card);
extern struct scatterlist *mmc_alloc_sg(int sg_len, int *err);

static void mmc_cmdq_dispatch_req(struct request_queue *q)
{
	struct mmc_queue *mq = q->queuedata;

	wake_up(&mq->card->host->cmdq_ctx.wait);
}
/**
 * mmc_blk_cmdq_dump_status
 * @q: request queue
 * @dump_type: dump scenario
 *	BLK_DUMP_WARNING: scenario of io latency warning
 *	BLK_DUMP_PANIC: scenario of system panic
 *
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
void mmc_blk_cmdq_dump_status(struct request_queue *q, enum BLK_DUMP_TYPE dump_type)
#else
void mmc_blk_cmdq_dump_status(struct request_queue *q, enum blk_dump_scenario dump_type)
#endif
{
	struct mmc_card *card;
	struct mmc_host *host;
	struct mmc_queue *mq = q->queuedata;
	if (!mq)
		return;
	card = mq->card;
	if (!card)
		return;
	host = card->host;
	if (!host)
		return;
	pr_err("active_reqs = 0x%lx, data_active_reqs = 0x%lx, curr_state = 0x%lx\r\n",
		host->cmdq_ctx.active_reqs, host->cmdq_ctx.data_active_reqs, host->cmdq_ctx.curr_state);
}

/**
 * mmc_blk_cmdq_setup_queue
 * @mq: mmc queue
 * @card: card to attach to this queue
 *
 * Setup queue for CMDQ supporting MMC card
 */
void mmc_blk_cmdq_setup_queue(struct mmc_queue *mq, struct mmc_card *card)
{
	u64 limit = BLK_BOUNCE_HIGH;/*lint !e501*/
	struct mmc_host *host = card->host;

	if (mmc_dev(host)->dma_mask && *mmc_dev(host)->dma_mask)
		limit = *mmc_dev(host)->dma_mask;

	blk_queue_prep_rq(mq->queue, mmc_prep_request);
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, mq->queue);

	if (mmc_can_erase(card))
		mmc_queue_setup_discard(mq->queue, card);

	blk_queue_bounce_limit(mq->queue, limit);
	blk_queue_max_hw_sectors(mq->queue, min(host->max_blk_count,
						host->max_req_size / 512));
	blk_queue_max_segment_size(mq->queue, host->max_seg_size);
	blk_queue_max_segments(mq->queue, host->max_segs);
}


static void mmc_cmdq_softirq_done(struct request *rq)
{
	struct mmc_queue *mq = rq->q->queuedata;

	mq->cmdq_complete_fn(rq);
}

static void mmc_cmdq_error_work(struct work_struct *work)
{
        struct mmc_queue *mq = container_of(work, struct mmc_queue,
									cmdq_err_work);

        mq->cmdq_error_fn(mq);
}

enum blk_eh_timer_return mmc_cmdq_rq_timed_out(struct request *req)
{
	struct mmc_queue *mq = req->q->queuedata;

	pr_err("%s: request with req: 0x%pK, tag: %d, flags: 0x%llx timed out\n",
			__func__, req, req->tag, req->cmd_flags);

	return mq->cmdq_req_timed_out(req);
}

int mmc_cmdq_init(struct mmc_queue *mq, struct mmc_card *card)
{
	int i, ret = 0;
	int mmc_alloc_size = 0;
	/* one slot is reserved for dcmd requests */
	int q_depth = card->ext_csd.cmdq_depth - 1;

	card->cmdq_init = false;
	spin_lock_init(&card->host->cmdq_ctx.cmdq_ctx_lock);
	init_waitqueue_head(&card->host->cmdq_ctx.queue_empty_wq);
	init_waitqueue_head(&card->host->cmdq_ctx.wait);

	mq->mqrq_cmdq = kzalloc(
			sizeof(struct mmc_queue_req) * q_depth, GFP_KERNEL);
	if (!mq->mqrq_cmdq) {
		pr_warn("%s: unable to allocate mqrq's for q_depth %d\n",
			mmc_card_name(card), q_depth);
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < q_depth; i++) {
		/* TODO: reduce the sg allocation by delaying them */
		mq->mqrq_cmdq[i].sg = mmc_alloc_sg(card->host->max_segs, &ret);
		if (ret) {
			pr_warn("%s: unable to allocate cmdq sg of size %d\n",
				mmc_card_name(card),
				card->host->max_segs);
			goto free_mqrq_sg;
		}
		mmc_alloc_size++;
	}

	ret = blk_queue_init_tags(mq->queue, q_depth, NULL, 0);
	if (ret) {
		pr_warn("%s: unable to allocate cmdq tags %d\n",
				mmc_card_name(card), q_depth);
		goto free_mqrq_sg;
	}
#ifdef CONFIG_HISI_BLK
	card->mmc_tags = mq->queue->queue_tags;
	card->mmc_tags_depth = q_depth;
#endif

	blk_queue_softirq_done(mq->queue, mmc_cmdq_softirq_done);
	INIT_WORK(&mq->cmdq_err_work, mmc_cmdq_error_work);
	init_completion(&mq->cmdq_shutdown_complete);

	blk_queue_rq_timed_out(mq->queue, mmc_cmdq_rq_timed_out);
	blk_queue_rq_timeout(mq->queue, 29 * HZ);

	card->cmdq_init = true;
	goto out;

free_mqrq_sg:
	/* only can free to the Nth sg which failed to allocate */
	for (i = 0; i < mmc_alloc_size; i++)
		kfree(mq->mqrq_cmdq[i].sg);
	kfree(mq->mqrq_cmdq);
	mq->mqrq_cmdq = NULL;
out:
	return ret;
}

void mmc_cmdq_clean(struct mmc_queue *mq, struct mmc_card *card)
{
	int i;
	int q_depth = card->ext_csd.cmdq_depth - 1;

	blk_free_tags(mq->queue->queue_tags);
	mq->queue->queue_tags = NULL;
	blk_queue_free_tags(mq->queue);

	for (i = 0; i < q_depth; i++)
		kfree(mq->mqrq_cmdq[i].sg);
	kfree(mq->mqrq_cmdq);
	mq->mqrq_cmdq = NULL;
}

static struct request *mmc_peek_request(struct mmc_queue *mq)
{
	struct request_queue *q = mq->queue;
	mq->cmdq_req_peeked = NULL;

	spin_lock_irq(q->queue_lock);
	if (!blk_queue_stopped(q))
		mq->cmdq_req_peeked = blk_peek_request(q);
	spin_unlock_irq(q->queue_lock);

	return mq->cmdq_req_peeked;
}

static int mmc_check_blk_queue_start_tag(struct request_queue *q, struct request *req)
{
	int ret;

	spin_lock_irq(q->queue_lock);
	ret = blk_queue_start_tag( q, req);
	spin_unlock_irq(q->queue_lock);

	return ret;
}

static inline void mmc_cmdq_ready_wait(struct mmc_host *host, struct mmc_queue *mq)
{
	struct mmc_cmdq_context_info *ctx = &host->cmdq_ctx;
	struct request_queue *q = mq->queue;

	/*
	 * Wait until all of the following conditions are true:
	 * 1. There is a request pending in the block layer queue
	 *    to be processed.
	 * 2. If the peeked request is flush/discard then there shouldn't
	 *    be any other direct command active.
	 * 3. cmdq state should be unhalted.
	 * 4. cmdq state shouldn't be in error state.
	 * 5. free tag available to process the new request.
	 */
	wait_event(ctx->wait, kthread_should_stop()
			|| (!test_bit(CMDQ_STATE_DCMD_ACTIVE, &ctx->curr_state)
			&& !test_bit(CMDQ_STATE_QUEUE_HUNGUP, &ctx->curr_state)
			&& !mmc_host_halt(host)
			&& !mmc_card_suspended(host->card)
			&& !test_bit(CMDQ_STATE_ERR, &ctx->curr_state)
			&& mmc_peek_request(mq)
			&& !mmc_check_blk_queue_start_tag(q, mq->cmdq_req_peeked)));/*lint !e666 */
}

static int mmc_cmdq_thread(void *d)
{
	struct mmc_queue *mq = d;
	struct mmc_card *card = mq->card;
	struct mmc_host *host = card->host;

	current->flags |= PF_MEMALLOC;

	while(1) {
		int ret = 0;

		mmc_cmdq_ready_wait(host, mq);
		if (kthread_should_stop()) {
			pr_err("%s: cmdq thread stop, maybe hungtask!!", __func__);
			break;
		}

		ret = mq->cmdq_issue_fn(mq, mq->cmdq_req_peeked);

		/*
		 * Don't requeue if issue_fn fails, just bug on.
		 * We don't expect failure here and there is no recovery other
		 * than fixing the actual issue if there is any.
		 * Also we end the request if there is a partition switch error,
		 * so we should not requeue the request here.
		 */
		if (ret)
			//BUG_ON(1);
			pr_err("%s: cmdq request issue fail\n", __func__);
	}

	return 0;
}

int mmc_cmdq_init_queue(struct mmc_queue *mq, struct mmc_card * card,
			spinlock_t *lock, const char *subname)
{
	int ret;

	mq->queue = blk_init_queue(mmc_cmdq_dispatch_req, lock);
	if (!mq->queue)
		return -ENOMEM;
	mmc_blk_cmdq_setup_queue(mq, card);
	ret = mmc_cmdq_init(mq, card);
	if (ret) {
		pr_err("%s: %d: cmdq: unable to set-up\n",
			mmc_hostname(card->host), ret);
		blk_cleanup_queue(mq->queue);
	} else {
		sema_init(&mq->thread_sem, 1);
		mq->queue->queuedata = mq;

		mq->thread = kthread_run(mmc_cmdq_thread, mq, "mmc-cmdqd/%d%s",
								card->host->index, subname ? subname : "");
		if (IS_ERR(mq->thread)) {
			ret = PTR_ERR(mq->thread);
			pr_err("%s: %d: cmdq: failed to start mmc-cmdqd thread\n",
					mmc_hostname(card->host), ret);
		}

#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
		if (card->ext_csd.man_bkops_en && hisi_mmc_is_bkops_needed(card))
			hisi_mmc_manual_bkops_config(mq->queue);
#endif
		return ret;
	}
	return ret;
}

/*this reset func may cause dead lock of claim_host*/
