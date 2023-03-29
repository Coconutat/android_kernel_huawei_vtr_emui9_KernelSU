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

#include "hisi_overlay_utils.h"
#include "hisi_block_algorithm.h"
#include "hisi_overlay_cmdlist_utils.h"

static int hisi_check_two_wch_wb_finish(struct hisi_fb_data_type *hisifd, uint32_t last_cmdlist_idxs,
	int last_mctl_idx, uint32_t cmdlist_idxs, int mctl_idx)
{
	int ret = 0;
	int ret1 = 0;
	int ret_cmdlist_state = 0;
	uint32_t timeout_interval = 0;
	int times = 0;
	int i = 0;
	struct timeval tv0;
	struct timeval tv1;

	if (g_fpga_flag == 0) {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	} else {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	}

	hisifb_get_timestamp(&tv0);
REDO_ret:
	/*lint -e666 -e665*/
	ret = wait_event_interruptible_timeout(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH0],
		(hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0] == 1), msecs_to_jiffies(timeout_interval));
	/*lint +e666 +e665*/
	if (ret == -ERESTARTSYS) {
		if (times < 50) {
			times++;
			mdelay(10);
			goto REDO_ret;
		}
	}

REDO_ret1:
	/*lint -e666 -e665*/
	ret1 = wait_event_interruptible_timeout(hisifd->cmdlist_info->cmdlist_wb_wq[WB_TYPE_WCH1],
		(hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH1] == 1), msecs_to_jiffies(timeout_interval));
	/*lint +e666 +e665*/
	if (ret1 == -ERESTARTSYS) {
		if (times < 50) {
			times++;
			mdelay(10);
			goto REDO_ret1;
		}
	}
	hisifb_get_timestamp(&tv1);

	ret_cmdlist_state = hisi_cmdlist_check_cmdlist_state(hisifd, (cmdlist_idxs | last_cmdlist_idxs));
	if ((ret <= 0) || (ret1 <= 0) ||(ret_cmdlist_state < 0)) {
		HISI_FB_ERR("compose timeout, GLB_CPU_OFF_INTS=0x%x, ret=%d, ret1=%d, ret_rch_state=%d, diff =%d usecs,wb_done_wch0=%d, wb_done_wch1=%d\n",
			inp32(hisifd->dss_base + GLB_CPU_OFF_INTS), ret, ret1, ret_cmdlist_state, hisifb_timestamp_diff(&tv0, &tv1),
			hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH0], hisifd->cmdlist_info->cmdlist_wb_done[WB_TYPE_WCH1]);
		ret = -ETIMEDOUT;
	} else {
		/* remove last mctl ch & ov */
		hisi_remove_mctl_mutex(hisifd, last_mctl_idx, last_cmdlist_idxs);

		/* remove mctl ch & ov */
		hisi_remove_mctl_mutex(hisifd, mctl_idx, cmdlist_idxs);

		ret = 0;
	}

	for (i = 0; i < WB_TYPE_WCH0_WCH1; i++) {
		hisifd->cmdlist_info->cmdlist_wb_flag[i] = 0;
		hisifd->cmdlist_info->cmdlist_wb_done[i] = 0;
	}

	return ret;
}

static uint32_t hisi_get_block_info_layers_channels(dss_overlay_block_t *block_infos, uint32_t block_nums)
{
	dss_overlay_block_t *block_info;
	dss_layer_t *layer_info;
	uint32_t chn_idx = 0;
	int i = 0;
	int j = 0;

	if (block_infos == NULL) {
		return 0;
	}

	if (block_nums > HISI_DSS_OV_BLOCK_NUMS) {
		return 0;
	}

	for (i = 0; i < block_nums; i++) {
		block_info = &block_infos[i];
		for (j = 0; j < block_info->layer_nums; j++) {
			layer_info = &block_info->layer_infos[j];
			if ((layer_info->chn_idx >= DSS_CHN_MAX_DEFINE) || (layer_info->chn_idx < DSS_RCHN_D2)) {
				continue;
			}

			if (layer_info->img.shared_fd < 0 || (layer_info->img.phy_addr == 0 && layer_info->img.vir_addr == 0)) {
				continue;
			}

			if (!(chn_idx & (1 << layer_info->chn_idx))) {
				chn_idx |= 1 << layer_info->chn_idx;
			}
		}
	}

	return chn_idx;
}


