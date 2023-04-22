
#include <linux/delay.h>

#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include "sdhci.h"
#include <linux/mmc/cmdq_hci.h>
#include <linux/hisi/util.h>
#include <linux/pm_runtime.h>


extern void sdhci_dumpregs(struct sdhci_host *host);
extern int sdhci_card_busy_data0(struct mmc_host *mmc);

#ifdef CONFIG_MMC_CQ_HCI
void sdhci_cmdq_reset(struct mmc_host *mmc, u8 mask)
{
	struct sdhci_host *host = mmc_priv(mmc);
	sdhci_reset(host, mask);
}

static void sdhci_cmdq_clean_irqs(struct mmc_host *mmc, u32 clean)
{
	struct sdhci_host *host = mmc_priv(mmc);
	sdhci_writel(host, clean, SDHCI_INT_STATUS);
}

static void sdhci_cmdq_clear_set_irqs(struct mmc_host *mmc, bool clear)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 ier = 0;

	if (clear) {
		ier = SDHCI_INT_CMDQ_EN | SDHCI_INT_ERROR_MASK;
		sdhci_writel(host, ier, SDHCI_INT_ENABLE);
		sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
	} else {
		ier = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
			SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT |
			SDHCI_INT_INDEX | SDHCI_INT_END_BIT | SDHCI_INT_CRC |
			SDHCI_INT_TIMEOUT | SDHCI_INT_DATA_END |
			SDHCI_INT_RESPONSE;

		sdhci_writel(host, ier, SDHCI_INT_ENABLE);
		sdhci_writel(host, ier, SDHCI_SIGNAL_ENABLE);
	}
}
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
/*this func can only used when cmdq halt, return the device status*/
static int sdhci_cmdq_send_status(struct mmc_host *mmc, u32* status)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 mask, arg, opcode, val, timeout;
	int flags;

	sdhci_reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);

	arg = mmc->card->rca << 16;;
	sdhci_writel(host, arg, SDHCI_ARGUMENT);

	opcode = MMC_SEND_STATUS;
	flags = SDHCI_CMD_RESP_SHORT | SDHCI_CMD_CRC | SDHCI_CMD_INDEX;
	sdhci_writew(host, SDHCI_MAKE_CMD(opcode, flags), SDHCI_COMMAND);

	timeout = 10;
	mask = SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;

	while (0 == (mask & (val = sdhci_readl(host, SDHCI_INT_STATUS)))) {
		if (timeout == 0) {
			pr_err("%s: send cmd%d timeout\n", __func__, opcode);
			sdhci_dumpregs(host);
			break;
		}
		timeout--;
		mdelay(1);
	}

	/* clean interupt*/
	sdhci_writel(host, val, SDHCI_INT_STATUS);

	if (val & SDHCI_INT_ERROR_MASK) {
		pr_err("%s: send cmd%d err val = 0x%x\n", __func__, opcode, val);
		sdhci_dumpregs(host);
		return -1;
	} else {
		*status = sdhci_readl(host,  SDHCI_RESPONSE);
		pr_err("%s: status = 0x%x\n", __func__, *status);
		return 0;
	}

}
#endif

/*this func can only used when cmdq halt, send cmd48*/
static void sdhci_cmdq_send_cmd48(struct mmc_host *mmc, u32 tag, bool entire)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 mask, arg, opcode, val, timeout;
	int flags;

	sdhci_reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
	/* send cmd48 */
	/* CMD48 arg:
	[31:21] reserved
	[20:16]: TaskID
	[15:4]: reserved
	[3:0] TM op-code
	*/
	if (true == entire)
		arg = 1;
	else
		arg = 2 | tag << 16;
	sdhci_writel(host, arg, SDHCI_ARGUMENT);

	opcode = MMC_CMDQ_TASK_MGMT;
	flags = SDHCI_CMD_CRC | SDHCI_CMD_INDEX;
	if (host->cq_host->quirks & CMDQ_QUIRK_CHECK_BUSY)
		/* arasan cannot send R1B cmd */
		flags |= SDHCI_CMD_RESP_SHORT;
	else
		flags |= SDHCI_CMD_RESP_SHORT_BUSY;

	sdhci_writew(host, SDHCI_MAKE_CMD(opcode, flags), SDHCI_COMMAND);

	timeout = 10;
	mask = SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;

	while (0 == (mask & (val = sdhci_readl(host, SDHCI_INT_STATUS)))) {
		if (timeout == 0) {
			pr_err("%s: send cmd%d timeout\n", __func__, opcode);
			sdhci_dumpregs(host);
			break;
		}
		timeout--;
		mdelay(1);
	}

	if (val & SDHCI_INT_ERROR_MASK) {
		pr_err("%s: send cmd%d err val = 0x%x\n", __func__, opcode, val);
		sdhci_dumpregs(host);
	}
	/* clean interupt*/
	sdhci_writel(host, val, SDHCI_INT_STATUS);
	return;

}


