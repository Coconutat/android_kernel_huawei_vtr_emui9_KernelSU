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
#include <soc_sctrl_interface.h>
#include <soc_ufs_sysctrl_interface.h>
#include <linux/hisi/hisi_idle_sleep.h>
#include "ufshcd.h"
#include "ufs-kirin.h"
#include "dsm_ufs.h"

static void set_rhold(struct ufs_kirin_host *host)
{
	if (ufs_sctrl_readl(host, SCDEEPSLEEPED_OFFSET) & EFUSE_RHOLD_BIT)
		ufs_sctrl_writel(host,
			(MASK_UFS_MPHY_RHOLD | BIT_UFS_MPHY_RHOLD),
			UFS_DEVICE_RESET_CTRL);
}

void ufs_clk_init(struct ufs_hba *hba)
{
	return;
}

static inline bool ufs_kirin_need_memrepair(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	if ((BIT(0) & ufs_sctrl_readl(host, 0x3A0)) &&
	    (BIT(19) & ufs_sctrl_readl(host, 0x010))) {
		pr_err("%s need memrepair\n", __func__);
		return true;
	} else {
		pr_err("%s no need memrepair\n", __func__);
		return false;
	}
}

void ufs_soc_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 reg;
	int ret;

	pr_info("%s ++\n", __func__);

	/*CS LOW TEMP 207M*/
	writel(BIT(SOC_SCTRL_SCPERDIS4_gt_clk_ufs_subsys_START),
		SOC_SCTRL_SCPERDIS4_ADDR(host->sysctrl));
	writel(0x003F0007, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));
	writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
		SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));

	writel(1<<SOC_UFS_Sysctrl_UFS_UMECTRL_ufs_ies_en_mask_START,\
		SOC_UFS_Sysctrl_UFS_UMECTRL_ADDR(host->ufs_sys_ctrl));

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) | 0, \
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	/* efuse indicates enable rhold or not */
	set_rhold(host);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL); /* HC_PSW powerup */
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL); /* notify PWR ready */
	/*STEP 4 CLOSE MPHY REF CLOCK*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	reg = ((0x3 << 2) | (0x7 << (2 + 16))); /*Bit[4:2], div =4*/
	writel(reg,
	       SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));
	reg = ufs_sys_ctrl_readl(host, PHY_CLK_CTRL);
	reg = reg & (~MASK_SYSCTRL_REF_CLOCK_SEL) & (~MASK_SYSCTRL_CFG_CLOCK_FREQ);
	if (host->caps & USE_HISI_MPHY_TC) {
		reg = reg | 0x14;
	} else {
		/* BUS 207M / 4 = 0x34 syscfg clk */
		reg = reg | 0x34;
	}
	ufs_sys_ctrl_writel(host, reg, PHY_CLK_CTRL); /* set cfg clk freq */

	/* enable ref_clk_en override(bit5) & override value = 1(bit4), with mask */
	ufs_sys_ctrl_writel(host, 0x00300030, UFS_DEVICE_RESET_CTRL);

	/* bypass ufs clk gate */
	ufs_sys_ctrl_set_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
	ufs_sys_ctrl_set_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL); /* open psw clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL); /* disable ufshc iso */
	/* phy iso is not effective */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PHY_ISO_CTRL, PHY_ISO_EN); /* disable phy iso */
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL); /* notice iso disable */

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_arst_ufs_START+16) |\
		1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_arst_ufs_START,\
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN); /* disable lp_reset_n */
	mdelay(1);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN,
			      PHY_CLK_CTRL); /* open clock of M-PHY */
	if (host->caps & USE_HISI_MPHY_TC) {
		ufs_i2c_writel(hba, (unsigned int) BIT(6),
			       SC_RSTDIS); /*enable Device Reset*/
		ufs_i2c_readl(hba, &reg, SC_UFS_REFCLK_RST_PAD);
		reg = reg & (~(BIT(2) | BIT(10)));
		/*output enable, For EMMC to high dependence, open
		 * DA_UFS_OEN_RST
		 * and DA_UFS_OEN_REFCLK*/
		ufs_i2c_writel(hba, reg, SC_UFS_REFCLK_RST_PAD);
		/* FPGA is not usable, only for open the clk, to match
		 * clk_unprepare_enble later in suspend&fullreset func */
		ret = clk_prepare_enable(host->clk_ufsio_ref);
		if (ret) {
			pr_err("%s ,clk_prepare_enable failed\n", __func__);
			return;
		}
		mdelay(2);
		ufs_i2c_writel(hba, (unsigned int)BIT(6),
				SC_RSTEN); /*disable Device Reset*/
	} else {
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
				    UFS_DEVICE_RESET_CTRL); /* reset device */
		ret = clk_prepare_enable(host->clk_ufsio_ref);
		if (ret) {
			pr_err("%s ,clk_prepare_enable failed\n", __func__);
			return;
		}

		mdelay(1);

		ufs_sys_ctrl_writel(
		    host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
		    UFS_DEVICE_RESET_CTRL); /* disable Device Reset */
	}
	mdelay(40);

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) |\
		1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START,\
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	reg = readl(SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));
	if (reg & (1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START))
		mdelay(1);

	host->need_memrepair = ufs_kirin_need_memrepair(hba);

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	pr_info("%s --\n", __func__);
	return;
}

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