bool hisi_offline_chn_include_online_chn(dss_overlay_block_t *offline_block_infos, uint32_t offline_block_nums)
{
	uint32_t offline_chn_idx;
	uint32_t online_pre_chn_idx;
	uint32_t online_pre_pre_chn_idx;
	struct hisi_fb_data_type *hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];

	if (offline_block_infos == NULL) {
		return false;
	}

	if (is_mipi_cmd_panel(hisifd_primary)) {
		return false;
	}

	offline_chn_idx = hisi_get_block_info_layers_channels(offline_block_infos, offline_block_nums);
	online_pre_chn_idx = hisi_get_block_info_layers_channels((dss_overlay_block_t *)(hisifd_primary->ov_req_prev.ov_block_infos_ptr),
															 hisifd_primary->ov_req_prev.ov_block_nums);
	if (offline_chn_idx & online_pre_chn_idx) {
		HISI_FB_ERR("offline_chn_idx(0x%x) be included at online_chn_idx(0x%x),\n", offline_chn_idx, online_pre_chn_idx);
		return true;
	}

	online_pre_pre_chn_idx = hisi_get_block_info_layers_channels((dss_overlay_block_t *)(hisifd_primary->ov_req_prev_prev.ov_block_infos_ptr),
																 hisifd_primary->ov_req_prev_prev.ov_block_nums);

	if (offline_chn_idx & online_pre_pre_chn_idx) {
		HISI_FB_INFO("offline_chn_idx(0x%x) be included at online_pre_pre_chn_idx(0x%x) \n", offline_chn_idx, online_pre_pre_chn_idx);
	}

	return false;
}

