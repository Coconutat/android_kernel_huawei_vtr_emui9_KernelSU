/*
 * hi6402_hifi_config.c -- adapt 64xx hifi misc to 6402
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hisi/hi64xx_hifi_misc.h>
#include <linux/hisi/hi64xx/hi6402_dsp_regs.h>
#include "hi64xx_hifi_debug.h"
#include "hi6402_hifi_config.h"
#include "hi64xx_hifi_img_dl.h"
#include "../soundtrigger/soundtrigger_dma_drv.h"

static unsigned int hi6402_sc_fs_ctrls_h[] = {
	HI64xx_SC_FS_S1_CTRL_H,
	HI64xx_SC_FS_S2_CTRL_H,
	HI64xx_SC_FS_S3_CTRL_H,
	HI64xx_SC_FS_S4_CTRL_H,
	HI64xx_SC_FS_MISC_CTRL,
};

static void hi6402_hifi_autoclk_enable(bool enable)
{
	if (enable) {
		/* clr sc_mad_mic_bp only when mainpga selected mainmic. */
		if (hi64xx_hifi_read_reg(HI64xx_CODEC_MAINPGA_SEL)
			& (1 << HI64xx_CODEC_MAINPGA_SEL_BIT)) {
			/* mainpga selected hsmic. */
			hi64xx_hifi_write_reg(HI64xx_SC_MAD_CTRL0, 0x4d);
		} else {
			/* mainpga selected mainmic. */
			hi64xx_hifi_write_reg(HI64xx_SC_MAD_CTRL0, 0x45);
		}
	} else {
		hi64xx_hifi_write_reg(HI64xx_SC_MAD_CTRL0, 0x3e);
	}
}

static int hi6402_hifi_cfg_sw_mode(int val)
{
	u32 loopcount = 0;
	u32 state = 0xffff;

	while (state != val && loopcount < 1000) {
		udelay(60);
		state = hi64xx_hifi_read_reg(HI64xx_CFG_REG_CLK_STATUS);
		loopcount++;
	}

	HI64XX_DSP_INFO("%s cfg switch %s wait %d us\n", __FUNCTION__,
		val == HI6402_CFG_SW_ENTER ? "++" : "--", loopcount * 60);

	if (state != val) {
		return -1;
	} else {
		return 0;
	}
}

static void hi6402_hifi_apb_clk(bool enable)
{
	if (enable) {
		hi64xx_hifi_reg_clr_bit(HI6402_APB_CLK_CFG_REG, HI6402_APB_CLK_EN_BIT);
		hi64xx_hifi_reg_set_bit(HI6402_APB_CLK_CFG_REG, HI6402_PERI_CLK_SEL_BIT);
		hi64xx_hifi_reg_set_bit(HI6402_APB_CLK_CFG_REG, HI6402_APB_CLK_EN_BIT);
	} else {
		hi64xx_hifi_reg_clr_bit(HI6402_APB_CLK_CFG_REG, HI6402_APB_CLK_EN_BIT);
		hi64xx_hifi_reg_clr_bit(HI6402_APB_CLK_CFG_REG, HI6402_PERI_CLK_SEL_BIT);
		hi64xx_hifi_reg_set_bit(HI6402_APB_CLK_CFG_REG, HI6402_APB_CLK_EN_BIT);
	}
}