static inline void ufs_kirin_memrepair_suspend(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	pr_err("%s ufs need memory repair\n", __func__);
	ufs_sys_ctrl_writel(host, 0x00E00000, UFS_CRG_UFS_CFG);
	udelay(1);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);

	return;
}

static inline void ufs_kirin_memrepair_resume(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	int i;

	pr_err("%s ufs need memory repair\n", __func__);
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);
	for (i = 0; i < 400; i++) {
		if (BIT(19) & ufs_pctrl_readl(host, 0x18C))
			break;
		udelay(1);
	}
	if (i > 399)
		pr_err("%s ufs memory repair timeout\n", __func__);
	ufs_sys_ctrl_writel(host, 0x00E000E0, UFS_CRG_UFS_CFG);

	return;
}

int ufs_kirin_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 0 when idle*/
	hisi_idle_sleep_vote(ID_UFS, 0);

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	if (host->in_suspend) {
		WARN_ON(1);/*lint !e730*/
		return 0;
	}

	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	udelay(10);
	/* set ref_dig_clk override of PHY PCS to 0 */
	ufs_sys_ctrl_writel(host, 0x00100000, UFS_DEVICE_RESET_CTRL);
	/* close device's refclk */
	clk_disable_unprepare(host->clk_ufsio_ref);

	if (host->need_memrepair) {
		ufs_kirin_memrepair_suspend(hba);
	}

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

	if (host->need_memrepair) {
		ufs_kirin_memrepair_resume(hba);
	}

	/* open device's refclk */
	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return ret;
	}

	udelay(1);
	/* set ref_dig_clk override of PHY PCS to 1 */
	ufs_sys_ctrl_writel(host, 0x00100010, UFS_DEVICE_RESET_CTRL);
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);

	host->in_suspend = false;
	return 0;
}