static void hisi_dss_offline_composer_on(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	int prev_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	fbi = hisifd->fbi;
	if (NULL == fbi) {
		HISI_FB_ERR("fbi is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];

	down(&hisifd->offline_composer_sr_sem);
	mutex_lock(&(hisifd_primary->vsync_ctrl.vsync_lock));
	hisifd_primary->vsync_ctrl.vsync_ctrl_offline_enabled = 1;
	mutex_unlock(&(hisifd_primary->vsync_ctrl.vsync_lock));

	prev_refcount = (hisifd->offline_composer_sr_refcount)++;
	if (!prev_refcount) {
		if (fbi->fbops->fb_blank)
			fbi->fbops->fb_blank(FB_BLANK_UNBLANK, fbi);
	}

	up(&hisifd->offline_composer_sr_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

static void hisi_dss_offline_composer_off(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	int new_refcount = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	fbi = hisifd->fbi;
	if (NULL == fbi) {
		HISI_FB_ERR("fbi is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];

	down(&hisifd->offline_composer_sr_sem);
	new_refcount = --(hisifd->offline_composer_sr_refcount);
	if (new_refcount < 0) {
		HISI_FB_ERR("dss new_refcount err");
	}

	if (!new_refcount) {
		if (fbi->fbops->fb_blank)
			fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, fbi);
	}

	mutex_lock(&(hisifd_primary->vsync_ctrl.vsync_lock));
	hisifd_primary->vsync_ctrl.vsync_ctrl_offline_enabled = 0;
	mutex_unlock(&(hisifd_primary->vsync_ctrl.vsync_lock));

	up(&hisifd->offline_composer_sr_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

static int hisi_add_clear_module_reg_node(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req,
	int cmdlist_idxs,
	bool enable_cmdlist,
	int *use_comm_mmbuf)
{
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	if ((cmdlist_idxs < 0) || (cmdlist_idxs >= HISI_DSS_CMDLIST_IDXS_MAX)) {
		HISI_FB_ERR("cmdlist_idxs is invalid");
		return -EINVAL;
	}


	ret = hisi_dss_module_init(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("hisi_dss_module_init failed! ret = %d\n", ret);
		goto err_return;
	}

	hisi_dss_prev_module_set_regs(hisifd, pov_req, cmdlist_idxs, enable_cmdlist, use_comm_mmbuf);

	return 0;

err_return:
	return ret;
}

static void hisi_offline_clear(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, int enable_cmdlist, uint32_t cmdlist_idxs, bool reset, bool debug)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL Point!\n");
		return ;
	}

	if (pov_req == NULL) {
		HISI_FB_ERR("pov_req is NULL Point!\n");
		return ;
	}

	hisi_drm_layer_offline_clear(hisifd, pov_req);

	pov_h_block_infos = (dss_overlay_block_t *)pov_req->ov_block_infos_ptr;
	if (pov_h_block_infos == NULL) {
		if (pov_req) {
			kfree(pov_req);
			pov_req = NULL;
		}
		HISI_FB_ERR("fb%d, invalid pov_h_block_infos!\n", hisifd->index);
		return ;
	}

	if (g_debug_ovl_offline_composer == 2) {
		dumpDssOverlay(hisifd, pov_req);
	}

	if (enable_cmdlist) {
		if (g_debug_ovl_cmdlist || debug) {
			dumpDssOverlay(hisifd, pov_req);
			hisi_cmdlist_dump_all_node(hisifd, pov_req, cmdlist_idxs);
		}

		if (reset) {
			if (g_debug_ovl_offline_composer_hold) {
				mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
			}

			hisi_cmdlist_config_reset(hisifd, pov_req, cmdlist_idxs);
		}

		hisi_cmdlist_del_node(hisifd, pov_req, cmdlist_idxs);
		hisi_dss_mmbuf_info_clear(hisifd, 0);
	}

	if (pov_req) {
		if (pov_h_block_infos) {
			kfree(pov_h_block_infos);
			pov_h_block_infos = NULL;
		}

		kfree(pov_req);
		pov_req = NULL;
	}
}

static int hisi_get_ov_data_from_user(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, void __user *argp)
{
	int ret = 0;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	uint32_t ov_block_size = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("user data is invalid\n");
		return -EINVAL;
	}

	ret = copy_from_user(pov_req, argp, sizeof(dss_overlay_t));
	if (ret) {
		HISI_FB_ERR("fb%d, copy_from_user failed!\n", hisifd->index);
		return -EINVAL;
	}

	pov_req->release_fence = -1;
	if ((pov_req->ov_block_nums <= 0) ||
		(pov_req->ov_block_nums > HISI_DSS_OV_BLOCK_NUMS)) {
		HISI_FB_ERR("fb%d, ov_block_nums(%d) is out of range!\n",
			hisifd->index, pov_req->ov_block_nums);
		return -EINVAL;
	}

	ov_block_size = pov_req->ov_block_nums * sizeof(dss_overlay_block_t);
	pov_h_block_infos = (dss_overlay_block_t *)kmalloc(ov_block_size, GFP_ATOMIC);
	if (!pov_h_block_infos) {
		HISI_FB_ERR("fb%d, pov_h_block_infos alloc failed!\n", hisifd->index);
		return -EINVAL;
	}
	memset(pov_h_block_infos, 0, ov_block_size);

	ret = copy_from_user(pov_h_block_infos, (dss_overlay_block_t *)pov_req->ov_block_infos_ptr,
		ov_block_size);
	if (ret) {
		HISI_FB_ERR("fb%d, dss_overlay_block_t copy_from_user failed!\n",
			hisifd->index);
		kfree(pov_h_block_infos);
		pov_h_block_infos = NULL;
		return -EINVAL;
	}

	ret = hisi_dss_check_userdata(hisifd, pov_req, pov_h_block_infos);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_dss_check_userdata failed!\n", hisifd->index);
		kfree(pov_h_block_infos);
		pov_h_block_infos = NULL;
		return -EINVAL;
	}
	pov_req->ov_block_infos_ptr = (uint64_t)pov_h_block_infos;

	return ret;
}

static int32_t hisi_get_wb_type(dss_overlay_t *pov_req)
{
	int32_t wb_type = -1;
	dss_wb_layer_t *wb_layer = NULL;

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}
	wb_layer = &(pov_req->wb_layer_infos[0]);

	if (pov_req->wb_layer_nums == MAX_DSS_DST_NUM) {
		wb_type = WB_TYPE_WCH0_WCH1;
	} else {
		if (wb_layer->chn_idx == DSS_WCHN_W0) {
			wb_type = WB_TYPE_WCH0;
		} else if (wb_layer->chn_idx == DSS_WCHN_W1) {
			wb_type = WB_TYPE_WCH1;
		} else if (wb_layer->chn_idx == DSS_WCHN_W2) {
			wb_type = WB_TYPE_WCH2;
		} else {
			HISI_FB_ERR("wb_layer->chn_idx(%d) no support\n", wb_layer->chn_idx);
		}
	}

	return wb_type;
}

static bool hisi_check_csc_config_needed(dss_overlay_t *pov_req_h_v)
{
	dss_overlay_block_t *pov_h_v_block = NULL;

	if (NULL == pov_req_h_v) {
		HISI_FB_ERR("pov_req_h_v is NULL");
		return false;
	}

	pov_h_v_block = (dss_overlay_block_t *)(pov_req_h_v->ov_block_infos_ptr);

	// check whether csc config needed or not
	if ((pov_h_v_block->layer_nums == 1) &&
		(isYUV(pov_h_v_block->layer_infos[0].img.format))) {
		if ((pov_req_h_v->wb_layer_nums == 1) &&
			isYUV(pov_req_h_v->wb_layer_infos[0].dst.format)) {
			return false;
		} else if ((pov_req_h_v->wb_layer_nums == 2) &&
			isYUV(pov_req_h_v->wb_layer_infos[0].dst.format) &&
			isYUV(pov_req_h_v->wb_layer_infos[1].dst.format)) {
			return false;
		} else {
			; //do nothing
		}
	}

	return true;
}

static int hisi_vactive0_start_wait(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req, int use_comm_mmbuf)
{
	struct hisi_fb_data_type *online_hisifd = NULL;
	struct timeval tv0;
	struct timeval tv1;
	int ret = 1;
	uint32_t timeout_interval = 0;
	int times = 0;
	uint32_t prev_vactive0_start = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	if (g_fpga_flag == 0) {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	} else {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	}

	if (use_comm_mmbuf & (1 << DSS_OVL0)) {
		online_hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	}

	if (use_comm_mmbuf & (1 << DSS_OVL1)) {
		online_hisifd = hisifd_list[EXTERNAL_PANEL_IDX];
	}

	if (online_hisifd && online_hisifd->panel_power_on) {
		hisifb_get_timestamp(&tv0);
		if (is_mipi_cmd_panel(online_hisifd) && (online_hisifd->vactive0_start_flag == 0)) {
		REDO_CMD:
			ret = wait_event_interruptible_timeout(online_hisifd->vactive0_start_wq, online_hisifd->vactive0_start_flag, msecs_to_jiffies(timeout_interval));
		} else if (!is_mipi_cmd_panel(online_hisifd)) {
			prev_vactive0_start = online_hisifd->vactive0_start_flag;
		REDO_VIDEO:
			ret = wait_event_interruptible_timeout(online_hisifd->vactive0_start_wq,(prev_vactive0_start != online_hisifd->vactive0_start_flag),
												   msecs_to_jiffies(timeout_interval));
		}

		if (ret == -ERESTARTSYS) {
			if (times < 50) {
				times++;
				mdelay(10);
				if (is_mipi_cmd_panel(online_hisifd)) goto REDO_CMD;
				if (is_mipi_video_panel(online_hisifd)) goto REDO_VIDEO;
			}
		}

		if (ret <= 0) {
			hisifb_get_timestamp(&tv1);

			HISI_FB_ERR("fb%d, wait_for vactive0_start_flag timeout!ret=%d, "
				"vactive0_start_flag=%d, TIMESTAMP_DIFF is %u us!\n",
				online_hisifd->index, ret, online_hisifd->vactive0_start_flag,
				hisifb_timestamp_diff(&tv0, &tv1));

			ret = -ETIMEDOUT;
		} else {
			ret = 0;
		}
	} else {
		ret = 0;
	}

	return ret;
}

static void hisi_tuning_offline_clk_rate(struct hisi_fb_data_type *hisifd)
{
	uint64_t offline_clk_rate;
	uint64_t online_clk_rate;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is null.\n");
		return;
	}

	if (IS_ERR(hisifd->dss_pri_clk)) {
		HISI_FB_ERR("fb%d: dss_pri_clk is null.\n", hisifd->index);
		return;
	}

	if (hisifd->need_tuning_clk) {
		offline_clk_rate = hisifd->dss_vote_cmd.dss_pri_clk_rate;
		online_clk_rate = hisifd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate;

		if (offline_clk_rate >= online_clk_rate) {
			clk_set_rate(hisifd->dss_pri_clk, offline_clk_rate);
		}

		HISI_FB_DEBUG("fb%d set online(%llu) -> offline(%llu).\n",
			hisifd->index, online_clk_rate, offline_clk_rate);
	}
}

static void hisi_recovery_online_clk_rate(struct hisi_fb_data_type *hisifd)
{
	uint64_t online_clk_rate;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is null.\n");
		return;
	}

	if (IS_ERR(hisifd->dss_pri_clk)) {
		HISI_FB_ERR("fb%d: dss_pri_clk is null.\n", hisifd->index);
		return;
	}

	if (hisifd->need_tuning_clk) {
		online_clk_rate = hisifd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate;

		clk_set_rate(hisifd->dss_pri_clk, online_clk_rate);

		hisifd->dss_vote_cmd.dss_pri_clk_rate = online_clk_rate;

		hisifd->need_tuning_clk = false;

		HISI_FB_DEBUG("fb%d recovery online(%llu)\n", hisifd->index, online_clk_rate);
	}
}

