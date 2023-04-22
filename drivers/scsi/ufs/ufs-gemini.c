/*
 * Copyright (c) 2013-2015, Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "ufshcd :" fmt

#include <linux/types.h>
#include <linux/gpio.h>
#include <soc_sctrl_interface.h>
#include <soc_ufs_sysctrl_interface.h>
#include <linux/hisi/hisi_idle_sleep.h>
#include "ufshcd.h"
#include "ufs-kirin.h"
#include "dsm_ufs.h"

void ufs_clk_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret;

	pr_info("%s ++\n", __func__);
	pr_info("UFS use abb clk\n");

	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return;
	}
	clk_disable_unprepare(host->clk_ufsio_ref);
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	if (ufs_sys_ctrl_readl(host, PHY_CLK_CTRL) & BIT_SYSCTRL_REF_CLOCK_EN)
		mdelay(1);
	/* use abb clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_SRC_SEl, UFS_SYSCTRL);
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_ISO_EN, PHY_ISO_EN);

	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return;
	}
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL); /* open mphy ref clk */
	pr_info("%s --\n", __func__);
	return;
}

void ufs_soc_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 reg;

	pr_info("%s ++\n", __func__);

	ufs_pericrg_writel(host, RST_UFS, PERRSTEN3_OFFSET);

	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL); /* HC_PSW powerup */
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL); /* notify PWR ready */
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
		UFS_DEVICE_RESET_CTRL);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 0))
			pr_err("%s: could not set gpio %d output push down\n", __func__, host->reset_gpio);
	}

	ufs_pericrg_writel(host, 0 | BIT(14 + 16), CLKDIV17_OFFSET); /* set hc hclk div */
	ufs_pericrg_writel(host, (0x3 << 9) | (0x3 << (9 + 16)), CLKDIV16_OFFSET); /* set mphy cfg clk div */

	reg = ufs_sys_ctrl_readl(host, PHY_CLK_CTRL);
	reg = reg & (~MASK_SYSCTRL_REF_CLOCK_SEL) & (~MASK_SYSCTRL_CFG_CLOCK_FREQ);
	reg = reg | UFS_FREQ_CFG_CLK;
	ufs_sys_ctrl_writel(host, reg, PHY_CLK_CTRL); /* set cfg clk freq */
	/* bypass ufs clk gate */
	ufs_sys_ctrl_set_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
	ufs_sys_ctrl_set_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL); /* open psw clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL); /* disable ufshc iso */
	/* phy iso is effective */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PHY_ISO_CTRL, PHY_ISO_EN); /* disable phy iso */
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL); /* notice iso disable */
	ufs_pericrg_writel(host, UFS_ARESET, PERRSTDIS3_OFFSET); /* disable aresetn */
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN); /* disable lp_reset_n */
	mdelay(1);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 1))
			pr_err("%s: could not set gpio %d output pull up\n", __func__, host->reset_gpio);
	}
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
		UFS_DEVICE_RESET_CTRL);

	mdelay(20);

	/* enable the fix of linereset recovery, and enable rx_reset/tx_rest beat */
	/* enable ref_clk_en override(bit5) & override value = 1(bit4), with mask */
	ufs_sys_ctrl_writel(host, 0x03300330, UFS_DEVICE_RESET_CTRL);

	ufs_pericrg_writel(host, RST_UFS, PERRSTDIS3_OFFSET);
	if (ufs_pericrg_readl(host, PERRSTSTAT3_OFFSET) & RST_UFS)
		mdelay(1);

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	pr_info("%s --\n", __func__);
	return;
}
#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
/* the func to config key */
void ufs_kirin_uie_key_prepare(struct ufs_hba *hba, int key_index, void *key)
{
	struct ufs_kirin_host *host = hba->priv;
	int reg_value = 0;
	int key_cfg = 0;
	u32 key_reg_offset = 0;

	/*
	 * when writing key reg of the number 22 ~ 31,
	 * we must set reg apb_addr of ufs_sys_ctrl
	 */
	if (key_index > 21) {
		ufs_sys_ctrl_writel(host, 0x10001, UFS_APB_ADDR_MASK);
		key_cfg = key_index - 22;
	} else
		key_cfg = key_index;

	/* key operation start */
	reg_value = ufshcd_readl(hba, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	if ((reg_value >> 31) & 0x1) {
		/* TODO step 1st
		 * Verify that no pending transactions reference x-CRYPTOCFG
		 * in their CCI field, i.e. UTRD.CCI != x for all pending transactions
		 */

		/*step 2nd writing 0x0 to clear x-CRYPTOCFG reg*/
		ufshcd_writel(hba, 0, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	}

	/* step 3rd write the cryptographic key to x-CRYPTOKEY field
	 * The key is organized according to the algorithm-specific layout.
	 * Unused regions of CRYPTOKEY should be written with zeros.
	 * The key is written in little-endian format, sequentially
	 * and in one atomic set of operations.
	 */
	/* use the following way to  write key to improve efficiency */
	key_reg_offset = key_cfg * 0x80;
	memcpy(hba->key_reg_base + key_reg_offset, key, 64);
	mb();

	/* step 4th set x-CRYPTOCFG with CAPIDX, DUSIZE, and CFGE=1 */
	ufshcd_writel(hba, 0x80000108, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	/* key operation end */

	/* clear reg apb_addr of ufs_sys_ctrl */
	if (key_index > 21)
		ufs_sys_ctrl_writel(host, 0x10000, UFS_APB_ADDR_MASK);
}
#endif

#ifdef FEATURE_KIRIN_UFS_PSW
static int ufs_kirin_wait_ufssys_bitset_timeout(struct ufs_kirin_host *host,
		uint32_t mask, uint32_t off, int timeout_us)
{
	while (--timeout_us > 0) {
		if (mask & ufs_sys_ctrl_readl(host, off))
			break;
		udelay(1);
	}
	if (timeout_us <= 0) {
		pr_err("%s: wait ufs sys bit time out\n", __func__);
		return -1;
	}
	return 0;
}
#endif

int ufs_kirin_suspend_before_set_link_state(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
#ifdef FEATURE_KIRIN_UFS_PSW
	struct ufs_kirin_host *host = hba->priv;

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	/*step1:store BUSTHRTL register*/
	host->busthrtl_backup = ufshcd_readl(hba, UFS_REG_OCPTHRTL);
	/*enable PowerGating*/
	ufshcd_rmwl(hba, LP_PGE, LP_PGE, UFS_REG_OCPTHRTL);
#endif
	return 0;
}

int ufs_kirin_resume_after_set_link_state(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
#ifdef FEATURE_KIRIN_UFS_PSW
	struct ufs_kirin_host *host = hba->priv;

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	ufshcd_writel(hba, host->busthrtl_backup, UFS_REG_OCPTHRTL);
#endif
	return 0;
}

int ufs_kirin_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 0 when idle*/
	hisi_idle_sleep_vote(ID_UFS, 0);

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	pr_info("ufs_sys_ctrl 0x3C: 0x%x\n", ufs_sys_ctrl_readl(host, 0x3C));
	pr_info("ufs_sys_ctrl 0x40: 0x%x\n", ufs_sys_ctrl_readl(host, 0x40));

	if (host->in_suspend) {
		WARN_ON(1);/*lint !e730*/
		return 0;
	}

	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	udelay(10);
	/* set ref_dig_clk override of PHY PCS to 0 */
	ufs_sys_ctrl_writel(host, 0x00100000, UFS_DEVICE_RESET_CTRL);

#ifdef FEATURE_KIRIN_UFS_PSW
	/*step 10: poll HC_LP_CTRL(0x0C)bit0*/
	if (ufs_kirin_wait_ufssys_bitset_timeout(host,
			BIT_SYSCTRL_LP_PWR_GATE, HC_LP_CTRL, 10000)) {
		pr_err("%s: LP_PWR_GATE time out\n", __func__);
		return -1;
	}

	/*step 11:set PSW_CLK_CTRL(0x14) bit[4] to 0,close psw clk*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL);

	/*step 12:set HC_LP_CTRL(0x0C) bit[16] to 1,set PSW_POWER_CTRL(0x04) bit[16] to 1*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);

	/*step 13:set UFS_SC RESET_CTRL_EN(0x1C) bit[0] to 0,reset UFSHCPSW area*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN);

	/*step 14:set UFS_SC PSW_POWER_CTRL(0x04) bit[0] to 0,power off psw area*/
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL);
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL);
#endif

	udelay(100);
	clk_disable_unprepare(host->clk_ufsio_ref);
	if (CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET)) {
		pr_err("%s:disable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	/* close ufsphy cfg clk */
	ufs_pericrg_writel(host, CLK_UFSPHY, PERDIS7_OFFSET);
	if (CLK_UFSPHY & ufs_pericrg_readl(host, PERCLKEN7_OFFSET)) {
		pr_err("%s:disable phy cfg clk err. PERCLKEN7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	/* ufs_pericrg_writel(host, CLK_UFS_SUBSYS, PERDIS5_OFFSET); */

	host->in_suspend = true;

	return 0;
}

int ufs_kirin_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;

	/* ufs_pericrg_writel(host, CLK_UFS_SUBSYS, PEREN5_OFFSET); */

	/* open ufsphy cfg clk */
	ufs_pericrg_writel(host, CLK_UFSPHY, PEREN7_OFFSET);
	if (!(CLK_UFSPHY & ufs_pericrg_readl(host, PERCLKEN7_OFFSET))) {
		pr_err("%s:enable phy cfg clk err. PERCLKEN7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return ret;
	}
	if (!(0x1 & ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET))) {
		pr_err("%s:enable clk ref err. PERI_CTRL3 = 0x%x\n", __func__,
			ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET));
	}
	if (!(CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET))) {
		pr_err("%s:enable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	udelay(1);
	/* set ref_dig_clk override of PHY PCS to 1 */
	ufs_sys_ctrl_writel(host, 0x00100010, UFS_DEVICE_RESET_CTRL);
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);

#ifdef FEATURE_KIRIN_UFS_PSW
	/*step5: set UFS_SC PSW_POWER_CTRL(0x04) bit[0] to 1,power up psw area*/
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL);
	udelay(10);

	/*step6:set UFS_SC HC_LP_CTRL(0x0C) bit[8] to 1,make LP_pwr_ready effective*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL);

	/*step7:set UFS_SC PSW_CLK_CTRL(0x14) bit[4] to 1,enable clk of PSW*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL);

	/*step8:set UFS_SC RESET_CTRL_EN(0x1C) bit[0] to 1,reset PSW area*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN);

	if (ufs_kirin_wait_ufssys_bitset_timeout(host,
			BIT_STATUS_LP_RESETCOMPLETE, PHY_RESET_STATUS, 10000)) {
		pr_err("%s: wait lp_resetcomplete time out\n", __func__);
		return -1;
	}

	/*step 9 set PSW_POWER_CTRL(0x04) bit[16] to 0,disable Isolation*/

	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);
	if (BIT_UFS_PSW_ISO_CTRL & ufs_sys_ctrl_readl(host, PSW_POWER_CTRL)) {
		pr_err("%s: set psw_iso_ctrl fail\n", __func__);
		mdelay(1);
	}
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL);
#endif

	host->in_suspend = false;
	return 0;
}