void ufs_kirin_device_hw_reset(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	if (likely(!(host->caps & USE_HISI_MPHY_TC)))
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
							UFS_DEVICE_RESET_CTRL);
	else
		ufs_i2c_writel(hba, (unsigned int) BIT(6), SC_RSTDIS);
	mdelay(1);

	if (likely(!(host->caps & USE_HISI_MPHY_TC)))
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
			    			UFS_DEVICE_RESET_CTRL);
	else
		ufs_i2c_writel(hba, (unsigned int)BIT(6), SC_RSTEN);
	/* some device need at least 40ms */
	mdelay(40);
}
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

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8127, 0x0), 0x98); /* MPHY CBOVRCTRL4 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8128, 0x0), 0x1); /* MPHY CBOVRCTRL5 */

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

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8127, 0x0), 0x82); /* MPHY CBOVRCTRL4 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8128, 0x0), 0x1); /* MPHY CBOVRCTRL5 */

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

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0092, 0x4), 0xA);/* RX_Hibern8Time_Capability*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0092, 0x5), 0xA);/* RX_Hibern8Time_Capability*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008f, 0x4), 0xA);/* RX_Min_ActivateTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008f, 0x5), 0xA);/* RX_Min_ActivateTime*/

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

	/*for hisi MPHY*/
	hisi_mphy_updata_temp_sqvref(hba, host);

	/* ARIES's FPGA not maintain anymore, so don't need to add
	 * the new "ufs-kirin-use-snps-mphy-ac" in so many product 's dts,
	 * and use snps-mphy-ac in branch by default here */
	/* if (host->caps & USE_SNPS_MPHY_AC) { */
	err = ufs_kirin_dme_setup_snps_asic_mphy(hba);
	if (err)
		return err;

	/* disable auto H8 */
	reg = ufshcd_readl(hba, REG_CONTROLLER_AHIT);
	reg = reg & (~UFS_AHIT_AH8ITV_MASK);
	ufshcd_writel(hba, reg, REG_CONTROLLER_AHIT);

	/*for hisi MPHY*/
	hisi_mphy_updata_vswing_fsm_ocs5(hba, host);

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
	/*for hisi MPHY*/
	hisi_mphy_busdly_config(hba, host);

	pr_info("%s --\n", __func__);

	return err;
}

static void hisi_mphy_link_post_config(struct ufs_hba *hba,
			struct ufs_kirin_host *host)
{
	uint32_t tx_lane_num = 1;
	uint32_t rx_lane_num = 1;

	if (host->caps & USE_HISI_MPHY_TC) {
		/*set the PA_TActivate to 128. need to check in ASIC...*/
		/* H8's workaround */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x15a8, 0x0), 5);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80da, 0x0), 0x2d);

		ufshcd_dme_get(hba, UIC_ARG_MIB(0x1561), &tx_lane_num);
		ufshcd_dme_get(hba, UIC_ARG_MIB(0x1581), &rx_lane_num);

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x4),
			       0x0); /*RX_MC_PRESENT*/
		if (tx_lane_num > 1 && rx_lane_num > 1) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x5),
				       0x0); /*RX_MC_PRESENT*/
		}
	}
}

int ufs_kirin_link_startup_post_change(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2044), 0x0); /* Unipro DL_AFC0CreditThreshold */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2045), 0x0); /* Unipro DL_TC0OutAckThreshold */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x2040), 0x9); /* Unipro DL_TC0TXFCThreshold */

	/*for hisi MPHY*/
	hisi_mphy_link_post_config(hba, host);

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

void ufs_kirin_pwr_change_pre_change(struct ufs_hba *hba)
{
	uint32_t value;
	pr_info("%s ++\n", __func__);
#ifdef CONFIG_HISI_DEBUG_FS
	pr_info("device manufacturer_id is 0x%x\n", hba->manufacturer_id);
#endif
	/*ARIES platform need to set SaveConfigTime to 0x13, and change sync length to maximum value */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xD0A0), 0x13); /* VS_DebugSaveConfigTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1552), 0x4f); /* g1 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1554), 0x4f); /* g2 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1556), 0x4f); /* g3 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x15a7), 0xA); /* PA_Hibern8Time */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x15a8), 0xA); /* PA_Tactivate */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd085, 0x0), 0x01);

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
/*lint +e648 +e845*/

/**
 * Soc init will reset host controller, all register value will lost
 * including memory address, doorbell and AH8 AGGR
 */
void ufs_kirin_full_reset(struct ufs_hba *hba)
{
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
