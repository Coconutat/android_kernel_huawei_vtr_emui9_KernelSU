/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include "hisi_dpe_utils.h"

DEFINE_SEMAPHORE(hisi_fb_dss_inner_clk_sem);

static int dss_inner_clk_refcount = 0;
static unsigned  int g_comform_value = 0;
static unsigned  int g_acm_State = 0;
static unsigned  int g_gmp_State = 0;
static unsigned int g_led_rg_csc_value[9];
static unsigned int g_is_led_rg_csc_set;
unsigned int g_led_rg_para1 = 7;
unsigned int g_led_rg_para2 = 30983;

#define OFFSET_FRACTIONAL_BITS	(11)
#define ROUND1(x,y)	((x) / (y) + ((x) % (y)  ? 1 : 0))

static uint32_t sbl_al_calib_lut[33] = {/*lint -save -e* */
	0x0000, 0x0800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800, 0x4000, 0x4800,
	0x5000, 0x5800, 0x6000, 0x6800, 0x7000, 0x7800, 0x8000, 0x87FF, 0x8FFF, 0x97FF,
	0x9FFF, 0xA7FF, 0xAFFF, 0xB7FF, 0xBFFF, 0xC7FF, 0xCFFF, 0xD7FF, 0xDFFF, 0xE7FF,
	0xEFFF, 0xF7FF, 0xFFFF
};/*lint -restore */

static uint32_t s_calc_al_change_lut[32] = {/*lint -save -e* */
	0x0000, 0x001E, 0x0046, 0x0078, 0x00C8, 0x012C, 0x0258, 0x03E8, 0x0640, 0x09C4,
	0x0FA0, 0x1B58, 0x32C8, 0x5DC0, 0x9C40, 0xFFFF, 0x0000, 0x000F, 0x0028, 0x003C,
	0x0064, 0x00DC, 0x01F4, 0x03E8, 0x04B0, 0x0578, 0x0AF0, 0x157c, 0x2AF8, 0x4E20,
	0x94C0,0xFFFF
};/*lint -restore */

static int get_lcd_frame_rate(struct hisi_panel_info *pinfo)
{
	return pinfo->pxl_clk_rate/(pinfo->xres + pinfo->pxl_clk_rate_div *
		(pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width))/(pinfo->yres +
		pinfo->ldi.v_back_porch + pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width);
}

struct dss_vote_cmd * get_dss_vote_cmd(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	int frame_rate = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return NULL;
	}

	pinfo = &(hisifd->panel_info);
	pdss_vote_cmd = &(hisifd->dss_vote_cmd);

	frame_rate = get_lcd_frame_rate(pinfo);

	/* FIXME: TBD  */
	if (g_fpga_flag == 1) {
		if (pdss_vote_cmd->dss_pclk_dss_rate == 0) {
			pdss_vote_cmd->dss_pri_clk_rate = 40 * 1000000UL;
			pdss_vote_cmd->dss_pclk_dss_rate = 20 * 1000000UL;
			pdss_vote_cmd->dss_pclk_pctrl_rate = 20 * 1000000UL;
		}
	} else {
		if (pdss_vote_cmd->dss_pclk_dss_rate == 0) {
			if ((pinfo->xres * pinfo->yres) >= (RES_4K_PHONE)) {
				pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_08V_RATE;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				hisifd->core_clk_upt_support = 0;
			} else if ((pinfo->xres * pinfo->yres) >= (RES_1440P)) {
				if (frame_rate >= 110) {
					pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_08V_RATE;
					pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
					pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
					hisifd->core_clk_upt_support = 0;
				} else {
					pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
					pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
					pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
					hisifd->core_clk_upt_support = 1;
				}
			} else if ((pinfo->xres * pinfo->yres) >= (RES_1080P)) {
				pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				hisifd->core_clk_upt_support = 1;
			} else {
				pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_07V_RATE;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				hisifd->core_clk_upt_support = 1;
			}
		}
	}

	return pdss_vote_cmd;
}

int set_dss_vote_cmd(struct hisi_fb_data_type *hisifd, dss_vote_cmd_t dss_vote_cmd)
{
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -1;
	}

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_DEBUG("fb%d is not support!\n", hisifd->index);
		return -1;
	}

	if ((dss_vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_08V_RATE)
		&& (dss_vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_07V_RATE)) {
		HISI_FB_ERR("no support set dss_pri_clk_rate(%llu)!\n", dss_vote_cmd.dss_pri_clk_rate);
		return -1;
	}

	if (dss_vote_cmd.dss_pri_clk_rate == hisifd->dss_vote_cmd.dss_pri_clk_rate) {
		return ret;
	}

	ret = clk_set_rate(hisifd->dss_pri_clk, dss_vote_cmd.dss_pri_clk_rate);
	if (ret < 0) {
		HISI_FB_ERR("set dss_pri_clk_rate(%llu) failed, error=%d!\n", dss_vote_cmd.dss_pri_clk_rate, ret);
		return -1;
	}

	hisifd->dss_vote_cmd.dss_pri_clk_rate = dss_vote_cmd.dss_pri_clk_rate;

	return ret;
}

#define PERI_VOLTAGE_LEVEL0_070V		(0) // 0.7v
#define PERI_VOLTAGE_LEVEL1_080V		(2) // 0.8v
int dpe_get_voltage_value(dss_vote_cmd_t *vote_cmd)
{
	if (!vote_cmd) {
		HISI_FB_ERR("vote_cmd is null\n");
		return -1;
	}
	switch (vote_cmd->dss_voltage_level) {
		case PERI_VOLTAGE_LEVEL0:
			return PERI_VOLTAGE_LEVEL0_070V; // 0.7v
		case PERI_VOLTAGE_LEVEL1:
			return PERI_VOLTAGE_LEVEL1_080V; // 0.8v
		default:
			HISI_FB_ERR("not support dss_voltage_level is %d\n", vote_cmd->dss_voltage_level);
			return -1;
	}
}

int dpe_get_voltage_level(int votage_value)
{
	switch (votage_value) {
		case PERI_VOLTAGE_LEVEL0_070V: // 0.7v
			return PERI_VOLTAGE_LEVEL0;
		case PERI_VOLTAGE_LEVEL1_080V: // 0.8v
			return PERI_VOLTAGE_LEVEL1;
		default:
			HISI_FB_ERR("not support votage_value is %d\n", votage_value);
			return PERI_VOLTAGE_LEVEL0;
	}
}

