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

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include "ufshcd.h"
#include "ufs-kirin.h"

/*lint -e785*/
static struct i2c_board_info ufs_i2c_board_info = {
    /* FIXME*/
    .type = "i2c_ufs",
    .addr = UFS_I2C_SLAVE,
};
/*lint +e785*/
int ufs_i2c_readl(struct ufs_hba *hba, u32 *value, u32 addr)
{
	int ret = -1;
	struct ufs_kirin_host *host = hba->priv;

	u32 temp = cpu_to_be32(addr);
	if (host->i2c_client) {
		ret = i2c_master_send(
			host->i2c_client, (char *)(&temp), (int)sizeof(u32));
		if (ret < 0)
			pr_err("%s ufs_i2c_write fail\n", __func__);
		ret = i2c_master_recv(host->i2c_client, (char *)(&temp),
				      (int)sizeof(u32));
		if (ret < 0)
			pr_err("%s ufs_i2c_readl fail\n", __func__);
	} else
		pr_err("%s ufs_i2c_readl fail client empty\n", __func__);
	*value = temp;
	return ret;
}

int ufs_i2c_writel(struct ufs_hba *hba, u32 val, u32 addr)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret = -1;
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
			pr_err("%s ufs_i2c_write fail\n", __func__);
	} else
		pr_err("%s ufs_i2c_write fail client empty\n", __func__);
	return ret;
}

int create_i2c_client(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	struct i2c_adapter *adapter;

	adapter = i2c_get_adapter(UFS_I2C_BUS);
	if (!adapter) {
		pr_err("%s i2c_get_adapter error\n", __func__);
		return -EIO;
	}
	host->i2c_client = i2c_new_device(adapter, &ufs_i2c_board_info);
	if (!host->i2c_client) {
		pr_err("%s i2c_new_device error\n", __func__);
		return -EIO;
	}
	return 0;
}

/* select the I2C 's CS singnal for HISI MPHY board in FPGA */
void i2c_chipsel_gpio_config(struct ufs_kirin_host *host, struct device *dev)
{
	int err = 0;

	host->chipsel_gpio = of_get_named_gpio(dev->of_node, "cs-gpios", 0);
	if (!gpio_is_valid(host->chipsel_gpio)) {
		pr_err("%s: gpio of host->chipsel_gpio is not valid,check DTS\n", __func__);
	}
	err = gpio_request((unsigned int)host->chipsel_gpio, "cs-gpio");
	if (err < 0) {
		pr_err("Can`t request cs chipsel gpio %d\n", host->chipsel_gpio);
	}
	err = gpio_direction_output((unsigned int)host->chipsel_gpio, 0);
	if (err < 0) {
		pr_err("%s: could not set gpio %d output push down\n", __func__, host->chipsel_gpio);
	}
}

/*lint -e648 -e845*/
void hisi_mphy_updata_temp_sqvref(struct ufs_hba *hba,
				struct ufs_kirin_host *host)
{
	/*in low temperature to solve the PLL'S oscill */
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL((u32)0x00c1, 0x0), 0x1); /*RG_PLL_CP*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL((u32)0x00d4, 0x0), 0x51); /*RG_PLL_DMY0*/
	/*rate A->B's VC0 stable time*/
	/* ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00db, 0x0),
		       0x5);*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00f0, 0x4), 0x1); /*RX enable lane 0*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00f0, 0x5), 0x1); /*RX enable lane 1*/
	/* H8's workaround*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00f1, 0x4), 0x7); /*RX_SQ_VERF, lane 0*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00f1, 0x5), 0x7); /*RX_SQ_VERF, lane 1*/
}

void hisi_mphy_updata_vswing_fsm_ocs5(struct ufs_hba *hba,
				struct ufs_kirin_host *host)
{
	uint32_t value = 0;
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00c2, 0x4), 0x1); /*RX_MC_PRESENT*/
	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL(0x00c2, 0x5), 0x1); /*RX_MC_PRESENT*/
	/*disable vSwing change*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00c7, 0x0),
		0x3); /*meaure the power, can close it*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00c8, 0x0),
		0x3); /*meaure the power, can close it*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x007a, 0x0), 0x1c);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007a, 0x1), 0x1c);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x007c, 0x0), 0xd4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007c, 0x1), 0xd4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x4), 0x2); /*RX_STALL*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x5), 0x2); /*RX_STALL*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x4), 0x2); /*RX_SLEEP*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x5), 0x2); /*RX_SLEEP*/

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cc, 0x4), 0x3); /*RX_HS_CLK_EN*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cc, 0x5), 0x3); /*RX_HS_CLK_EN*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x4), 0x3); /*RX_LS_CLK_EN*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x5), 0x3); /*RX_LS_CLK_EN*/
	/*enhance the accuracy of squelch detection*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ce, 0x4), 0x3); /*RX_H8_EXIT*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ce, 0x5), 0x3); /*RX_H8_EXIT*/

	/* try to solve the OCS=5 */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x4),
		0x20); /*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x5),
		0x20); /*RX_HS_DATA_VALID_TIMER_VAL0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x4),
		0x1); /*RX_HS_DATA_VALID_TIMER_VAL1*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x5),
		0x1); /*RX_HS_DATA_VALID_TIMER_VAL1*/

	/* set the HS-prepare length and sync length to MAX value, try
	* to solve the data check error problem,
	* the device seems not receive the write cmd. */
	/* PA_TxHsG1SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1552, 0x0), 0x4F);
	/* PA_TxHsG2SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1554, 0x0), 0x4F);
	/* PA_TxHsG3SyncLength , can not set MPHY's register directly */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1556, 0x0), 0x4F);

	/*enlarge TX_LS_PREPARE_LENGTH*/
	/*enable override*/
	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL((u32)0xd0f0, 0x0),
		&value); /* Unipro VS_mphy_disable */
	value |= (1 << 3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0f0, 0x0), value);
	/*Set to max value 0xf*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0f4, 0x0), 0xf);

	ufshcd_dme_set(
		hba, UIC_ARG_MIB_SEL((u32)0xd085, 0x0), 0x1); /* update */
}
/*lint -restore*/

