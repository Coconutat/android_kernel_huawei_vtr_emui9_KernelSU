/*
 * Synopsys Secure Digital Host Controller Interface.
 * Copyright (C) 2011 - 2012 Michal Simek <monstr@monstr.eu>
 * Copyright (c) 2012 Wind River Systems, Inc.
 * Copyright (C) 2013 Pengutronix e.K.
 * Copyright (C) 2013 Xilinx Inc.
 *
 * Based on sdhci-of-arasan.c
 *
 * Copyright (c) 2007 Freescale Semiconductor, Inc.
 * Copyright (c) 2009 MontaVista Software, Inc.
 *
 * Authors: Xiaobo Xie <X.Xie@freescale.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
/*lint --e{750}*/
#include <linux/bootdevice.h>
#include <linux/module.h>
#include "sdhci-pltfm.h"
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/mmc/cmdq_hci.h>
#include <linux/pm_runtime.h>
#include <linux/mmc/card.h>
#include <soc_pmctrl_interface.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include "sdhci-dwc-mshc.h"

#define DRIVER_NAME "sdhci-mshc"

#define CLK_CTRL_TIMEOUT_SHIFT		16
#define CLK_CTRL_TIMEOUT_MASK		(0xf << CLK_CTRL_TIMEOUT_SHIFT)
#define CLK_CTRL_TIMEOUT_MIN_EXP	13

#define SDHCI_MSHC_MIN_FREQ				400000
#define SDHCI_MSHC_XIN_CLK_FREQ			200000000

static u32 xin_clk = SDHCI_MSHC_XIN_CLK_FREQ;

extern void sdhci_dumpregs(struct sdhci_host *host);

/**
 * struct sdhci_mshc_data
 * @clk:						Pointer to the sd clock
 * @clock:					record current clock rate
 * @tuning_current_sample:		record current sample when soft tuning
 * @tuning_init_sample:			record the optimum sample of soft tuning
 * @tuning_sample_count:		record the tuning count for soft tuning
 * @tuning_move_sample:		record the move sample when data or cmd error occor
 * @tuning_move_count:		record the move count
 * @tuning_sample_flag:			record the sample OK or NOT of soft tuning
 * @tuning_strobe_init_sample:	default strobe sample
 * @tuning_strobe_move_sample:	record the strobe move sample when data or cmd error occor
 * @tuning_strobe_move_count:	record the strobe move count
 */
struct sdhci_mshc_data {
	struct clk *clk;
	unsigned int clock;

	u32 enhanced_strobe_enabled;

	int tuning_128_flag;
	int tuning_loop_max;

	int tuning_count;
	int tuning_phase_best;
	u64  tuning_phase_record_low;
	u64  tuning_phase_record_high;

	int tuning_move_phase;
	int tuning_move_count;
	int tuning_phase_max;
	int tuning_phase_min;

	int tuning_strobe_phase_init;
	int tuning_strobe_move_phase;
	int tuning_strobe_move_count;
	int tuning_strobe_phase_max;
	int tuning_strobe_phase_min;

	bool need_delay_measure;
};

#define TUNING_PHASE_INVALID	-1

#define MMC_I2C_SLAVE (0x5A)
#define MMC_I2C_BUS 3

/*lint -e785*/
static struct i2c_board_info mmc_i2c_board_info = {
    /* FIXME*/
    .type = "i2c_mmc",
    .addr = MMC_I2C_SLAVE,
};
/*lint +e785*/

static void sdhci_i2c_writel(struct sdhci_host *host, u32 val, u32 addr)
{
	int ret;

	union i2c_fmt {
		unsigned char chars[8];
		u32 addr_val[2];
	} data;
	data.addr_val[0] = cpu_to_be32(addr);
	data.addr_val[1] = val;
	if (host->i2c_client) {
		ret = i2c_master_send(host->i2c_client, (char *)data.chars,
				      (int)sizeof(union i2c_fmt));
		if (ret < 0)
			pr_err("%s fail\n", __func__);
	} else {
		pr_err("%s  fail client empty\n", __func__);
	}
}

static u32 sdhci_i2c_readl(struct sdhci_host *host, u32 addr)
{
	int ret;
	u32 val = 0;
	u32 buf = cpu_to_be32(addr);
	struct i2c_msg msg[2];

	if (host->i2c_client) {
		msg[0].addr = host->i2c_client->addr;
		msg[0].flags = host->i2c_client->flags & I2C_M_TEN;
		msg[0].len = sizeof(addr);
		msg[0].buf = (u8 *)(&buf);

		msg[1].addr = host->i2c_client->addr;
		msg[1].flags = host->i2c_client->flags & I2C_M_TEN;
		msg[1].flags |= I2C_M_RD;
		msg[1].len = sizeof(val);
		msg[1].buf = (u8 *)&val;

		ret = i2c_transfer(host->i2c_client->adapter, (struct i2c_msg *)msg, 2);
		if (ret < 0)
			pr_err("%s  fail\n", __func__);
	} else {
		pr_err("%s  fail client empty\n", __func__);
	}

	return val;
}

static int create_i2c_client(struct sdhci_host *host)
{
	struct i2c_adapter *adapter;

	adapter = i2c_get_adapter(MMC_I2C_BUS);
	if (!adapter) {
		pr_err("%s i2c_get_adapter error\n", __func__);
		return -EIO;
	}
	host->i2c_client = i2c_new_device(adapter, &mmc_i2c_board_info);
	if (!host->i2c_client) {
		pr_err("%s i2c_new_device error\n", __func__);
		return -EIO;
	}
	return 0;
}

static void sdhci_mshc_i2c_gpio_config(struct device *dev)
{
	int err = 0;
	int chipsel_gpio;

	chipsel_gpio = of_get_named_gpio(dev->of_node, "i2c-gpios", 0);
	if (!gpio_is_valid(chipsel_gpio)) {
		pr_err("%s: chipsel_gpio %d is not valid,check DTS\n", __func__, chipsel_gpio);
	}
	err = gpio_request((unsigned int)chipsel_gpio, "i2c-gpio");
	if (err < 0) {
		pr_err("Can`t request chipsel gpio %d\n", chipsel_gpio);
	}
	err = gpio_direction_output((unsigned int)chipsel_gpio, 0);
	if (err < 0) {
		pr_err("%s: could not set gpio %d output push down\n", __func__, chipsel_gpio);
	}
}

static int sdhci_mshc_i2c_setup(struct platform_device *pdev)
{
	int ret = 0;
	struct sdhci_host *host = platform_get_drvdata(pdev);

	if ((host->quirks2 & SDHCI_QUIRK2_HISI_COMBO_PHY_TC) && !host->i2c_client) {
		sdhci_mshc_i2c_gpio_config(&pdev->dev);
		ret = create_i2c_client(host);
		if (ret)
			dev_err(&pdev->dev, "create i2c client error\n");
	}

	return ret;
}