void ufs_kirin_device_hw_reset(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
			    UFS_DEVICE_RESET_CTRL);
	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 0))
			pr_err("%s: could not set gpio %d output push down\n", __func__, host->reset_gpio);
	}

	mdelay(1);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 1))
			pr_err("%s: could not set gpio %d output pull up\n", __func__, host->reset_gpio);
	}
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
			    UFS_DEVICE_RESET_CTRL);
	/* some device need at least 40ms */
	mdelay(40);
}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
void ufs_kirin_set_vol(struct ufs_hba *hba, int v_tx, int v_rx)
{
	pr_err("ufs v_tx:%d v_rx:%d\n", v_tx, v_rx);
	if ((v_rx > 0) && (v_rx < 4)) {
		ufs_kirin_mphy_write(hba, 0x004A, 0x0090);
		ufs_kirin_mphy_write(hba, 0x90cb, 0x0080);
		ufs_kirin_mphy_write(hba, 0x90ce, 0x0010);
		if (v_rx == 3) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E4);
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E0);
		}
		if (v_rx == 2) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3884);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3880);
		}
		if (v_rx == 1) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A24);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A20);
		}
		ufs_kirin_mphy_write(hba, 0x9005, 0x4000);
		ufs_kirin_mphy_write(hba, 0x9005, 0x0000);
	}
	if ((v_tx > 0) && (v_tx < 4)) {
		ufs_kirin_mphy_write(hba, 0x004A, 0x0090);
		ufs_kirin_mphy_write(hba, 0x90C3, 0x0010);
		ufs_kirin_mphy_write(hba, 0x90C9, 0x0001);
		ufs_kirin_mphy_write(hba, 0x90C5, 0x0002);

		if (v_tx == 3) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E4);
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E0);
		}
		if (v_tx == 2) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3884);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3880);
		}
		if (v_tx == 1) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A24);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A20);
		}
	}
}
#endif