static void hi6402_hifi_mad_auto_clk(bool enable)
{
	if (enable) {
		HI64XX_DSP_INFO("set mad mode[enable] \n");

		/* sc_dsp_en lowpower 0:disable 1:enable */
		hi64xx_hifi_reg_set_bit(HI64xx_IRQ_SC_DSP_CTRL0, 0);
		/* sc_dsp_bp hifi clk enable 0:hardware ctl  1:software ctl */
		hi64xx_hifi_reg_clr_bit(HI64xx_IRQ_SC_DSP_CTRL0, 1);

		/* sc_mad_mode 0:disable 1:enable */
		hi64xx_hifi_reg_set_bit(HI64xx_IRQ_SC_DSP_CTRL0, 4);
		/* sc_dsp_runstall_bp 0:hardware ctl  1:software ctl */
		hi64xx_hifi_reg_clr_bit(HI64xx_IRQ_SC_DSP_CTRL0, 5);
		/* sc_dsp_hifi_div_bp:0:hardware 1:software */
		hi64xx_hifi_reg_clr_bit(HI64xx_IRQ_SC_DSP_CTRL0, 6);

	} else {
		HI64XX_DSP_INFO("set mad mode[disable] \n");

		/* open hifi clk */
		hi64xx_hifi_reg_clr_bit(HI64xx_IRQ_SC_DSP_CTRL0, 0);
		hi64xx_hifi_reg_set_bit(HI64xx_IRQ_SC_DSP_CTRL0, 1);
		hi64xx_hifi_reg_clr_bit(HI64xx_IRQ_SC_DSP_CTRL0, 4);
		hi64xx_hifi_reg_set_bit(HI64xx_IRQ_SC_DSP_CTRL0, 5);
		hi64xx_hifi_reg_set_bit(HI64xx_IRQ_SC_DSP_CTRL0, 6);

	}
}

static int hi6402_hifi_cfg_clk(bool enable)
{
	int ret = 0;

	/* request setup cfg clk switch */
	hi64xx_hifi_reg_set_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
				HI64xx_CFG_REG_CLK_SW_REQ_BIT);

	/* wait 6402 dsp enter cfg clk sel mode */
	ret = hi6402_hifi_cfg_sw_mode(HI6402_CFG_SW_ENTER);
	if(0 != ret) {
		HI64XX_DSP_ERROR("%s dsp state err\n",__FUNCTION__);
	}

	/* switch hi6402 cfg_clk */
	if(enable) {/* sw ->hifi */
		hi6402_hifi_apb_clk(enable);
		hi6402_hifi_mad_auto_clk(true);
		hi64xx_hifi_reg_clr_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
				HI64xx_CFG_REG_TMUX_CLKB_BP_BIT);
		hi64xx_hifi_reg_set_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
				HI64xx_CFG_REG_CLK_SEL_BIT);
	}else {/* sw->ap */
		hi64xx_hifi_reg_clr_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
				HI64xx_CFG_REG_CLK_SEL_BIT);
		hi64xx_hifi_reg_set_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
				HI64xx_CFG_REG_TMUX_CLKB_BP_BIT);
		hi6402_hifi_mad_auto_clk(false);
		hi6402_hifi_apb_clk(enable);
	}
	/* notify dsp cfg clk switch done */
	hi64xx_hifi_reg_clr_bit(HI64xx_CFG_REG_CLK_CTRL_REG,
			HI64xx_CFG_REG_CLK_SW_REQ_BIT);
	return ret;
}

static int hi6402_hifi_suspend(void)
{
	int ret = 0;

	hi6402_hifi_autoclk_enable(true);

	ret = hi6402_hifi_cfg_clk(true);
	if (ret)
		HI64XX_DSP_ERROR("6402 cfg clk switch to dsp err\n");

	return ret;
}

static int hi6402_hifi_resume(void)
{
	int ret = 0;

	ret = hi6402_hifi_cfg_clk(false);

	if (ret)
		HI64XX_DSP_ERROR("6402 cfg clk switch to ap err\n");

	hi6402_hifi_autoclk_enable(false);

	return ret;
}

static void hi6402_hifi_runstall_cfg(bool pull_down)
{
	IN_FUNCTION;

	if (pull_down) {
		/* Pull down runstall of HIFI*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);
	} else {
		/* Pull up runstall of HIFI*/
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);
	}
	OUT_FUNCTION;
}