void sdhci_mshc_dumpregs(struct sdhci_host *host)
{
	pr_info(DRIVER_NAME ": tuning ctrl: 0x%08x | tuning stat: 0x%08x\n",
		sdhci_readl(host, SDHCI_AT_CTRL_R),
		sdhci_readl(host, SDHCI_AT_STAT_R));
	pr_info(DRIVER_NAME ": Dbg Port1: 0x%08x | Dbg Port2: 0x%08x\n",
		sdhci_readl(host, SDHCI_DEBUG_PORT1),
		sdhci_readl(host, SDHCI_DEBUG_PORT2));
	pr_info(DRIVER_NAME ": MSHC_CTRL: 0x%08x \n",
		sdhci_readb(host, SDHCI_MSHC_CTRL_R));
	pr_info(DRIVER_NAME ": LHEN_IN: 0x%08x \n",
		sdhci_sctrl_readl(host,SCTRL_SCPERCTRL7));

	if (!(host->quirks2 & SDHCI_QUIRK2_HISI_COMBO_PHY_TC)) {
		pr_info(DRIVER_NAME ": PHY init ctrl: 0x%08x | init stat: 0x%08x\n",
			sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL),
			sdhci_phy_readl(host, COMBO_PHY_PHYINITSTAT));
		pr_info(DRIVER_NAME ": PHY imp ctrl: 0x%08x | imp stat: 0x%08x\n",
			sdhci_phy_readl(host, COMBO_PHY_IMPCTRL),
			sdhci_phy_readl(host, COMBO_PHY_IMPSTATUS));
		pr_info(DRIVER_NAME ": PHY dly ctrl: 0x%08x\n",
			sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1));
		pr_info(DRIVER_NAME ": PHY dly ctrl1: 0x%08x | dly ctrl2: 0x%08x\n",
			sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1),
			sdhci_phy_readl(host,  COMBO_PHY_DLY_CTL2));
		pr_info(DRIVER_NAME ": PHY drv1: 0x%08x | drv2: 0x%08x\n",
			sdhci_phy_readl(host, COMBO_PHY_IOCTL_RONSEL_1),
			sdhci_phy_readl(host, COMBO_PHY_IOCTL_RONSEL_2));

	}
}

void sdhci_hisi_dump_clk_reg(void)
{

}

static unsigned int sdhci_mshc_get_min_clock(struct sdhci_host *host)
{
	return SDHCI_MSHC_MIN_FREQ;
}

static unsigned int sdhci_mshc_get_timeout_clock(struct sdhci_host *host)
{
	u32 div;
	unsigned long freq;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	div = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	div = (div & CLK_CTRL_TIMEOUT_MASK) >> CLK_CTRL_TIMEOUT_SHIFT;

	freq = clk_get_rate(sdhci_mshc->clk);
	freq /= (u32)(1 << (CLK_CTRL_TIMEOUT_MIN_EXP + div));

	pr_debug("%s: freq=%lx\n", __func__, freq);

	return freq;
}

static void sdhci_mshc_hardware_reset(struct sdhci_host *host)
{
	u32 count = 0;

	if (!host->sysctrl) {
		pr_err("%s: sysctrl is null, can't reset mmc!\n", __func__);
		return;
	}
	/* eMMC reset */
	sdhci_sctrl_writel(host, IP_RST_EMMC_ORLAN, SCTRL_SCPERRSTEN1);
	while (!(IP_RST_EMMC_ORLAN & sdhci_sctrl_readl(host,SCTRL_SCPERRSTSTAT1))) {
		if (count > 0xFFF) {
			pr_err("%s: emmc reset timeout\n", __func__);
			break;
		}
		count++;
	}

	/* phy reset */
	sdhci_mmc_sys_writel(host, IP_PRST_EMMCPHY, MMC1_SYSCTRL_PERRSTEN0);
	if (host->quirks2 & SDHCI_QUIRK2_HISI_COMBO_PHY_TC) {
		sdhci_i2c_writel(host, IP_RST_PHY_APB, SC_EMMC_RSTEN);
	}
}

static void sdhci_mshc_hardware_disreset(struct sdhci_host *host)
{
	int count = 0;

	if (!host->sysctrl) {
		pr_err("%s: sysctrl is null, can't reset mmc!\n", __func__);
		return;
	}

	/* phy dis-reset */
	sdhci_mmc_sys_writel(host, IP_PRST_EMMCPHY ,MMC1_SYSCTRL_PERRSTDIS0);
	if (host->quirks2 & SDHCI_QUIRK2_HISI_COMBO_PHY_TC) {
		sdhci_i2c_writel(host, IP_RST_PHY_APB, SC_EMMC_RSTDIS);
	}

	/* eMMC dis-reset */
	sdhci_sctrl_writel(host, IP_DISRST_EMMC_ORLAN, SCTRL_SCPERRSTDIS1);
	while (IP_DISRST_EMMC_ORLAN & sdhci_sctrl_readl(host, SCTRL_SCPERRSTSTAT1)) {
		if (count > 0xFFF) {
			pr_err("emmc dis-reset timeout\n");
			break;
		}
		count++;
	}
}

void sdhci_mshc_set_strobe_move_para(struct sdhci_host *host)
{
	u32 reg_val;
	u32 move_step;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	sdhci_mshc->tuning_strobe_phase_init =
		(reg_val >> DLY_3_CODE_SHIFT) & DLY_3_CODE_MASK;

	move_step = sdhci_mshc->tuning_strobe_phase_init/16;
	if (!move_step) {
		pr_err("strobe_phase_init: 0x%x\n", sdhci_mshc->tuning_strobe_phase_init);
		move_step = 1;
	}

	sdhci_mshc->tuning_strobe_phase_max = sdhci_mshc->tuning_strobe_phase_init + move_step*2;
	sdhci_mshc->tuning_strobe_phase_min = sdhci_mshc->tuning_strobe_phase_init - move_step*2;
	if (sdhci_mshc->tuning_strobe_phase_min < 0)
		sdhci_mshc->tuning_strobe_phase_min = 0;
}

void sdhci_combo_phy_delay_measurement(struct sdhci_host *host, struct mmc_ios *ios)
{
	u32 count;
	u32 reg_val;
	u32 delaymeas_code;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	if (!sdhci_mshc->need_delay_measure)
		return;

	reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL);
	/*bit0:dlymeas_en, bit2:init_en*/
	reg_val |= (INIT_EN | DLYMEAS_EN);
	sdhci_phy_writel(host, reg_val, COMBO_PHY_PHYINITCTRL);

	reg_val =  sdhci_phy_readl(host, COMBO_PHY_PHYCTRL2);
	/*phy_dlymeas_update on*/
	reg_val |= PHY_DLYMEAS_UPDATE;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_PHYCTRL2);

	count = 0;
	do {
		if (count > 30) {
			pr_err("%s wait for delay measurement done timeout\n", __func__);
			break;
		}
		udelay(100);
		count++;
		reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL);
	} while (reg_val & INIT_EN);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL);
	delaymeas_code = reg_val & DLY_CPDE_1T_MASK;

	/*set tx_clk as 1/5T*/
	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	reg_val &= ~(DLY_1_CODE_MASK << DLY_1_CODE_SHIFT);
	delaymeas_code = delaymeas_code/5;
	reg_val |= ((DLY_1_CODE_MASK & (delaymeas_code)) << DLY_1_CODE_SHIFT);
	sdhci_phy_writel(host, reg_val, COMBO_PHY_DLY_CTL1);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYCTRL2);
	/*phy_dlymeas_update off at dlymeas_done*/
	reg_val &= ~PHY_DLYMEAS_UPDATE;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_PHYCTRL2);

	sdhci_mshc_set_strobe_move_para(host);

	/* FPGA may has glitch after change clock, reset rx*/
	if (host->quirks2 & SDHCI_QUIRK2_HISI_COMBO_PHY_TC) {
		reg_val = sdhci_mmc_sys_readl(host, MMC1_SYCTRLS_CTRL1);
		reg_val |= CRESETN_RX;
		sdhci_mmc_sys_writel(host, reg_val, MMC1_SYCTRLS_CTRL1);

		reg_val = sdhci_mmc_sys_readl(host, MMC1_SYCTRLS_CTRL1);
		reg_val &= ~CRESETN_RX;
		sdhci_mmc_sys_writel(host, reg_val, MMC1_SYCTRLS_CTRL1);
	}

	sdhci_mshc->need_delay_measure = false;

	pr_info("eMMC phy delay measurement done\n");
}

