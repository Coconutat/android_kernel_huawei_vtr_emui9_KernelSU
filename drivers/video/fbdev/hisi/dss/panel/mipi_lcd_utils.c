/* Copyright (c) 2017-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "hisi_fb.h"
#include "mipi_lcd_utils.h"
uint32_t g_mipi_lcd_name = 0;


static void init_dsc_para(struct dsc_panel_info *vesa_dsc, uint32_t basic_para[],
	uint32_t rc_buf_thresh[], uint32_t rc_minqp[], uint32_t rc_maxqp[], uint32_t rc_offset[])
{
	if (NULL == vesa_dsc) {
		HISI_FB_ERR("NULL vesa_dsc .\n");
		return;
	}

	vesa_dsc->bits_per_component = basic_para[0];
	vesa_dsc->linebuf_depth = basic_para[1];
	vesa_dsc->bits_per_pixel = basic_para[2];
	vesa_dsc->initial_xmit_delay = basic_para[3];

	vesa_dsc->slice_width = basic_para[4];
	vesa_dsc->slice_height = basic_para[5];

	vesa_dsc->first_line_bpg_offset = basic_para[6];
	vesa_dsc->mux_word_size = basic_para[7];

	/* DSC_CTRL */
	vesa_dsc->block_pred_enable = basic_para[8];

	/* RC_PARAM3 */
	vesa_dsc->initial_offset = basic_para[9];

	/* FLATNESS_QP_TH */
	vesa_dsc->flatness_min_qp = basic_para[10];
	vesa_dsc->flatness_max_qp = basic_para[11];

	/* DSC_PARAM4 */
	vesa_dsc->rc_edge_factor= basic_para[12];
	vesa_dsc->rc_model_size = basic_para[13];

	/* DSC_RC_PARAM5: 0x330b0b */
	vesa_dsc->rc_tgt_offset_lo = basic_para[14];
	vesa_dsc->rc_tgt_offset_hi = basic_para[15];
	vesa_dsc->rc_quant_incr_limit1 = basic_para[16];
	vesa_dsc->rc_quant_incr_limit0 = basic_para[17];

	/* DSC_RC_BUF_THRESH0: 0xe1c2a38 */
	vesa_dsc->rc_buf_thresh0 = rc_buf_thresh[0];
	vesa_dsc->rc_buf_thresh1 = rc_buf_thresh[1];
	vesa_dsc->rc_buf_thresh2 = rc_buf_thresh[2];
	vesa_dsc->rc_buf_thresh3 = rc_buf_thresh[3];

	/* DSC_RC_BUF_THRESH1: 0x46546269 */
	vesa_dsc->rc_buf_thresh4 = rc_buf_thresh[4];
	vesa_dsc->rc_buf_thresh5 = rc_buf_thresh[5];
	vesa_dsc->rc_buf_thresh6 = rc_buf_thresh[6];
	vesa_dsc->rc_buf_thresh7 = rc_buf_thresh[7];

	/* DSC_RC_BUF_THRESH2: 0x7077797b */
	vesa_dsc->rc_buf_thresh8 = rc_buf_thresh[8];
	vesa_dsc->rc_buf_thresh9 = rc_buf_thresh[9];
	vesa_dsc->rc_buf_thresh10 = rc_buf_thresh[10];
	vesa_dsc->rc_buf_thresh11 = rc_buf_thresh[11];

	/* DSC_RC_BUF_THRESH3: 0x7d7e0000 */
	vesa_dsc->rc_buf_thresh12 = rc_buf_thresh[12];
	vesa_dsc->rc_buf_thresh13 = rc_buf_thresh[13];

	/* DSC_RC_RANGE_PARAM0: 0x1020100 */
	vesa_dsc->range_min_qp0 = rc_minqp[0];
	vesa_dsc->range_max_qp0 = rc_maxqp[0];
	vesa_dsc->range_bpg_offset0 = rc_offset[0];
	vesa_dsc->range_min_qp1 = rc_minqp[1];
	vesa_dsc->range_max_qp1 = rc_maxqp[1];
	vesa_dsc->range_bpg_offset1 = rc_offset[1];

	/* DSC_RC_RANGE_PARAM1: 0x94009be */
	vesa_dsc->range_min_qp2 = rc_minqp[2];
	vesa_dsc->range_max_qp2 = rc_maxqp[2];
	vesa_dsc->range_bpg_offset2 = rc_offset[2];
	vesa_dsc->range_min_qp3 = rc_minqp[3];
	vesa_dsc->range_max_qp3 = rc_maxqp[3];
	vesa_dsc->range_bpg_offset3 = rc_offset[3];

	/* DSC_RC_RANGE_PARAM2, 0x19fc19fa */
	vesa_dsc->range_min_qp4 = rc_minqp[4];
	vesa_dsc->range_max_qp4 = rc_maxqp[4];
	vesa_dsc->range_bpg_offset4 = rc_offset[4];
	vesa_dsc->range_min_qp5 = rc_minqp[5];
	vesa_dsc->range_max_qp5 = rc_maxqp[5];
	vesa_dsc->range_bpg_offset5 = rc_offset[5];

	/* DSC_RC_RANGE_PARAM3, 0x19f81a38 */
	vesa_dsc->range_min_qp6 = rc_minqp[6];
	vesa_dsc->range_max_qp6 = rc_maxqp[6];
	vesa_dsc->range_bpg_offset6 = rc_offset[6];
	vesa_dsc->range_min_qp7 = rc_minqp[7];
	vesa_dsc->range_max_qp7 = rc_maxqp[7];
	vesa_dsc->range_bpg_offset7 = rc_offset[7];

	/* DSC_RC_RANGE_PARAM4, 0x1a781ab6 */
	vesa_dsc->range_min_qp8 = rc_minqp[8];
	vesa_dsc->range_max_qp8 = rc_maxqp[8];
	vesa_dsc->range_bpg_offset8 = rc_offset[8];
	vesa_dsc->range_min_qp9 = rc_minqp[9];
	vesa_dsc->range_max_qp9 = rc_maxqp[9];
	vesa_dsc->range_bpg_offset9 = rc_offset[9];

	/* DSC_RC_RANGE_PARAM5, 0x2af62b34 */
	vesa_dsc->range_min_qp10 = rc_minqp[10];
	vesa_dsc->range_max_qp10 = rc_maxqp[10];
	vesa_dsc->range_bpg_offset10 = rc_offset[10];
	vesa_dsc->range_min_qp11 = rc_minqp[11];
	vesa_dsc->range_max_qp11 = rc_maxqp[11];
	vesa_dsc->range_bpg_offset11 = rc_offset[11];

	/* DSC_RC_RANGE_PARAM6, 0x2b743b74 */
	vesa_dsc->range_min_qp12 = rc_minqp[12];
	vesa_dsc->range_max_qp12 = rc_maxqp[12];
	vesa_dsc->range_bpg_offset12 = rc_offset[12];
	vesa_dsc->range_min_qp13 = rc_minqp[13];
	vesa_dsc->range_max_qp13 = rc_maxqp[13];
	vesa_dsc->range_bpg_offset13 = rc_offset[13];

	/* DSC_RC_RANGE_PARAM7, 0x6bf40000 */
	vesa_dsc->range_min_qp14 = rc_minqp[14];
	vesa_dsc->range_max_qp14 = rc_maxqp[14];
	vesa_dsc->range_bpg_offset14 = rc_offset[14];
}