/*lint -e648 -e845*/
/* snps asic mphy specific configuration */
int ufs_kirin_dme_setup_snps_asic_mphy(struct ufs_hba *hba)
{
	uint32_t value = 0;
	int err = 0;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x1); /* Unipro VS_mphy_disable */
	if (host->caps & USE_RATE_B) {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x156A, 0x0), 0x2); /* PA_HSSeries */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8114, 0x0), 0x1); /* MPHY CBRATESEL */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8121, 0x0), 0x2D); /* MPHY CBOVRCTRL2 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8122, 0x0), 0x1); /* MPHY CBOVRCTRL3 */

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x4), 0x58); /* MPHY RXOVRCTRL4 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x5), 0x58); /* MPHY RXOVRCTRL4 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x4), 0xB); /* MPHY RXOVRCTRL5 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x5), 0xB); /* MPHY RXOVRCTRL5 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x4), 0x1); /* MPHY RXSQCONTROL rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x5), 0x1); /* MPHY RXSQCONTROL rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
	} else {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x156A, 0x0), 0x1); /* PA_HSSeries */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8114, 0x0), 0x0); /* MPHY CBRATESEL */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8121, 0x0), 0x4C); /* MPHY CBOVRCTRL2 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8122, 0x0), 0x1); /* MPHY CBOVRCTRL3 */

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x4), 0x18); /* MPHY RXOVRCTRL4 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x5), 0x18); /* MPHY RXOVRCTRL4 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x4), 0xD); /* MPHY RXOVRCTRL5 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x5), 0xD); /* MPHY RXOVRCTRL5 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x4), 0x1); /* MPHY RXSQCONTROL rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x5), 0x1); /* MPHY RXSQCONTROL rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
	}

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8113, 0x0), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008F, 0x4), 0x7); /* Tactive RX */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008F, 0x5), 0x7); /* Tactive RX */

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0095, 0x4), 0x4F); /* Gear3 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0095, 0x5), 0x4F); /* Gear3 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0094, 0x4), 0x4F); /* Gear2 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0094, 0x5), 0x4F); /* Gear2 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008B, 0x4), 0x4F); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008B, 0x5), 0x4F); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x000F, 0x0), 0x5); /* Thibernate Tx */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x000F, 0x1), 0x5); /* Thibernate Tx */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);

	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), &value); /* Unipro VS_mphy_disable */
	if (value != 0x1)
		pr_warn("Warring!!! Unipro VS_mphy_disable is 0x%x\n", value);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x0); /* Unipro VS_mphy_disable */
	if (likely(!hba->host->is_emulator)) {
		err = ufs_kirin_check_hibern8(hba);
		if (err)
			pr_err("ufs_kirin_check_hibern8 error\n");
	}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	ufs_kirin_set_vol(hba, hba->v_tx, hba->v_rx);