int sdhci_cmdq_discard_task(struct mmc_host *mmc, u32 tag, bool entire)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 mask, arg, opcode, val, val1, val2, mode;
	int flags;
	unsigned long timeout;
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	u32 status;
#endif

	sdhci_reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);

	timeout = 10;
	mask = SDHCI_CMD_INHIBIT;

	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (timeout == 0) {
			pr_err("%s: sdhci never released "
				"inhibit bit(s).\n", __func__);
			sdhci_dumpregs(host);
			return -EIO;
		}
		timeout--;
		mdelay(1);
	}

	/* set trans mode reg */
	mode = sdhci_readw(host, SDHCI_TRANSFER_MODE);
	if (host->quirks2 & SDHCI_QUIRK2_CLEAR_TRANSFERMODE_REG_BEFORE_CMD) {
		sdhci_writew(host, 0x0, SDHCI_TRANSFER_MODE);
	} else {
		/* clear Auto CMD settings for no data CMDs */
		val = mode & ~(SDHCI_TRNS_AUTO_CMD12 | SDHCI_TRNS_AUTO_CMD23);
		sdhci_writew(host, val, SDHCI_TRANSFER_MODE);
	}

	/* enable interupt status, */
	/* but not let the interupt be indicated to system */
	val1 = sdhci_readl(host, SDHCI_INT_ENABLE);
	val = val1 | SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;
	sdhci_writel(host, val, SDHCI_INT_ENABLE);
	val2 = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	val = val2 & ~(SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK);
	sdhci_writel(host, val, SDHCI_SIGNAL_ENABLE);

#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	if (sdhci_cmdq_send_status(mmc, &status)) {
		pr_err("%s: cmd13 error\n", __func__);
		return -EIO;
	} else {
		if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
		    R1_CURRENT_STATE(status) == R1_STATE_RCV)
		    goto send_cmd12;
		else
			goto send_cmd48;
	}
#endif

#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
send_cmd12:
#endif
	/* send cmd12 */
	arg = 0;
	sdhci_writel(host, arg, SDHCI_ARGUMENT);

	opcode = MMC_STOP_TRANSMISSION;
	flags = SDHCI_CMD_RESP_SHORT | SDHCI_CMD_CRC |
			SDHCI_CMD_INDEX | SDHCI_CMD_ABORTCMD;
	sdhci_writew(host, SDHCI_MAKE_CMD(opcode, flags), SDHCI_COMMAND);

	timeout = 10;
	mask = SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;

	while (0 == (mask & (val = sdhci_readl(host, SDHCI_INT_STATUS)))) {
		if (timeout == 0) {
			pr_err("%s: send cmd%d timeout\n", __func__, opcode);
			sdhci_dumpregs(host);
			break;
		}
		timeout--;
		mdelay(1);
	}

	if (val & SDHCI_INT_ERROR_MASK) {
		pr_err("%s: send cmd%d err val = 0x%x\n", __func__,opcode,  val);
		sdhci_dumpregs(host);
	}
	/* clean interupt*/
	sdhci_writel(host, val, SDHCI_INT_STATUS);

	/* wait busy */
	timeout = 1000;
	while (host->cq_host->ops->card_busy(mmc)) {
		if (timeout == 0) {
			pr_err("%s: CMD12 wait busy timeout after stop\n", __func__);
			sdhci_dumpregs(host);
			break;
		}
		timeout--;
		mdelay(1);
	}
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
send_cmd48:
#endif
	sdhci_cmdq_send_cmd48(mmc, tag, entire);

	/* recovery interupt enable & mask */
	sdhci_writel(host, val1, SDHCI_INT_ENABLE);
	sdhci_writel(host, val2, SDHCI_SIGNAL_ENABLE);
	/* recovery trans mode */
	sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);

	if (val & SDHCI_INT_RESPONSE) {
		return 0;
	} else {
		pr_err("%s: discard cmdq fail\n", __func__);
		return -EIO;
	}
}
EXPORT_SYMBOL(sdhci_cmdq_discard_task);

static int sdhci_cmdq_tuning_move(struct mmc_host *mmc, int is_move_strobe, int flag)
{
	struct sdhci_host *host = mmc_priv(mmc);

	return host->ops->tuning_move(host, is_move_strobe, flag);
}

static void sdhci_cmdq_set_data_timeout(struct mmc_host *mmc, u32 val)
{
	struct sdhci_host *host = mmc_priv(mmc);

	sdhci_writeb(host, val, SDHCI_TIMEOUT_CONTROL);
}

