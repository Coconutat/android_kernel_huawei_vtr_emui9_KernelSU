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

#include <linux/mfd/hisi_pmic.h>
#include <soc_sctrl_interface.h>
#include <soc_ufs_sysctrl_interface.h>
#include <linux/hisi/hisi_idle_sleep.h>
#include "ufshcd.h"
#include "ufs-kirin.h"
#include "dsm_ufs.h"

void ufs_clk_init(struct ufs_hba *hba)
{
	return;
}

void ufs_soc_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 reg;
	int i;

	dev_info(hba->dev, "%s ++\n", __func__);

	/* eS LOW TEMP 207M */
	writel(BIT(SOC_SCTRL_SCPERDIS4_gt_clk_ufs_subsys_START),
		SOC_SCTRL_SCPERDIS4_ADDR(host->sysctrl));
	writel(0x003F0006, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));
	writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
		SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));

	if (!(host->caps & USE_HISI_MPHY_TC)) {
		/* enable the MPHY's sram_bypass, TODO... */
		ufs_sys_ctrl_clr_bits(host, PHY_SRAM_BYPASS_BIT, UFS_SYS_PHY_SRAM_MEM_CTRL_S);
		/* disable sram_ext_ld_done*/
		ufs_sys_ctrl_clr_bits(host, SRAM_EXT_LD_DONE_BIT, UFS_SYS_PHY_SRAM_MEM_CTRL_S);
	}

	writel(1<<SOC_UFS_Sysctrl_UFS_UMECTRL_ufs_ies_en_mask_START,\
		SOC_UFS_Sysctrl_UFS_UMECTRL_ADDR(host->ufs_sys_ctrl));

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) | 0, \
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL); /* HC_PSW powerup */
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL); /* notify PWR ready */
	/*STEP 4 CLOSE MPHY REF CLOCK*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	reg = ufs_sys_ctrl_readl(host, PHY_CLK_CTRL);
	reg = reg & (~MASK_SYSCTRL_REF_CLOCK_SEL) & (~MASK_SYSCTRL_CFG_CLOCK_FREQ);
	if (host->caps & USE_HISI_MPHY_TC) {
		reg = reg | 0x14;
	} else {
		/* syscfg clk is fixed 38.4Mhz */
		reg = reg | 0x26;
		/* set ref clk freq 38.4Mhz */
		reg = reg | SYSCTRL_REF_CLOCK_SEL;
	}
	ufs_sys_ctrl_writel(host, reg, PHY_CLK_CTRL); /* set cfg clk freq */

	if (!(host->caps & USE_HISI_MPHY_TC)) {
		/* Enable: pcs_pwr_stable_sc,pma_pwr_en_sc. Disable: pma_pwr_stable_sc,ref_clk_en_app, ref_clk_en_unipro */
		ufs_sys_ctrl_writel(host, 0x4300, UFS_SYS_UFS_POWER_GATING);
	}

	/* bypass ufs clk gate */
	ufs_sys_ctrl_set_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
	ufs_sys_ctrl_set_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL); /* open psw clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL); /* disable ufshc iso */
	/* phy iso is not effective */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PHY_ISO_CTRL, PHY_ISO_EN); /* disable phy iso */
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL); /* notice iso disable */

	for (i = 400; i > 0; i--) {
		if(BIT_UFS_PSW_MEM_REPAIR_ACK_MASK & ufs_pctrl_readl(host, PCTRL_PERI_STAT64_OFFSET))
			break;
		udelay(1);
	}
	if (i <= 0)
		pr_err("ufs memory repair fail\n");

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

		mdelay(2);
		ufs_i2c_writel(hba, (unsigned int)BIT(6),
				SC_RSTEN); /*disable Device Reset*/
	} else {
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
				    UFS_DEVICE_RESET_CTRL); /* reset device */
		/* To improve the ref clock jitter, use PMU's output directly */
		/* change the PMU's device ref clk to 38.4Mhz, if after onchiprom's
		* linkstartup's PA_MaxRxHSGear = 0x4 */
		/* close the device clk */
		hisi_pmic_reg_write(0x43, 0);
		/* choose the device clk 19.2Mhz */
		hisi_pmic_reg_write(0x02E3, 0);
		/* open the device clk */
		hisi_pmic_reg_write(0x43, 1);

		mdelay(1);

		ufs_sys_ctrl_writel(
		    host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
		    UFS_DEVICE_RESET_CTRL); /* disable Device Reset */
	}
	mdelay(10);

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) |\
		1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START,\
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	reg = readl(SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));
	if (reg & (1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START))
		mdelay(1);

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	dev_info(hba->dev, "%s --\n", __func__);
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
	hisi_pmic_reg_write(0x43, 0);

	host->in_suspend = true;

	return 0;
}