void hisi_mphy_V200_updata(struct ufs_hba *hba, struct ufs_kirin_host *host)
{
	/*in low temperature to solve the PLL'S oscill */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x0009, 0x4), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x0009, 0x5), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00df, 0x0), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0023, 0x0), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0023, 0x1), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00a3, 0x4), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00a3, 0x5), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x4), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x5), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0003, 0x4), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0003, 0x5), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0004, 0x4), 0x64);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0004, 0x5), 0x64);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x4), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x5), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x4), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x5), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x4), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x5), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0059, 0x0), 0x0f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0059, 0x1), 0x0f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005a, 0x0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005a, 0x1), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005b, 0x0), 0x0f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005b, 0x1), 0x0f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005c, 0x0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005c, 0x1), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005d, 0x0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005d, 0x1), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005e, 0x0), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005e, 0x1), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005f, 0x0), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x005f, 0x1), 0x0a);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007a, 0x0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007a, 0x1), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007b, 0x0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007b, 0x1), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x4), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x5), 0x4);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f6, 0x4), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f6, 0x5), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c7, 0x0), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c8, 0x0), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c5, 0x0), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c6, 0x0), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x4), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x5), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x4), 0x10);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x5), 0x10);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f4, 0x4), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f4, 0x5), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f3, 0x4), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f3, 0x5), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f2, 0x4), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f2, 0x5), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f6, 0x4), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f6, 0x5), 0x2);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f5, 0x4), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f5, 0x5), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fc, 0x4), 0x1f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fc, 0x5), 0x1f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fd, 0x4), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fd, 0x5), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fb, 0x4), 0x5);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00fb, 0x5), 0x5);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0011, 0x4), 0x11);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0011, 0x5), 0x11);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd085, 0x0), 0x1);
	/* Trigger UniPro update */
	mdelay(40); /* wait 40ms */
}
/*lint -e845 -e648*/

void adapt_pll_to_power_mode(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	/*FIXME is need to distinguish FASTMODE FAST SLOW SLOWAUTO */
	if (host->caps & USE_HS_GEAR1) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x0),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x0),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x0),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x0),
				       0x2); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x0),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x0),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x0),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x0),
				       0x2); /*RG_PLL_RXHSGR*/
		}
	} else if (host->caps & USE_HS_GEAR2) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x4c),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x1); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x1); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x41),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x1); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x1); /*RG_PLL_RXHSGR*/
		}
	} else if (host->caps & USE_HS_GEAR3) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x4c),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x0); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x0); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x41),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x0); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x0); /*RG_PLL_RXHSGR*/
		}
	}
}
/*lint -save -e845*/

void deemphasis_config(struct ufs_kirin_host *host, struct ufs_hba *hba,
				struct ufs_pa_layer_attr *dev_req_params)
{
	uint32_t value = 0;
	if (host->caps & USE_HISI_MPHY_TC) {
			/*  de-emphasis level map
			5????bx0000: 0 dB
			5????bx0001: 0.72 dB
			5????bx0010: 1.45 dB
			5????bx0011: 2.18 dB
			5????bx0100: 2.92 dB
			5????bx0101: 3.67 dB
			5????bx0110: 4.44 dB
			5????bx0111: 5.22 dB
			5????bx1110: 6.02 dB
			5????bx1111: 6.85 dB
			*/
			/* the de-emphasis level you want to select, for
			* example ,
			* value = 0x5, it's 3.67 dB */

			if (host->caps & USE_HS_GEAR3) {
				value = 0x26; /*4.44 db*/
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007e, 0x0),
					0x5);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0025, 0x0),
					0x22);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007d, 0x0),
					0x22);
				if ((dev_req_params->lane_tx > 1) &&
					(dev_req_params->lane_rx > 1)) {
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007e, 0x1),
						0x5);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0025, 0x1),
						0x22);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007d, 0x1),
						0x22);
				}
			} else {
				value = 0x6f; /*6.85 db*/
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007e, 0x0),
					0x5);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0025, 0x0),
					0x15);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007d, 0x0),
					0x15);
				if ((dev_req_params->lane_tx > 1) &&
					(dev_req_params->lane_rx > 1)) {
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007e, 0x1),
						0x5);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0025, 0x1),
						0x15);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007d, 0x1),
						0x15);
				}
			}
			ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0037, 0x0),
					value);
			ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007b, 0x0),
					value);
			if ((dev_req_params->lane_tx > 1) &&
				(dev_req_params->lane_rx > 1)) {
				ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0037, 0x1),
						value);
				ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007b, 0x1),
						value);
			}
		}
}

/*lint -restore*/