static void hi6402_hifi_watchdog_enable(bool enable)
{
	IN_FUNCTION;
	if (enable) {
		/*bit3:wd_pclk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_APB_CLK_CFG, 3);
	} else {
		/*bit3:wd_pclk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);
	}
	OUT_FUNCTION;
}

static void hi6402_hifi_notify_dsp(void)
{
	unsigned int wait_cnt = 5;

	hi64xx_hifi_reg_set_bit(HI64xx_DSP_DSP_NMI, 2);

	while (wait_cnt) {
		if (0x0 == (hi64xx_hifi_read_reg(HI64xx_DSP_DSP_NMI) & 0x6))
			break;
		usleep_range((unsigned long)100, (unsigned long)110);
		wait_cnt--;
	}

	if (0 == wait_cnt)
		HI64XX_DSP_ERROR("dsp do not handle msg, DSP_NMI:0x%x\n",
			hi64xx_hifi_read_reg(HI64xx_DSP_DSP_NMI));
}


/* when hifi is not running, ap access APB register with 0x20007020 bit[2]=1'b0
 * when hifi is running, ap & hifi access APB register with 0x20007020 bit[2]=0'b0*/
static void hi6402_hifi_ram2axi_cfg(bool enable)
{
	unsigned int val;

	IN_FUNCTION;

	if (enable) {
		val = hi64xx_hifi_read_reg(HI64xx_DSP_RAM2AXI_CTRL);
		if(0 == (val & 0x1))
			HI64XX_DSP_ERROR("reg error! 0x20007020 = 0x%x\n", val);

		hi64xx_hifi_write_reg(HI64xx_DSP_RAM2AXI_CTRL, 0x1);
	} else {
		val = hi64xx_hifi_read_reg(HI64xx_DSP_RAM2AXI_CTRL);
		if(0 == (val & 0x1))
			HI64XX_DSP_ERROR("reg error! 0x20007020 = 0x%x\n", val);

		hi64xx_hifi_write_reg(HI64xx_DSP_RAM2AXI_CTRL, 0x5);
	}

	OUT_FUNCTION;
}

static void hi6402_hifi_clk_enable(bool enable)
{
	IN_FUNCTION;

	if (enable) {
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 3);
		/*bit6:hifi_div_clk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_CLK_CFG, 6);
	} else {
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 3);
		/*bit6:hifi_div_clk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_CLK_CFG, 6);
	}

	OUT_FUNCTION;
}

static void hi6402_hifi_init(void)
{
	IN_FUNCTION;

	/* 1.reset dsp_pd_srst_req */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 4);

	/* 2.close dspif clocks, and soft reset dspif */
	hi64xx_hifi_write_reg(HI6402_DSP_I2S_DSPIF_CLK_EN, 0x0);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_DAC_DP_CLK_EN_1, 4);

	/* 3.turn on dsp_top_mtcmos_ctrl*/
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 0);
	/* 4.enable dsp_top_iso_ctrl */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 1);

	/* 5.sc_dsp_en dsp low power enable */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 0);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 5);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 6);

	/* pull up runstall */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);

	/* 6.apb_pd_pclk_en Open APB clock of power-off area */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_APB_CLK_CFG, 6);

	/* 7.Release dsp_pd_srst_req */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SW_RST_REQ, 4);

	/* disable watchdog */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);

	/* sc_dsp_sft_clk_en */
	hi6402_hifi_clk_enable(true);

	OUT_FUNCTION;
}

static void hi6402_hifi_deinit(void)
{
	IN_FUNCTION;

	/* Close watchdog clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);

	/* Close HIFI clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 0);
	hi6402_hifi_clk_enable(false);

	/* Close APB clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 6);

	/* Close DSPIF clocks, and soft reset DSPIF */
	hi64xx_hifi_write_reg(HI6402_DSP_I2S_DSPIF_CLK_EN, 0x0);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_DAC_DP_CLK_EN_1, 4);

	/* Enable isolation cell */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 1);

	/* Soft reset HIFI */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 4);

	/* Turn off power of power-off area */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 0);

	/* Pull up runstall of HIFI */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);

	/* disable axi */
	hi64xx_hifi_write_reg(HI64xx_DSP_RAM2AXI_CTRL, 0x05);

	OUT_FUNCTION;
}

