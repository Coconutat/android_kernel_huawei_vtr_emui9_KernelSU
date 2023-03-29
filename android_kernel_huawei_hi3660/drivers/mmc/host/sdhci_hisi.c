#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include "sdhci.h"
#include <linux/version.h>


extern int sdhci_card_busy_data0(struct mmc_host *mmc);
extern void sdhci_dumpregs(struct sdhci_host *host);
/**
 * sdhci_trylock_hostlock - try to claim the sdhci host lock;
 * @host:	sdhci host;
 * @flags:  store the flags for restore;
 * try to claim the sdhci host lock;
 */
static int sdhci_trylock_hostlock(struct sdhci_host *host,unsigned long *flags)
{
	int locked = 0;
	unsigned int trycount = 100000;
	do{
		locked = spin_trylock_irqsave(&host->lock, *flags);/*lint !e730 !e550 !e1072 !e666*/
		if(locked)
			break;
		udelay(10);/*lint !e778 !e774 !e747*/
	}while(--trycount>0);
	return locked;
}


/**
 * sdhci_send_command_direct - send cmd diretly without irq system;
 * @mmc:	mmc host;
 * @mrq:    mmc request;
 * sometimes the irq system cannot work.so we send cmd and polling the
 * int status register instead of waiting for the response interrupt;
 */
int sdhci_send_command_direct(struct mmc_host *mmc, struct mmc_request *mrq)
{
	u32 val, val1, val2, timeout, mask, opcode;
	int ret = 0;
	struct sdhci_host *host;
	unsigned long flags;

	host = mmc_priv(mmc);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 76))
	WARN_ON(host->mrq != NULL);/*lint !e730*/
	host->mrq = mrq;
#else
	WARN_ON(host->cmd != NULL);/*lint !e730*/
#endif
	opcode = mrq->cmd->opcode;

	/*Get hostlock timeout, the abnormal context may have the locker*/
	if(!sdhci_trylock_hostlock(host, &flags)) {
		pr_err("%s, can't get the hostlock!\n", __func__);
		ret = -EIO;
		goto out;
	}

	mask = SDHCI_INT_ERROR_MASK;
	val = sdhci_readl(host, SDHCI_INT_STATUS);
	if (mask & val) {
		pr_err("%s host is in err or busy status 0x%x\n", __func__, val);
		ret = -EIO;
		goto unlock;
	}

	/* enable interupt status, */
	/* but not let the interupt be indicated to system */
	val1 = sdhci_readl(host, SDHCI_INT_ENABLE);
	val = val1 | SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;
	sdhci_writel(host, val, SDHCI_INT_ENABLE);
	val2 = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	//val = val2 & ~(SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK);
	sdhci_writel(host, 0x0, SDHCI_SIGNAL_ENABLE);

	sdhci_send_command(host, mrq->cmd);

	timeout = 10;
	mask = SDHCI_INT_RESPONSE | SDHCI_INT_ERROR_MASK;
	while (0 == (mask & (val = sdhci_readl(host, SDHCI_INT_STATUS)))) {
		if (timeout == 0) {
			pr_err("%s: send cmd%d timeout\n", __func__, opcode);
			sdhci_dumpregs(host);
			ret = -ETIMEDOUT;
			goto reg_recovery;
		}
		timeout--;
		mdelay(1);/*lint !e730 !e778 !e774 !e747*/
	}

	if (val & SDHCI_INT_ERROR_MASK) {
		pr_err("%s: send cmd%d err val = 0x%x\n", __func__, opcode, val);
		sdhci_dumpregs(host);
		ret =  -EIO;
		goto reg_recovery;
	}

	/* wait busy;*/
	timeout = 1000;
	while (sdhci_card_busy_data0(mmc)) {
		if (timeout == 0) {
			pr_err("%s: wait busy timeout after stop\n", __func__);
			sdhci_dumpregs(host);
			ret = -ETIMEDOUT;
			goto reg_recovery;
		}
		timeout--;
		mdelay(1);/*lint !e730 !e778 !e774 !e747*/
	}

	if (!ret)
		host->cmd->resp[0] = sdhci_readl(host, SDHCI_RESPONSE);