void get_vesa_dsc_para(struct hisi_panel_info *pinfo, uint8_t panel_mode)
{
	struct dsc_panel_info *vesa_dsc = NULL;

	uint32_t basic_para_8bpc_8bpp[18] = {8, 9, 8, 512, 719, 7, 12, 48, 1, 6144, 3, 12, 6, 8192, 0x3, 0x3, 0x0b, 0x0b};
	uint32_t rc_buf_thresh_8bpc_8bpp[14] = {0xe, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e};
	uint32_t rc_minqp_8bpc_8bpp[15] =  {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 5, 5, 5, 7, 13};
	uint32_t rc_maxqp_8bpc_8bpp[15] = {4, 4, 5, 6, 7, 7, 7, 8, 9, 10, 11, 12, 13, 13, 15};
	uint32_t rc_offset_8bpc_8bpp[15] =  {2, 0, 0, 0x3e, 0x3c, 0x3a, 0x38, 0x38, 0x38, 0x36, 0x36, 0x34, 0x34, 0x34, 0x34};

	uint32_t basic_para_10bpc_8bpp[18] = {10, 11, 8, 512, 719, 7, 12, 48, 1, 6144, 7, 16, 6, 8192, 0x3, 0x3, 0x0f, 0x0f};
	uint32_t rc_buf_thresh_10bpc_8bpp[14] = {0xe, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e};
	uint32_t rc_minqp_10bpc_8bpp[15] =  {0,4,5,5,7,7,7,7,7,7,9,9,9,11,17};
	uint32_t rc_maxqp_10bpc_8bpp[15] = {8,8,9,10,11,11,11,12,13,14,15,16,17,17,19};
	uint32_t rc_offset_10bpc_8bpp[15] =  {2, 0, 0, 0x3e, 0x3c, 0x3a, 0x38, 0x38, 0x38, 0x36, 0x36, 0x34, 0x34, 0x34, 0x34};

	uint32_t basic_para_10bpc_10bpp[18] = {10, 11, 10, 512, 719, 7, 12, 48, 1, 5632, 7, 16, 6, 8192, 0x3, 0x3, 0x0f, 0x0f};
	uint32_t rc_buf_thresh_10bpc_10bpp[14] = {0xe, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e};
	uint32_t rc_minqp_10bpc_10bpp[15] =  {0,4,5,6,7,7,7,7,7,7,9,9,9,11,17};
	uint32_t rc_maxqp_10bpc_10bpp[15] = {7,8,9,10,11,11,11,12,13,13,14,15,16,17,19};
	uint32_t rc_offset_10bpc_10bpp[15] =  {2, 0, 0, 0x3e, 0x3c, 0x3a, 0x38, 0x38, 0x38, 0x36, 0x36, 0x36, 0x34, 0x34, 0x34};

	if (NULL == pinfo) {
		HISI_FB_ERR("pinfo is null .\n");
		return;
	}
	vesa_dsc = &(pinfo->vesa_dsc);

	if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_DUAL) {
		if (panel_mode == MODE_8BIT) {
			init_dsc_para(vesa_dsc, basic_para_8bpc_8bpp, rc_buf_thresh_8bpc_8bpp, rc_minqp_8bpc_8bpp, rc_maxqp_8bpc_8bpp, rc_offset_8bpc_8bpp);
		} else if (panel_mode == MODE_10BIT_VIDEO_3X) {
			init_dsc_para(vesa_dsc, basic_para_10bpc_10bpp, rc_buf_thresh_10bpc_10bpp, rc_minqp_10bpc_10bpp, rc_maxqp_10bpc_10bpp, rc_offset_10bpc_10bpp);
		}
	} else if ((pinfo->ifbc_type == IFBC_TYPE_VESA3_75X_DUAL)) {
		init_dsc_para(vesa_dsc, basic_para_10bpc_8bpp, rc_buf_thresh_10bpc_8bpp, rc_minqp_10bpc_8bpp, rc_maxqp_10bpc_8bpp, rc_offset_10bpc_8bpp);
	}

	return;
}