static void hi6402_soundtrigger_fasttrans_ctrl(bool enable, bool fm)
{
	IN_FUNCTION;

	if (enable) {
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_3, 0x66);/*S2_OL_PGA 192K*/
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_5, 0x11);/*S3_OL_PGA 16K*/

		if (fm) {
			hi64xx_hifi_write_reg(HI6402_DSP_S2_CTRL_L, 0x0C);/*S2_CTRL  48K*/
		} else {
			hi64xx_hifi_write_reg(HI6402_DSP_S2_CTRL_L, 0x0E);/*S2_CTRL  192K*/
		}

		hi64xx_hifi_write_reg(HI6402_DSP_S3_CTRL_L, 0x09);/*S3_CTRL*/
		hi64xx_hifi_write_reg(HI6402_SLIM_UP_EN, 0xFF);/*SLIMBUS INPUT ENABLE*/
	} else {
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_3, 0x00);/*S2_OL_PGA 192K*/
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_5, 0x00);/*S3_OL_PGA 16K*/

		if (fm) {
			hi64xx_hifi_write_reg(HI6402_DSP_S2_CTRL_L, 0x0C);/*S2_CTRL  48K*/
		} else {
			hi64xx_hifi_write_reg(HI6402_DSP_S2_CTRL_L, 0x04);/*S2_CTRL  disable*/
		}
		hi64xx_hifi_write_reg(HI6402_DSP_S3_CTRL_L, 0x00);
		hi64xx_hifi_write_reg(HI6402_SLIM_UP_EN, 0xAA);
	}

	OUT_FUNCTION;
}

static void hi6402_soundtrigger_fasttrans_ctrl_4smartpa(bool enable, bool fm)
{
	IN_FUNCTION;

	if (enable) {
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_5, 0x66);/*S3_OL_PGA 16K*/
		hi64xx_hifi_write_reg(HI6402_DSP_S3_CTRL_L, 0x0e);/*S3_CTRL*/
		hi64xx_hifi_write_reg(HI6402_SLIM_UP_EN, 0xFF);/*SLIMBUS INPUT ENABLE*/
	} else {
		hi64xx_hifi_write_reg(HI6402_SLIM_CTRL_5, 0x00);/*S3_OL_PGA 16K*/
		hi64xx_hifi_write_reg(HI6402_DSP_S3_CTRL_L, 0x00);
		hi64xx_hifi_write_reg(HI6402_SLIM_UP_EN, 0xAA);
	}

	OUT_FUNCTION;
}

/* dsp_if bypass config bit 6,7 */
static unsigned int hi6402_sc_src_lr_ctrls_m[] = {
	HI6402_SC_S1_SRC_LR_CTRL_M,
	HI6402_SC_S2_SRC_LR_CTRL_M,
	HI6402_SC_S3_SRC_LR_CTRL_M,
	HI6402_SC_S4_SRC_LR_CTRL_M,
	HI6402_SC_MISC_SRC_CTRL_H,
};

static void hi6402_dsp_if_set_bypass(unsigned int dsp_if_id, bool enable)
{
	unsigned int addr = 0;
	unsigned int bit = 0;

	unsigned int i2s_id = dsp_if_id / 2;
	unsigned int direct =
		(dsp_if_id & 0x1) ? HI64XX_HIFI_PCM_OUT : HI64XX_HIFI_PCM_IN;

	IN_FUNCTION;

	BUG_ON(i2s_id >= ARRAY_SIZE(hi6402_sc_src_lr_ctrls_m));

	bit = (direct == HI64XX_HIFI_PCM_IN) ? 6 : 7;
	if(dsp_if_id == HI64XX_HIFI_DSP_IF_PORT_8)
		bit = 4;
	addr = hi6402_sc_src_lr_ctrls_m[i2s_id];

	if (enable) {
		hi64xx_hifi_reg_set_bit(addr, bit);
	} else {
		hi64xx_hifi_reg_clr_bit(addr, bit);
	}

	OUT_FUNCTION;
}