reg_recovery:
	/* clean interupt, cmdq int cannot be cleaned*/
	val &= (~SDHCI_INT_CMDQ);
	sdhci_writel(host, val, SDHCI_INT_STATUS);
	/* recovery interupt enable & mask */
	sdhci_writel(host, val1, SDHCI_INT_ENABLE);
	sdhci_writel(host, val2, SDHCI_SIGNAL_ENABLE);
unlock:
	spin_unlock_irqrestore(&host->lock, flags);
out:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 76))
	host->mrq = NULL;
#endif
	host->cmd = NULL;

	return ret;
}

void sdhci_retry_req(struct sdhci_host *host,struct mmc_request *mrq)
{
	int ret;
	if (host->mmc->card && mmc_card_mmc(host->mmc->card)
		&& ((mrq->cmd && mrq->cmd->error)
			|| (mrq->sbc && mrq->sbc->error)
			|| (mrq->data && (mrq->data->error || (mrq->data->stop && mrq->data->stop->error))))
		&& (!(host->flags & SDHCI_EXE_SOFT_TUNING))
		&& (host->mmc->ios.timing >= MMC_TIMING_MMC_HS200)
		&& host->ops->tuning_move) {
			if ((mrq->data && mrq->data->error && (host->mmc->ios.timing == MMC_TIMING_MMC_HS400))
				|| host->mmc->card->ext_csd.strobe_enhanced_en) {
				ret = host->ops->tuning_move(host, TUNING_STROBE, TUNING_FLAG_NOUSE);
			}
			else {
				ret = host->ops->tuning_move(host, TUNING_CLK, TUNING_FLAG_NOUSE);
			}
			/*cmd5 no need to retry*/
			if ((!ret) && mrq->cmd && (mrq->cmd->opcode != MMC_SLEEP_AWAKE)) {
				mrq->cmd->retries++;
				if (mrq->cmd->data && mrq->cmd->data->error) {
					mrq->cmd->data->error = 0;
				}
				if (mrq->sbc && mrq->sbc->error) {
					mrq->sbc->error = 0;
				}
				if (mrq->cmd->data && mrq->cmd->data->stop && mrq->cmd->data->stop->error) {
					mrq->cmd->data->stop->error = 0;
				}
				if (!mrq->cmd->error) {
					mrq->cmd->error = -EILSEQ;
				}
			}
	}
	return;
}

/* In Libra, do not do vcc power up an off operation in first exception */
int need_vcc_off(struct mmc_host *mmc)
{
	int vcc_off = 1;

	if (mmc->is_coldboot_on_reset_fail) {
		if (mmc->reset_num == 1)
			vcc_off = 0;
	}

	return vcc_off;
}


void sdhci_set_vmmc_power(struct sdhci_host *host, unsigned short vdd)
{
	struct mmc_host *mmc = host->mmc;

	if (!need_vcc_off(mmc))
		return;

	if (host->quirks2 & SDHCI_QUIRK2_USE_1_8_V_VMMC) {
		if (vdd == 0) {
			if (mmc->regulator_enabled) {
				if (!regulator_disable(mmc->supply.vmmc))
					mmc->regulator_enabled = false;
				else
					pr_err("%s: regulator vmmc disable failed !\n", __func__);
			}
		}
		else {
			if (regulator_set_voltage(mmc->supply.vmmc, 2950000,
						    2950000)) {
				pr_err("%s: regulator vmmc set_voltage failed !\n", __func__);
			}
			if (!mmc->regulator_enabled) {
				if (!regulator_enable(mmc->supply.vmmc))
					mmc->regulator_enabled = true;
				else
					pr_err("%s: regulator vmmc enable failed !\n", __func__);
			}
		}
	}
	else {
		mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, vdd);
	}
}