#endif
	pr_info("%s --\n", __func__);
	return err;
}


int ufs_kirin_link_startup_pre_change(struct ufs_hba *hba)
{
	int err = 0;
	uint32_t value = 0;
	uint32_t reg = 0;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	err = ufs_kirin_dme_setup_snps_asic_mphy(hba);
	if (err)
		return err;

	ufshcd_writel(hba, UFS_HCLKDIV_NORMAL_VALUE, UFS_REG_HCLKDIV);

	/* disable auto H8 */
	reg = ufshcd_readl(hba, REG_CONTROLLER_AHIT);
	reg = reg & (~UFS_AHIT_AH8ITV_MASK);
	ufshcd_writel(hba, reg, REG_CONTROLLER_AHIT);

	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,\
		UFS_DEVICE_RESET_CTRL); /* disable Device Reset */

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x155E, 0x0), 0x0); /* Unipro PA_Local_TX_LCC_Enable */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), 0x0); /* close Unipro VS_Mk2ExtnSupport */
	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), &value);
	if (0 != value) {
		/* Ensure close success */
		pr_warn("Warring!!! close VS_Mk2ExtnSupport failed\n");
	}
	/*FPGA with HISI PHY not configure equalizer*/
	if (35 == host->tx_equalizer) {
		ufs_kirin_mphy_write(hba, 0x1002, 0xAC78);
		ufs_kirin_mphy_write(hba, 0x1102, 0xAC78);
		ufs_kirin_mphy_write(hba, 0x1003, 0x2440);
		ufs_kirin_mphy_write(hba, 0x1103, 0x2440);
	} else if (60 == host->tx_equalizer) {
		ufs_kirin_mphy_write(hba, 0x1002, 0xAA78);
		ufs_kirin_mphy_write(hba, 0x1102, 0xAA78);
		ufs_kirin_mphy_write(hba, 0x1003, 0x2640);
		ufs_kirin_mphy_write(hba, 0x1103, 0x2640);
	}

	pr_info("%s --\n", __func__);

	return err;
}