void dss_inner_clk_common_enable(struct hisi_fb_data_type *hisifd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;
	int prev_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	down(&hisi_fb_dss_inner_clk_sem);

	prev_refcount = dss_inner_clk_refcount++;
	if (!prev_refcount && !fastboot_enable) {
		//first DSS LP
		outp32(dss_base + GLB_MODULE_CLK_SEL, 0x00000000); //core clk, pclk
		outp32(dss_base + DSS_VBIF0_AIF + AIF_MODULE_CLK_SEL, 0x00000000);//axi  clk
		outp32(dss_base + DSS_VBIF1_AIF + AIF_MODULE_CLK_SEL, 0x00000000);//mmbuf  clk

		//second DSS LP  axi
		//cmd
		outp32(dss_base + DSS_CMDLIST_OFFSET + CMD_CLK_SEL, 0x00000000);
		//aif0
		outp32(dss_base + DSS_VBIF0_AIF + AIF_CLK_SEL0, 0x00000000);
		//aif0
		outp32(dss_base + DSS_VBIF0_AIF + AIF_CLK_SEL1, 0x00000000);
		//mmu
		outp32(dss_base + DSS_SMMU_OFFSET + SMMU_LP_CTRL, 0x00000001);
		//aif1
		outp32(dss_base + DSS_VBIF1_AIF + AIF_CLK_SEL0, 0x00000000);
		//aif1
		outp32(dss_base + DSS_VBIF1_AIF + AIF_CLK_SEL1, 0x00000000);

		//third DSS LP core
		//mif
		outp32(dss_base + DSS_MIF_OFFSET + MIF_CLK_CTL,  0x00000001);
		//mctl mutex0
		outp32(dss_base + DSS_MCTRL_CTL0_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl mutex1
		outp32(dss_base + DSS_MCTRL_CTL1_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl mutex2
		outp32(dss_base + DSS_MCTRL_CTL2_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl mutex3
		outp32(dss_base + DSS_MCTRL_CTL3_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl mutex4
		outp32(dss_base + DSS_MCTRL_CTL4_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl mutex5
		outp32(dss_base + DSS_MCTRL_CTL5_OFFSET + MCTL_CTL_CLK_SEL, 0x00000000);
		//mctl sys
		outp32(dss_base + DSS_MCTRL_SYS_OFFSET + MCTL_MCTL_CLK_SEL, 0x00000000);
		//mctl sys
		outp32(dss_base + DSS_MCTRL_SYS_OFFSET + MCTL_MOD_CLK_SEL, 0x00000000);
		//rch_v0
		outp32(dss_base + DSS_RCH_VG0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_v1
		outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_v2
		outp32(dss_base + DSS_RCH_VG2_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_g0
		outp32(dss_base + DSS_RCH_G0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_g1
		outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_d0
		outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_d1
		outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_d2
		outp32(dss_base + DSS_RCH_D2_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//rch_d3
		outp32(dss_base + DSS_RCH_D3_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//wch0
		outp32(dss_base + DSS_WCH0_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//wch1
		outp32(dss_base + DSS_WCH1_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//wch2
		outp32(dss_base + DSS_WCH2_DMA_OFFSET + CH_CLK_SEL, 0x00000000);
		//ov0
		outp32(dss_base + DSS_OVL0_OFFSET + OVL6_OV_CLK_SEL, 0x00000000);
		//ov2
		outp32(dss_base + DSS_OVL2_OFFSET + OVL6_OV_CLK_SEL, 0x00000000);
		//ov1
		outp32(dss_base + DSS_OVL1_OFFSET + OVL2_OV_CLK_SEL, 0x00000000);
		//0v3
		outp32(dss_base + DSS_OVL3_OFFSET + OVL2_OV_CLK_SEL, 0x00000000);
	}

	HISI_FB_DEBUG("fb%d, dss_inner_clk_refcount=%d\n",
		hisifd->index, dss_inner_clk_refcount);

	up(&hisi_fb_dss_inner_clk_sem);
}

void dss_inner_clk_common_disable(struct hisi_fb_data_type *hisifd)
{
	int new_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	down(&hisi_fb_dss_inner_clk_sem);
	new_refcount = --dss_inner_clk_refcount;
	if (new_refcount < 0) {
		HISI_FB_ERR("dss new_refcount err");
	}

	if (!new_refcount) {
		;
	}

	HISI_FB_DEBUG("fb%d, dss_inner_clk_refcount=%d\n",
		hisifd->index, dss_inner_clk_refcount);
	up(&hisi_fb_dss_inner_clk_sem);
}

void dss_inner_clk_pdp_enable(struct hisi_fb_data_type *hisifd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	if (fastboot_enable)
		return ;

	// itf0 clk
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_MODULE_CLK_SEL, 0x00000006);

	outp32(dss_base + DSS_DPP_OFFSET + DPP_CLK_SEL, 0x00001500);
	outp32(dss_base + DSS_IFBC_OFFSET + IFBC_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_DSC_OFFSET + DSC_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_LDI0_OFFSET + LDI_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_DBUF0_OFFSET + DBUF_CLK_SEL, 0x00000000);
}

void dss_inner_clk_pdp_disable(struct hisi_fb_data_type *hisifd)
{
}

void dss_inner_clk_sdp_enable(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dss_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	// itf1 clk
	outp32(dss_base + DSS_LDI1_OFFSET + LDI_MODULE_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_LDI1_OFFSET + LDI_CLK_SEL, 0x00000000);
	outp32(dss_base + DSS_DBUF1_OFFSET + DBUF_CLK_SEL, 0x00000000);
}

void dss_inner_clk_sdp_disable(struct hisi_fb_data_type *hisifd)
{
}

void init_dpp(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dpp_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	outp32(dpp_base + DPP_IMG_SIZE_BEF_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));
	outp32(dpp_base + DPP_IMG_SIZE_AFT_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));

}

void init_sbl(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *dpp_base = NULL;
	char __iomem *sbl_base = NULL;
	uint32_t tmp = 0;

	if (hisifd == NULL) {
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (pinfo == NULL) {
		return;
	}

	if (pinfo->sbl_support != 1) {
		return;
	}

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_SBL)) {
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
		sbl_base = hisifd->dss_base + DSS_DPP_SBL_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	//mode 1
	//enable cabc, backlight mode 1
	outp32(sbl_base + SBL_REG_CALC_CONTROL_0, 0x86);
	outp32(sbl_base + SBL_REG_CALC_CONTROL_1, 0x10);

	//enable ca
	outp32(sbl_base + SBL_REG_CORE1_VC_CONTROL_0, 0x03);

	//enable Iridix
	outp32(sbl_base + SBL_REG_CORE1_IRDX_CONTROL_0, 0x7);

	//res
	tmp = pinfo->xres;
	outp32(sbl_base + SBL_REG_FRMT_FRAME_WIDTH_7_TO_0, (tmp & 0xff));
	outp32(sbl_base + SBL_REG_FRMT_FRAME_WIDTH_15_TO_8, ((tmp >> 8) & 0xff));

	tmp = pinfo->yres;
	outp32(sbl_base + SBL_REG_FRMT_FRAME_HEIGHT_7_TO_0, (tmp & 0xff));
	outp32(sbl_base + SBL_REG_FRMT_FRAME_HEIGHT_15_TO_8, ((tmp >> 8) & 0xff));

	//ambient light scale
	outp32(sbl_base + SBL_REG_CALC_AL_SCALE_7_TO_0, 0xff);
	outp32(sbl_base + SBL_REG_CALC_AL_SCALE_15_TO_8, 0xff);

	//back light scale
	outp32(sbl_base + SBL_REG_CALC_BACKLIGHT_SCALE_7_TO_0, 0xff);
	outp32(sbl_base + SBL_REG_CALC_BACKLIGHT_SCALE_15_TO_8, 0xff);

	//input bl  scale
	outp32(sbl_base + SBL_REG_CALC_BL_PANEL_MAX_7_TO_0, 0xff);
	outp32(sbl_base + SBL_REG_CALC_BL_PANEL_MAX_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_SBC2_TF_ASYM, 0x01);

	outp32(sbl_base + SBL_REG_CALC_GAIN_AA_MANUAL_7_TO_0, 0x00);
	outp32(sbl_base + SBL_REG_CALC_GAIN_AA_MANUAL_11_TO_8, 0x06);

	outp32(sbl_base + SBL_REG_CALC_SBC1_TF_DEPTH_7_TO_0, 0x01);
	outp32(sbl_base + SBL_REG_CALC_SBC1_TF_DEPTH_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_SBC1_TF_STEP_7_TO_0, 0xff);
	outp32(sbl_base + SBL_REG_CALC_SBC1_TF_STEP_15_TO_8, 0xff);

	outp32(sbl_base + SBL_REG_CALC_SBC2_TF_DEPTH_7_TO_0, 0x20);
	outp32(sbl_base + SBL_REG_CALC_SBC2_TF_DEPTH_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_SBC2_TF_STEP_7_TO_0, 0xff);
	outp32(sbl_base + SBL_REG_CALC_SBC2_TF_STEP_15_TO_8, 0xff);

	//calibration
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_A_7_TO_0, 0x08);
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_A_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_B_7_TO_0, 0x5f);
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_B_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_C_7_TO_0, 0x03);
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_C_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_D_7_TO_0, 0x00);
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_D_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_E_7_TO_0, 0x07);
	outp32(sbl_base + SBL_REG_CALC_CALIBRATION_E_15_TO_8, 0x00);

	//strength limit
	outp32(sbl_base + SBL_REG_CALC_STRENGTH_LIMIT_7_TO_0, 0xc0);
	outp32(sbl_base + SBL_REG_CALC_STRENGTH_LIMIT_9_TO_8, 0x02);

	//AL temporal filter
	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_A_7_TO_0, 0x25);
	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_A_15_TO_8, 0x50);

	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_B_7_TO_0, 0x06);
	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_B_15_TO_8, 0x00);

	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_C_7_TO_0, 0x22);
	outp32(sbl_base + SBL_REG_CALC_AL_RTF_FILTER_C_15_TO_8, 0x02);

	outp32(sbl_base + SBL_REG_CORE1_DTHR_CONTROL, 0x00);

	outp32(sbl_base + SBL_REG_CORE1_IRDX_BRIGHTPR, 0x80);

	outp32(sbl_base + SBL_REG_CORE1_IRDX_CONTRAST, 0x90);

	outp32(sbl_base + SBL_REG_CORE1_IRDX_DARKENH_7_TO_0, 0x00);
	outp32(sbl_base + SBL_REG_CORE1_IRDX_DARKENH_15_TO_8, 0x30);

	outp32(sbl_base + SBL_REG_CORE1_LOGO_TOP, 0x0);
	outp32(sbl_base + SBL_REG_CORE1_LOGO_LEFT, 0x0);

	for(tmp = 0; tmp < 33; tmp++){
		outp32(sbl_base + SBL_CALC_AL_CALIB_LUT_ADDR_I, tmp);
		outp32(sbl_base + SBL_CALC_AL_CALIB_LUT_DATA_W_7_TO_0, (sbl_al_calib_lut[tmp] & 0xff));
		outp32(sbl_base + SBL_CALC_AL_CALIB_LUT_DATA_W_15_TO_8, ((sbl_al_calib_lut[tmp] >> 8) & 0xff));
	}
	for(tmp = 0; tmp < 32; tmp++){
		outp32(sbl_base + SBL_CALC_AL_CHANGE_LUT_ADDR_I, tmp);
		outp32(sbl_base + SBL_CALC_AL_CHANGE_LUT_DATA_W_7_TO_0, (s_calc_al_change_lut[tmp] & 0xff));
		outp32(sbl_base + SBL_CALC_AL_CHANGE_LUT_DATA_W_15_TO_8, ((s_calc_al_change_lut[tmp] >> 8) & 0xff));
	}

	outp32(dpp_base + DPP_SBL, 0x0);
}