static void get_porch_clk_para(struct hisi_panel_info *pinfo, uint8_t panel_mode)
{
	if (NULL == pinfo) {
		HISI_FB_ERR("NULL pinfo .\n");
		return;
	}

	if (panel_mode == MODE_8BIT) {
		pinfo->type = PANEL_MIPI_CMD;
		pinfo->ldi.h_back_porch = pinfo->ldi.hbp_store_cmd;
		pinfo->ldi.h_front_porch = pinfo->ldi.hfp_store_cmd;
		pinfo->ldi.h_pulse_width = pinfo->ldi.hpw_store_cmd;
		pinfo->ldi.v_back_porch = pinfo->ldi.vbp_store_cmd;
		pinfo->ldi.v_front_porch = pinfo->ldi.vfp_store_cmd;
		pinfo->ldi.v_pulse_width = pinfo->ldi.vpw_store_cmd;
		pinfo->pxl_clk_rate = pinfo->ldi.pxl_clk_store_cmd;
	} else if (panel_mode == MODE_10BIT_VIDEO_3X) {
		pinfo->type = PANEL_MIPI_VIDEO;
		pinfo->ldi.h_back_porch = pinfo->ldi.hbp_store_vid;
		pinfo->ldi.h_front_porch = pinfo->ldi.hfp_store_vid;
		pinfo->ldi.h_pulse_width = pinfo->ldi.hpw_store_vid;
		pinfo->ldi.v_back_porch = pinfo->ldi.vbp_store_vid;
		pinfo->ldi.v_front_porch = pinfo->ldi.vfp_store_vid;
		pinfo->ldi.v_pulse_width = pinfo->ldi.vpw_store_vid;
		pinfo->pxl_clk_rate = pinfo->ldi.pxl_clk_store_vid;
	}

	HISI_FB_DEBUG("panel_mode [%d], type[%d]:\n"
		"hbp[%d], hfp[%d], hpw[%d], vbp[%d], vfp[%d], vpw[%d], pxl_clk[%llu], dsi[%d].\n",
		panel_mode, pinfo->type,
		pinfo->ldi.h_back_porch,
		pinfo->ldi.h_front_porch,
		pinfo->ldi.h_pulse_width,
		pinfo->ldi.v_back_porch,
		pinfo->ldi.v_front_porch,
		pinfo->ldi.v_pulse_width,
		pinfo->pxl_clk_rate,
		pinfo->mipi.dsi_bit_clk_upt);

	if (pinfo->pxl_clk_rate_div > 1) {
			pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
	}

	return;
}

