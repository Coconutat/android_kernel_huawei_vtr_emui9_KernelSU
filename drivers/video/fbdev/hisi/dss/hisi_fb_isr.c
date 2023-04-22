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

#include "hisi_fb.h"
#include "hisi_overlay_utils.h"


/*******************************************************************************
** handle isr
*/
static bool need_panel_mode_swtich(struct hisi_fb_data_type *hisifd, uint32_t isr_s2)
{

	if (hisifd->panel_mode_switch_isr_handler
		&& hisifd->panel_info.panel_mode_swtich_support
		&& (hisifd->panel_info.mode_switch_to != hisifd->panel_info.current_mode)) {
		if (!(isr_s2 & BIT_LDI_UNFLOW)) {
			return true;
		}
	}

	return false;
}
static bool need_dsi_bit_clk_upt(struct hisi_fb_data_type *hisifd)
{
	if (hisifd->mipi_dsi_bit_clk_upt_isr_handler
		&& hisifd->panel_info.dsi_bit_clk_upt_support
		&& hisifd->panel_info.mipi.dsi_bit_clk_upt
		&& (hisifd->panel_info.mipi.dsi_bit_clk_upt != hisifd->panel_info.mipi.dsi_bit_clk)) {
		return true;
	}

	return false;
}

static void hisifb_display_effect_flags_config(struct hisi_fb_data_type *hisifd)
{
	if (hisifd->color_temperature_flag > 0) {
		hisifd->color_temperature_flag--;
	}

	if (hisifd->display_effect_flag > 0) {
		hisifd->display_effect_flag--;
	}

	if (hisifd->panel_info.xcc_set_in_isr_support
		&& hisifd->xcc_coef_set == 1) {
		dpe_set_xcc_cscValue(hisifd);
		hisifd->xcc_coef_set = 0;
	}
}

static void dss_pdp_isr_vactive0_end_handle(struct hisi_fb_data_type *hisifd, uint32_t isr_s2, struct hisifb_secure *secure_ctrl)
{
	uint32_t temp = 0;

	hisifd->vactive0_end_flag = 1;

	hisifb_display_effect_flags_config(hisifd);

	if (PARA_UPDT_DOING == hisifd->pipe_clk_ctrl.pipe_clk_updt_state) {
		;
	} else if (need_panel_mode_swtich(hisifd, isr_s2)) {
		if ((inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CTRL) & 0x1) == 0) {
			hisifd->panel_mode_switch_isr_handler(hisifd, hisifd->panel_info.mode_switch_to);
		}
		hisifd->panel_info.mode_switch_state = PARA_UPDT_END;
	} else if (need_dsi_bit_clk_upt(hisifd)) {
		if (!(isr_s2 & BIT_VACTIVE0_START)) {
			hisifd->mipi_dsi_bit_clk_upt_isr_handler(hisifd);
		}
	} else if (hisifd->panel_info.fps_updt_support
		&& hisifd->fps_upt_isr_handler
		&& (hisifd->panel_info.fps_updt != hisifd->panel_info.fps || hisifd->panel_info.fps_updt_force_update)) {
		hisifd->fps_upt_isr_handler(hisifd);
	}

	if (secure_ctrl->notify_secure_switch) {
		secure_ctrl->notify_secure_switch(hisifd);
	}

	if (g_err_status & DSS_PDP_LDI_UNDERFLOW) {
		temp = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_DBG_CNT);
		HISI_FB_INFO("fb%d, BIT_VACTIVE0_END: frame_no=%d, dpp_dbg =0x%x\n",
			hisifd->index, hisifd->ov_req.frame_no, temp);
		g_err_status &= ~DSS_PDP_LDI_UNDERFLOW;
	}

	return;
}