static void init_dsc(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dsc_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct dsc_panel_info *dsc = NULL;

	uint32_t dsc_en = 0;
	uint32_t pic_width = 0;
	uint32_t pic_height = 0;
	uint32_t chunk_size = 0;
	uint32_t groups_per_line = 0;
	uint32_t rbs_min = 0;
	uint32_t hrd_delay = 0;
	uint32_t target_bpp_x16 =0;
	uint32_t num_extra_mux_bits = 0;
	uint32_t slice_bits = 0;
	uint32_t final_offset = 0;
	uint32_t final_scale = 0;
	uint32_t nfl_bpg_offset = 0;
	uint32_t groups_total = 0;
	uint32_t slice_bpg_offset = 0;
	uint32_t scale_increment_interval = 0;
	uint32_t initial_scale_value = 0;
	uint32_t scale_decrement_interval = 0;
	uint32_t adjustment_bits =0;
	uint32_t adj_bits_per_grp = 0;
	uint32_t bits_per_grp = 0;
	uint32_t slices_per_line = 0;
	uint32_t pic_line_grp_num = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);
	dsc = &(pinfo->vesa_dsc);

	dsc_base = hisifd->dss_base + DSS_DSC_OFFSET;

	if ((pinfo->ifbc_type == IFBC_TYPE_VESA2X_SINGLE) ||
		(pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE)) {
		// dual_dsc_en = 0, dsc_if_bypass = 1, reset_ich_per_line = 0
		dsc_en = 0x5;
		pic_width = DSS_WIDTH(pinfo->xres);
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VESA_CLK_SEL, 0);
	} else {
		// dual_dsc_en = 1, dsc_if_bypass = 0, reset_ich_per_line = 1
		dsc_en = 0xb;
		pic_width = DSS_WIDTH(pinfo->xres / 2);
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VESA_CLK_SEL, 1);
	}

	pic_height =  DSS_HEIGHT(pinfo->yres);
	chunk_size = ROUND1((dsc->slice_width + 1) * dsc->bits_per_pixel, 8);

	groups_per_line = (dsc->slice_width + 3) / 3;
	rbs_min = dsc->rc_model_size - dsc->initial_offset + dsc->initial_xmit_delay * dsc->bits_per_pixel +
		groups_per_line * dsc->first_line_bpg_offset;
	hrd_delay = ROUND1(rbs_min, dsc->bits_per_pixel);

	target_bpp_x16 = dsc->bits_per_pixel * 16;
	slice_bits = 8 * chunk_size * (dsc->slice_height + 1);

	num_extra_mux_bits = 3 * (dsc->mux_word_size + (4 * dsc->bits_per_component + 4) - 2);
	while ((num_extra_mux_bits > 0) && ((slice_bits - num_extra_mux_bits) % dsc->mux_word_size))
		num_extra_mux_bits--;

	final_offset =  dsc->rc_model_size - ((dsc->initial_xmit_delay * target_bpp_x16 + 8) >> 4) + num_extra_mux_bits; //4336(0x10f0)
	final_scale = 8 * dsc->rc_model_size / (dsc->rc_model_size - final_offset);
	nfl_bpg_offset = ROUND1(dsc->first_line_bpg_offset << OFFSET_FRACTIONAL_BITS, dsc->slice_height); //793(0x319)
	groups_total = groups_per_line * (dsc->slice_height + 1);
	slice_bpg_offset = ROUND1((1 << OFFSET_FRACTIONAL_BITS) *
		(dsc->rc_model_size - dsc->initial_offset + num_extra_mux_bits), groups_total); // 611(0x263)
	scale_increment_interval = (1 << OFFSET_FRACTIONAL_BITS) * final_offset /
		((final_scale - 9) * (nfl_bpg_offset + slice_bpg_offset)); // 903(0x387)

	initial_scale_value = 8 * dsc->rc_model_size / (dsc->rc_model_size - dsc->initial_offset);
	if (groups_per_line < initial_scale_value - 8)	{
		initial_scale_value = groups_per_line + 8;
	}

	if (initial_scale_value > 8) {
		scale_decrement_interval = groups_per_line / (initial_scale_value - 8);
	} else {
		scale_decrement_interval = 4095;
	}

	adjustment_bits = (8 - (dsc->bits_per_pixel * (dsc->slice_width + 1)) % 8) % 8;
	adj_bits_per_grp = dsc->bits_per_pixel * 3 - 3;
	bits_per_grp = dsc->bits_per_pixel * 3;
	slices_per_line = (pic_width > dsc->slice_width) ? 1 : 0;
	pic_line_grp_num = ((dsc->slice_width + 3)/3)*(slices_per_line+1)-1;

	set_reg(dsc_base + DSC_REG_DEFAULT, 0x1, 1, 0);

	// dsc_en
	set_reg(dsc_base + DSC_EN, dsc_en, 4, 0);

	// bits_per_component, convert_rgb, bits_per_pixel
	set_reg(dsc_base + DSC_CTRL, dsc->bits_per_component | (dsc->linebuf_depth << 4) | (dsc->block_pred_enable << 10) |
		(0x1 << 11) | (dsc->bits_per_pixel << 16), 26, 0);

	// pic_width, pic_height
	set_reg(dsc_base + DSC_PIC_SIZE, (pic_width << 16) | pic_height, 32, 0);

	// slice_width, slice_height
	set_reg(dsc_base + DSC_SLICE_SIZE, (dsc->slice_width << 16) | dsc->slice_height, 32, 0);

	// chunk_size
	set_reg(dsc_base + DSC_CHUNK_SIZE, chunk_size, 16, 0);

	// initial_xmit_delay, initial_dec_delay = hrd_delay -initial_xmit_delay
	set_reg(dsc_base + DSC_INITIAL_DELAY, dsc->initial_xmit_delay |
		((hrd_delay - dsc->initial_xmit_delay) << 16), 32, 0);

	// initial_scale_value, scale_increment_interval
	set_reg(dsc_base + DSC_RC_PARAM0, initial_scale_value | (scale_increment_interval << 16), 32, 0);

	// scale_decrement_interval, first_line_bpg_offset
	set_reg(dsc_base + DSC_RC_PARAM1, (dsc->first_line_bpg_offset << 16) | scale_decrement_interval, 21, 0);

	// nfl_bpg_offset, slice_bpg_offset
	set_reg(dsc_base + DSC_RC_PARAM2, nfl_bpg_offset | (slice_bpg_offset << 16), 32, 0);

	//DSC_RC_PARAM3
	set_reg(dsc_base + DSC_RC_PARAM3,
		((final_offset << 16) | dsc->initial_offset), 32, 0);

	//DSC_FLATNESS_QP_TH
	set_reg(dsc_base + DSC_FLATNESS_QP_TH,
		((dsc->flatness_max_qp << 16) | (dsc->flatness_min_qp << 0)), 24, 0);

	//DSC_RC_PARAM4
	set_reg(dsc_base + DSC_RC_PARAM4,
		((dsc->rc_edge_factor << 20) | (dsc->rc_model_size << 0)), 24, 0);
	//DSC_RC_PARAM5
	set_reg(dsc_base + DSC_RC_PARAM5,
		((dsc->rc_tgt_offset_lo << 20) |(dsc->rc_tgt_offset_hi << 16) |
		(dsc->rc_quant_incr_limit1 << 8) |(dsc->rc_quant_incr_limit0 << 0)), 24, 0);

	//DSC_RC_BUF_THRESH
	set_reg(dsc_base + DSC_RC_BUF_THRESH0,
		((dsc->rc_buf_thresh0 << 24) | (dsc->rc_buf_thresh1 << 16) |
		(dsc->rc_buf_thresh2 << 8) | (dsc->rc_buf_thresh3 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_BUF_THRESH1,
		((dsc->rc_buf_thresh4 << 24) | (dsc->rc_buf_thresh5 << 16) |
		(dsc->rc_buf_thresh6 << 8) | (dsc->rc_buf_thresh7 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_BUF_THRESH2,
		((dsc->rc_buf_thresh8 << 24) | (dsc->rc_buf_thresh9 << 16) |
		(dsc->rc_buf_thresh10 << 8) | (dsc->rc_buf_thresh11 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_BUF_THRESH3,
		((dsc->rc_buf_thresh12 << 24) | (dsc->rc_buf_thresh13 << 16)), 32, 0);

	//DSC_RC_RANGE_PARAM
	set_reg(dsc_base + DSC_RC_RANGE_PARAM0,
		((dsc->range_min_qp0 << 27) | (dsc->range_max_qp0 << 22) |
		(dsc->range_bpg_offset0 << 16) | (dsc->range_min_qp1 << 11) |
		(dsc->range_max_qp1 << 6) | (dsc->range_bpg_offset1 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM1,
		((dsc->range_min_qp2 << 27) | (dsc->range_max_qp2 << 22) |
		(dsc->range_bpg_offset2 << 16) | (dsc->range_min_qp3 << 11) |
		(dsc->range_max_qp3 << 6) | (dsc->range_bpg_offset3 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM2,
		((dsc->range_min_qp4 << 27) | (dsc->range_max_qp4 << 22) |
		(dsc->range_bpg_offset4 << 16) | (dsc->range_min_qp5 << 11) |
		(dsc->range_max_qp5 << 6) | (dsc->range_bpg_offset5 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM3,
		((dsc->range_min_qp6 << 27) | (dsc->range_max_qp6 << 22) |
		(dsc->range_bpg_offset6 << 16) | (dsc->range_min_qp7 << 11) |
		(dsc->range_max_qp7 << 6) | (dsc->range_bpg_offset7 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM4,
		((dsc->range_min_qp8 << 27) | (dsc->range_max_qp8 << 22) |
		(dsc->range_bpg_offset8 << 16) | (dsc->range_min_qp9 << 11) |
		(dsc->range_max_qp9 << 6) | (dsc->range_bpg_offset9 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM5,
		((dsc->range_min_qp10 << 27) | (dsc->range_max_qp10 << 22) |
		(dsc->range_bpg_offset10 << 16) | (dsc->range_min_qp11 << 11) |
		(dsc->range_max_qp11 << 6) | (dsc->range_bpg_offset11 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM6,
		((dsc->range_min_qp12 << 27) | (dsc->range_max_qp12 << 22) |
		(dsc->range_bpg_offset12 << 16) | (dsc->range_min_qp13 << 11) |
		(dsc->range_max_qp13 << 6) | (dsc->range_bpg_offset13 << 0)), 32, 0);
	set_reg(dsc_base + DSC_RC_RANGE_PARAM7,
		((dsc->range_min_qp14 << 27) | (dsc->range_max_qp14 << 22) |
		(dsc->range_bpg_offset14 << 16)), 32, 0);

	// adjustment_bits
	set_reg(dsc_base + DSC_ADJUSTMENT_BITS, adjustment_bits, 4, 0);

	// bits_per_grp, adj_bits_per_grp
	set_reg(dsc_base + DSC_BITS_PER_GRP, bits_per_grp | (adj_bits_per_grp << 8), 14, 0);

	//slices_per_line, pic_line_grp_num
	set_reg(dsc_base + DSC_MULTI_SLICE_CTL, slices_per_line |
		(pic_line_grp_num << 16), 32, 0);

	//dsc_out_mode
	if ((chunk_size % 3 == 0)) {
		set_reg(dsc_base + DSC_OUT_CTRL, 0x0, 1, 0);
	} else if ((chunk_size % 2 == 0)) {
		set_reg(dsc_base + DSC_OUT_CTRL, 0x1, 1, 0);
	} else {
		HISI_FB_ERR("fb%d, chunk_size should be mode by 3 or 2, but chunk_size = %u\n",
			hisifd->index, chunk_size);
		return;
	}

	set_reg(dsc_base + DSC_CLK_SEL, 0x0, 32, 0);
	set_reg(dsc_base + DSC_CLK_EN, 0x7, 32, 0);
	set_reg(dsc_base + DSC_MEM_CTRL, 0x0, 32, 0);
	set_reg(dsc_base + DSC_ST_DATAIN, 0x0, 28, 0);
	set_reg(dsc_base + DSC_ST_DATAOUT, 0x0, 16, 0);
	set_reg(dsc_base + DSC0_ST_SLC_POS, 0x0, 28, 0);
	set_reg(dsc_base + DSC1_ST_SLC_POS, 0x0, 28, 0);
	set_reg(dsc_base + DSC0_ST_PIC_POS, 0x0, 28, 0);
	set_reg(dsc_base + DSC1_ST_PIC_POS, 0x0, 28, 0);
	set_reg(dsc_base + DSC0_ST_FIFO, 0x0, 14, 0);
	set_reg(dsc_base + DSC1_ST_FIFO, 0x0, 14, 0);
	set_reg(dsc_base + DSC0_ST_LINEBUF, 0x0, 24, 0);
	set_reg(dsc_base + DSC1_ST_LINEBUF, 0x0, 24, 0);
	set_reg(dsc_base + DSC_ST_ITFC, 0x0, 10, 0);
	set_reg(dsc_base + DSC_RD_SHADOW_SEL, 0x1, 1, 0);
	set_reg(dsc_base + DSC_REG_DEFAULT, 0x0, 1, 0);
}

void init_ifbc(struct hisi_fb_data_type *hisifd)
{
	char __iomem *ifbc_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	uint32_t mipi_idx = 0;
	uint32_t comp_mode = 0;

	uint32_t ifbc_out_mode = 0;
	uint32_t dpk_mode_sel = 0;
	uint32_t dup_mode_sel = 0;
	uint32_t porch_num = 0;
	uint32_t insert_byte_num = 0;
	uint32_t insert_byte = 0;
	uint32_t num_pad = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);
	if ((pinfo->ifbc_type < IFBC_TYPE_NONE) || (pinfo->ifbc_type >= IFBC_TYPE_MAX)) {
		HISI_FB_ERR("pinfo->ifbc_type is invalid");
		return;
	}

	/* VESA_CLK_SEL is set to 0 for initial, 1 is needed only by vesa dual pipe compress */
	set_reg(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_VESA_CLK_SEL, 0, 1, 0);

	if (pinfo->ifbc_type == IFBC_TYPE_NONE)
		return ;

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_IFBC))
		return;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ifbc_base = hisifd->dss_base + DSS_IFBC_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	mipi_idx = is_dual_mipi_panel(hisifd) ? 1 : 0;
	comp_mode = g_mipi_ifbc_division[mipi_idx][pinfo->ifbc_type].comp_mode;

	if (is_ifbc_vesa_panel(hisifd)) {
		init_dsc(hisifd);

		// select comp_mode
		set_reg(ifbc_base + IFBC_CTRL, comp_mode, 3, 0);
		return ;
	}

	// git ifbc_out_mode
	if (pinfo->bpp == LCD_RGB888)
		ifbc_out_mode = 1;
	else if (pinfo->bpp == LCD_RGB565)
		ifbc_out_mode = 0;

	if (((pinfo->ifbc_type == IFBC_TYPE_ORISE2X) && (pinfo->ifbc_cmp_dat_rev0 == 1)) ||
		((pinfo->ifbc_type == IFBC_TYPE_RSP3X) &&
		(pinfo->type != PANEL_MIPI_VIDEO) &&
		(pinfo->xres % 3 != 0)))
		if (pinfo->ifbc_auto_sel != 0) {
			HISI_FB_ERR("fb%d, auto_sel = %u not support!", hisifd->index, pinfo->ifbc_auto_sel);
			return ;
		}

	if (pinfo->ifbc_type == IFBC_TYPE_ORISE2X) {
		if ((pinfo->xres % 2 != 0) &&
			(pinfo->yres % 2 != 0)) {
			HISI_FB_ERR("fb%d, IFBC_ORISE2X not support (xres = %u, yres = %u)!",
				hisifd->index, pinfo->xres, pinfo->yres);
			return;
		}

		dpk_mode_sel = 1;
		dup_mode_sel = 2;
		porch_num = 0;
	} else if (pinfo->ifbc_type == IFBC_TYPE_ORISE3X) {
		dpk_mode_sel = 0;
		dup_mode_sel = 3;
		porch_num = 5;
	} else if (pinfo->ifbc_type == IFBC_TYPE_HIMAX2X) {
		if ((pinfo->xres % 2 != 0) &&
			(pinfo->yres % 2 != 0)) {
			HISI_FB_ERR("fb%d, IFBC_HIMAX2X not support (xres = %u, yres = %u)!",
				hisifd->index, pinfo->xres, pinfo->yres);
			return;
		}

		dpk_mode_sel = 1;
		dup_mode_sel = 2;
		porch_num = 0;
	} else if (pinfo->ifbc_type == IFBC_TYPE_RSP2X) {
		dpk_mode_sel = 1;
		dup_mode_sel = 0;
		porch_num = 0;

		if ((pinfo->type == PANEL_MIPI_CMD) ||
			(pinfo->type == PANEL_DUAL_MIPI_CMD))
			num_pad = (4 - pinfo->xres % 4) % 4;
		else
			num_pad = 0;
	} else if (pinfo->ifbc_type == IFBC_TYPE_RSP3X) {
		if ((pinfo->yres % 2 != 0) || (pinfo->yres < 8)) {
			HISI_FB_ERR("fb%d, IFBC_RSP3X not support (xres = %u, yres = %u)!",
				hisifd->index, pinfo->xres, pinfo->yres);
			return;
		}

		dpk_mode_sel = 2;
		dup_mode_sel = 1;
		porch_num = 0;

		insert_byte = DSS_WIDTH(pinfo->xres) / (mipi_idx + 1) + 1;

		if ((pinfo->type == PANEL_MIPI_VIDEO) ||
			(pinfo->type == PANEL_DUAL_MIPI_VIDEO)) {
			insert_byte_num = (3 - insert_byte % 3) % 3;
			num_pad = (8 - pinfo->xres % 8) % 8;
		} else if ((pinfo->type == PANEL_MIPI_CMD) ||
			(pinfo->type == PANEL_DUAL_MIPI_CMD)) {
			insert_byte_num = 0;
			num_pad = (4 - pinfo->xres % 4) % 4;
		}
	}

	// reg_default
	//set_reg(ifbc_base + IFBC_REG_DEFAULT, 0x1, 1, 0);

	// ifbc_rd_shadow = 1
	//set_reg(ifbc_base + IFBC_RD_SHADOW, 0x1, 1, 0);

	// set ifbc_size
	set_reg(ifbc_base + IFBC_SIZE,
		((DSS_WIDTH(pinfo->xres) << 16) | DSS_HEIGHT(pinfo->yres)), 32, 0);

	// set IFBC_CTRL
	set_reg(ifbc_base + IFBC_CTRL, comp_mode, 3, 0);
	set_reg(ifbc_base + IFBC_CTRL, ifbc_out_mode, 1, 3);
	set_reg(ifbc_base + IFBC_CTRL, pinfo->ifbc_cmp_dat_rev0, 1, 4);
	set_reg(ifbc_base + IFBC_CTRL, pinfo->ifbc_cmp_dat_rev1, 1, 5);
	set_reg(ifbc_base + IFBC_CTRL, pinfo->ifbc_auto_sel, 1, 6);

	if (pinfo->ifbc_auto_sel == 0) {
		set_reg(ifbc_base + IFBC_CTRL, dpk_mode_sel, 3, 7);
		set_reg(ifbc_base + IFBC_CTRL, dup_mode_sel, 3, 10);
		set_reg(ifbc_base + IFBC_CTRL, porch_num, 8, 13);

		// set insert_byte_num, insert_byte
		set_reg(ifbc_base + IFBC_INSERT, insert_byte_num | (insert_byte << 16), 32, 0);

		// set num_pad
		set_reg(ifbc_base + IFBC_PAD, num_pad, 3, 0);
	}

	//orise3x initialization
	if (pinfo->ifbc_type == IFBC_TYPE_ORISE3X) {
		if (pinfo->ifbc_orise_ctr == 1) {
			set_reg(ifbc_base + IFBC_CORE_GT, 0x0, 2, 0);

			if (pinfo->ifbc_orise_ctl == IFBC_ORISE_CTL_8LINE)
				set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0208, 32, 0);
			else if (pinfo->ifbc_orise_ctl == IFBC_ORISE_CTL_16LINE)
				set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0210, 32, 0);
			else if (pinfo->ifbc_orise_ctl == IFBC_ORISE_CTL_32LINE)
				set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0220, 32, 0);
			else
				set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0200, 32, 0);

			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0300, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0419, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0500, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x063f, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0700, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0801, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0900, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0a64, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0b00, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0c5c, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0d00, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0e01, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x0f00, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x10a0, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x1100, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x125f, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x1300, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x14a0, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x1500, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x16ff, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x1700, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x200c, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x2100, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4000, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4100, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4200, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4300, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4800, 32, 0);
			set_reg(ifbc_base + IFBC_ORISE_CTL, 0x4900, 32, 0);
		} else {
			set_reg(ifbc_base + IFBC_CORE_GT, 0x2, 2, 0);
		}
	}

	// set ifbc_clk_sel
	set_reg(ifbc_base + IFBC_CLK_SEL, 0x0, 32, 0);

	// ifbc_rd_shadow = 1
	//set_reg(ifbc_base + IFBC_RD_SHADOW, 0x0, 1, 0);

	//enable ifbc
	set_reg(ifbc_base + IFBC_EN, 0x3, 2, 0);
}

/*lint -e438 -e550*/
void init_post_scf(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dpp_base = NULL;
	char __iomem *scf_lut_base = NULL;
	char __iomem *scf_base;
	int ihright;
	int ihright1;
	int ivbottom;

	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	pinfo = &(hisifd->panel_info);

	dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
	scf_lut_base = hisifd->dss_base + DSS_POST_SCF_LUT_OFFSET;

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF)) {
		return;
	}

	// ARSR1P memory shutdown
	//outp32(dpp_base + DPP_ARSR1P_MEM_CTRL, 0X4);

	pinfo->post_scf_support = 1;
	hisi_dss_post_scl_load_filter_coef(hisifd, false, scf_lut_base, SCL_COEF_RGB_IDX);

	scf_base = hisifd->dss_base + DSS_POST_SCF_OFFSET;

	outp32(scf_base + ARSR1P_SKIN_THRES_Y, 512<<16 | 83<<8 | 75);
	outp32(scf_base + ARSR1P_SKIN_THRES_U, 819<<16 | 10<<8 | 5);
	outp32(scf_base + ARSR1P_SKIN_THRES_V, 682<<16 | 12<<8 | 6);
	outp32(scf_base + ARSR1P_SKIN_EXPECTED, 145<<16 | 113<<8 | 150);
	outp32(scf_base + ARSR1P_SKIN_CFG, 3<<16 | 10<<8 | 6);

	outp32(scf_base + ARSR1P_SHOOT_CFG1, 2<<24 | 20);
	outp32(scf_base + ARSR1P_SHOOT_CFG2, 512<<18 | 0<<9 | (-16 & 0x1ff));

	outp32(scf_base + ARSR1P_SHARP_CFG1, 64<<24 | 48<<16 | 7<<8 | 3);
	outp32(scf_base + ARSR1P_SHARP_CFG2, 64<<24 | 48<<16 | 7<<8 | 3);
	outp32(scf_base + ARSR1P_SHARP_CFG3, 40<<16 | 0);
	outp32(scf_base + ARSR1P_SHARP_CFG4, 0<<16 | 0);
	outp32(scf_base + ARSR1P_SHARP_CFG5, 80<<16 | 0);
	outp32(scf_base + ARSR1P_SHARP_CFG6, 40<<24 | 24<<16 | 16<<8 | 6);

	outp32(scf_base + ARSR1P_SHARP_CFG7, 1<<24 | 80<<16 | 1<<9 | 0<<8 | 16);
	outp32(scf_base + ARSR1P_SHARP_CFG8, 4<<21 | 640);
	outp32(scf_base + ARSR1P_SHARP_CFG9, 1<<24 | 5120);
	outp32(scf_base + ARSR1P_SHARP_CFG10, 0);
	outp32(scf_base + ARSR1P_SHARP_CFG11, 0);

	outp32(scf_base + ARSR1P_DIFF_CTRL, 20<<8 | 15);

	outp32(scf_base + ARSR1P_LSC_CFG1, 15<<26 | 780<<13 | 1040);
	outp32(scf_base + ARSR1P_LSC_CFG2, 0);
	outp32(scf_base + ARSR1P_LSC_CFG3, 128<<16 | 1536);


	ihright1 = ((int)pinfo->xres - 1) * ARSR1P_INC_FACTOR;
	ihright = ihright1 + 2 * ARSR1P_INC_FACTOR;
	if (ihright >= ((int)pinfo->xres) * ARSR1P_INC_FACTOR){
	    ihright = ((int)pinfo->xres) * ARSR1P_INC_FACTOR - 1;
	}

	ivbottom = ((int)pinfo->yres - 1) * ARSR1P_INC_FACTOR;
	if (ivbottom >= ((int)pinfo->yres) * ARSR1P_INC_FACTOR){
	    ivbottom = ((int)pinfo->yres) * ARSR1P_INC_FACTOR - 1;
	}

	outp32(scf_base + ARSR1P_IHLEFT, 0x0);
	outp32(scf_base + ARSR1P_IHRIGHT, ihright);
	outp32(scf_base + ARSR1P_IHLEFT1, 0x0);
	outp32(scf_base + ARSR1P_IHRIGHT1, ihright1);
	outp32(scf_base + ARSR1P_IVTOP, 0x0);
	outp32(scf_base + ARSR1P_IVBOTTOM, ivbottom);
	outp32(scf_base + ARSR1P_IHINC, ARSR1P_INC_FACTOR);
	outp32(scf_base + ARSR1P_IVINC, ARSR1P_INC_FACTOR);

	if (pinfo->arsr1p_sharpness_support) {
	    outp32(scf_base + ARSR1P_MODE, 0x6e);
	} else {
	    outp32(scf_base + ARSR1P_MODE, 0x1);
	}

	return;
}
/*lint +e438 +e550*/

void init_dbuf(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dbuf_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int sram_valid_num = 0;
	int sram_max_mem_depth = 0;
	int sram_min_support_depth = 0;

	uint32_t thd_rqos_in = 0;
	uint32_t thd_rqos_out = 0;
	uint32_t thd_wqos_in = 0;
	uint32_t thd_wqos_out = 0;
	uint32_t thd_cg_in = 0;
	uint32_t thd_cg_out = 0;
	uint32_t thd_wr_wait = 0;
	uint32_t thd_cg_hold = 0;
	uint32_t thd_flux_req_befdfs_in = 0;
	uint32_t thd_flux_req_befdfs_out = 0;
	uint32_t thd_flux_req_aftdfs_in = 0;
	uint32_t thd_flux_req_aftdfs_out = 0;
	uint32_t thd_dfs_ok = 0;
	uint32_t dfs_ok_mask = 0;
	uint32_t thd_flux_req_sw_en = 1;

	int dfs_time = 0;
	int dfs_time_min = 0;
	int depth = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		dbuf_base = hisifd->dss_base + DSS_DBUF0_OFFSET;
		if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_DBUF)) {
			return;
		}

		if (pinfo->xres * pinfo->yres >= RES_4K_PHONE) {
			dfs_time_min = DFS_TIME_MIN_4K;
		} else {
			dfs_time_min = DFS_TIME_MIN;
		}

		dfs_time = DFS_TIME;
		depth = DBUF0_DEPTH;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		dbuf_base = hisifd->dss_base + DSS_DBUF1_OFFSET;

		dfs_time = DFS_TIME;
		dfs_time_min = DFS_TIME_MIN;
		depth = DBUF1_DEPTH;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	/*
	** int K = 0;
	** int Tp = 1000000  / pinfo->pxl_clk_rate;
	** K = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch + pinfo->xres +
	**	pinfo->ldi.h_front_porch) / pinfo->xres;
	** thd_cg_out = dfs_time / (Tp * K * 6);
	*/
	if (pinfo->pxl_clk_rate_div <= 0)
		pinfo->pxl_clk_rate_div = 1;

	thd_cg_out = (dfs_time * pinfo->pxl_clk_rate * pinfo->xres) /
		(((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch) * pinfo->pxl_clk_rate_div
		+ pinfo->xres) * 6 * 1000000UL);

	sram_valid_num = thd_cg_out / depth;
	thd_cg_in = (sram_valid_num + 1) * depth - 1;

	sram_max_mem_depth = (sram_valid_num + 1) * depth;

	thd_rqos_in = thd_cg_out * 85 / 100;
	thd_rqos_out = thd_cg_out;
	thd_flux_req_befdfs_in = GET_FLUX_REQ_IN(sram_max_mem_depth);
	thd_flux_req_befdfs_out = GET_FLUX_REQ_OUT(sram_max_mem_depth);

	sram_min_support_depth = dfs_time_min * pinfo->xres / (1000000 / 60 / (pinfo->yres +
		pinfo->ldi.v_back_porch + pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width) * (DBUF_WIDTH_BIT / 3 / BITS_PER_BYTE));

	//thd_flux_req_aftdfs_in   =[(sram_valid_num+1)*depth - 50*HSIZE/((1000000/60/(VSIZE+VFP+VBP+VSW))*6)]/3
	thd_flux_req_aftdfs_in = (sram_max_mem_depth - sram_min_support_depth) / 3;
	//thd_flux_req_aftdfs_out  =  2*[(sram_valid_num+1)* depth - 50*HSIZE/((1000000/60/(VSIZE+VFP+VBP+VSW))*6)]/3
	thd_flux_req_aftdfs_out = 2 * (sram_max_mem_depth - sram_min_support_depth) / 3;

	thd_dfs_ok = thd_flux_req_befdfs_in;

	HISI_FB_DEBUG("sram_valid_num=%d,\n"
		"thd_rqos_in=0x%x\n"
		"thd_rqos_out=0x%x\n"
		"thd_cg_in=0x%x\n"
		"thd_cg_out=0x%x\n"
		"thd_flux_req_befdfs_in=0x%x\n"
		"thd_flux_req_befdfs_out=0x%x\n"
		"thd_flux_req_aftdfs_in=0x%x\n"
		"thd_flux_req_aftdfs_out=0x%x\n"
		"thd_dfs_ok=0x%x\n",
		sram_valid_num,
		thd_rqos_in,
		thd_rqos_out,
		thd_cg_in,
		thd_cg_out,
		thd_flux_req_befdfs_in,
		thd_flux_req_befdfs_out,
		thd_flux_req_aftdfs_in,
		thd_flux_req_aftdfs_out,
		thd_dfs_ok);

	outp32(dbuf_base + DBUF_FRM_SIZE, pinfo->xres * pinfo->yres);
	outp32(dbuf_base + DBUF_FRM_HSIZE, DSS_WIDTH(pinfo->xres));
	outp32(dbuf_base + DBUF_SRAM_VALID_NUM, sram_valid_num);

	outp32(dbuf_base + DBUF_THD_RQOS, (thd_rqos_out<< 16) | thd_rqos_in);
	outp32(dbuf_base + DBUF_THD_WQOS, (thd_wqos_out << 16) | thd_wqos_in);
	outp32(dbuf_base + DBUF_THD_CG, (thd_cg_out << 16) | thd_cg_in);
	outp32(dbuf_base + DBUF_THD_OTHER, (thd_cg_hold << 16) | thd_wr_wait);
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_BEF, (thd_flux_req_befdfs_out << 16) | thd_flux_req_befdfs_in);
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_AFT, (thd_flux_req_aftdfs_out << 16) | thd_flux_req_aftdfs_in);
	outp32(dbuf_base + DBUF_THD_DFS_OK, thd_dfs_ok);
	outp32(dbuf_base + DBUF_FLUX_REQ_CTRL, (dfs_ok_mask << 1) | thd_flux_req_sw_en);

	outp32(dbuf_base + DBUF_DFS_LP_CTRL, 0x1);
}

static void init_ldi_pxl_div(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *ldi_base = NULL;
	uint32_t ifbc_type = 0;
	uint32_t mipi_idx = 0;
	uint32_t pxl0_div2_gt_en = 0;
	uint32_t pxl0_div4_gt_en = 0;
	uint32_t pxl0_divxcfg = 0;
	uint32_t pxl0_dsi_gt_en = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == EXTERNAL_PANEL_IDX)
            return;

	ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;

	ifbc_type = pinfo->ifbc_type;
	if ((ifbc_type  < IFBC_TYPE_NONE) || (ifbc_type  >= IFBC_TYPE_MAX)) {
		HISI_FB_ERR("ifbc_type is invalid");
		return;
	}

	mipi_idx = is_dual_mipi_panel(hisifd) ? 1 : 0;

	pxl0_div2_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div2_gt_en;
	pxl0_div4_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div4_gt_en;
	pxl0_divxcfg = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_divxcfg;
	pxl0_dsi_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_dsi_gt_en;

	set_reg(ldi_base + LDI_PXL0_DIV2_GT_EN, pxl0_div2_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DIV4_GT_EN, pxl0_div4_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_GT_EN, 0x1, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DSI_GT_EN, pxl0_dsi_gt_en, 2, 0);
	set_reg(ldi_base + LDI_PXL0_DIVXCFG, pxl0_divxcfg, 3, 0);
}

void init_ldi(struct hisi_fb_data_type *hisifd, bool fastboot_enable)
{
	char __iomem *ldi_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_rect_t rect = {0,0,0,0};
	uint32_t te0_enable = 0;
	uint32_t te1_enable = 0;
	uint32_t te_source = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
		te0_enable = 1;
		if (g_fpga_flag == 1) {
			set_reg(hisifd->dss_base + GLB_TP_SEL, 0x2, 2, 0);
		}
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		te1_enable = 1;
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;
	mipi_ifbc_get_rect(hisifd, &rect);

	init_ldi_pxl_div(hisifd);

	if (is_dual_mipi_panel(hisifd)) {
		if (is_mipi_video_panel(hisifd)) {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, (pinfo->ldi.h_back_porch + DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, 0);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		} else {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, pinfo->ldi.h_back_porch << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		}

		outp32(ldi_base + LDI_OVERLAP_SIZE,
			pinfo->ldi.dpi0_overlap_size | (pinfo->ldi.dpi1_overlap_size << 16));

		/* dual_mode_en */
		set_reg(ldi_base + LDI_CTRL, 1, 1, 5);

		/* split mode */
		set_reg(ldi_base + LDI_CTRL, 0, 1, 16);

		//dual lcd: 0x1, dual mipi: 0x0
		set_reg(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_DSI1_CLK_SEL, 0x0, 1, 0);
	}
	if (is_mipi_video_panel(hisifd)) {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
				pinfo->ldi.h_front_porch | ((pinfo->ldi.h_back_porch + DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, 0);
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	} else {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
				pinfo->ldi.h_front_porch | (pinfo->ldi.h_back_porch << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	}
	outp32(ldi_base + LDI_VRT_CTRL0,
		pinfo->ldi.v_front_porch | (pinfo->ldi.v_back_porch << 16));
	outp32(ldi_base + LDI_VRT_CTRL1, DSS_HEIGHT(pinfo->ldi.v_pulse_width));
	outp32(ldi_base + LDI_VRT_CTRL2, DSS_HEIGHT(rect.h));

	outp32(ldi_base + LDI_PLR_CTRL,
		pinfo->ldi.vsync_plr | (pinfo->ldi.hsync_plr << 1) |
		(pinfo->ldi.pixelclk_plr << 2) | (pinfo->ldi.data_en_plr << 3));

	//sensorhub int msk
	//outp32(ldi_base + LDI_SH_MASK_INT, 0x0);

	// bpp
	set_reg(ldi_base + LDI_CTRL, pinfo->bpp, 2, 3);
	// bgr
	set_reg(ldi_base + LDI_CTRL, pinfo->bgr_fmt, 1, 13);

	// for ddr pmqos
	outp32(ldi_base + LDI_VINACT_MSK_LEN,
		pinfo->ldi.v_front_porch);

	//cmd event sel
	outp32(ldi_base + LDI_CMD_EVENT_SEL, 0x1);

	if (is_mipi_cmd_panel(hisifd))
		set_reg(ldi_base + LDI_FRM_VALID_DBG, 0x1, 1, 4);

	// for 1Hz LCD and mipi command LCD
	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x1, 1, 0);

		//DSI_TE_CTRL
		// te_source = 0, select te_pin
		// te_source = 1, select te_triger
		te_source = 0;

		// dsi_te_hard_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 0);
		// dsi_te0_pin_p , dsi_te1_pin_p
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 2, 1);
		// dsi_te_hard_sel
		set_reg(ldi_base + LDI_DSI_TE_CTRL, te_source, 1, 3);
		if (te_source == 0) {
			// dsi_te_pin_hd_sel
			set_reg(ldi_base + LDI_DSI_TE_CTRL, te0_enable, 1, 6);
			set_reg(ldi_base + LDI_DSI_TE_CTRL, te1_enable, 1, 7);
		} else {
			// dsi_te_tri_sel
			set_reg(ldi_base + LDI_DSI_TE_CTRL, te0_enable, 1, 4);
			set_reg(ldi_base + LDI_DSI_TE_CTRL, te1_enable, 1, 5);
		}
		// dsi_te_mask_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 1, 8);
		// dsi_te_mask_dis
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 9);
		// dsi_te_mask_und
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 13);
		// dsi_te_pin_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 17);

		//TBD:(dsi_te_hs_num+vactive)*htotal/clk_pxl0_div+0.00004<1/60+vs_te_time+(vactive*hotal) /clk_ddic_rd
		set_reg(ldi_base + LDI_DSI_TE_HS_NUM, 0x0, 32, 0);
		set_reg(ldi_base + LDI_DSI_TE_HS_WD, 0x24024, 32, 0);

		// dsi_te0_vs_wd = lcd_te_width / T_pxl_clk, experience lcd_te_width = 2us
		if (pinfo->pxl_clk_rate_div== 0) {
			HISI_FB_ERR("pxl_clk_rate_div is NULL, not support !\n");
			pinfo->pxl_clk_rate_div = 1;
		}
		set_reg(ldi_base + LDI_DSI_TE_VS_WD,
			(0x3FC << 12) | (2 * pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div / 1000000), 32, 0);
		//set_reg(ldi_base + LDI_DSI_TE_VS_WD, 0x3FC0FF, 32, 0);
		//set_reg(ldi_base + LDI_DSI_TE_VS_WD, 0x3FC01F, 32, 0);
	} else {
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x1, 1, 1);
	}
	//ldi_data_gate(hisifd, true);

	// normal
	set_reg(ldi_base + LDI_WORK_MODE, 0x1, 1, 0);


	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(ldi_base + LDI_FRM_MSK,
			(hisifd->frame_update_flag == 1) ? 0x0 : 0x1, 1, 0);
	}

	// ldi disable
	if (!fastboot_enable)
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);

	if (pinfo->dpi01_exchange_flag == 1){
		set_reg(ldi_base + LDI_DPI_SET, 0x01, 1, 0);
	}
	HISI_FB_DEBUG("-.!\n");
}