#define TC_IOC_BASE	0x25000
void sdhci_set_pinctrl(struct sdhci_host *host, u32 reg)
{
	u32 val;
	val = sdhci_i2c_readl(host, TC_IOC_BASE + reg);
	val &= ~(0xF << 4);
	val |= (0x7 << 4);
	sdhci_i2c_writel(host, val, TC_IOC_BASE + reg);
}

void sdhci_mshc_pinctrl_init(struct sdhci_host *host)
{
	sdhci_set_pinctrl(host, 0x800);
	sdhci_set_pinctrl(host, 0x808);
	sdhci_set_pinctrl(host, 0x80C);
	sdhci_set_pinctrl(host, 0x810);
	sdhci_set_pinctrl(host, 0x814);
	sdhci_set_pinctrl(host, 0x818);
	sdhci_set_pinctrl(host, 0x81C);
	sdhci_set_pinctrl(host, 0x820);
	sdhci_set_pinctrl(host, 0x824);

}

static void sdhci_combo_phy_zq_cal(struct sdhci_host *host)
{
	u32 count;
	u32 reg_val;

	//sdhci_mshc_pinctrl_init(host);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_IMPCTRL);
	/*zcom_rsp_dly set as d'60*/
	reg_val &= ~(IMPCTRL_ZCOM_RSP_DLY_MASK);
	reg_val |= (IMPCTRL_ZCOM_RSP_DLY_MASK & 0x3C);
	sdhci_phy_writel(host, reg_val, COMBO_PHY_IMPCTRL);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL);
	/*disable delay measuremnt to wait zq calc ready*/
	reg_val &= ~DLYMEAS_EN;
	/*zcal_en set 1 to start impedance cal*/
	reg_val |= ZCAL_EN;
	/*int_en & zcal_en all set to 1*/
	reg_val |= INIT_EN;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_PHYINITCTRL);

	count = 0;
	do {
		if (count > 30) {
			pr_err("%s wait  timeout\n", __func__);
			break;
		}
		udelay(100);
		count++;
		reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL);
	} while (reg_val & (INIT_EN |ZCAL_EN));

	pr_err("%s the IMPSTATUS is %x\n", __func__,
			sdhci_phy_readl(host, COMBO_PHY_IMPSTATUS));
}

static void sdhci_combo_phy_init(struct sdhci_host *host)
{
	u32 reg_val;

	reg_val = sdhci_phy_readl(host, COMBO_PHY_PHYINITCTRL);
	reg_val |= BIST_CLK;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_PHYINITCTRL);

	/* clear data output enable delay code */
	sdhci_phy_writel(host, 0x0, COMBO_PHY_DATENA_DLY);

	reg_val = PUPD_EN_DATA | PUPD_EN_STROBE |PUPD_EN_CMD
			| PULLUP_DATA | PULLUP_CMD;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_IOCTL_PUPD);

	/*set drv 40o*/
	reg_val = EMMC_RONSEL_0;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_IOCTL_RONSEL_1);
	reg_val = sdhci_phy_readl(host, COMBO_PHY_IOCTL_RONSEL_2);
	reg_val |= EMMC_RONSEL_2;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_IOCTL_RONSEL_2);

	/*enable phy input output*/
	reg_val = DA_EMMC_E | DA_EMMC_IE;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_IOCTL_IOE);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	reg_val |= INV_TX_CLK;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_DLY_CTL1);

	/*do ZQ Calibration and so on, to be finish*/
	sdhci_combo_phy_zq_cal(host);
}

void sdhci_mshc_config_strobe_clk(struct sdhci_host *host, unsigned timing)
{
	u16 reg_u16;

	if (timing == MMC_TIMING_MMC_DDR52
		|| timing == MMC_TIMING_MMC_HS400) {
		reg_u16 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg_u16 |= SDHCI_CTRL_INV_DATA;
		sdhci_writew(host, reg_u16, SDHCI_HOST_CONTROL2);
	}

}

void sdhci_mshc_OD_enable(struct sdhci_host *host)
{

	/* configure OD enable */

	pr_debug("%s: end!\n", __func__);
}

int sdhci_disable_open_drain(struct sdhci_host *host)
{

	pr_debug("%s\n", __func__);
	return 0;
}

static int sdhci_of_mshc_enable_enhanced_strobe(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	u16 reg_val;

	reg_val = sdhci_readw(host, SDHCI_EMMC_CTRL_R);
	reg_val |= ENH_STROBE_ENABLE;
	sdhci_writew(host, reg_val, SDHCI_EMMC_CTRL_R);

	sdhci_mshc->enhanced_strobe_enabled = 1;

	return 0;
}

static void sdhci_mshc_hw_reset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	sdhci_mshc_hardware_reset(host);
	sdhci_mshc_hardware_disreset(host);
	sdhci_combo_phy_init(host);
	/* after reset, delay measure is need */
	sdhci_mshc->need_delay_measure = true;
}

int sdhci_chk_busy_before_send_cmd(struct sdhci_host *host,
	struct mmc_command* cmd)
{
	unsigned long timeout;

	/* We shouldn't wait for busy for stop commands */
	if ((cmd->opcode != MMC_STOP_TRANSMISSION) && (cmd->opcode != MMC_SEND_STATUS)) {
		/* Wait busy max 10 s */
		timeout = 10000;
		while (!(sdhci_readl(host, SDHCI_PRESENT_STATE) & SDHCI_DATA_0_LVL_MASK)) {
			if (timeout == 0) {
				pr_err("%s: wait busy 10s time out.\n", mmc_hostname(host->mmc));
				sdhci_dumpregs(host);
				cmd->error = -ENOMSG;
				tasklet_schedule(&host->finish_tasklet);
				return -ENOMSG;
			}
			timeout--;
			mdelay(1);
		}
	}
	return 0;
}

void sdhci_mshc_set_version(struct sdhci_host *host)
{
	u16 ctrl;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl |= SDHCI_CTRL_HOST_VER4_ENABLE;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

}