static void dss_pdp_isr_vactive0_start_handle(struct hisi_fb_data_type *hisifd, uint32_t isr_s2)
{
	uint32_t temp = 0;

	if (hisifd->ov_vactive0_start_isr_handler) {
		hisifd->ov_vactive0_start_isr_handler(hisifd);
	}

	if (g_err_status & DSS_PDP_LDI_UNDERFLOW) {
		temp = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_DBG_CNT);
		HISI_FB_INFO("fb%d, BIT_VACTIVE0_START: frame_no=%d, dpp_dbg=0x%x\n",
			hisifd->index, hisifd->ov_req.frame_no, temp);
	}

	if ((PARA_UPDT_NEED == hisifd->pipe_clk_ctrl.pipe_clk_updt_state)
		|| (PARA_UPDT_DOING == hisifd->pipe_clk_ctrl.pipe_clk_updt_state)) {
		disable_ldi(hisifd);
		hisifd->pipe_clk_ctrl.pipe_clk_updt_state = PARA_UPDT_DOING;

		if (hisifd->pipe_clk_updt_isr_handler) {
			hisifd->pipe_clk_updt_isr_handler(hisifd);
		}
	}
	if (need_panel_mode_swtich(hisifd, isr_s2)) {
		disable_ldi(hisifd);
		hisifd->panel_info.mode_switch_state = PARA_UPDT_DOING;
	}

	return;
}

irqreturn_t dss_pdp_isr(int irq, void *ptr)
{
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2 = 0;
	uint32_t isr_s2_dpp = 0;
	uint32_t isr_s2_smmu = 0;
	uint32_t mask = 0;
	uint32_t isr_te_vsync = 0;
	uint32_t temp = 0;
	struct timeval tv;
	struct hisifb_secure *secure_ctrl = NULL;
	dss_module_reg_t *dss_module = NULL;
	struct hisifb_video_idle_ctrl *video_idle_ctrl = NULL;

	hisifd = (struct hisi_fb_data_type *)ptr;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return IRQ_NONE;
	}
	secure_ctrl = &(hisifd->secure_ctrl);
	dss_module = &(hisifd->dss_module);
	video_idle_ctrl = &(hisifd->video_idle_ctrl);

	isr_s1 = inp32(hisifd->dss_base + GLB_CPU_PDP_INTS);
	isr_s2 = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS);
	isr_s2_dpp = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_INTS);
	isr_s2_smmu = inp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_INTS, isr_s2_dpp);
	outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, isr_s2);
	outp32(hisifd->dss_base + GLB_CPU_PDP_INTS, isr_s1);

	isr_s1 &= ~(inp32(hisifd->dss_base + GLB_CPU_PDP_INT_MSK));
	isr_s2 &= ~(inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK));
	isr_s2_dpp &= ~(inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_INT_MSK));

	if (is_mipi_cmd_panel(hisifd)) {
		isr_te_vsync = BIT_LCD_TE0_PIN;
	} else {
		isr_te_vsync = BIT_VSYNC;
	}


	if (isr_s2 & BIT_VACTIVE0_END) {
		dss_pdp_isr_vactive0_end_handle(hisifd, isr_s2, secure_ctrl);
	}

	if (isr_s2 & BIT_VACTIVE0_START) {
		dss_pdp_isr_vactive0_start_handle(hisifd, isr_s2);
	}

	if (isr_s2 & isr_te_vsync) {
		if (hisifd->vsync_isr_handler) {
			hisifd->vsync_isr_handler(hisifd);
		}

		if (hisifd->buf_sync_signal) {
			hisifd->buf_sync_signal(hisifd);
		}

		if (g_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_PDP_SMMU_ERR | DSS_SDP_SMMU_ERR)) {
			temp = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_DBG_CNT);
			g_err_status &= ~DSS_PDP_LDI_UNDERFLOW;
			HISI_FB_INFO("isr_te_vsync:frame_no = %d,dpp_dbg = 0x%x!\n", hisifd->ov_req.frame_no, temp);
		}

		if (g_debug_ldi_underflow) {
			hisifb_get_timestamp(&tv);
			HISI_FB_INFO("isr_te_vsync:frame_no = %d,isr_s2 = 0x%x\n", hisifd->ov_req.frame_no, isr_s2);
		}
	}

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(hisifd->dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, mask);

		if (g_debug_ldi_underflow_clear) {
			if (is_mipi_cmd_panel(hisifd)) {
				if (g_ldi_data_gate_en == 0) {
					if (hisifd->ldi_underflow_wq) {
						hisifb_pipe_clk_set_underflow_flag(hisifd, true);
						disable_ldi(hisifd);
						queue_work(hisifd->ldi_underflow_wq, &hisifd->ldi_underflow_work);
					}
				}
			} else {
				if (hisifd->ldi_underflow_wq) {
					hisifb_pipe_clk_set_underflow_flag(hisifd, true);
					disable_ldi(hisifd);
					queue_work(hisifd->ldi_underflow_wq, &hisifd->ldi_underflow_work);
				}
			}
		}

		if (g_debug_ldi_underflow) {
			if (g_debug_ovl_online_composer) {
				if (hisifd->dss_debug_wq)
					queue_work(hisifd->dss_debug_wq, &hisifd->dss_debug_work);
			}
		}

		g_err_status |= DSS_PDP_LDI_UNDERFLOW;

		if (hisifd->ldi_data_gate_en == 0) {
			//FIXME:
			temp = inp32(hisifd->dss_base + DSS_DPP_OFFSET + DPP_DBG_CNT);
			HISI_FB_INFO("ldi underflow! frame_no = %d,dpp_dbg = 0x%x, vactive0_start_flag = %d!\n",
				hisifd->ov_req.frame_no,temp, hisifd->vactive0_start_flag);

			if (hisifd->dss_underflow_debug_workqueue && !g_fake_lcd_flag)
				queue_work(hisifd->dss_underflow_debug_workqueue, &hisifd->dss_underflow_debug_work);
		}
	}

		if (isr_s2 & BIT_FRM_END) {
			if (hisifd->panel_info.hiace_support && hisifd->hiace_end_wq) {
				queue_work(hisifd->hiace_end_wq, &hisifd->hiace_end_work);
			}
		}

	if (isr_s2_dpp & BIT_CE_END_IND) {
		if (hisifd->panel_info.acm_ce_support && hisifd->dpp_ce_end_wq) {
			queue_work(hisifd->dpp_ce_end_wq, &hisifd->dpp_ce_end_work);
		}
	}

	return IRQ_HANDLED;
}