void deinit_ldi(struct hisi_fb_data_type *hisifd)
{
	char __iomem *ldi_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	set_reg(ldi_base + LDI_CTRL, 0, 1, 0);
}

void enable_ldi(struct hisi_fb_data_type *hisifd)
{
	char __iomem *ldi_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	/* ldi enable */
	set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
}

void disable_ldi(struct hisi_fb_data_type *hisifd)
{
	char __iomem *ldi_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	/* ldi disable */
	set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);
}

//set pixel clock to the exact value which is larger than 288M
int dpe_recover_pxl_clock(struct hisi_fb_data_type *hisifd)
{
	if ((hisifd->panel_info.pxl_clk_rate > DSS_MAX_PXL0_CLK_288M)
		&& (hisifd->index == PRIMARY_PANEL_IDX)) {
		if (clk_set_rate(hisifd->dss_pxl0_clk, hisifd->panel_info.pxl_clk_rate) < 0) {
			HISI_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed!\n",
				hisifd->index, hisifd->panel_info.pxl_clk_rate);
			if (g_fpga_flag == 0)
				return -1;
		}
	}

	return 0;
}

void ldi_frame_update(struct hisi_fb_data_type *hisifd, bool update)
{
	char __iomem *ldi_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;

		if (is_mipi_cmd_panel(hisifd)) {
			set_reg(ldi_base + LDI_FRM_MSK, (update ? 0x0 : 0x1), 1, 0);
			if (update)
				set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
	}
}