int sdhci_mshc_enable_dma(struct sdhci_host *host)
{
	u16 ctrl;
	if (host->runtime_suspended)
		return 0;

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl |= SDHCI_CTRL_HOST_VER4_ENABLE;
	if (host->flags & SDHCI_USE_64_BIT_DMA)
		ctrl |= SDHCI_CTRL_64BIT_ADDR;
	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);

	ctrl = (u16)sdhci_readb(host, SDHCI_HOST_CONTROL);
	ctrl &= ~SDHCI_CTRL_DMA_MASK;
	if (host->flags & SDHCI_USE_ADMA)
		ctrl |= SDHCI_CTRL_ADMA2;
	else
		ctrl |= SDHCI_CTRL_SDMA;
	sdhci_writeb(host, (u8)ctrl, SDHCI_HOST_CONTROL);

	return 0;
}

extern void sdhci_set_transfer_irqs(struct sdhci_host *host);
static void sdhci_mshc_restore_transfer_para(struct sdhci_host *host)
{
	u16 mode;

	sdhci_mshc_enable_dma(host);
	sdhci_set_transfer_irqs(host);

	mode = SDHCI_TRNS_BLK_CNT_EN;
	mode |= SDHCI_TRNS_MULTI;
	if (host->flags & SDHCI_REQ_USE_DMA)
		mode |= SDHCI_TRNS_DMA;
	sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);
	/* Set the DMA boundary value and block size */
	sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
		512), SDHCI_BLOCK_SIZE);
}

void sdhci_mshc_select_card_type(struct sdhci_host *host)
{
	u8 reg_u8;
	u16 reg_u16;
	u32 reg_u32;

	reg_u16 = sdhci_readw(host, SDHCI_EMMC_CTRL_R);
	reg_u16 |= CARD_IS_EMMC;
	sdhci_writew(host, reg_u16, SDHCI_EMMC_CTRL_R);

	reg_u8 = sdhci_readb(host, SDHCI_MSHC_CTRL_R);
	reg_u8 &= ~CMD_CONFLICT_CHECK;
	reg_u8 |=  SW_CG_DIS;
	reg_u8 |= ACCESS_ALL_REGION;
	sdhci_writeb(host, reg_u8, SDHCI_MSHC_CTRL_R);

	reg_u32 = sdhci_readl(host, SDHCI_MBIU_CTRL);
	reg_u32 &= ~WRITE_OSRC_LMT_MASK;
	reg_u32 |= WRITE_OSRC_LMT;
	sdhci_writel(host, reg_u32, SDHCI_MBIU_CTRL);
}

void sdhci_mshc_set_uhs_signaling(struct sdhci_host *host, unsigned timing)
{
	u16 ctrl;

	sdhci_mshc_config_strobe_clk(host, timing);
	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl &= ~SDHCI_CTRL_UHS_MASK;
	if (timing == MMC_TIMING_MMC_HS200)
		ctrl |= SDHCI_CTRL_HS_HS200;
	else if (timing == MMC_TIMING_MMC_HS400)
		ctrl |= SDHCI_CTRL_HS_HS400;
	else if (timing == MMC_TIMING_MMC_HS)
		ctrl |= SDHCI_CTRL_HS_SDR;
	else if (timing == MMC_TIMING_MMC_DDR52)
		ctrl |= SDHCI_CTRL_HS_DDR;

	sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
}


#define TUNING_LOOP_64		64
#define TUNING_LOOP_128	128
#define TUNING_MIN_CONTINUOUS	20
static void sdhci_mshc_init_tuning_para(struct sdhci_mshc_data *sdhci_mshc)
{
	sdhci_mshc->tuning_count = 0;
	sdhci_mshc->tuning_phase_record_low = 0;
	sdhci_mshc->tuning_phase_record_high = 0;
	sdhci_mshc->tuning_phase_best = TUNING_PHASE_INVALID;

	sdhci_mshc->tuning_move_phase = TUNING_PHASE_INVALID;
	sdhci_mshc->tuning_move_count = 0;
	sdhci_mshc->tuning_phase_max = 64;
	sdhci_mshc->tuning_phase_min = 0;

	sdhci_mshc->tuning_strobe_phase_init = 0;
	sdhci_mshc->tuning_strobe_move_phase = TUNING_PHASE_INVALID;
	sdhci_mshc->tuning_strobe_move_count = 0;
	sdhci_mshc->tuning_strobe_phase_max = 0;
	sdhci_mshc->tuning_strobe_phase_min = 0;

	sdhci_mshc->tuning_loop_max = TUNING_LOOP_64;
	if (sdhci_mshc->tuning_128_flag) {
		sdhci_mshc->tuning_loop_max = TUNING_LOOP_128;
		sdhci_mshc->tuning_phase_max = 128;
	}
}

static void sdhci_mshc_set_tuning_strobe_phase(struct sdhci_host *host, u32 val)
{
	u32 reg_val;

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	reg_val &= ~(DLY_3_CODE_MASK << DLY_3_CODE_SHIFT);
	reg_val |= (val << DLY_3_CODE_SHIFT);
	sdhci_phy_writel(host, reg_val, COMBO_PHY_DLY_CTL1);
}

static void sdhci_mshc_set_tuning_phase(struct sdhci_host *host, u32 val)
{
	u32 reg_val;

	if (!(host->flags & SDHCI_EXE_SOFT_TUNING)) {
		reg_val = (u16)sdhci_readw(host, SDHCI_AT_CTRL_R);
		reg_val |= SW_TUNE_EN;
		sdhci_writew(host, (u16)reg_val, SDHCI_AT_CTRL_R);
	}

	reg_val = sdhci_readl(host, SDHCI_AT_STAT_R);
	reg_val &= ~CENTER_PH_CODE_MASK;
	reg_val |= val;
	sdhci_writel(host, reg_val, SDHCI_AT_STAT_R);

	if (!(host->flags & SDHCI_EXE_SOFT_TUNING)) {
		reg_val = (u16)sdhci_readw(host, SDHCI_AT_CTRL_R);
		reg_val &= ~SW_TUNE_EN;
		sdhci_writew(host, (u16)reg_val, SDHCI_AT_CTRL_R);
	}
}

static void sdhci_mshc_save_tuning_phase(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	if (sdhci_mshc->tuning_count < TUNING_LOOP_64)
		sdhci_mshc->tuning_phase_record_low |= (u64)0x1 << (sdhci_mshc->tuning_count);
	else
		sdhci_mshc->tuning_phase_record_high |= (u64)0x1 << (sdhci_mshc->tuning_count - TUNING_LOOP_64);
}

static void sdhci_mshc_add_tuning_phase(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	sdhci_mshc->tuning_count++;

	sdhci_mshc_set_tuning_phase(host, sdhci_mshc->tuning_count);
}