int switch_panel_mode(struct hisi_fb_data_type *hisifd, uint8_t mode_switch_to)
{
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;

	if (NULL == hisifd) {
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);
	if (NULL == pinfo) {
		return -EINVAL;
	}

	HISI_FB_DEBUG("+ .\n");

	get_porch_clk_para(pinfo, mode_switch_to);

	ret = clk_set_rate(hisifd->dss_pxl0_clk, pinfo->pxl_clk_rate);
	if (ret < 0) {
		HISI_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed, error=%d!\n",
			hisifd->index, pinfo->pxl_clk_rate, ret);
		get_porch_clk_para(pinfo, pinfo->current_mode);
		return ret;
	}

	get_vesa_dsc_para(pinfo, mode_switch_to);


	init_ifbc(hisifd);

	mipi_init(hisifd, hisifd->mipi_dsi0_base);

	if (mode_switch_to == MODE_10BIT_VIDEO_3X) {
		set_reg(hisifd->mipi_dsi0_base + MIPIDSI_MODE_CFG_OFFSET, 0x0, 1, 0);
	}

	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 7, 8);
		set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 4, 16);
		set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 24);
	}


	init_ldi(hisifd, true);

	dpe_interrupt_unmask(hisifd);

	HISI_FB_DEBUG("- .\n");
	return 0;
}

/*lint -save -e527 -e569*/
static int get_jdi_project_id_nt36860c(char *out)
{
	int i = 0;
	int ret = 0;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	uint32_t read_value[12] = {0};
	uint32_t tmp_value = 0;
	char project_id_reg[] = {0x29};

	struct dsi_cmd_desc project_id_cmd[] = {
		{DTYPE_GEN_READ1, 0, 50, WAIT_TYPE_US,
			sizeof(project_id_reg), project_id_reg},
	};

	char page22_reg[] = {0xff, 0x22};
	char mtp_load[] = {0xfb, 0x01};

	struct dsi_cmd_desc page22_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page22_reg), page22_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	char page10_reg[] = {0xff, 0x10};
	struct dsi_cmd_desc page10_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page10_reg), page10_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];

	if ((NULL == hisifd_primary) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}

	mipi_dsi_cmds_tx(page22_cmd, ARRAY_SIZE(page22_cmd), hisifd_primary->mipi_dsi0_base);
	mipi_dsi_lread_reg(read_value, project_id_cmd, 10, hisifd_primary->mipi_dsi0_base);
	mipi_dsi_cmds_tx(page10_cmd, ARRAY_SIZE(page10_cmd), hisifd_primary->mipi_dsi0_base);

	for (i = 0; i < (10 + 3) / 4; i++) {
		HISI_FB_INFO("0x%x\n", read_value[i]);
	}

	for (i = 0; i < 10; i++) {
		switch (i % 4) {
			case 0:
				tmp_value = read_value[i / 4] & 0xFF;
				break;
			case 1:
				tmp_value = (read_value[i / 4] >> 8) & 0xFF;
				break;
			case 2:
				tmp_value = (read_value[i / 4] >> 16) & 0xFF;
				break;
			case 3:
				tmp_value = (read_value[i / 4] >> 24) & 0xFF;
				break;
			default:
				break;
		}

		if (tmp_value == 0) {
			out[i] = tmp_value + '0';
		} else {
			out[i] = tmp_value;
		}
		HISI_FB_INFO("0x%x\n", out[i]);
	}
	return ret;
}