static bool hi6402_check_dp_clk(void)
{
	unsigned int count = 1000;
	while(--count) {
		if(1 == hi64xx_hifi_read_reg(HI64xx_CODEC_DP_CLK_EN)) {
			return true;
		} else {
			usleep_range(100, 110);
		}
	}

	return false;
}

static int hi6402_dsp_if_set_sample_rate(unsigned int dsp_if_id,
						unsigned int sample_rate_in, unsigned int sample_rate_out)
{
	unsigned int addr = 0;
	unsigned char mask = 0;
	unsigned char sample_rate_index = 0;

	unsigned int i2s_id = dsp_if_id / 2;
	unsigned int direct =
		(dsp_if_id & 0x1) ? HI64XX_HIFI_PCM_OUT : HI64XX_HIFI_PCM_IN;

	IN_FUNCTION;

	WARN_ON(i2s_id >= ARRAY_SIZE(hi6402_sc_fs_ctrls_h));
	addr = hi6402_sc_fs_ctrls_h[i2s_id];

	if (!hi64xx_get_sample_rate_index(sample_rate_in, &sample_rate_index)) {
		HI64XX_DSP_ERROR("sample_rate_in is invalid %d!! \n", sample_rate_in);
		return 0;
	}

	if (HI64XX_HIFI_DSP_IF_PORT_8 != dsp_if_id) {
		mask = (direct == HI64XX_HIFI_PCM_IN) ? 0xf : 0xf0;
		sample_rate_index = (direct == HI64XX_HIFI_PCM_IN)
							? sample_rate_index : sample_rate_index << 4;
	} else {
		mask = (direct == HI64XX_HIFI_PCM_IN) ? 0x70 : 0xc;
		if (HI64XX_HIFI_PCM_OUT == direct) {
			if (HI64XX_HIFI_PCM_SAMPLE_RATE_48K > sample_rate_index) {
				HI64XX_DSP_ERROR("unsupport sample_rate_in %d!! \n", sample_rate_in);
				return 0;
			}
			sample_rate_index = sample_rate_index - HI64XX_HIFI_PCM_SAMPLE_RATE_48K;
		}
		sample_rate_index = (direct == HI64XX_HIFI_PCM_IN)
							? sample_rate_index << 4 : sample_rate_index << 2;
	}

	hi64xx_hifi_reg_write_bits(addr, sample_rate_index, mask);

	OUT_FUNCTION;

	return 0;
}

int hi6402_hifi_config_init(struct snd_soc_codec *codec,
				struct hi64xx_resmgr *resmgr,
				struct hi64xx_irq *irqmgr,
				enum bustype_select bus_sel)
{
	struct hi64xx_dsp_config dsp_config;
	struct hi64xx_hifi_img_dl_config dl_config;
	int ret = 0;

	if (!codec || !resmgr|| !irqmgr)
		return -EINVAL;

	HI64XX_DSP_INFO("%s++\n", __FUNCTION__);

	memset(&dsp_config, 0, sizeof(dsp_config));
	memset(&dl_config, 0, sizeof(dl_config));