int ufs_kirin_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;
	if (hba->is_hs_gear4_dev)
		hisi_pmic_reg_write(0x02E3, 1);
	else
		hisi_pmic_reg_write(0x02E3, 0);
	hisi_pmic_reg_write(0x43, 1);
	/* 250us to ensure the clk stable */
	udelay(250);
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

/* Workaround: PWM-amplitude reduce & PMC and H8's glitch */
static void mphy_amplitude_glitch_workaround(struct ufs_hba *hba)
{
	uint16_t value3 = 0;
	uint16_t value4 = 0;
	uint32_t reg;
	uint16_t table[11][2] = {
			/* tx_ana_ctrl_leg_pull_en, tx_ana_ctrl_post */
			{252, 3},
			{252, 3},
			{252, 3},
			{255, 3},
			{255, 6},
			{1020, 6},
			{1020, 6},
			{1023, 6},
			{1023, 6},
			{4092, 6},
			{4092, 7}
	};

	reg = ufs_kirin_mphy_read(hba, 0x200C);
	reg = reg & 0xF; /* RAWCMN_DIG_TX_CAL_CODE[3:0] */
	if (reg >= sizeof(table)/sizeof(table[0]))
		reg = 0;
	value3 = table[reg][0]<<1;
	value4 = table[reg][1];
	ufs_kirin_mphy_write(hba, 0x10A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[14:1](tx_ana_ctrl_leg_pull_en) */
	ufs_kirin_mphy_write(hba, 0x11A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[14:1](tx_ana_ctrl_leg_pull_en) */
	ufs_kirin_mphy_write(hba, 0x10A6, value4); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_3[8:0](tx_ana_ctrl_post) */
	ufs_kirin_mphy_write(hba, 0x11A6, value4); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_3[8:0](tx_ana_ctrl_post) */

	ufs_kirin_mphy_write(hba, 0x10A4, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_1 */
	ufs_kirin_mphy_write(hba, 0x11A4, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_1 */
	ufs_kirin_mphy_write(hba, 0x10A5, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_2 */
	ufs_kirin_mphy_write(hba, 0x11A5, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_2 */
	ufs_kirin_mphy_write(hba, 0x10A7, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_4 */
	ufs_kirin_mphy_write(hba, 0x11A7, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_4 */
	ufs_kirin_mphy_write(hba, 0x10A8, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_5 */
	ufs_kirin_mphy_write(hba, 0x11A8, 0); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_5 */

	value3 |= (1<<15);
	ufs_kirin_mphy_write(hba, 0x10A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[15] = 1 */
	ufs_kirin_mphy_write(hba, 0x11A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[15] = 1 */

	ufs_kirin_mphy_write(hba, 0x10A3, value3 | 1); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[0](TX_ANA_LOAD_CLK) */
	ufs_kirin_mphy_write(hba, 0x11A3, value3 | 1); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[0](TX_ANA_LOAD_CLK) */
	ufs_kirin_mphy_write(hba, 0x10A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[0](TX_ANA_LOAD_CLK) */
	ufs_kirin_mphy_write(hba, 0x11A3, value3); /* LANEN_DIG_ANA_TX_EQ_OVRD_OUT_0[0](TX_ANA_LOAD_CLK) */
}

/*lint -e648 -e845*/
/* snps asic mphy specific configuration */
int ufs_kirin_dme_setup_snps_asic_mphy(struct ufs_hba *hba)
{
	uint32_t value1 = 0;
	uint32_t value2 = 0;
	int err = 0;

	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x1); /* Unipro VS_mphy_disable */

	if (ufs_sctrl_readl(host, SCDEEPSLEEPED_OFFSET) & EFUSE_RHOLD_BIT) {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8013, 0x4), 0x2); /* MPHY RXRHOLDCTRLOPT */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8013, 0x5), 0x2); /* MPHY RXRHOLDCTRLOPT */
	} else {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8013, 0x4), 0); /* MPHY RXRHOLDCTRLOPT */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8013, 0x5), 0); /* MPHY RXRHOLDCTRLOPT */
	}
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

	/* MPHY CBREFCLKCTRL2, indicate refclk is open when calibration */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8132, 0x0), 0x80);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x811F, 0x0), 0x1); /* MPHY CBCRCTRL: enable CR port */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

	/* Workaround: PWM-amplitude reduce & PMC and H8's glitch clean begin */
	ufs_kirin_mphy_write(hba, 0x203B, 0x30); /* RAWCMN_DIG_AON_CMN_SUP_OVRD_IN[5:4] = 2'b11, open phy clk during H8 */
	/* Workaround: PWM-amplitude reduce & PMC and H8's glitch clean end */

	/* Workaround: clear P-N abnormal common voltage begin*/
	ufs_kirin_mphy_write(hba, 0x10e0, 0x10);/* LANEN_ANA_TX_OVRD_MEAS[5]=0,LANEN_DIG_ANA_TX_OVRD_MEAS[4]=1 */
	ufs_kirin_mphy_write(hba, 0x11e0, 0x10);/* LANEN_ANA_TX_OVRD_MEAS[5]=0,LANEN_DIG_ANA_TX_OVRD_MEAS[4]=1 */
	/* Workaround: clear P-N abnormal common voltage end*/

	/* close AFE calibration */
	ufs_kirin_mphy_write(hba, 0x401c, 0x0004);
	ufs_kirin_mphy_write(hba, 0x411c, 0x0004);

	/* slow process */
	value1 = ufs_kirin_mphy_read(hba, 0x401e);
	value2 = ufs_kirin_mphy_read(hba, 0x411e);
	ufs_kirin_mphy_write(hba, 0x401e, value1 | 0x1);
	ufs_kirin_mphy_write(hba, 0x411e, value2 | 0x1);
	value1 = ufs_kirin_mphy_read(hba, 0x401f);
	value2 = ufs_kirin_mphy_read(hba, 0x411f);
	ufs_kirin_mphy_write(hba, 0x401f, value1 | 0x1);
	ufs_kirin_mphy_write(hba, 0x411f, value2 | 0x1);

	err = ufs_update_hc_fw(hba);
	if (err) {
		pr_err("phy firmware update error\n");
		return err;
	}
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x156A, 0x0), 0x2); /* PA_HSSeries */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8114, 0x0), 0x1); /* MPHY CBRATESEL */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x4), 0x1); /* MPHY RXSQCONTROL rx0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x5), 0x1); /* MPHY RXSQCONTROL rx1 */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

	if (host->caps & RX_VCO_VREF)
		/* SUP_ANA_BG1: rx_vco_vref = 501mV */
		ufs_kirin_mphy_write(hba, 0x0042, 0x28);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8113, 0x0), 0x1);/* CBENBLCPBATTRWR */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);
	/* RX_HS_G1_PREPARE_LENGTH_CAPABILITY */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008c, 0x4), 0xF); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008c, 0x5), 0xF); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */

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

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8113, 0x0), 0x0);/* CBENBLCPBATTRWR */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);

	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), &value1); /* Unipro VS_mphy_disable */
	if (value1 != 0x1)
		pr_warn("Warring!!! Unipro VS_mphy_disable is 0x%x\n", value1);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x0); /* Unipro VS_mphy_disable */
	if (likely(!hba->host->is_emulator)) {
		err = ufs_kirin_check_hibern8(hba);
		if (err)
			pr_err("ufs_kirin_check_hibern8 error\n");
	}

	if (likely(!hba->host->is_emulator))
		mphy_amplitude_glitch_workaround(hba);

	/* disable override ref_clk_en */
	ufs_kirin_mphy_write(hba, 0x203B, 0x0);

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

	/*FIXME is it good for FPGA condition*/
	if (!(host->caps & USE_HISI_MPHY_TC)) {
		err = ufs_kirin_dme_setup_snps_asic_mphy(hba);
		if (err)
			return err;
	}

	/* disable auto H8 */
	reg = ufshcd_readl(hba, REG_CONTROLLER_AHIT);
	reg = reg & (~UFS_AHIT_AH8ITV_MASK);
	ufshcd_writel(hba, reg, REG_CONTROLLER_AHIT);

	/*for hisi MPHY*/
	hisi_mphy_updata_vswing_fsm_ocs5(hba, host);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x155E, 0x0), 0x0); /* Unipro PA_Local_TX_LCC_Enable */

	/* enlarge the VS_AdjustTrailingClocks and VS_DebugSaveConfigTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd086, 0x0), 0xF0); /* Unipro VS_AdjustTrailingClocks */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0a0, 0x0), 0x3); /* Unipro VS_DebugSaveConfigTime */
	/* Unipro PA_AdaptAfterLRSTInPA_INIT, use PA_PeerRxHsAdaptInitial value */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x15D5, 0x0), 0x1);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), 0x0); /* close Unipro VS_Mk2ExtnSupport */
	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), &value);
	if (0 != value) {
		/* Ensure close success */
		pr_warn("Warring!!! close VS_Mk2ExtnSupport failed\n");
	}
	if (!(host->caps & USE_HISI_MPHY_TC)) {
		if (35 == host->tx_equalizer) {
			ufshcd_dme_set(
				hba, UIC_ARG_MIB_SEL((u32)0x0037, 0x0), 0x1);
			ufshcd_dme_set(
				hba, UIC_ARG_MIB_SEL((u32)0x0037, 0x1), 0x1);
		} else if (60 == host->tx_equalizer) {
			ufshcd_dme_set(
				hba, UIC_ARG_MIB_SEL((u32)0x0037, 0x0), 0x2);
			ufshcd_dme_set(
				hba, UIC_ARG_MIB_SEL((u32)0x0037, 0x1), 0x2);
		}
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

void set_device_clk(struct ufs_hba *hba)
{
	uint32_t max_rx_hsgear;
	/* Read PA_MaxRxHSGear(0x1587) to see if it is UFS3.0 device
	which supports HS gear 4, this checking must be done after linkstartup */
	ufshcd_dme_get(hba, UIC_ARG_MIB(0x1587), &max_rx_hsgear);
	if (max_rx_hsgear >= 0x4) {
		/*The B_REFCLK_FREQ was changed to 38.4MHz in
		the xloader*/

		hba->is_hs_gear4_dev = 1;

		/* close the device clk */
		hisi_pmic_reg_write(0x43, 0);
		/* choose the device clk 38.4Mhz */
		hisi_pmic_reg_write(0x02E3, 1);
		/* open the device clk */
		hisi_pmic_reg_write(0x43, 1);

		mdelay(2);
	}
	else {
		hba->is_hs_gear4_dev = 0;
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
		ufshcd_rmwl(hba, (LP_AH8_PGE | LP_PGE), 0, UFS_REG_OCPTHRTL);

	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09a), 0x80000000); /* select received symbol cnt */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09c), 0x00000005); /* reset counter0 and enable */

	set_device_clk(hba);

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

	/*
	 * Cancer platform need a full reset when error handler occurs.
	 * If a request sending in ufshcd_queuecommand passed through
	 * ufshcd_state check. And eh may reset host controller, a NOC
	 * error happens. 1000ms sleep is enough for waiting those requests.
	 **/
	msleep(1000);

	ufs_soc_init(hba);

	enable_irq(hba->irq);
#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_enable_volt_irq(hba);
#endif
}

/*lint -restore*/