static uint32_t hisi_get_timeout_interval(void)
{
	uint32_t timeout_interval = 0;

	if (g_fpga_flag == 0) {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	} else {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	}
	return timeout_interval;
}

static int hisi_get_mctl_idx(int32_t wb_type, int ovl_idx)
{
	int mctl_idx = 0;

	if (WB_TYPE_WCH2 == wb_type) {
		mctl_idx = DSS_MCTL5;
	} else {
		mctl_idx = ovl_idx;
	}
	return mctl_idx;
}

static bool hisi_get_parallel_compose_flag(struct hisi_fb_data_type *hisifd, uint32_t wb_compose_type)
{
	bool parallel_compose_flag = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return false;
	}

	if ((hisifd->wb_info.to_be_continued == DSS_LAYER_PARALLEL_COMPOSE) && (wb_compose_type == DSS_WB_COMPOSE_PRIMARY)) {
		parallel_compose_flag = true;
	}
	return parallel_compose_flag;
}

static bool hisi_get_debug_flag(void)
{
	bool debug = false;

	if (g_debug_ovl_offline_composer_hold || g_debug_ovl_block_composer) {
		debug = true;
	}
	return debug;
}

static int hisi_ov_block_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req, dss_overlay_block_t *pov_h_block_infos, int32_t wb_type)
{
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	dss_wb_layer_t *wb_layer4block = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_overlay_t *pov_req_h_v = NULL;
	dss_overlay_block_t *pov_h_v_block = NULL;
	dss_rect_ltrb_t clip_rect;
	dss_rect_t aligned_rect;
	dss_rect_t wb_ov_block_rect;
	int enable_cmdlist = 0;
	int ret = 0;
	int k = 0;
	int i = 0;
	int m = 0;
	int n = 0;
	int block_num = 0;
	bool has_base = false;
	bool csc_needed = true;
	bool rdma_stretch_enable = false;
	bool last_block = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == pov_h_block_infos) {
		HISI_FB_ERR("pov_h_block_infos is NULL Pointer!\n");
		return -EINVAL;
	}

	wb_layer4block = &(pov_req->wb_layer_infos[0]);

	////////////////////////////////////////////////////////////////////////////
	// get vertical block ov
	pov_req_h_v = &(hisifd->ov_req);
	pov_req_h_v->ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos));

	enable_cmdlist = g_enable_ovl_cmdlist_offline;

	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);
		ret = get_ov_block_rect(pov_req, pov_h_block, wb_layer4block, &block_num, hisifd->ov_block_rects,false);//lint !e747
		if ((ret != 0) || (block_num == 0) || (block_num >= HISI_DSS_CMDLIST_BLOCK_MAX)) {
			HISI_FB_ERR("get_ov_block_rect failed! ret = %d, block_num[%d]\n", ret, block_num);
			ret = -1;
			return ret;
		}

		for (k = 0; k < block_num; k++) {
			has_base = false;

			ret = get_block_layers(pov_req, pov_h_block, *hisifd->ov_block_rects[k], pov_req_h_v);
			if (ret != 0) {
				HISI_FB_ERR("get_block_layers err ret = %d\n", ret);
				return ret;
			}

			ret = rect_across_rect(*hisifd->ov_block_rects[k], wb_layer4block->src_rect, &wb_ov_block_rect);
			if (ret == 0) {
				HISI_FB_ERR("no cross! ov_block_rects[%d]{%d %d %d %d}, wb src_rect{%d %d %d %d}\n", k,
					hisifd->ov_block_rects[k]->x, hisifd->ov_block_rects[k]->y,
					hisifd->ov_block_rects[k]->w, hisifd->ov_block_rects[k]->h,
					wb_layer4block->src_rect.x, wb_layer4block->src_rect.y,
					wb_layer4block->src_rect.w, wb_layer4block->src_rect.h);
				continue;
			}

			if (g_debug_ovl_offline_composer == 1) {
				HISI_FB_INFO("fb%d, get ov_req_h_v from kernel.\n", hisifd->index);
				dumpDssOverlay(hisifd, pov_req_h_v);
			}

			if (k == 0) {
				ret = hisi_dss_module_init(hisifd);
				if (ret != 0) {
					HISI_FB_ERR("hisi_dss_module_init failed! ret = %d\n", ret);
					return ret;
				}
			}

			pov_h_v_block = (dss_overlay_block_t *)(pov_req_h_v->ov_block_infos_ptr);

			// check whether csc config needed or not
			csc_needed = hisi_check_csc_config_needed(pov_req_h_v);

			hisi_dss_aif_handler(hisifd, pov_req_h_v, pov_h_v_block);

			ret = hisi_dss_ovl_base_config(hisifd, pov_req_h_v, pov_h_v_block,
				&wb_ov_block_rect, pov_req_h_v->ovl_idx, m);
			if (ret != 0) {
				HISI_FB_ERR("hisi_dss_ovl_init failed! ret = %d\n", ret);
				return ret;
			}

			for (i = 0; i < pov_h_v_block->layer_nums; i++) {
				layer = &(pov_h_v_block->layer_infos[i]);
				memset(&clip_rect, 0, sizeof(dss_rect_ltrb_t));
				memset(&aligned_rect, 0, sizeof(dss_rect_ltrb_t));
				rdma_stretch_enable = false;

				ret = hisi_ov_compose_handler(hisifd, pov_req_h_v, pov_h_v_block, layer, &wb_layer4block->dst_rect,
					&wb_ov_block_rect, &clip_rect, &aligned_rect, &rdma_stretch_enable, &has_base, csc_needed, enable_cmdlist);
				if (ret != 0) {
					HISI_FB_ERR("fb%d, hisi_ov_compose_handler failed! ret = %d\n", hisifd->index, ret);
					return ret;
				}
			}

			if (m == 0) {
				for (n = 0; n < pov_req_h_v->wb_layer_nums; n++) {
					wb_layer = &(pov_req_h_v->wb_layer_infos[n]);
					if (k == (block_num -1)) {
						last_block = true;
					}

					ret = hisi_wb_compose_handler(hisifd, pov_req_h_v, wb_layer,
						&wb_ov_block_rect, last_block, wb_type, csc_needed, enable_cmdlist);
					if (ret != 0) {
						HISI_FB_ERR("hisi_dss_write_back_config failed, ret = %d\n", ret);
						return ret;
					}
				}
			}

			ret = hisi_dss_mctl_ov_config(hisifd, pov_req, pov_req_h_v->ovl_idx, has_base, false);
			if (ret != 0) {
				HISI_FB_ERR("hisi_dss_mctl_config failed! ret = %d\n", ret);
				return ret;
			}

			ret = hisi_dss_ov_module_set_regs(hisifd, pov_req, pov_req_h_v->ovl_idx, enable_cmdlist, 0, 0, false);
			if (ret != 0) {
				HISI_FB_ERR("hisi_dss_module_config failed! ret = %d\n", ret);
				return ret;
			}

			if (k == g_debug_ovl_offline_block_num) {
				break;
			}

			if (k < (block_num - 1)) {
				ret = hisi_dss_module_init(hisifd);
				if (ret != 0) {
					HISI_FB_ERR("hisi_dss_module_init failed! ret = %d\n", ret);
					return ret;
				}
			}
		}
	}
	return ret;
}