static int get_project_id_nt36870(char *out)
{
	int i = 0;
	int ret = 0;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	uint32_t read_value = 0;
	char project_id_reg[] = {0x58};

	struct dsi_cmd_desc project_id_cmd[] = {
		{DTYPE_DCS_READ, 0, 50, WAIT_TYPE_US,
			sizeof(project_id_reg), project_id_reg},
	};

	char page21_reg[] = {0xff, 0x21};
	char mtp_load[] = {0xfb, 0x01};

	struct dsi_cmd_desc page21_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page21_reg), page21_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	char page10_reg[] = {0xff, 0x10};
	struct dsi_cmd_desc page10_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page10_reg), page10_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if ((NULL == hisifd_primary) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}
	mipi_dsi_cmds_tx(page21_cmd, ARRAY_SIZE(page21_cmd), hisifd_primary->mipi_dsi0_base);
	for (i = 0; i < 10; i++) {
		char *data = project_id_cmd[0].payload;
		*data = 0x58 + i;
		ret = mipi_dsi_cmds_rx(&read_value, project_id_cmd, ARRAY_SIZE(project_id_cmd), hisifd_primary->mipi_dsi0_base);
		if (ret) {
			HISI_FB_ERR("read error\n");
			return -1;
		}
		if (read_value == 0) {
			out[i] = read_value + '0';
		} else {
			out[i] = read_value;
		}
		HISI_FB_INFO("project_id_cmd[0].payload[0] = 0x%x\n", project_id_cmd[0].payload[0]);
		HISI_FB_INFO("read_value = 0x%x\n", read_value);
		HISI_FB_INFO("+++++++out[%d] = 0x%x\n", i, out[i]);
	}
	mipi_dsi_cmds_tx(page10_cmd, ARRAY_SIZE(page10_cmd), hisifd_primary->mipi_dsi0_base);
	return ret;
}

static int get_project_id_on_udp(char *out)
{
	int ret = -1;

	if (NULL == out) {
		return -1;
	}

	if (g_mipi_lcd_name == JDI_2LANE_NT36860C) {
		ret = get_jdi_project_id_nt36860c(out);
	} else if (g_mipi_lcd_name == LG_2LANE_NT36870) {
		ret = get_project_id_nt36870(out);
	} else if (g_mipi_lcd_name == SHARP_2LANE_NT36870) {
		ret = get_project_id_nt36870(out);
	} else {
		HISI_FB_INFO("panel is not supported .\n");
	}
	return ret;
}

int hostprocessing_get_project_id_for_udp(char *out)
{
	static bool is_first_access = true;
	int ret = -1;
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];

	if ((NULL == hisifd) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}

	if (!is_first_access) {
		HISI_FB_DEBUG("you have accessed before .\n");
		return -1;
	}

	if (g_fastboot_enable_flag == 1) {
		ret = get_project_id_on_udp(out);
	} else {
		down(&hisifd->blank_sem);
		if (hisifd->panel_power_on) {
			hisifb_activate_vsync(hisifd);
			get_project_id_on_udp(out);
			hisifb_deactivate_vsync(hisifd);
		} else {
			HISI_FB_ERR("panel is off .\n");
		}
		up(&hisifd->blank_sem);
	}
	is_first_access = false;
	return ret;
}
EXPORT_SYMBOL(hostprocessing_get_project_id_for_udp);
/*lint -restore*/