irqreturn_t dss_sdp_isr(int irq, void *ptr)
{
	char __iomem *ldi_base;
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2 = 0;
	uint32_t isr_s2_smmu = 0;
	uint32_t mask = 0;

	hisifd = (struct hisi_fb_data_type *)ptr;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return IRQ_NONE;
	}

	ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;

	isr_s1 = inp32(hisifd->dss_base + GLB_CPU_SDP_INTS);
	isr_s2 = inp32(ldi_base + LDI_CPU_ITF_INTS);
	isr_s2_smmu = inp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(ldi_base + LDI_CPU_ITF_INTS, isr_s2);
	outp32(hisifd->dss_base + GLB_CPU_SDP_INTS, isr_s1);

	isr_s1 &= ~(inp32(hisifd->dss_base + GLB_CPU_SDP_INT_MSK));
	isr_s2 &= ~(inp32(ldi_base + LDI_CPU_ITF_INT_MSK));

	if (isr_s2 & BIT_VACTIVE0_END) {
		hisifd->vactive0_end_flag = 1;
	}

	if (isr_s2 & BIT_VACTIVE0_START) {
		if (hisifd->ov_vactive0_start_isr_handler)
			hisifd->ov_vactive0_start_isr_handler(hisifd);
	}

	if (isr_s2 & BIT_VSYNC) {
		if (hisifd->vsync_isr_handler) {
			hisifd->vsync_isr_handler(hisifd);
		}

		if (hisifd->buf_sync_signal) {
			hisifd->buf_sync_signal(hisifd);
		}
	}

	if (isr_s2 & BIT_LDI_UNFLOW) {
		mask = inp32(ldi_base + LDI_CPU_ITF_INT_MSK);
		mask |= BIT_LDI_UNFLOW;
		outp32(ldi_base + LDI_CPU_ITF_INT_MSK, mask);
		if (g_debug_ldi_underflow_clear) {
			if (is_mipi_cmd_panel(hisifd)) {
				if (g_ldi_data_gate_en == 0) {
					if (hisifd->ldi_underflow_wq) {
						disable_ldi(hisifd);
						queue_work(hisifd->ldi_underflow_wq, &hisifd->ldi_underflow_work);
					}
				}
			} else {
				if (hisifd->ldi_underflow_wq) {
					disable_ldi(hisifd);
					queue_work(hisifd->ldi_underflow_wq, &hisifd->ldi_underflow_work);

					if (hisifd->dss_underflow_debug_workqueue)
						queue_work(hisifd->dss_underflow_debug_workqueue, &hisifd->dss_underflow_debug_work);
				}
			}
		}

		if (g_debug_ldi_underflow) {
			if (g_debug_ovl_online_composer) {
				if (hisifd->dss_debug_wq)
					queue_work(hisifd->dss_debug_wq, &hisifd->dss_debug_work);
			}
		}

		g_err_status |= DSS_SDP_LDI_UNDERFLOW;

		HISI_FB_INFO("ldi1 underflow! frame_no = %d!\n", hisifd->ov_req.frame_no);
	}

	return IRQ_HANDLED;
}