int hisi_ov_offline_play(struct hisi_fb_data_type *hisifd, void __user *argp)
{
	dss_overlay_t *pov_req = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	int use_comm_mmbuf = 0;
	int ret = 0;
	int ret_cmdlist_state = 0;
	int times = 0;
	uint32_t cmdlist_idxs = 0;
	int enable_cmdlist = 0;
	int mctl_idx = 0;
	int32_t wb_type = 0;
	uint32_t wb_compose_type = 0;
	uint32_t timediff = 0;
	struct timeval tv0;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	struct timeval tv4;
	struct timeval tv5;
	bool reset = false;
	bool debug = false;
	struct list_head lock_list;
	uint32_t timeout_interval = 0;
	bool parallel_compose_flag = false;
	static uint32_t s_dmd_offline_compose_timeout_count = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&lock_list);

	timeout_interval = hisi_get_timeout_interval();

	if (g_debug_ovl_offline_composer)
		HISI_FB_INFO("+.\n");

	if (g_debug_ovl_offline_composer_timediff & 0x1)
		hisifb_get_timestamp(&tv0);

	hisi_dss_offline_composer_on(hisifd);

	hisi_tuning_offline_clk_rate(hisifd);

	down(&(hisifd->cmdlist_info->cmdlist_wb_common_sem));

	////////////////////////////////////////////////////////////////////////////
	// get horizontal block ov
	pov_req = (dss_overlay_t *)kmalloc(sizeof(dss_overlay_t), GFP_ATOMIC);
	if (!pov_req) {
		ret = -1;
		HISI_FB_ERR("fb%d, dss_overlay_t alloc failed!\n", hisifd->index);
		goto err_return_sem0;
	}

	hisifb_dss_overlay_info_init(pov_req);
	ret = hisi_get_ov_data_from_user(hisifd, pov_req, argp);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisi_get_ov_data_from_user failed! ret = %d\n", hisifd->index, ret);
		kfree(pov_req);
		pov_req = NULL;
		goto err_return_sem0;
	}

	if (g_debug_ovl_offline_composer == 1) {
		HISI_FB_INFO("fb%d, get ov_req from user.\n", hisifd->index);
		dumpDssOverlay(hisifd, pov_req);
	}

	ret = hisifb_offline_layerbuf_lock(hisifd, pov_req, &lock_list);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, hisifb_offline_layerbuf_lock failed! ret=%d\n", hisifd->index, ret);
		goto err_return_sem0;
	}

	hisifd->mmbuf_info = hisi_dss_mmbuf_info_get(hisifd, 0);

	enable_cmdlist = g_enable_ovl_cmdlist_offline;

	if (enable_cmdlist) {
		hisifd->set_reg = hisi_cmdlist_set_reg;

		hisi_cmdlist_data_get_offline(hisifd, pov_req);

		ret = hisi_cmdlist_get_cmdlist_idxs(pov_req, NULL, &cmdlist_idxs);
		if (ret != 0) {
			HISI_FB_ERR("hisi_cmdlist_get_cmdlist_idxs failed! ret = %d\n", ret);
			goto err_return_sem0;
		}

	} else {
		hisifd->set_reg = hisifb_set_reg;
	}

	wb_compose_type = pov_req->wb_compose_type;
	wb_type = hisi_get_wb_type(pov_req);
	if ((wb_type < WB_TYPE_WCH0) || (wb_type >= WB_TYPE_MAX)) {
		HISI_FB_ERR("fb%d, hisi_get_wb_type failed!\n", hisifd->index);
		ret = -1;
		goto err_return_sem0;
	}

	mctl_idx = hisi_get_mctl_idx(wb_type, pov_req->ovl_idx);

	parallel_compose_flag = hisi_get_parallel_compose_flag(hisifd, wb_compose_type);

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);

	if (hisi_offline_chn_include_online_chn(pov_h_block_infos, pov_req->ov_block_nums)) {
		HISI_FB_ERR("fb%d, hisi_offline_ch_include_online_ch failed!,wb_compose_type=%d\n", hisifd->index, pov_req->wb_compose_type);
		ret = -1;
		goto err_return_sem0;
	}

	hisi_drm_layer_offline_config(hisifd, pov_req);

	ret = hisi_ov_block_config(hisifd, pov_req, pov_h_block_infos, wb_type);
	if (ret != 0)
		goto err_return_sem0;

	ret = hisi_add_clear_module_reg_node(hisifd, pov_req, cmdlist_idxs, enable_cmdlist, &use_comm_mmbuf);
	if (ret != 0) {
		HISI_FB_ERR("hisi_add_clear_module_reg_node failed! ret = %d\n", ret);
		goto err_return_sem0;
	}

	if (enable_cmdlist) {
		//Next frame
		//wmb();
		hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);
	}

	hisifb_buf_sync_handle_offline(hisifd, pov_req);

	up(&(hisifd->cmdlist_info->cmdlist_wb_common_sem));

	down(&(hisifd->cmdlist_info->cmdlist_wb_sem[wb_type]));

	hisifd->cmdlist_info->cmdlist_wb_flag[wb_type] = 1;
	hisifd->cmdlist_info->cmdlist_wb_done[wb_type] = 0;

	if (enable_cmdlist) {
		if (g_debug_ovl_offline_composer_timediff & 0x2)
			hisifb_get_timestamp(&tv2);

		//wait primary vactive_start int
		if (use_comm_mmbuf) {
			ret = hisi_vactive0_start_wait(hisifd, pov_req, use_comm_mmbuf);
			if (ret != 0) {
				HISI_FB_ERR("fb%d, hisi_vactive0_start_wait failed!\n", hisifd->index);
				goto err_return_sem1;
			}
		}

		if (g_debug_ovl_offline_composer_timediff & 0x2) {
			hisifb_get_timestamp(&tv3);
			timediff = hisifb_timestamp_diff(&tv2, &tv3);
			if (timediff >= g_debug_ovl_offline_composer_time_threshold)
				HISI_FB_ERR("wb_compose_type=%d, OFFLINE_VACTIVE_TIMEDIFF is %u us!\n", wb_compose_type, timediff);
		}

		if (parallel_compose_flag) {
			hisi_cmdlist_exec(hisifd, cmdlist_idxs | hisifd->wb_info.cmdlist_idxs);
			hisi_cmdlist_config_start(hisifd, hisifd->wb_info.mctl_idx, hisifd->wb_info.cmdlist_idxs, hisifd->wb_info.wb_compose_type);
			hisi_cmdlist_config_start(hisifd, mctl_idx, cmdlist_idxs, wb_compose_type);
		} else {
			if (pov_req->to_be_continued == DSS_LAYER_SERIAL_COMPOSE) {
				hisi_cmdlist_exec(hisifd, cmdlist_idxs);
				hisi_cmdlist_config_start(hisifd, mctl_idx, cmdlist_idxs, wb_compose_type);
			}
		}
	}

	if (pov_req->to_be_continued == DSS_LAYER_PARALLEL_COMPOSE) {
		goto return_parallel_compose;
	}

	if (parallel_compose_flag) {
		ret = hisi_check_two_wch_wb_finish(hisifd, hisifd->wb_info.cmdlist_idxs, hisifd->wb_info.mctl_idx, cmdlist_idxs, mctl_idx);
		if (ret < 0) {
			reset = true;
		}
	} else {
		hisifb_get_timestamp(&tv4);
	REDO:
		ret = wait_event_interruptible_timeout(hisifd->cmdlist_info->cmdlist_wb_wq[wb_type],
			(hisifd->cmdlist_info->cmdlist_wb_done[wb_type] == 1),
			msecs_to_jiffies(timeout_interval));

		if (ret == -ERESTARTSYS) {
			if (times < 50) {
				times++;
				mdelay(10);
				goto REDO;
			}
		}
		hisifb_get_timestamp(&tv5);

		hisifd->cmdlist_info->cmdlist_wb_flag[wb_type] = 0;
		hisifd->cmdlist_info->cmdlist_wb_done[wb_type] = 0;

		ret_cmdlist_state = hisi_cmdlist_check_cmdlist_state(hisifd, cmdlist_idxs);
		if ((ret <= 0) ||(ret_cmdlist_state < 0)) {
			HISI_FB_ERR("compose timeout, use_comm_mmbuf=%d, wb_compose_type=%d, GLB_CPU_OFF_INTS=0x%x,"
				"ret=%d, ret_rch_state=%d, diff =%d usecs!\n",
				use_comm_mmbuf, wb_compose_type, inp32(hisifd->dss_base + GLB_CPU_OFF_INTS), ret, ret_cmdlist_state,
				hisifb_timestamp_diff(&tv4, &tv5));
			s_dmd_offline_compose_timeout_count++;
			if (s_dmd_offline_compose_timeout_count >= OFFLINE_COMPOSE_TIMEOUT_EXPECT_COUNT) {
				if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
					dsm_client_record(lcd_dclient, "compose timeout, use_comm_mmbuf = %d, wb_compose_type = %d, GLB_CPU_OFF_INTS = 0x%x,"
						"ret = %d, ret_rch_state = %d, diff = %d usecs!\n",
						use_comm_mmbuf, wb_compose_type, inp32(hisifd->dss_base + GLB_CPU_OFF_INTS), ret, ret_cmdlist_state,
						hisifb_timestamp_diff(&tv4, &tv5));
					dsm_client_notify(lcd_dclient, DSM_LCD_OFFLINE_COMPOSE_TIMEOUT_NO);
				}
				s_dmd_offline_compose_timeout_count = 0;
			}
			ret = -ETIMEDOUT;
			reset = true;

			debug = hisi_get_debug_flag();
		} else {
			s_dmd_offline_compose_timeout_count = 0;
			/* remove mctl ch & ov */
			hisi_remove_mctl_mutex(hisifd, mctl_idx, cmdlist_idxs);
			ret = 0;
		}
	}