int ufs_kirin_link_startup_post_change(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2044), 0x0); /* Unipro DL_AFC0CreditThreshold */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2045), 0x0); /* Unipro DL_TC0OutAckThreshold */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2040), 0x9); /* Unipro DL_TC0TXFCThreshold */

	if (host->caps & BROKEN_CLK_GATE_BYPASS) {
		/* not bypass ufs clk gate */
		ufs_sys_ctrl_clr_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
		ufs_sys_ctrl_clr_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);
	}

	if (host->hba->caps & UFSHCD_CAP_AUTO_HIBERN8)
		/* disable power-gating in auto hibernate 8 */
		ufshcd_rmwl(hba, LP_AH8_PGE, 0, UFS_REG_OCPTHRTL);

	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09a), 0x80000000); /* select received symbol cnt */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09c), 0x00000005); /* reset counter0 and enable */

	pr_info("%s --\n", __func__);
	return 0;
}
/*lint +e648 +e845*/

#ifdef CONFIG_SCSI_UFS_KIRIN_LINERESET_CHECK
/*lint -e550 -e732 -e527 -e529*/
static bool ufs_kirin_can_checking(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	if (hba->ufshcd_state != UFSHCD_STATE_OPERATIONAL) {
		return false;
	}
	if (hba->is_hibernate) {
		return false;
	}
	if (hba->pm_op_in_progress) {
		return false;
	}
	if (host->in_suspend) {
		return false;
	}

	return true;
}

static bool ufs_kirin_check_err_fsm(u32 lane0_tx_state,
								u32 lane0_rx_state, u32 lane1_rx_state)
{
	if (((lane0_tx_state == 0x6) || (lane0_tx_state == 0x7))
		&& ((lane0_rx_state == 0x2) || (lane1_rx_state == 0x2)))
		return true;
	return false;
}