#define cror(data, shift) ((data >> shift) | (data << (sizeof(data)*8 - shift)))
static int sdhci_mshc_find_best_phase(struct sdhci_host *host)
{
	u64 tuning_phase_record;
	u32 loop = 0;
	u32 max_loop;
	u32 expect_min_continuous;
	u32 max_continuous_len = 0;
	u32 current_continuous_len = 0;
	u32 max_continuous_start = 0;
	u32 current_continuous_start = 0;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	max_loop = sdhci_mshc->tuning_loop_max;
	expect_min_continuous = TUNING_MIN_CONTINUOUS;

	pr_err("tuning_phase_record_low: 0x%llx,\n", sdhci_mshc->tuning_phase_record_low);
	pr_err("tuning_phase_record_high: 0x%llx,\n", sdhci_mshc->tuning_phase_record_high);

	tuning_phase_record = sdhci_mshc->tuning_phase_record_low;
	if ((tuning_phase_record & 0x1) && (tuning_phase_record & ((u64)0x1 << 63)))
		max_loop = sdhci_mshc->tuning_loop_max * 2;

	do {
		if (tuning_phase_record & 0x1) {
			if (!current_continuous_len)
				current_continuous_start = loop;

			current_continuous_len++;
			if (loop == max_loop -1) {
				if (current_continuous_len > max_continuous_len) {
					max_continuous_len = current_continuous_len;
					max_continuous_start = current_continuous_start;
				}
			}
		} else {
			if (current_continuous_len) {
				if (current_continuous_len > max_continuous_len) {
					max_continuous_len = current_continuous_len;
					max_continuous_start = current_continuous_start;
				}

				current_continuous_len = 0;
			}
		}

		loop++;
		tuning_phase_record = cror(tuning_phase_record, 1);
	} while (loop < max_loop);

	if (!max_continuous_len)
		return -1;

	if (max_continuous_len < expect_min_continuous)
		pr_info("max continuous len less than %d\n", expect_min_continuous);

	if (max_continuous_len > TUNING_LOOP_64)
		max_continuous_len = TUNING_LOOP_64;

	sdhci_mshc->tuning_phase_best = max_continuous_start + max_continuous_len/2;
	if (sdhci_mshc->tuning_phase_best >= TUNING_LOOP_64)
		sdhci_mshc->tuning_phase_best = sdhci_mshc->tuning_phase_best - TUNING_LOOP_64;
	pr_err("soft tuning best phase:0x%x\n", sdhci_mshc->tuning_phase_best);

	return 0;
}

static int sdhci_mshc_tuning_move_strobe(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	int loop = 0;
	int ret = 0;
	int move_step ;

	move_step = sdhci_mshc->tuning_strobe_phase_init/16;
	if (!move_step) {
		move_step = 1;
	}

	/* first move tuning, set soft tuning optimum sample. When second or more
	    move tuning, use the sample near optimum sample */
	for (loop = 0; loop < 2; loop++) {
		sdhci_mshc->tuning_strobe_move_count++;
		sdhci_mshc->tuning_strobe_move_phase = sdhci_mshc->tuning_strobe_phase_init
			+ ((sdhci_mshc->tuning_strobe_move_count % 2) ? move_step: -move_step)
			   * (sdhci_mshc->tuning_strobe_move_count / 2);

		if ((sdhci_mshc->tuning_strobe_move_phase >= sdhci_mshc->tuning_strobe_phase_max)
			|| (sdhci_mshc->tuning_strobe_move_phase < sdhci_mshc->tuning_strobe_phase_min))
			continue;
		else
			break;
	}

	if ((sdhci_mshc->tuning_strobe_move_phase >= sdhci_mshc->tuning_strobe_phase_max)
		|| (sdhci_mshc->tuning_strobe_move_phase < sdhci_mshc->tuning_strobe_phase_min)){
		pr_err("%s: tuning move fail.\n", __func__);
		sdhci_mshc->tuning_strobe_move_phase = TUNING_PHASE_INVALID;
		ret = -1;
	} else {
		sdhci_mshc_set_tuning_strobe_phase(host, sdhci_mshc->tuning_strobe_move_phase);
		pr_err("%s: tuning move to sample=%d\n", __func__, sdhci_mshc->tuning_strobe_move_phase);
	}

	return ret;
}

static int sdhci_mshc_tuning_move_clk(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	int loop = 0;
	int ret = 0;
	int move_step = 2;

	/* soft tuning fail or error then return error */
	if (sdhci_mshc->tuning_phase_best >= sdhci_mshc->tuning_loop_max) {
		pr_err("%s: soft tuning fail, can not move tuning, tuning_init_sample=%d.\n",
				__func__, sdhci_mshc->tuning_phase_best);
		return -1;
	}
	/* first move tuning, set soft tuning optimum sample. When second or more move
	    tuning, use the sample near optimum sample */
	for (loop = 0; loop < 2; loop++) {
		sdhci_mshc->tuning_move_count++;
		sdhci_mshc->tuning_move_phase = sdhci_mshc->tuning_phase_best
			+ ((sdhci_mshc->tuning_move_count % 2) ? move_step : -move_step)
			   * (sdhci_mshc->tuning_move_count / 2);

		if ((sdhci_mshc->tuning_move_phase >= sdhci_mshc->tuning_phase_max)
			|| (sdhci_mshc->tuning_move_phase < sdhci_mshc->tuning_phase_min)) {
			continue;
		} else {
			break;
		}
	}

	if ((sdhci_mshc->tuning_move_phase >= sdhci_mshc->tuning_loop_max)
		|| (sdhci_mshc->tuning_move_phase < sdhci_mshc->tuning_phase_min)) {
		pr_err("%s: tuning move fail.\n", __func__);
		sdhci_mshc->tuning_move_phase = TUNING_PHASE_INVALID;
		ret = -1;
	} else {
		sdhci_mshc_set_tuning_phase(host, sdhci_mshc->tuning_move_phase);
		pr_err("%s: tuning move to phase=%d\n", __func__, sdhci_mshc->tuning_move_phase);
	}

	return ret;
}

static int sdhci_mshc_tuning_move(struct sdhci_host *host, int is_move_strobe, int flag)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	/* set tuning_strobe_move_count to 0, next tuning move will begin from optimum sample */
	if (flag) {
		sdhci_mshc->tuning_move_count = 0;
		sdhci_mshc->tuning_strobe_move_count = 0;
		return 0;
	}

	if (is_move_strobe) {
		return sdhci_mshc_tuning_move_strobe(host);
	} else {
		return sdhci_mshc_tuning_move_clk(host);
	}
}

static const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const u8 tuning_blk_pattern_8bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