void single_frame_update(struct hisi_fb_data_type *hisifd)
{
	char __iomem *ldi_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
		if (is_mipi_cmd_panel(hisifd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}

	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;

		if (is_mipi_cmd_panel(hisifd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		;
	}
}

void dpe_interrupt_clear(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dss_base = 0;
	uint32_t clear = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_PDP_INTS, clear);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, clear);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INTS, clear);

		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH1_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH2_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH3_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INTS, clear);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_SDP_INTS, clear);
		outp32(dss_base + DSS_LDI1_OFFSET + LDI_CPU_ITF_INTS, clear);
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_OFF_INTS, clear);
	} else {
		HISI_FB_ERR("fb%d, not support this device!\n", hisifd->index);
	}

}

void dpe_interrupt_unmask(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dss_base = 0;
	uint32_t unmask = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	pinfo = &(hisifd->panel_info);
	dss_base = hisifd->dss_base;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		unmask = ~0;
		//unmask &= ~(BIT_DPP_INTS | BIT_ITF0_INTS | BIT_DSS_GLB_INTS | BIT_MMU_IRPT_NS);
		unmask &= ~(BIT_DPP_INTS | BIT_ITF0_INTS | BIT_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_PDP_INT_MSK, unmask);

		unmask = ~0;
		if (is_mipi_cmd_panel(hisifd)) {
			unmask &= ~(BIT_LCD_TE0_PIN | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
		} else {
			unmask &= ~(BIT_VSYNC | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
		}
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, unmask);

		unmask = ~0;
		//unmask &= ~(BIT_CE_END_IND | BIT_BACKLIGHT_INTP);
		if ((pinfo->acm_ce_support == 1) && HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE))
			unmask &= ~(BIT_CE_END_IND);
		if (pinfo->hiace_support == 1)
			unmask &= ~(BIT_HIACE_IND);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, unmask);

	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		unmask = ~0;
		//unmask &= ~(BIT_SDP_ITF1_INTS  | BIT_SDP_DSS_GLB_INTS | BIT_SDP_MMU_IRPT_NS);
		unmask &= ~(BIT_SDP_ITF1_INTS | BIT_SDP_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_SDP_INT_MSK, unmask);

		unmask = ~0;
		unmask &= ~(BIT_VSYNC | BIT_VACTIVE0_START | BIT_VACTIVE0_END);
		outp32(dss_base +  DSS_LDI1_OFFSET + LDI_CPU_ITF_INT_MSK, unmask);
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		unmask = ~0;
		unmask &= ~(BIT_OFF_WCH0_INTS | BIT_OFF_WCH1_INTS | BIT_OFF_WCH0_WCH1_FRM_END_INT | BIT_OFF_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, unmask);
		unmask = ~0;
		unmask &= ~(BIT_OFF_CAM_WCH2_FRMEND_INTS);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, unmask);
	} else {
		HISI_FB_ERR("fb%d, not support this device!\n", hisifd->index);
	}

}