int ufs_kirin_daemon_thread(void *d)
{
	struct ufs_hba *hba = d;
	struct ufs_kirin_host *host = hba->priv;
	unsigned long flags;
	u32 link_state;
	u32 lane0_tx_state, lane0_rx_state, lane1_rx_state;
	u32 linereset_ind;
	u32 lane0_tx_state_p;
	u32 check_times;

	do {
		msleep(1000);
		if (!ufs_kirin_can_checking(hba))
			continue;
		ufs_sys_ctrl_writel(host, 0x08081010, PHY_MPX_TEST_CTRL);
		wmb();
		link_state = ufs_sys_ctrl_readl(host, PHY_MPX_TEST_OBSV);

		lane0_tx_state = (link_state & (0xF << 24)) >> 24;
		lane0_rx_state = (link_state & (0x7 << 8)) >> 8;
		lane1_rx_state = (link_state & (0x7 << 16)) >> 16;

		if (ufs_kirin_check_err_fsm(lane0_tx_state, lane0_rx_state, lane1_rx_state)) {
			msleep(5);
			if (!ufs_kirin_can_checking(hba))
				continue;
			hba->reg_uecpa = ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
			check_times = 0;
re_check:
			lane0_tx_state_p = lane0_tx_state;
			msleep(50);
			if (!ufs_kirin_can_checking(hba))
				continue;
			ufs_sys_ctrl_writel(host, 0x08081010, PHY_MPX_TEST_CTRL);
			wmb();
			link_state = ufs_sys_ctrl_readl(host, PHY_MPX_TEST_OBSV);

			lane0_tx_state = (link_state & (0xF << 24)) >> 24;
			lane0_rx_state = (link_state & (0x7 << 8)) >> 8;
			lane1_rx_state = (link_state & (0x7 << 16)) >> 16;

			if ((lane0_tx_state == lane0_tx_state_p) && ((lane0_rx_state == 0x2) || (lane1_rx_state == 0x2))) {
				msleep(5);
				if (!ufs_kirin_can_checking(hba))
					continue;
				hba->reg_uecpa = ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
				linereset_ind = (hba->reg_uecpa & (0x1 << 4)) >> 4;
				if (linereset_ind) {
					check_times = 0;
					goto re_check;
				} else if (check_times < 3) {
					check_times++;
					goto re_check;
				} else {
					dev_err(hba->dev, "unipro register error happy, reset hba\n");
					spin_lock_irqsave(hba->host->host_lock, flags);
					/* block commands from scsi mid-layer */
					scsi_block_requests(hba->host);

					/* transfer error masks to sticky bits */
					hba->ufshcd_state = UFSHCD_STATE_EH_SCHEDULED;
					hba->force_host_reset = 1;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
					queue_kthread_work(&hba->eh_worker, &hba->eh_work);
#else
					kthread_queue_work(&hba->eh_worker, &hba->eh_work);
#endif
					spin_unlock_irqrestore(hba->host->host_lock, flags);
					msleep(4000);
				}
			}

		}
	} while (1);

	return 0;
}
/*lint +e550 +e732 +e527 +e529*/
#endif

/*lint -save -e529 -e438 -e732 -e845*/
void ufs_kirin_pwr_change_pre_change(struct ufs_hba *hba)
{
	uint32_t value;
	pr_info("%s ++\n", __func__);
#ifdef CONFIG_HISI_DEBUG_FS
	pr_info("device manufacturer_id is 0x%x\n", hba->manufacturer_id);
#endif

	if (UFS_VENDOR_SKHYNIX == hba->manufacturer_id) {
		pr_info("H**** device must set VS_DebugSaveConfigTime 0x10\n");
		ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xD0A0), 0x10); /* VS_DebugSaveConfigTime */
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x1556), 0x48); /* sync length */
		/* no need to update in unipro register */
		/* ufshcd_dme_set(hba, UIC_ARG_MIB(0xD085), 0x01); */
	}

	ufshcd_dme_get(hba, UIC_ARG_MIB(0x15A8), &value);
	if (value < 0x7)
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x15A8), 0x7); /* update PaTactive */

	ufshcd_dme_set(hba, UIC_ARG_MIB(0x155c), 0x0); /* PA_TxSkip */

	/*PA_PWRModeUserData0 = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b0), 8191);
	/*PA_PWRModeUserData1 = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b1), 65535);
	/*PA_PWRModeUserData2 = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b2), 32767);
	/*DME_FC0ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd041), 8191);
	/*DME_TC0ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd042), 65535);
	/*DME_AFC0ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd043), 32767);
	/*PA_PWRModeUserData3 = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b3), 8191);
	/*PA_PWRModeUserData4 = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b4), 65535);
	/*PA_PWRModeUserData5 = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b5), 32767);
	/*DME_FC1ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd044), 8191);
	/*DME_TC1ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd045), 65535);
	/*DME_AFC1ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd046), 32767);


	pr_info("%s --\n", __func__);
	return;
}

/**
 * Soc init will reset host controller, all register value will lost
 * including memory address, doorbell and AH8 AGGR
 */
void ufs_kirin_full_reset(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	/*
	 * disable ref clock, clk_init will re-enable
	 */
	clk_disable_unprepare(host->clk_ufsio_ref);

	ufs_clk_init(hba);

#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_disable_volt_irq(hba);
#endif
	disable_irq(hba->irq);

	/* wait for 1s to be sure axi entered to idle state */
	msleep(1000);

	ufs_soc_init(hba);

	enable_irq(hba->irq);
#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_enable_volt_irq(hba);
#endif
}

/*lint -restore*/