int sdhci_mshc_soft_tuning(struct sdhci_host *host, u32 opcode, bool set)
{
	struct sdhci_mshc_data *sdhci_mshc = ((struct sdhci_pltfm_host *)sdhci_priv(host))->priv;
	int tuning_loop_counter;
	int ret = 0;
	const u8 *tuning_blk_pattern = NULL;
	u8 *tuning_blk = NULL;
	int blksz = 0;
	u16 reg_val;
	u32 hw_tuning_phase;

	pr_err("after tuning: 0x%x\n", sdhci_readl(host, SDHCI_AT_STAT_R));
	hw_tuning_phase = CENTER_PH_CODE_MASK & sdhci_readl(host, SDHCI_AT_STAT_R);

	sdhci_mshc_init_tuning_para(sdhci_mshc);
	tuning_loop_counter = sdhci_mshc->tuning_loop_max;

	host->flags |= SDHCI_EXE_SOFT_TUNING;
	pr_err("%s: now, start tuning soft...\n", __func__);

	if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
		if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_8) {
			tuning_blk_pattern = tuning_blk_pattern_8bit;
			blksz = 128;
		} else if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
			tuning_blk_pattern = tuning_blk_pattern_4bit;
			blksz = 64;
		}
	} else {
		tuning_blk_pattern = tuning_blk_pattern_4bit;
		blksz = 64;
	}
	tuning_blk = kzalloc(blksz, GFP_KERNEL);
	if (!tuning_blk) {
		ret = -ENOMEM;
		goto err;
	}

	reg_val = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg_val &= ~SDHCI_CTRL_TUNED_CLK;
	sdhci_writew(host, reg_val, SDHCI_HOST_CONTROL2);

	reg_val = sdhci_readw(host, SDHCI_AT_CTRL_R);
	reg_val |= SW_TUNE_EN;
	sdhci_writew(host, reg_val, SDHCI_AT_CTRL_R);

	sdhci_mshc_set_tuning_phase(host, 0);

	while (tuning_loop_counter--) {
		struct mmc_request mrq = { NULL };
		struct mmc_command cmd = { 0 };
		struct mmc_data data = { 0 };
		struct scatterlist sg;

		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		cmd.error = 0;

		data.blksz = blksz;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;
		data.error = 0;
		data.timeout_ns = 50000000;//50ms

		sg_init_one(&sg, tuning_blk, blksz);

		mrq.cmd = &cmd;
		mrq.data = &data;

		mmc_wait_for_req(host->mmc, &mrq);
		/* no cmd or data error and tuning data is ok, then set sample flag */
		if (!cmd.error && !data.error && tuning_blk
			&& (memcmp(tuning_blk, tuning_blk_pattern, sizeof(blksz)) == 0)) {//lint !e668
			sdhci_mshc_save_tuning_phase(host);
		}

		sdhci_mshc_add_tuning_phase(host);
	}
	/*lint -e668*/
	kfree(tuning_blk);/*kfree will check NULL*/
	/*lint +e668*/

	ret = sdhci_mshc_find_best_phase(host);
	if (ret) {
		pr_err("can not find best phase\n");
		goto err;
	}

	if (set) {
		sdhci_mshc_set_tuning_phase(host, sdhci_mshc->tuning_phase_best);
	} else {
		sdhci_mshc_set_tuning_phase(host, hw_tuning_phase);
		sdhci_mshc->tuning_phase_best = hw_tuning_phase;
	}

err:
	reg_val = sdhci_readw(host, SDHCI_AT_CTRL_R);
	reg_val &= ~SW_TUNE_EN;
	sdhci_writew(host, reg_val, SDHCI_AT_CTRL_R);

	reg_val = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg_val |= SDHCI_CTRL_TUNED_CLK;
	sdhci_writew(host, reg_val, SDHCI_HOST_CONTROL2);

	/* restore block size after tuning */
	sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,512), SDHCI_BLOCK_SIZE);

	host->flags &= ~SDHCI_EXE_SOFT_TUNING;
	return ret;
}

/*
 * Synopsys controller's max busy timeout is 6.99s. Although we do not
 * use the timout clock, but the function mmc_do_calc_max_discard use it,
 * if the the card's default erase timeout is more than 6.99s, the discard
 * cannot be supported.
 * As we set the request timout 30s, so we set a busy timout value which
 * is less than it, choose 1 << 29, the busy clk is 20M,
 * timout 1 << 29 / 20000 = 26.8s
 * */
u32 sdhci_mshc_get_max_timeout_count(struct sdhci_host *host)
{
	return 1 << 29;
}


static struct sdhci_ops sdhci_mshc_ops = {
	.get_min_clock = sdhci_mshc_get_min_clock,
	.set_clock = sdhci_set_clock,
	.enable_dma = sdhci_mshc_enable_dma,
	.get_max_clock = sdhci_pltfm_clk_get_max_clock,
	.get_timeout_clock = sdhci_mshc_get_timeout_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_mshc_set_uhs_signaling,
	.tuning_soft = sdhci_mshc_soft_tuning,
	.tuning_move = sdhci_mshc_tuning_move,
	.enable_enhanced_strobe = sdhci_of_mshc_enable_enhanced_strobe,
	.check_busy_before_send_cmd = sdhci_chk_busy_before_send_cmd,
	.restore_transfer_para = sdhci_mshc_restore_transfer_para,
	.hw_reset = sdhci_mshc_hw_reset,
	.select_card_type = sdhci_mshc_select_card_type,
	.dumpregs = sdhci_mshc_dumpregs,
	.delay_measurement = sdhci_combo_phy_delay_measurement,
	.get_max_timeout_count = sdhci_mshc_get_max_timeout_count,
};

static struct sdhci_pltfm_data sdhci_mshc_pdata = {
	.ops = &sdhci_mshc_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN |
			SDHCI_QUIRK2_USE_1_8_V_VMMC,
};

#ifdef CONFIG_PM_SLEEP
/**
 * sdhci_mshc_suspend - Suspend method for the driver
 * @dev:	Address of the device structure
 * Returns 0 on success and error value on error
 *
 * Put the device in a low power state.
 */
static int sdhci_mshc_suspend(struct device *dev)
{
	int ret;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	u32 count;

	dev_info(dev, "%s: suspend +\n", __func__);

	ret = sdhci_suspend_host(host);
	if (ret)
		return ret;

	/* enable phy io retention */
	sdhci_sctrl_writel(host, RG_EMMC_LHEN_IN_RET, SCTRL_SCPERCTRL7);

	clk_disable_unprepare(sdhci_mshc->clk);

	/* close phy, cq ,hclk clock */
	count = 0;
	sdhci_mmc_sys_writel(host, GT_CLK_EMMC_OR, MMC1_SYSCTRL_PERDIS0);
	while ((GT_CLK_EMMC_OR & sdhci_mmc_sys_readl(host, MMC1_SYSCTRL_PERCLKEN0))) {
		if (count > 0xFFF) {
			dev_err(dev, "emmc clk disable timeout \n");
			break;
		}
		count++;
	}

	sdhci_mshc_hardware_reset(host);

	dev_info(dev, "%s: suspend -\n", __func__);

	return 0;
}

/**
 * sdhci_mshc_resume - Resume method for the driver
 * @dev:	Address of the device structure
 * Returns 0 on success and error value on error
 *
 * Resume operation after suspend
 */