static int sdhci_cmdq_dump_vendor_regs(struct mmc_host *mmc)
{
	struct sdhci_host *host = mmc_priv(mmc);

	if (host->runtime_suspended)
		return -1;

	sdhci_dumpregs(host);
	return 0;
}

void sdhci_cmdq_enter(struct mmc_host *mmc)
{
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	struct sdhci_host *host = mmc_priv(mmc);
	u16 reg_16;
	int timeout;

	/*need to set multitransfer for cmdq*/
	reg_16 = sdhci_readw(host, SDHCI_TRANSFER_MODE);
	reg_16 |= SDHCI_TRNS_MULTI;
	sdhci_writew(host, reg_16, SDHCI_TRANSFER_MODE);

	timeout = 10000;//10ms
	while (sdhci_card_busy_data0(mmc)) {
		timeout--;
		if (!timeout) {
			pr_err("%s: wait busy timeout\n", __func__);
			break;
		}
		udelay(1);
	}

#endif
}

void sdhci_cmdq_exit(struct mmc_host *mmc)
{
#ifdef CONFIG_MMC_SDHCI_DWC_MSHC
	int timeout;

	timeout = 10000;//10ms
	while (sdhci_card_busy_data0(mmc)) {
		timeout--;
		if (!timeout) {
			pr_err("%s: wait busy timeout\n", __func__);
			break;
		}
		udelay(1);
	}
#endif
}


#else
void sdhci_cmdq_reset(struct mmc_host *mmc, u8 mask)
{

}

static void sdhci_cmdq_clean_irqs(struct mmc_host *mmc, u32 clean)
{

}

static u32 sdhci_cmdq_clear_set_irqs(struct mmc_host *mmc, bool clear)
{
	return 0;
}

static int sdhci_cmdq_discard_task(struct mmc_host *mmc, u32 tag, bool entire)
{
	return 0;
}

static int sdhci_cmdq_tuning_move(struct mmc_host *mmc, int is_move_strobe, int flag)
{
	return 0;
}

static void sdhci_cmdq_set_data_timeout(struct mmc_host *mmc, u32 val)
{

}

static int sdhci_cmdq_dump_vendor_regs(struct mmc_host *mmc)
{
	return 0;
}

void sdhci_cmdq_enter(struct mmc_host *mmc)
{

}

void sdhci_cmdq_exit(struct mmc_host *mmc)
{

}

#endif

static const struct cmdq_host_ops sdhci_cmdq_ops = {
	.reset = sdhci_cmdq_reset,
	.clean_irqs = sdhci_cmdq_clean_irqs,
	.clear_set_irqs = sdhci_cmdq_clear_set_irqs,
	.set_data_timeout = sdhci_cmdq_set_data_timeout,
	.dump_vendor_regs = sdhci_cmdq_dump_vendor_regs,
	.card_busy = sdhci_card_busy_data0,
	.discard_task = sdhci_cmdq_discard_task,
	.tuning_move = sdhci_cmdq_tuning_move,
	.enter = sdhci_cmdq_enter,
	.exit = sdhci_cmdq_exit,
};


#ifdef CONFIG_MMC_CQ_HCI
int sdhci_get_cmd_err(u32 intmask)
{
	if (intmask & SDHCI_INT_TIMEOUT)
		return -ETIMEDOUT;
	else if (intmask & (SDHCI_INT_CRC | SDHCI_INT_END_BIT |
			    SDHCI_INT_INDEX))
		return -EILSEQ;
	return 0;
}

int sdhci_get_data_err(u32 intmask)
{
	if (intmask & SDHCI_INT_DATA_TIMEOUT)
		return -ETIMEDOUT;
	else if (intmask & (SDHCI_INT_DATA_END_BIT | SDHCI_INT_DATA_CRC))
		return -EILSEQ;
	else if (intmask & SDHCI_INT_ADMA_ERROR)
		return -EIO;
	return 0;
}

irqreturn_t sdhci_cmdq_irq(struct mmc_host *mmc, u32 intmask)
{
	return cmdq_irq(mmc, intmask);
}

#else
static irqreturn_t sdhci_cmdq_irq(struct mmc_host *mmc, u32 intmask)
{
	pr_err("%s: rxd cmdq-irq when disabled !!!!\n", mmc_hostname(mmc));
	return IRQ_NONE;
}
#endif

void sdhci_cmdq_init(struct sdhci_host *host, struct mmc_host *mmc)
{
		bool dma64;
		int ret;

		dma64 = (host->flags & SDHCI_USE_64_BIT_DMA) ?
			true : false;
		ret = cmdq_init(host->cq_host, mmc, dma64);
		if (ret)
			pr_err("%s: CMDQ init: failed (%d)\n",
			       mmc_hostname(host->mmc), ret);
		else
			host->cq_host->ops = &sdhci_cmdq_ops;
}