err_return_sem1:
	hisifb_offline_layerbuf_unlock(hisifd, &lock_list);
	if (parallel_compose_flag) {
		hisi_offline_clear(hisifd, pov_req, enable_cmdlist, hisifd->wb_info.cmdlist_idxs | cmdlist_idxs, reset, debug);
		memset(&hisifd->wb_info, 0, sizeof(dss_wb_info_t));
	} else {
		hisi_offline_clear(hisifd, pov_req, enable_cmdlist, cmdlist_idxs, reset, debug);
	}
	up(&(hisifd->cmdlist_info->cmdlist_wb_sem[wb_type]));

	hisi_recovery_online_clk_rate(hisifd);

	hisi_dss_offline_composer_off(hisifd);

	if (g_debug_ovl_offline_composer_timediff & 0x1) {
		hisifb_get_timestamp(&tv1);
		timediff = hisifb_timestamp_diff(&tv0, &tv1);
		if (timediff >= g_debug_ovl_offline_composer_time_threshold) {
			HISI_FB_ERR("use_comm_mmbuf=%d, wb_compose_type=%d, wb_type=%d, OFFLINE_TIMEDIFF is %u us!\n",
				use_comm_mmbuf, wb_compose_type, wb_type, timediff);
		}
	}

	if (g_debug_ovl_offline_composer)
		HISI_FB_INFO("-.\n");

	return ret;