static int sdhci_mshc_resume(struct device *dev)
{
	int ret;
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	u32 count = 0;

	dev_info(dev, "%s: resume +\n", __func__);

	sdhci_mshc_hardware_disreset(host);

	/* disrest phy, cq ,hclk clock */
	count = 0;
	sdhci_mmc_sys_writel(host, GT_CLK_EMMC_OR, MMC1_SYSCTRL_PEREN0);
	while (GT_CLK_EMMC_OR & sdhci_mmc_sys_readl(host, MMC1_SYSCTRL_PERCLKEN0)) {
		if (count > 0xFFF) {
			dev_err(dev, "emmc1 disreset timeout \n");
			break;
		}
		count++;
	}

	/* enable phy io retention */
	sdhci_sctrl_writel(host, RG_EMMC_LHEN_IN_NOR, SCTRL_SCPERCTRL7);

	ret = clk_set_rate(sdhci_mshc->clk, xin_clk);
	if (ret)
		dev_err(dev, "Error setting desired xin_clk=%u, get clk=%lu.\n", xin_clk, clk_get_rate(sdhci_mshc->clk));
	ret = clk_prepare_enable(sdhci_mshc->clk);
	if (ret) {
		dev_err(dev, "Cannot enable SD clock.\n");
		return ret;
	}
	pr_debug("%s: sdhci_mshc->clk=%lu.\n",
			__func__, clk_get_rate(sdhci_mshc->clk));

	sdhci_combo_phy_init(host);

	pr_debug("%s: host->mmc->ios.clock=%d, timing=%d.\n", __func__, host->mmc->ios.clock, host->mmc->ios.timing);

	sdhci_mshc->need_delay_measure = true;
	ret = sdhci_resume_host(host);
	if (ret)
		return ret;
	/*use soft tuning sample send wake up cmd then retuning
	 *phase set need to after sdhci_resume_host,because phase may
	 *be clear by host control2 set.The flow need to be provided
	 *by soc,fix me;
	 */
	if (host->mmc->ios.timing >= MMC_TIMING_MMC_HS200) {
		pr_info("%s: tuning_move_sample=%d, tuning_init_sample=%d,"
			"tuning_strobe_move_sample=%d, tuning_strobe_init_sample=%d.\n",
			__func__, sdhci_mshc->tuning_move_phase, sdhci_mshc->tuning_phase_best,
			sdhci_mshc->tuning_strobe_move_phase, sdhci_mshc->tuning_strobe_phase_init);
		if (sdhci_mshc->tuning_move_phase == TUNING_PHASE_INVALID) {
			sdhci_mshc->tuning_move_phase = sdhci_mshc->tuning_phase_best;
		}
		sdhci_mshc_set_tuning_phase(host, sdhci_mshc->tuning_move_phase);
		if (sdhci_mshc->tuning_strobe_move_phase == TUNING_PHASE_INVALID) {
			sdhci_mshc->tuning_strobe_move_phase = sdhci_mshc->tuning_strobe_phase_init;
		}
		sdhci_mshc_set_tuning_strobe_phase(host,sdhci_mshc->tuning_strobe_move_phase);

		pr_err("resume,host_clock = %d, mmc_clock = %d\n", host->clock, host->mmc->ios.clock);
		//sdhci_set_clock(host, host->mmc->ios.clock);
	}

	dev_info(dev, "%s: resume -\n", __func__);

	return 0;
}
#endif /* ! CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
int sdhci_mshc_runtime_suspend_host(struct sdhci_host *host)
{
	unsigned long flags;

	mmc_retune_timer_stop(host->mmc);
	mmc_retune_needed(host->mmc);

	spin_lock_irqsave(&host->lock, flags);
	host->ier &= SDHCI_INT_CARD_INT;
	sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);
	spin_unlock_irqrestore(&host->lock, flags);

	synchronize_hardirq(host->irq);

	spin_lock_irqsave(&host->lock, flags);
	host->runtime_suspended = true;
	spin_unlock_irqrestore(&host->lock, flags);

	return 0;
}

static int sdhci_mshc_runtime_suspend(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	int ret = 0;
	u32 reg_val;
	u16 reg_u16;

	ret = sdhci_mshc_runtime_suspend_host(host);

	reg_u16 = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	reg_u16 &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg_u16, SDHCI_CLOCK_CONTROL);
	udelay(5);

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	reg_val &= ~INV_TX_CLK;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_DLY_CTL1);

	if (!IS_ERR(sdhci_mshc->clk))
		clk_disable_unprepare(sdhci_mshc->clk);

	/* close phy, cq ,hclk clock */
	sdhci_mmc_sys_writel(host, EMMC_SYS_GT_CLK_MASK, EMMC_SYS_CRG_CFG1);

	return ret;
}

extern void sdhci_set_default_irqs(struct sdhci_host *host);
static int sdhci_mshc_runtime_resume_host(struct sdhci_host *host)
{
	unsigned long flags;
	u16 reg_val;

	sdhci_set_default_irqs(host);

	reg_val = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	reg_val |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg_val, SDHCI_CLOCK_CONTROL);

	spin_lock_irqsave(&host->lock, flags);
	host->runtime_suspended = false;
#ifdef CONFIG_MMC_CQ_HCI
	if (host->mmc->cmdq_ops && host->mmc->cmdq_ops->restore_irqs) {
		host->mmc->cmdq_ops->restore_irqs(host->mmc);
	}
#endif
	spin_unlock_irqrestore(&host->lock, flags);

	return 0;
}

static int sdhci_mshc_runtime_resume(struct device *dev)
{
	struct sdhci_host *host = dev_get_drvdata(dev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;
	u32 reg_val;

	/* enable phy, cq ,hclk clock */
	sdhci_mmc_sys_writel(host, EMMC_SYS_GT_CLK_MASK | EMMC_SYS_GT_CLK, EMMC_SYS_CRG_CFG1);

	if (!IS_ERR(sdhci_mshc->clk)) {
		if (clk_prepare_enable(sdhci_mshc->clk))
			pr_warn("%s: clk_prepare_enable sdhci_arasan->clk failed.\n", __func__);
	}

	reg_val = sdhci_phy_readl(host, COMBO_PHY_DLY_CTL1);
	reg_val |= INV_TX_CLK;
	sdhci_phy_writel(host, reg_val, COMBO_PHY_DLY_CTL1);

	return sdhci_mshc_runtime_resume_host(host);
}
#endif

static const struct dev_pm_ops sdhci_mshc_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sdhci_mshc_suspend, sdhci_mshc_resume)
	SET_RUNTIME_PM_OPS(sdhci_mshc_runtime_suspend, sdhci_mshc_runtime_resume,
						   NULL)
};

static int sdhci_mshc_get_resource(struct platform_device *pdev, struct sdhci_host *host)
{
	struct device_node *np = NULL;
	struct resource *mem_res;

	/* get resource of mmc phy ctrl */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	host->mmc_phy = devm_ioremap_resource(&pdev->dev, mem_res);
	if (!host->mmc_phy) {
		dev_err(&pdev->dev, "cannot ioremap for mmc phy ctrl register\n");
		return -ENOMEM;
	}

	/* get resource of mmc sys ctrl */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	host->mmc_sys = devm_ioremap_resource(&pdev->dev, mem_res);
	if (!host->mmc_sys) {
		dev_err(&pdev->dev, "cannot ioremap for mmc sys ctrl register\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np) {
		dev_err(&pdev->dev,
			"can't find device node \"hisilicon,sysctrl\"\n");
		return -ENXIO;
	}

	host->sysctrl = of_iomap(np, 0);
	if (!host->sysctrl) {
		printk("sysctrl iomap error\n");
		return -ENOMEM;
	}

	return 0;
}

static void sdhci_mshc_populate_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_mshc_data *sdhci_mshc = ((struct sdhci_pltfm_host *)sdhci_priv(host))->priv;

	if (!np) {
		dev_err(&pdev->dev, "can not find device node\n");
		return;
	}

	if (of_get_property(np, "use-combo-phy-tc", NULL)) {
		host->quirks2 |= SDHCI_QUIRK2_HISI_COMBO_PHY_TC;
		pr_info("use combo phy testchip\n");
	}

	if (of_get_property(np, "tuning_phase_128", NULL)) {
		sdhci_mshc->tuning_128_flag = 1;
		pr_info("use tuning phase 128");
	}

}