	dsp_config.slimbus_load = false;
	dsp_config.codec_type = HI64XX_CODEC_TYPE_6402;
	dsp_config.msg_addr = HI6402_MSG_ADDR;
	dsp_config.para_addr = HI6402_PARA_ADDR;
	dsp_config.cmd0_addr = HI6402_DSP_CMD0;
	dsp_config.cmd1_addr = HI6402_DSP_CMD1;
	dsp_config.cmd2_addr = HI6402_DSP_CMD2;
	dsp_config.cmd3_addr = HI6402_DSP_CMD3;
	dsp_config.wtd_irq_num = IRQ_WTD;
	dsp_config.vld_irq_num = IRQ_CMD_VALID;
	dsp_config.dump_ocram_addr = HI6402_DUMP_OCRAM_ADDR;
	dsp_config.dump_ocram_size = HI6402_DUMP_OCRAM_SIZE;
	dsp_config.dump_log_addr = HI6402_DUMP_LOG_ADDR;
	dsp_config.dump_log_size = HI6402_DUMP_LOG_SIZE;
	dsp_config.msg_state_addr = HI6402_DSP_MSG_STATE_ADDR;
	dsp_config.wfi_state_addr = HI6402_DSP_WFI_STATE_ADDR;
	dsp_config.ocram_start_addr = HI6402_OCRAM_START_ADDR;
	dsp_config.ocram_size = HI6402_OCRAM_SIZE;
	dsp_config.itcm_start_addr = HI6402_ITCM_START_ADDR;
	dsp_config.itcm_size = HI6402_ITCM_SIZE;
	dsp_config.dtcm_start_addr = HI6402_DTCM_START_ADDR;
	dsp_config.dtcm_size = HI6402_DTCM_SIZE;
	dsp_config.bus_sel = bus_sel;
	dsp_config.mlib_to_ap_msg_addr = HI6402_MLIB_TO_AP_MSG_ADDR;
	dsp_config.mlib_to_ap_msg_size = HI6402_MLIB_TO_AP_MSG_SIZE;

	dsp_config.dsp_ops.init = hi6402_hifi_init;
	dsp_config.dsp_ops.deinit = hi6402_hifi_deinit;
	dsp_config.dsp_ops.clk_enable = hi6402_hifi_clk_enable;
	dsp_config.dsp_ops.ram2axi = hi6402_hifi_ram2axi_cfg;
	dsp_config.dsp_ops.runstall = hi6402_hifi_runstall_cfg;
	dsp_config.dsp_ops.wtd_enable = hi6402_hifi_watchdog_enable;
	dsp_config.dsp_ops.uart_enable = NULL;
	dsp_config.dsp_ops.notify_dsp = hi6402_hifi_notify_dsp;
	dsp_config.dsp_ops.suspend = hi6402_hifi_suspend;
	dsp_config.dsp_ops.resume = hi6402_hifi_resume;
	dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl = hi6402_soundtrigger_fasttrans_ctrl;
	dsp_config.dsp_ops.dsp_power_ctrl = NULL;
	dsp_config.dsp_ops.dsp_if_set_bypass = hi6402_dsp_if_set_bypass;
	dsp_config.dsp_ops.set_dsp_div = NULL;
	dsp_config.dsp_ops.check_dp_clk = hi6402_check_dp_clk;
	dsp_config.dsp_ops.check_i2s2_clk = NULL;
	dsp_config.dsp_ops.set_sample_rate = hi6402_dsp_if_set_sample_rate;
	dsp_config.dsp_ops.config_usb_low_power = NULL;

	if (codec->dev && of_property_read_bool(codec->dev->of_node, "hi6402-soundtrigger-if-4smartpa")) {
		HI64XX_DSP_INFO("%s: cust soundtrigger fasttrans ctrl for 4smartpa\n", __FUNCTION__);
		dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl = hi6402_soundtrigger_fasttrans_ctrl_4smartpa;
	}

	dl_config.dspif_clk_en_addr = HI6402_DSP_I2S_DSPIF_CLK_EN;

	ret = hi64xx_hifi_misc_init(codec, resmgr, irqmgr, &dsp_config);

	ret += hi64xx_soundtrigger_init(CODEC_HI6402);

	ret += hi64xx_hifi_img_dl_init(irqmgr, &dl_config);

	HI64XX_DSP_INFO("%s--\n", __FUNCTION__);

	return ret;
}
EXPORT_SYMBOL(hi6402_hifi_config_init);

void hi6402_hifi_config_deinit(void)
{
	hi64xx_hifi_misc_deinit();

	hi64xx_hifi_img_dl_deinit();
}
EXPORT_SYMBOL(hi6402_hifi_config_deinit);

MODULE_DESCRIPTION("hi64xx hifi misc driver");
MODULE_LICENSE("GPL");