irqreturn_t dss_adp_isr(int irq, void *ptr)
{
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t isr_s1 = 0;
	uint32_t isr_s2_smmu = 0;
	uint32_t isr_s3_copybit = 0;

	hisifd = (struct hisi_fb_data_type *)ptr;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return IRQ_NONE;
	}

	isr_s1 = inp32(hisifd->dss_base + GLB_CPU_OFF_INTS);
	isr_s2_smmu = inp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTSTAT_NS);

	outp32(hisifd->dss_base + DSS_SMMU_OFFSET + SMMU_INTCLR_NS, isr_s2_smmu);
	outp32(hisifd->dss_base + GLB_CPU_OFF_INTS, isr_s1);

	isr_s1 &= ~(inp32(hisifd->dss_base + GLB_CPU_OFF_INT_MSK));
	isr_s3_copybit = inp32(hisifd->dss_base + GLB_CPU_OFF_CAM_INTS);
	outp32(hisifd->dss_base + GLB_CPU_OFF_CAM_INTS, isr_s3_copybit);
	isr_s3_copybit &= ~(inp32(hisifd->dss_base + GLB_CPU_OFF_CAM_INT_MSK));

	if (isr_s1 & BIT_OFF_WCH0_INTS) {
		if (hisifd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH0] == 1) {
			hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0] = 1;
			wake_up_interruptible_all(&(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH0]));
		}
	}

	if (isr_s1 & BIT_OFF_WCH1_INTS) {
		if (hisifd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH1] == 1) {
			hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH1] = 1;
			wake_up_interruptible_all(&(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH1]));
		}
	}

	if (isr_s1 & BIT_OFF_WCH0_WCH1_FRM_END_INT) {
		if (hisifd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH0_WCH1] == 1) {
			hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0_WCH1] = 1;
			wake_up_interruptible_all(&(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH0_WCH1]));
		}
	}

	if (isr_s3_copybit & BIT_OFF_CAM_WCH2_FRMEND_INTS) {
		if (hisifd->copybit_info->copybit_flag == 1) {
			hisifd->copybit_info->copybit_done = 1;
			wake_up_interruptible_all(&(hisifd->copybit_info->copybit_wq));
		}

		if (hisifd->cmdlist_info->cmdlist_wb_flag[WB_TYPE_WCH2] == 1) {
			hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH2] = 1;
			wake_up_interruptible_all(&(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH2]));
		}
	}


	return IRQ_HANDLED;
}
/*lint -e838 -e730 -e732 -e502 -e715*/
irqreturn_t dss_mdc_isr(int irq, void *ptr)
{
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t isr_s1 = 0;

	hisifd = (struct hisi_fb_data_type *)ptr;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return IRQ_NONE;
	}

	isr_s1 = inp32(hisifd->media_common_base + GLB_CPU_OFF_INTS);
	outp32(hisifd->media_common_base + GLB_CPU_OFF_INTS, isr_s1);

	isr_s1 &= ~(inp32(hisifd->media_common_base + GLB_CPU_OFF_INT_MSK));
	if (isr_s1 & BIT_OFF_WCH1_INTS) {
		if (hisifd->media_common_info->mdc_flag == 1) {
				hisifd->media_common_info->mdc_done = 1;
				wake_up_interruptible_all(&(hisifd->media_common_info->mdc_wq));
		}
	}

	return IRQ_HANDLED;
}
/*lint +e838 +e730 +e732 +e502 +e715*/