void dpe_interrupt_mask(struct hisi_fb_data_type *hisifd)
{
	char __iomem *dss_base = 0;
	uint32_t mask = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	dss_base = hisifd->dss_base;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_PDP_INT_MSK, mask);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, mask);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH1_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH2_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH3_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INT_MSK, mask);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_SDP_INT_MSK, mask);
		outp32(dss_base + DSS_LDI1_OFFSET + LDI_CPU_ITF_INT_MSK, mask);
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, mask);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, mask);
	} else {
		HISI_FB_ERR("fb%d, not support this device!\n", hisifd->index);
	}

}

void ldi_data_gate(struct hisi_fb_data_type *hisifd, bool enble)
{
	char __iomem *ldi_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (!is_mipi_cmd_panel(hisifd)) {
		hisifd->ldi_data_gate_en = (enble ? 1 : 0);
		return ;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	if (g_ldi_data_gate_en == 1) {
		hisifd->ldi_data_gate_en = (enble ? 1 : 0);
		set_reg(ldi_base + LDI_CTRL, (enble ? 0x1 : 0x0), 1, 2);
	} else {
		hisifd->ldi_data_gate_en = 0;
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 2);
	}

	HISI_FB_DEBUG("ldi_data_gate_en=%d!\n", hisifd->ldi_data_gate_en);
}

/* dpp csc config */
#define CSC_ROW	(3)
#define CSC_COL	(5)

/*
** Rec.601 for Computer
** [ p00 p01 p02 cscidc2 cscodc2 ]
** [ p10 p11 p12 cscidc1 cscodc1 ]
** [ p20 p21 p22 cscidc0 cscodc0 ]
*/
static int CSC10B_YUV2RGB709_WIDE_MPREC0[CSC_ROW][CSC_COL] = {
	{0x400, 0x000, 0x64d, 0x000, 0x000},
	{0x400, 0x1f40, 0x1e21, 0x5fe, 0x000},
	{0x400, 0x76c, 0x000, 0x5fe, 0x000}
};

static int CSC10B_RGB2YUV709_WIDE_MPREC2[CSC_ROW][CSC_COL] = {
	{0x367, 0xb71, 0x128, 0x000, 0x000},
	{0x1e2b, 0x19d5, 0x800, 0x000, 0x202},
	{0x800, 0x18bc, 0x1f44, 0x000, 0x202},
};

static void init_csc10b(struct hisi_fb_data_type *hisifd, char __iomem * dpp_csc10b_base)
{
	int (*csc_coe)[CSC_COL];

	if (hisifd == NULL || dpp_csc10b_base == NULL) {
		HISI_FB_ERR("hisifd or dpp_csc10b_base is NULL!\n");
		return;
	}

	if (dpp_csc10b_base == (hisifd->dss_base + DSS_DPP_CSC_RGB2YUV10B_OFFSET)) {
		csc_coe = CSC10B_RGB2YUV709_WIDE_MPREC2;
		outp32(dpp_csc10b_base + CSC10B_MPREC, 0x2);
	} else if (dpp_csc10b_base == (hisifd->dss_base + DSS_DPP_CSC_YUV2RGB10B_OFFSET)) {
		csc_coe = CSC10B_YUV2RGB709_WIDE_MPREC0;
		outp32(dpp_csc10b_base + CSC10B_MPREC, 0x0);
	} else {
		return;
	}

	outp32(dpp_csc10b_base + CSC10B_IDC0, csc_coe[2][3]);
	outp32(dpp_csc10b_base + CSC10B_IDC1, csc_coe[1][3]);
	outp32(dpp_csc10b_base + CSC10B_IDC2, csc_coe[0][3]);
	outp32(dpp_csc10b_base + CSC10B_ODC0, csc_coe[2][4]);
	outp32(dpp_csc10b_base + CSC10B_ODC1, csc_coe[1][4]);
	outp32(dpp_csc10b_base + CSC10B_ODC2, csc_coe[0][4]);
	outp32(dpp_csc10b_base + CSC10B_P00, csc_coe[0][0]);
	outp32(dpp_csc10b_base + CSC10B_P01, csc_coe[0][1]);
	outp32(dpp_csc10b_base + CSC10B_P02, csc_coe[0][2]);
	outp32(dpp_csc10b_base + CSC10B_P10, csc_coe[1][0]);
	outp32(dpp_csc10b_base + CSC10B_P11, csc_coe[1][1]);
	outp32(dpp_csc10b_base + CSC10B_P12, csc_coe[1][2]);
	outp32(dpp_csc10b_base + CSC10B_P20, csc_coe[2][0]);
	outp32(dpp_csc10b_base + CSC10B_P21, csc_coe[2][1]);
	outp32(dpp_csc10b_base + CSC10B_P22, csc_coe[2][2]);

	outp32(dpp_csc10b_base + CSC10B_MODULE_EN, 0x1);
}

void init_dpp_csc(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("init_dpp_csc hisifd is NULL!\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (pinfo->acm_support || pinfo->arsr1p_sharpness_support || pinfo->post_scf_support) {
		// init csc10b rgb2yuv
		init_csc10b(hisifd, hisifd->dss_base + DSS_DPP_CSC_RGB2YUV10B_OFFSET);
		// init csc10b yuv2rgb
		init_csc10b(hisifd, hisifd->dss_base + DSS_DPP_CSC_YUV2RGB10B_OFFSET);

		//bit ext0 enable
		set_reg(hisifd->dss_base + DSS_DPP_BITEXT0_OFFSET + BIT_EXT0_CTL, 1, 1, 0);
	}
}

inline void acm_set_lut(char __iomem *address, uint32_t table[], uint32_t size)
{
	uint32_t data = 0;
	uint32_t index = 0;
	uint32_t i = 0;

	size /= 4;

	for (i = 0; i < size; i++) {
		index = i << 2;
		data = table[index] + (table[index + 1] << 8) + (table[index + 2] << 16) + (table[index + 3] << 24);
		outp32(address + (i << 2), data);
	}
}

inline void acm_set_lut_hue(char __iomem *address, uint32_t table[], uint32_t size)
{
	uint32_t data = 0;
	uint32_t index = 0;
	uint32_t i = 0;

	size /= 2;

	for (i = 0; i < size; i++) {
		index = i << 1;
		data = table[index] + (table[index + 1] << 16);
		outp32(address + (i << 2), data);
	}
}