static int sdhci_mshc_clock_init(struct platform_device *pdev, struct sdhci_mshc_data *sdhci_mshc)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;

	sdhci_mshc->clk = devm_clk_get(&pdev->dev, "clk_xin");
	if (IS_ERR(sdhci_mshc->clk)) {
		dev_err(&pdev->dev, "clk_xin clock not found.\n");
		ret = PTR_ERR(sdhci_mshc->clk);
		return ret;
	}

	if (of_device_is_available(np)) {
		if (of_property_read_u32(np, "clk-xin", &xin_clk))
			dev_err(&pdev->dev, "clk-xin  cannot get from dts, use the default value!\n");
	}

	ret = clk_set_rate(sdhci_mshc->clk, xin_clk);
	if (ret)
		dev_err(&pdev->dev, "Error setting desired xin_clk=%u, get clk=%lu.\n",
			xin_clk, clk_get_rate(sdhci_mshc->clk));
	ret = clk_prepare_enable(sdhci_mshc->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable SD clock.\n");
		return ret;

	}

	pr_info("%s: clk_xin=%lu\n", __func__, clk_get_rate(sdhci_mshc->clk));

	return 0;
}


#ifdef CONFIG_MMC_CQ_HCI
static void sdhci_mshc_cmdq_init(struct platform_device *pdev)
{
	int ret;
	u16 cmdq_reg_offset;
	struct sdhci_host *host = platform_get_drvdata(pdev);

	if (host->mmc->caps2 & MMC_CAP2_CMD_QUEUE) {
		cmdq_reg_offset = 0xFFF & sdhci_readw(host, SDHCI_VENDOR2_SPECIFIC_AREA);
		host->cq_host = cmdq_pltfm_init(pdev, host->ioaddr + cmdq_reg_offset);
		if (IS_ERR(host->cq_host)) {
			ret = PTR_ERR(host->cq_host);
			dev_err(&pdev->dev, "cmd queue platform init failed (%u)\n", ret);
			host->mmc->caps2 &= ~MMC_CAP2_CMD_QUEUE;
		}

		if (!(host->quirks2 & SDHCI_QUIRK2_BROKEN_64_BIT_DMA)) {
			host->cq_host->caps |= CMDQ_TASK_DESC_SZ_128;
		}
	}
}
#endif

/**
 * BUG: device rename krees old name, which would be realloced for other
 * device, pdev->name points to freed space, driver match may cause a panic
 * for wrong device
 */
static int sdhci_rename(struct platform_device *pdev)
{
	int ret = 0;
	static const char *const hi_mci0 = "hi_mci.0";
	struct sdhci_host *host = platform_get_drvdata(pdev);

	pdev->name = hi_mci0;
	ret = device_rename(&pdev->dev, hi_mci0);
	if (ret < 0) {
		dev_err(&pdev->dev, "dev set name hi_mci.0 fail \n");
		/* if failed, keep pdev->name same to dev.kobj.name */
		pdev->name = pdev->dev.kobj.name;
	}
	/* keep pdev->name pointing to dev.kobj.name */
	pdev->name = pdev->dev.kobj.name;
	host->hw_name = dev_name(&pdev->dev);

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_EMMC)
		set_bootdevice_name(&pdev->dev);
#endif

	return ret;
}

static int sdhci_mshc_probe(struct platform_device *pdev)
{
	int ret;
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_mshc_data *sdhci_mshc;

	if (get_bootdevice_type() != BOOT_DEVICE_EMMC) {
		pr_err("not boot from emmc\n");
		return -ENODEV;
	}

	sdhci_mshc = devm_kzalloc(&pdev->dev, sizeof(*sdhci_mshc), GFP_KERNEL);
	if (!sdhci_mshc)
		return -ENOMEM;

	host = sdhci_pltfm_init(pdev, &sdhci_mshc_pdata, sizeof(*sdhci_mshc));
	if (IS_ERR(host)) {
		ret = PTR_ERR(host);
		goto err_mshc_free;
	}

	pltfm_host = sdhci_priv(host);
	pltfm_host->priv = sdhci_mshc;

	ret = sdhci_mshc_get_resource(pdev, host);
	if (ret)
		goto err_pltfm_free;

	sdhci_get_of_property(pdev);
	sdhci_mshc_populate_dt(pdev);
	sdhci_mshc->enhanced_strobe_enabled = 0;
	sdhci_mshc->need_delay_measure = true;

	ret = sdhci_mshc_clock_init(pdev, sdhci_mshc);
	if (ret)
		goto err_pltfm_free;

	pltfm_host->clk = sdhci_mshc->clk;

	sdhci_mshc_i2c_setup(pdev);

	sdhci_mshc_hardware_reset(host);
	sdhci_mshc_hardware_disreset(host);

	sdhci_combo_phy_init(host);

#ifdef CONFIG_MMC_CQ_HCI
	sdhci_mshc_cmdq_init(pdev);
#endif

	if (host->mmc->pm_caps & MMC_PM_KEEP_POWER) {
		host->mmc->pm_flags |= MMC_PM_KEEP_POWER;
		host->quirks2 |= SDHCI_QUIRK2_HOST_OFF_CARD_ON;
	}

	/* import, ADMA support 64 or 32 bit address, here we use 32 bit. SDMA only support 32 bit mask. */
	if (!(host->quirks2 & SDHCI_QUIRK2_BROKEN_64_BIT_DMA)) {
		host->dma_mask = DMA_BIT_MASK(64); //lint !e598 !e648
	} else {
		host->dma_mask = DMA_BIT_MASK(32);
	}
	mmc_dev(host->mmc)->dma_mask = &host->dma_mask;

	ret = sdhci_rename(pdev);
	if (ret)
		goto clk_disable_all;

	ret = sdhci_add_host(host);
	if (ret)
		goto clk_disable_all;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 50);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, 1);

	pr_info("%s: done!\n", __func__);

	return 0;

clk_disable_all:
	clk_disable_unprepare(sdhci_mshc->clk);
err_pltfm_free:
	sdhci_pltfm_free(pdev);
err_mshc_free:
	devm_kfree(&pdev->dev, sdhci_mshc);

	pr_err("%s: error = %d!\n", __func__, ret);

	return ret;//lint !e593
}

static int sdhci_mshc_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_mshc_data *sdhci_mshc = pltfm_host->priv;

	pr_debug("%s:\n", __func__);

	pm_runtime_get_sync(&pdev->dev);
	sdhci_remove_host(host, 1);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	clk_disable_unprepare(sdhci_mshc->clk);

	sdhci_pltfm_free(pdev);

	return 0;
}

static const struct of_device_id sdhci_mshc_of_match[] = {
	{.compatible = "mshc,sdhci"},//lint !e785
	{} //lint !e785
};

MODULE_DEVICE_TABLE(of, sdhci_mshc_of_match);

static struct platform_driver sdhci_mshc_driver = {
	.driver = {
			   .name = "sdhci-mshc",
			   .of_match_table = sdhci_mshc_of_match,
			   .pm = &sdhci_mshc_dev_pm_ops,
			   }, //lint !e785
	.probe = sdhci_mshc_probe,
	.remove = sdhci_mshc_remove,
}; //lint !e785

module_platform_driver(sdhci_mshc_driver);

MODULE_DESCRIPTION("Driver for the Synopsys SDHCI Controller");
MODULE_AUTHOR("Yongheng Qin <qinyongheng@huawei.com>");
MODULE_LICENSE("GPL");