err_return_sem0:
	debug = hisi_get_debug_flag();
	hisifb_offline_layerbuf_unlock(hisifd, &lock_list);

	reset = true;
	if (pov_req) {
		hisi_offline_clear(hisifd, pov_req, enable_cmdlist, cmdlist_idxs, reset, debug);
	}
	up(&(hisifd->cmdlist_info->cmdlist_wb_common_sem));

	hisi_recovery_online_clk_rate(hisifd);

	hisi_dss_offline_composer_off(hisifd);

	if (g_debug_ovl_offline_composer)
		HISI_FB_INFO("-.\n");

	return ret;

return_parallel_compose:
	hisifb_offline_layerbuf_unlock(hisifd, &lock_list);
	if (pov_req) {
		if (pov_h_block_infos) {
			kfree(pov_h_block_infos);
			pov_h_block_infos = NULL;
		}

		kfree(pov_req);
		pov_req = NULL;
	}

	hisifd->wb_info.to_be_continued = DSS_LAYER_PARALLEL_COMPOSE;
	hisifd->wb_info.cmdlist_idxs = cmdlist_idxs;
	hisifd->wb_info.mctl_idx = mctl_idx;
	hisifd->wb_info.wb_compose_type = wb_compose_type;
	up(&(hisifd->cmdlist_info->cmdlist_wb_sem[wb_type]));

	hisi_recovery_online_clk_rate(hisifd);

	hisi_dss_offline_composer_off(hisifd);

	if (g_debug_ovl_offline_composer)
		HISI_FB_INFO("-.\n");

	return ret;
}