/*lint -e838*/
void init_acm(struct hisi_fb_data_type *hisifd)
{
	char __iomem *acm_base = NULL;
	char __iomem *acm_lut_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	uint32_t lut_sel = 0;

	if (hisifd == NULL) {
		HISI_FB_DEBUG("init_acm hisifd is NULL!\n");
		return;
	}

	pinfo = &(hisifd->panel_info);
	acm_base = hisifd->dss_base + DSS_DPP_ACM_OFFSET;

	if (pinfo->acm_support != 1) {
		outp32(acm_base + ACM_MEM_CTRL, 0x4);
		HISI_FB_DEBUG("fb%d, not support acm!\n", hisifd->index);
		return;
	}

	acm_lut_base = hisifd->dss_base + DSS_DPP_ACM_LUT_OFFSET;

	set_reg(acm_base + ACM_HUESEL, 0x0, 1, 0);
	set_reg(acm_base + ACM_SATA_OFFSET, 0x20, 6, 0);

	set_reg(acm_base + ACM_CSC_IDC0, 0x5FE, 11, 0);
	set_reg(acm_base + ACM_CSC_IDC1, 0x5FE, 11, 0);
	set_reg(acm_base + ACM_CSC_IDC2,  0x0, 11, 0);

	set_reg(acm_base + ACM_CSC_P00, 0x400, 13, 0);
	set_reg(acm_base + ACM_CSC_P01, 0x0,   13, 0);
	set_reg(acm_base + ACM_CSC_P02, 0x64D, 13, 0);
	set_reg(acm_base + ACM_CSC_P10, 0x400, 13, 0);
	set_reg(acm_base + ACM_CSC_P11, 0x1F40, 13, 0);
	set_reg(acm_base + ACM_CSC_P12, 0x1E21, 13, 0);
	set_reg(acm_base + ACM_CSC_P20, 0x400, 13, 0);
	set_reg(acm_base + ACM_CSC_P21, 0x76C, 13, 0);
	set_reg(acm_base + ACM_CSC_P22, 0x0,   13, 0);

	set_reg(acm_base + ACM_CSC_MRREC, 0x0, 3, 0);

	set_reg(acm_base + ACM_R0_H, ((pinfo->r0_hh & 0x3ff) << 16) | (pinfo->r0_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R1_H, ((pinfo->r1_hh & 0x3ff) << 16) | (pinfo->r1_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R2_H, ((pinfo->r2_hh & 0x3ff) << 16) | (pinfo->r2_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R3_H, ((pinfo->r3_hh & 0x3ff) << 16) | (pinfo->r3_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R4_H, ((pinfo->r4_hh & 0x3ff) << 16) | (pinfo->r4_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R5_H, ((pinfo->r5_hh & 0x3ff) << 16) | (pinfo->r5_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R6_H, ((pinfo->r6_hh & 0x3ff) << 16) | (pinfo->r6_lh & 0x3ff), 32, 0);

	set_reg(acm_base + ACM_LUT_DIS0, 0x80, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS1, 0x7F, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS2, 0x80, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS3, 0x81, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS4, 0x80, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS5, 0x7F, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS6, 0x80, 10, 0);
	set_reg(acm_base + ACM_LUT_DIS7, 0x80, 10, 0);

	set_reg(acm_base + ACM_LUT_PARAM0, 0x200, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM1, 0x204, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM2, 0x200, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM3, 0x1FC, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM4, 0x200, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM5, 0x204, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM6, 0x200, 16, 0);
	set_reg(acm_base + ACM_LUT_PARAM7, 0x200, 16, 0);

	if (pinfo->acm_lut_hue_table && pinfo->acm_lut_hue_table_len > 0) {
		acm_set_lut_hue(acm_lut_base + ACM_U_H_COEF, pinfo->acm_lut_hue_table, pinfo->acm_lut_hue_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_hue_table is NULL or hue_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_sata_table && pinfo->acm_lut_sata_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATA_COEF, pinfo->acm_lut_sata_table, pinfo->acm_lut_sata_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_sata_table is NULL or sata_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr0_table && pinfo->acm_lut_satr0_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR0_COEF, pinfo->acm_lut_satr0_table, pinfo->acm_lut_satr0_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr0_table is NULL or satr0_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr1_table && pinfo->acm_lut_satr1_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR1_COEF, pinfo->acm_lut_satr1_table, pinfo->acm_lut_satr1_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr1_table is NULL or satr1_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr2_table && pinfo->acm_lut_satr2_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR2_COEF, pinfo->acm_lut_satr2_table, pinfo->acm_lut_satr2_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr2_table is NULL or satr2_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr3_table && pinfo->acm_lut_satr3_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR3_COEF, pinfo->acm_lut_satr3_table, pinfo->acm_lut_satr3_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr3_table is NULL or satr3_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr4_table && pinfo->acm_lut_satr4_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR4_COEF, pinfo->acm_lut_satr4_table, pinfo->acm_lut_satr4_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr4_table is NULL or satr4_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr5_table && pinfo->acm_lut_satr5_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR5_COEF, pinfo->acm_lut_satr5_table, pinfo->acm_lut_satr5_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr5_table is NULL or satr5_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr6_table && pinfo->acm_lut_satr6_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR6_COEF, pinfo->acm_lut_satr6_table, pinfo->acm_lut_satr6_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr6_table is NULL or satr6_table_len less than 0!\n", hisifd->index);
		return;
	}

	if (pinfo->acm_lut_satr7_table && pinfo->acm_lut_satr7_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR7_COEF, pinfo->acm_lut_satr7_table, pinfo->acm_lut_satr7_table_len);
	} else {
		HISI_FB_ERR("fb%d, acm_lut_satr7_table is NULL or satr7_table_len less than 0!\n", hisifd->index);
		return;
	}

	lut_sel = inp32(acm_base + ACM_LUT_SEL);
	set_reg(acm_base + ACM_LUT_SEL, (~lut_sel) & 0x3FF, 10, 0);
	set_reg(acm_base + ACM_EN, 0x1, 1, 0);
	g_acm_State = 1;
	//acm reg dimming init
	hisi_effect_color_dimming_acm_reg_init(hisifd);
}

inline void xcc_set_coef(char __iomem *base_addr, struct hisi_panel_info *pinfo,
    uint32_t rectify_R, uint32_t rectify_G, uint32_t rectify_B)
{
	if (pinfo == NULL) {
		HISI_FB_ERR("xcc_set_coef pinfo is NULL!\n");
		return;
	}

	outp32(base_addr + LCP_XCC_COEF_00, pinfo->xcc_table[0]);
	outp32(base_addr + LCP_XCC_COEF_01, pinfo->xcc_table[1]
	    * g_led_rg_csc_value[0] / 32768 * rectify_R / 32768);
	outp32(base_addr + LCP_XCC_COEF_02, pinfo->xcc_table[2]);
	outp32(base_addr + LCP_XCC_COEF_03, pinfo->xcc_table[3]);
	outp32(base_addr + LCP_XCC_COEF_10, pinfo->xcc_table[4]);
	outp32(base_addr + LCP_XCC_COEF_11, pinfo->xcc_table[5]);
	outp32(base_addr + LCP_XCC_COEF_12, pinfo->xcc_table[6]
	    * g_led_rg_csc_value[4] / 32768 * rectify_G / 32768);
	outp32(base_addr + LCP_XCC_COEF_13, pinfo->xcc_table[7]);
	outp32(base_addr + LCP_XCC_COEF_20, pinfo->xcc_table[8]);
	outp32(base_addr + LCP_XCC_COEF_21, pinfo->xcc_table[9]);
	outp32(base_addr + LCP_XCC_COEF_22, pinfo->xcc_table[10]);
	outp32(base_addr + LCP_XCC_COEF_23, pinfo->xcc_table[11]
	    * g_led_rg_csc_value[8] / 32768 * DISCOUNT_COEFFICIENT(g_comform_value)
	    * rectify_B / 32768);
}

static bool _check_igm_lut(struct hisi_panel_info *pinfo, char __iomem *lcp_lut_base)
{
	uint32_t i = 0;
	uint32_t index = 0;
	uint32_t temp, reg;
	for (i = 0; i < pinfo->igm_lut_table_len / 2; i++) {
		index = i << 1;
		temp = inp32(lcp_lut_base + (LCP_U_DEGAMA_R_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->igm_lut_table_R[index]) {
			HISI_FB_ERR("igm[%d]: R verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_R[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->igm_lut_table_R[index]) {
			HISI_FB_ERR("igm[%d]: R verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_R[index]);
			return false;
		}

		index = i << 1;
		temp = inp32(lcp_lut_base + (LCP_U_DEGAMA_G_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->igm_lut_table_G[index]) {
			HISI_FB_ERR("igm[%d]: G verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_G[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->igm_lut_table_G[index]) {
			HISI_FB_ERR("igm[%d]: G verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_G[index]);
			return false;
		}

		index = i << 1;
		temp = inp32(lcp_lut_base + (LCP_U_DEGAMA_B_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->igm_lut_table_B[index]) {
			HISI_FB_ERR("igm[%d]: B verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_B[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->igm_lut_table_B[index]) {
			HISI_FB_ERR("igm[%d]: B verify fail, reg=%x, table=%x", index, reg, pinfo->igm_lut_table_B[index]);
			return false;
		}
	}

	reg = inp32(lcp_lut_base + LCP_U_DEGAMA_R_LAST_COEF);
	if(reg !=  pinfo->igm_lut_table_R[pinfo->igm_lut_table_len - 1]) {
		HISI_FB_ERR("igm[%d]: R verify fail, reg=%x, table=%x", pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_R[pinfo->igm_lut_table_len - 1]);
		return false;
	}
	reg = inp32(lcp_lut_base + LCP_U_DEGAMA_G_LAST_COEF);
	if(reg !=  pinfo->igm_lut_table_G[pinfo->igm_lut_table_len - 1]) {
		HISI_FB_ERR("igm[%d]: G verify fail, reg=%x, table=%x", pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_G[pinfo->igm_lut_table_len - 1]);
		return false;
	}
	reg = inp32(lcp_lut_base + LCP_U_DEGAMA_B_LAST_COEF);
	if(reg !=  pinfo->igm_lut_table_B[pinfo->igm_lut_table_len - 1]) {
		HISI_FB_ERR("igm[%d]: B verify fail, reg=%x, table=%x", pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_B[pinfo->igm_lut_table_len - 1]);
		return false;
	}

	return true;
}

static bool _check_gamma_lut(struct hisi_panel_info *pinfo, char __iomem *gamma_lut_base) {
	uint32_t i = 0;
	uint32_t index = 0;
	uint32_t temp, reg;

	for (i = 0; i < pinfo->gamma_lut_table_len / 2; i++) {
		index = i << 1;
		temp = inp32(gamma_lut_base + (U_GAMA_R_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->gamma_lut_table_R[index]) {
			HISI_FB_ERR("gamma[%d]: R verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_R[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->gamma_lut_table_R[index]) {
			HISI_FB_ERR("gamma[%d]: R verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_R[index]);
			return false;
		}

		index = i << 1;
		temp = inp32(gamma_lut_base + (U_GAMA_G_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->gamma_lut_table_G[index]) {
			HISI_FB_ERR("gamma[%d]: G verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_G[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->gamma_lut_table_G[index]) {
			HISI_FB_ERR("gamma[%d]: G verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_G[index]);
			return false;
		}

		index = i << 1;
		temp = inp32(gamma_lut_base + (U_GAMA_B_COEF +  i * 4));
		reg = temp & 0xFFF;
		if( reg != pinfo->gamma_lut_table_B[index]) {
			HISI_FB_ERR("gamma[%d]: B verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_B[index]);
			return false;
		}
		reg = (temp & 0xFFF0000)>>16;
		index += 1;
		if(reg !=  pinfo->gamma_lut_table_B[index]) {
			HISI_FB_ERR("gamma[%d]: B verify fail, reg=%x, table=%x", index, reg, pinfo->gamma_lut_table_B[index]);
			return false;
		}
	}

	reg = inp32(gamma_lut_base + U_GAMA_R_LAST_COEF);
	if(reg !=  pinfo->gamma_lut_table_R[pinfo->gamma_lut_table_len - 1]) {
		HISI_FB_ERR("gamma[%d]: R verify fail, reg=%x, table=%x",	pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_R[pinfo->igm_lut_table_len - 1]);
		return false;
	}
	reg = inp32(gamma_lut_base + U_GAMA_G_LAST_COEF);
	if(reg !=  pinfo->gamma_lut_table_G[pinfo->gamma_lut_table_len - 1]) {
		HISI_FB_ERR("gamma[%d]: G verify fail, reg=%x, table=%x", pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_G[pinfo->igm_lut_table_len - 1]);
		return false;
	}
	reg = inp32(gamma_lut_base + U_GAMA_B_LAST_COEF);
	if(reg !=  pinfo->gamma_lut_table_B[pinfo->gamma_lut_table_len - 1]) {
		HISI_FB_ERR("gamma[%d]: B verify fail, reg=%x, table=%x", pinfo->igm_lut_table_len - 1, reg, pinfo->igm_lut_table_B[pinfo->igm_lut_table_len - 1]);
		return false;
	}

	return true;
}

static void _write_igm_lut(struct hisi_panel_info *pinfo, char __iomem *lcp_lut_base)
{
	uint32_t i = 0;
	uint32_t index = 0;

	for (i = 0; i < pinfo->igm_lut_table_len / 2; i++) {
		index = i << 1;
		outp32(lcp_lut_base + (LCP_U_DEGAMA_R_COEF +  i * 4), pinfo->igm_lut_table_R[index] | pinfo->igm_lut_table_R[index+1] << 16);
		outp32(lcp_lut_base + (LCP_U_DEGAMA_G_COEF +  i * 4), pinfo->igm_lut_table_G[index] | pinfo->igm_lut_table_G[index+1] << 16);
		outp32(lcp_lut_base + (LCP_U_DEGAMA_B_COEF +  i * 4), pinfo->igm_lut_table_B[index] | pinfo->igm_lut_table_B[index+1] << 16);
	}
	outp32(lcp_lut_base + LCP_U_DEGAMA_R_LAST_COEF, pinfo->igm_lut_table_R[pinfo->igm_lut_table_len - 1]);
	outp32(lcp_lut_base + LCP_U_DEGAMA_G_LAST_COEF, pinfo->igm_lut_table_G[pinfo->igm_lut_table_len - 1]);
	outp32(lcp_lut_base + LCP_U_DEGAMA_B_LAST_COEF, pinfo->igm_lut_table_B[pinfo->igm_lut_table_len - 1]);

	return;
}

static void _write_gamma_lut(struct hisi_panel_info *pinfo, char __iomem *gamma_lut_base)
{
	uint32_t i = 0;
	uint32_t index = 0;

	for (i = 0; i < pinfo->gamma_lut_table_len / 2; i++) {
		index = i << 1;
		outp32(gamma_lut_base + (U_GAMA_R_COEF + i * 4), pinfo->gamma_lut_table_R[index] | pinfo->gamma_lut_table_R[index+1] << 16 );
		outp32(gamma_lut_base + (U_GAMA_G_COEF + i * 4), pinfo->gamma_lut_table_G[index] | pinfo->gamma_lut_table_G[index+1] << 16 );
		outp32(gamma_lut_base + (U_GAMA_B_COEF + i * 4), pinfo->gamma_lut_table_B[index] | pinfo->gamma_lut_table_B[index+1] << 16 );
	}
	outp32(gamma_lut_base + U_GAMA_R_LAST_COEF, pinfo->gamma_lut_table_R[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_G_LAST_COEF, pinfo->gamma_lut_table_G[pinfo->gamma_lut_table_len - 1]);
	outp32(gamma_lut_base + U_GAMA_B_LAST_COEF, pinfo->gamma_lut_table_B[pinfo->gamma_lut_table_len - 1]);

	return;
}

void init_igm_gmp_xcc_gm(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *dpp_base = NULL;
	char __iomem *lcp_base = NULL;
	char __iomem *lcp_lut_base = NULL;
	char __iomem *gamma_base = NULL;
	char __iomem *gamma_lut_base = NULL;
	uint32_t i = 0, j = 0, k = 0;
	uint32_t pos0 = 0, pos1 = 0;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;
	if (hisifd == NULL)	{
		HISI_FB_ERR("init_degmma_xcc_gmp hisifd is NULL!\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
		lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
		lcp_lut_base = hisifd->dss_base + DSS_DPP_LCP_LUT_OFFSET;
		gamma_base = hisifd->dss_base + DSS_DPP_GAMA_OFFSET;
		gamma_lut_base = hisifd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!\n", hisifd->index);
		return;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//Degamma
	if (pinfo->gamma_support == 1) {
		//disable degamma
		set_reg(lcp_base + LCP_DEGAMA_EN, 0x0, 1, 0);

		if (!hisifb_use_dynamic_degamma(hisifd, lcp_lut_base)) {
			//degamma
			if (pinfo->igm_lut_table_len > 0
				&& pinfo->igm_lut_table_R
				&& pinfo->igm_lut_table_G
				&& pinfo->igm_lut_table_B) {
				_write_igm_lut(pinfo, lcp_lut_base);

				//auto check test
				if (is_mipi_video_panel(hisifd)) {
					if(!_check_igm_lut(pinfo, lcp_lut_base))	{
						_write_igm_lut(pinfo, lcp_lut_base);
					}
				}
			}
		}

		//enable degamma
		set_reg(lcp_base + LCP_DEGAMA_EN, 0x1, 1, 0);
	} else {
		//degama memory shutdown
		outp32(lcp_base + LCP_DEGAMA_MEM_CTRL, 0x4);
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
			//enable xcc
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x0, 1, 0);
		}
	}

	//GMP
	if (pinfo->gmp_support == 1) {
		if (g_gmp_State == 0 || pinfo->colormode_support == 0) {
			//disable gmp
			set_reg(lcp_base + LCP_GMP_BYPASS_EN, 0x1, 1, 0);

			//gmp lut
			if (pinfo->gmp_lut_table_len > 0
				&& pinfo->gmp_lut_table_low32bit
				&& pinfo->gmp_lut_table_high4bit) {
				for (i = 0; i < pinfo->gmp_lut_table_len; i++) {
					pos1 = LCP_U_GMP_COEF + i * 0x800;
					for (j = 0; j < pinfo->gmp_lut_table_len; j++) {
						pos0 = pos1 + 0x80 * j;
						for (k = 0; k < pinfo->gmp_lut_table_len; k++) {
							outp32(lcp_lut_base + pos0 +  k * 8,
							pinfo->gmp_lut_table_low32bit[(i * pinfo->gmp_lut_table_len + j) * pinfo->gmp_lut_table_len + k]);
							outp32(lcp_lut_base + pos0 +  k * 8 + 4,
							pinfo->gmp_lut_table_high4bit[(i * pinfo->gmp_lut_table_len + j) * pinfo->gmp_lut_table_len + k]);
						}
					}
				}
			}
			//enable gmp
			set_reg(lcp_base + LCP_GMP_BYPASS_EN, 0x0, 1, 0);
			g_gmp_State = 1;
		}
	} else {
		//gmp memory shutdown
		outp32(lcp_base + LCP_GMP_MEM_CTRL, 0x4);
	}

	//GAMMA
	if (pinfo->gamma_support == 1) {
		//disable gamma
		set_reg(gamma_base + GAMA_EN, 0x0, 1, 0);

		if (!hisifb_use_dynamic_gamma(hisifd, gamma_lut_base)) {
			if (pinfo->gamma_lut_table_len > 0
				&& pinfo->gamma_lut_table_R
				&& pinfo->gamma_lut_table_G
				&& pinfo->gamma_lut_table_B) {
				_write_gamma_lut(pinfo, gamma_lut_base);

				//auto check test
				if (is_mipi_video_panel(hisifd)) {
					if(!_check_gamma_lut(pinfo, gamma_lut_base)) {
						_write_gamma_lut(pinfo, gamma_lut_base);
					}
				}
			}
		}

		//enable gamma
		set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);
	} else {
		//gama memory shutdown
		outp32(gamma_base + GAMA_MEM_CTRL, 0x4);
	}
}

void init_dither(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *dither_base = NULL;

	if (hisifd == NULL)	{
		HISI_FB_ERR("hisifd is NULL!\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (pinfo->dither_support != 1) {
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		dither_base = hisifd->dss_base + DSS_DPP_DITHER_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return ;
	}

	set_reg(dither_base + DITHER_CTL, 0x1A86, 28, 0);
	set_reg(dither_base + DITHER_PARA, 0x0, 3, 0);
	set_reg(dither_base + DITHER_MATRIX_PART1, 0x5D7F91B3, 32, 0);
	set_reg(dither_base + DITHER_MATRIX_PART0, 0x6E4CA280, 32, 0);
	set_reg(dither_base + DITHER_ERRDIFF_WEIGHT, 0x1232134, 28, 0);
	set_reg(dither_base + DITHER_FRC_01_PART0, 0x0, 32, 0);
	set_reg(dither_base + DITHER_FRC_01_PART1, 0xFFFF0000, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART0, 0, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART1, 0xFFFFFFFF, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART0, 0xFFFF0000, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART1, 0xFFFFFFFF, 32, 0);
}

void dpe_store_ct_cscValue(struct hisi_fb_data_type *hisifd, unsigned int csc_value[])
{
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pinfo = &(hisifd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return;
	}

	pinfo->xcc_table[1] = csc_value[0];
	pinfo->xcc_table[2] = csc_value[1];
	pinfo->xcc_table[3] = csc_value[2];
	pinfo->xcc_table[5] = csc_value[3];
	pinfo->xcc_table[6] = csc_value[4];
	pinfo->xcc_table[7] = csc_value[5];
	pinfo->xcc_table[9] = csc_value[6];
	pinfo->xcc_table[10] = csc_value[7];
	pinfo->xcc_table[11] = csc_value[8];

	return;
}

void dpe_update_g_comform_discount(unsigned int value)
{
	g_comform_value = value;
	HISI_FB_INFO(" g_comform_value = %d" , g_comform_value);
}

int dpe_set_ct_cscValue(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -1;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
			hisifd->color_temperature_flag = 2;              //disable dirty region
		}
	}

	return 0;
}

ssize_t dpe_show_ct_cscValue(struct hisi_fb_data_type *hisifd, char *buf)
{
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11]);
}

int dpe_set_xcc_cscValue(struct hisi_fb_data_type *hisifd)
{
	return 0;
}
/*lint -e550*/
int dpe_set_comform_ct_cscValue(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;
	char __iomem *lcp_base = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -1;
	}

	lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R <= 32768 && pinfo->color_temp_rectify_R) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B <= 32768 && pinfo->color_temp_rectify_B) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
			hisifd->color_temperature_flag = 2;              //disable dirty region
		}
	}

	return 0;
}
/*lint +e550*/
ssize_t dpe_show_comform_ct_cscValue(struct hisi_fb_data_type *hisifd, char *buf)
{
	struct hisi_panel_info *pinfo = NULL;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,g_comform_value = %d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11],
		g_comform_value);
}

void dpe_init_led_rg_ct_cscValue(void)
{
	g_led_rg_csc_value[0] = 32768;
	g_led_rg_csc_value[1] = 0;
	g_led_rg_csc_value[2] = 0;
	g_led_rg_csc_value[3] = 0;
	g_led_rg_csc_value[4] = 32768;
	g_led_rg_csc_value[5] = 0;
	g_led_rg_csc_value[6] = 0;
	g_led_rg_csc_value[7] = 0;
	g_led_rg_csc_value[8] = 32768;
	g_is_led_rg_csc_set = 0;

	return;
}

void dpe_store_led_rg_ct_cscValue(unsigned int csc_value[])
{
	g_led_rg_csc_value [0] = csc_value[0];
	g_led_rg_csc_value [1] = csc_value[1];
	g_led_rg_csc_value [2] = csc_value[2];
	g_led_rg_csc_value [3] = csc_value[3];
	g_led_rg_csc_value [4] = csc_value[4];
	g_led_rg_csc_value [5] = csc_value[5];
	g_led_rg_csc_value [6] = csc_value[6];
	g_led_rg_csc_value [7] = csc_value[7];
	g_led_rg_csc_value [8] = csc_value[8];
	g_is_led_rg_csc_set = 1;

	return;
}

int dpe_set_led_rg_ct_cscValue(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t color_temp_rectify_R = 32768;
	uint32_t color_temp_rectify_G = 32768;
	uint32_t color_temp_rectify_B = 32768;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -1;
	}

	if (pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (1 == g_is_led_rg_csc_set && 1 == pinfo->xcc_support) {
		HISI_FB_DEBUG("real set color temperature: g_is_led_rg_csc_set = %d, R = 0x%x, G = 0x%x, B = 0x%x .\n",
				g_is_led_rg_csc_set, g_led_rg_csc_value[0], g_led_rg_csc_value[4], g_led_rg_csc_value[8]);
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			outp32(lcp_base + LCP_XCC_COEF_00, pinfo->xcc_table[0]);
			outp32(lcp_base + LCP_XCC_COEF_01, pinfo->xcc_table[1]
				* g_led_rg_csc_value[0] / 32768 * color_temp_rectify_R / 32768);
			outp32(lcp_base + LCP_XCC_COEF_02, pinfo->xcc_table[2]);
			outp32(lcp_base + LCP_XCC_COEF_03, pinfo->xcc_table[3]);
			outp32(lcp_base + LCP_XCC_COEF_10, pinfo->xcc_table[4]);
			outp32(lcp_base + LCP_XCC_COEF_11, pinfo->xcc_table[5]);
			outp32(lcp_base + LCP_XCC_COEF_12, pinfo->xcc_table[6]
				* g_led_rg_csc_value[4] / 32768 * color_temp_rectify_G / 32768);
			outp32(lcp_base + LCP_XCC_COEF_13, pinfo->xcc_table[7]);
			outp32(lcp_base + LCP_XCC_COEF_20, pinfo->xcc_table[8]);
			outp32(lcp_base + LCP_XCC_COEF_21, pinfo->xcc_table[9]);
			outp32(lcp_base + LCP_XCC_COEF_22, pinfo->xcc_table[10]);
			outp32(lcp_base + LCP_XCC_COEF_23, pinfo->xcc_table[11]
				* g_led_rg_csc_value[8] / 32768 * DISCOUNT_COEFFICIENT(g_comform_value)
				* color_temp_rectify_B / 32768);
		}
	}

	return 0;
}

ssize_t dpe_show_led_rg_ct_cscValue(char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		g_led_rg_para1, g_led_rg_para2,
		g_led_rg_csc_value [0], g_led_rg_csc_value [1], g_led_rg_csc_value [2],
		g_led_rg_csc_value [3], g_led_rg_csc_value [4], g_led_rg_csc_value [5],
		g_led_rg_csc_value [6], g_led_rg_csc_value [7], g_led_rg_csc_value [8]);
}

ssize_t dpe_show_cinema_value(struct hisi_fb_data_type *hisifd, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "gamma type is = %d\n", hisifd->panel_info.gamma_type);
}

int dpe_set_cinema(struct hisi_fb_data_type *hisifd, unsigned int value)
{
	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd, NUll pointer warning.\n");
		return -1;
	}

	if(value == hisifd->panel_info.gamma_type) {
		HISI_FB_DEBUG("fb%d, cinema mode is already in %d!\n", hisifd->index, value);
		return 0;
	}

	hisifd->panel_info.gamma_type = value;
	return 0;
}

void dpe_update_g_acm_state(unsigned int value)
{
	return;
}

void dpe_set_acm_state(struct hisi_fb_data_type *hisifd)
{
	return;
}

ssize_t dpe_show_acm_state(char *buf)
{
	ssize_t ret = 0;

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_acm_State = %d\n", g_acm_State);

	return ret;
}

void dpe_update_g_gmp_state(unsigned int value)
{
	return;
}

void dpe_set_gmp_state(struct hisi_fb_data_type *hisifd)
{
	return;
}

ssize_t dpe_show_gmp_state(char *buf)
{
	ssize_t ret = 0;

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_gmp_State = %d\n", g_gmp_State);

	return ret;
}
/*lint -e838*/
void dpe_sbl_set_al_bl(struct hisi_fb_data_type *hisifd)
{
	char __iomem *sbl_base;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd, NUll pointer warning.\n");
		return;
	}

	sbl_base = hisifd->dss_base + DSS_DPP_SBL_OFFSET;

	set_reg(sbl_base + SBL_REG_CALC_BACKLIGHT_7_TO_0, (uint32_t)hisifd->sbl.sbl_backlight_l, 8, 0);
	set_reg(sbl_base + SBL_REG_CALC_BACKLIGHT_15_TO_8, (uint32_t)hisifd->sbl.sbl_backlight_h, 8, 0);
	set_reg(sbl_base + SBL_REG_CALC_AMBIENT_LIGHT_7_TO_0, (uint32_t)hisifd->sbl.sbl_ambient_light_l, 8, 0);
	set_reg(sbl_base + SBL_REG_CALC_AMBIENT_LIGHT_15_TO_8, (uint32_t)hisifd->sbl.sbl_ambient_light_h, 8, 0);

	return;
}
/*lint +e838*/

