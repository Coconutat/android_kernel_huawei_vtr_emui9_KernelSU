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

static void hisi_dss_copybit_composer_on(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;
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

	down(&hisifd->offline_composer_sr_sem);

	prev_refcount = (hisifd->offline_composer_sr_refcount)++;
	if (!prev_refcount) {
		if (fbi->fbops->fb_blank)
			fbi->fbops->fb_blank(FB_BLANK_UNBLANK, fbi);
	}

	up(&hisifd->offline_composer_sr_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

static void hisi_dss_copybit_composer_off(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;
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

	down(&hisifd->offline_composer_sr_sem);
	new_refcount = --(hisifd->offline_composer_sr_refcount);
	if (new_refcount < 0) {
		HISI_FB_ERR("dss new_refcount err");
	}

	if (!new_refcount) {
		if (fbi->fbops->fb_blank)
			fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, fbi);
	}

	up(&hisifd->offline_composer_sr_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

static int hisi_add_clear_module_reg_node(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req,
	int cmdlist_idxs,
	bool enable_cmdlist)
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

	hisi_dss_prev_module_set_regs(hisifd, pov_req, cmdlist_idxs, enable_cmdlist, NULL);

	return 0;

err_return:
	return ret;
}

static void hisi_copybit_clear(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, uint32_t cmdlist_idxs, bool reset, bool debug)
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

	pov_h_block_infos = (dss_overlay_block_t *)pov_req->ov_block_infos_ptr;
	if (pov_h_block_infos == NULL) {
		if (pov_req) {
			kfree(pov_req);
			pov_req = NULL;
		}
		HISI_FB_ERR("fb%d, invalid pov_h_block_infos!\n", hisifd->index);
		return ;
	}

	if (g_debug_ovl_cmdlist || debug) {
		dumpDssOverlay(hisifd, pov_req);
		hisi_cmdlist_dump_all_node(hisifd, pov_req, cmdlist_idxs);
	}

	if (reset) {
		if (g_debug_ovl_copybit_composer_hold) {
			mdelay(HISI_DSS_COMPOSER_HOLD_TIME);
		}

		hisi_cmdlist_config_reset(hisifd, pov_req, cmdlist_idxs);
	}

	hisi_cmdlist_del_node(hisifd, pov_req, cmdlist_idxs);

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
		}
	}

	return true;
}

int hisi_ov_copybit_play(struct hisi_fb_data_type *hisifd, void __user *argp)
{
	dss_overlay_t *pov_req = NULL;
	dss_overlay_t *pov_req_h_v = NULL;
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_overlay_block_t *pov_h_v_block = NULL;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	dss_wb_layer_t *wb_layer4block = NULL;
	dss_rect_ltrb_t clip_rect;
	dss_rect_t aligned_rect;
	bool rdma_stretch_enable = false;
	int i = 0;
	int k = 0;
	int n = 0;
	int m = 0;
	int ret = 0;
	int ret_cmdlist_state = 0;
	int block_num = 0;
	int times = 0;
	bool last_block = false;
	dss_rect_t wb_ov_block_rect;
	uint32_t cmdlist_idxs = 0;
	bool has_base = false;
	int mctl_idx = 0;
	uint32_t wb_compose_type = 0;
	uint32_t timediff = 0;
	struct timeval tv0;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	bool reset = false;
	bool debug = false;
	bool csc_needed = true;
	uint32_t timeout_interval = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (g_fpga_flag == 0) {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC;
	} else {
		timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	}

	if (g_debug_ovl_copybit_composer)
		HISI_FB_INFO("+.\n");

	if (g_debug_ovl_copybit_composer_timediff)
		hisifb_get_timestamp(&tv0);

	hisi_dss_copybit_composer_on(hisifd);

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

	if (g_debug_ovl_copybit_composer == 1) {
		HISI_FB_INFO("fb%d, get ov_req from user.\n", hisifd->index);
		dumpDssOverlay(hisifd, pov_req);
	}

	////////////////////////////////////////////////////////////////////////////
	// get vertical block ov
	pov_req_h_v = &(hisifd->ov_req);
	pov_req_h_v->ov_block_infos_ptr = (uint64_t)(&(hisifd->ov_block_infos));

	hisifd->set_reg = hisi_cmdlist_set_reg;

	hisi_cmdlist_data_get_offline(hisifd, pov_req);

	cmdlist_idxs = HISI_DSS_COPYBIT_CMDLIST_IDXS;

	mctl_idx = DSS_MCTL5;
	wb_compose_type = pov_req->wb_compose_type;
	wb_layer4block = &(pov_req->wb_layer_infos[0]);
	if ((wb_layer4block->chn_idx != DSS_WCHN_W2) || (wb_compose_type != DSS_WB_COMPOSE_COPYBIT)) {
		HISI_FB_ERR("fb%d, wchn_idx[%d] does not used by copybit play or config wb_compose_type = %d is err!!\n",
			hisifd->index, wb_layer4block->chn_idx, wb_compose_type);
		ret = -1;
		goto err_return_sem0;
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);

	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		ret = get_ov_block_rect(pov_req, pov_h_block, wb_layer4block, &block_num, hisifd->ov_block_rects, false);//lint !e747
		if ((ret != 0) || (block_num == 0) || (block_num >= HISI_DSS_CMDLIST_BLOCK_MAX)) {
			HISI_FB_ERR("get_ov_block_rect failed! ret = %d, block_num[%d]\n", ret, block_num);
			ret = -1;
			goto err_return_sem0;
		}

		for (k = 0; k < block_num; k++) {
			has_base = false;

			ret = get_block_layers(pov_req, pov_h_block, *hisifd->ov_block_rects[k], pov_req_h_v);
			if (ret != 0) {
				HISI_FB_ERR("get_block_layers err ret = %d\n", ret);
				goto err_return_sem0;
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

			if (g_debug_ovl_copybit_composer== 1) {
				HISI_FB_INFO("fb%d, get ov_req_h_v from kernel.\n", hisifd->index);
				dumpDssOverlay(hisifd, pov_req_h_v);
			}

			if (k == 0) {
				ret = hisi_dss_module_init(hisifd);
				if (ret != 0) {
					HISI_FB_ERR("hisi_dss_module_init failed! ret = %d\n", ret);
					goto err_return_sem0;
				}
			}

			pov_h_v_block = (dss_overlay_block_t *)(pov_req_h_v->ov_block_infos_ptr);

			// check whether csc config needed or not
			csc_needed = hisi_check_csc_config_needed(pov_req_h_v);

			hisi_dss_aif_handler(hisifd, pov_req_h_v, pov_h_v_block);

			for (i = 0; i < pov_h_v_block->layer_nums; i++) {
				layer = &(pov_h_v_block->layer_infos[i]);
				memset(&clip_rect, 0, sizeof(dss_rect_ltrb_t));
				memset(&aligned_rect, 0, sizeof(dss_rect_ltrb_t));
				rdma_stretch_enable = false;

				ret = hisi_ov_compose_handler(hisifd, pov_req_h_v, pov_h_v_block, layer, &wb_layer4block->dst_rect,
					&wb_ov_block_rect, &clip_rect, &aligned_rect, &rdma_stretch_enable, &has_base, csc_needed, true);
				if (ret != 0) {
					HISI_FB_ERR("fb%d, hisi_ov_compose_handler failed! ret = %d\n", hisifd->index, ret);
					goto err_return_sem0;
				}
			}

			if (m == 0) {
				for (n = 0; n < pov_req_h_v->wb_layer_nums; n++) {
					wb_layer = &(pov_req_h_v->wb_layer_infos[n]);
					if (k == (block_num -1)) {
						last_block = true;
					}

					ret = hisi_wb_compose_handler(hisifd, pov_req_h_v, wb_layer,
						&wb_ov_block_rect, last_block, 0, csc_needed, true);
					if (ret != 0) {
						HISI_FB_ERR("hisi_dss_write_back_config failed, ret = %d\n", ret);
						goto err_return_sem0;
					}
				}
			}


			if (k == g_debug_ovl_offline_block_num) {
				break;
			}

			if (k < (block_num - 1)) {
				ret = hisi_dss_module_init(hisifd);
				if (ret != 0) {
					HISI_FB_ERR("hisi_dss_module_init failed! ret = %d\n", ret);
					goto err_return_sem0;
				}
			}
		}
	}

	ret = hisi_add_clear_module_reg_node(hisifd, pov_req, cmdlist_idxs, true);
	if (ret != 0) {
		HISI_FB_ERR("hisi_add_clear_module_reg_node failed! ret = %d\n", ret);
		goto err_return_sem0;
	}

	//wmb();
	hisi_cmdlist_flush_cache(hisifd, cmdlist_idxs);

	hisifb_buf_sync_handle_offline(hisifd, pov_req);

	up(&(hisifd->cmdlist_info->cmdlist_wb_common_sem));
	down(&(hisifd->copybit_info->copybit_sem));
	hisifd->copybit_info->copybit_flag = 1;
	hisifd->copybit_info->copybit_done = 0;

	hisi_cmdlist_exec(hisifd, cmdlist_idxs);
	hisi_cmdlist_config_start(hisifd, mctl_idx, cmdlist_idxs, wb_compose_type);

	hisifb_get_timestamp(&tv2);
REDO:
	ret = wait_event_interruptible_timeout(hisifd->copybit_info->copybit_wq, (hisifd->copybit_info->copybit_done == 1),
		msecs_to_jiffies(timeout_interval));
	if (ret == -ERESTARTSYS) {
		if (times < 50) {
			times++;
			mdelay(10);
			goto REDO;
		}
	}
	hisifb_get_timestamp(&tv3);

	hisifd->copybit_info->copybit_flag = 0;
	ret_cmdlist_state = hisi_cmdlist_check_cmdlist_state(hisifd, cmdlist_idxs);
	if ((ret <= 0) ||(ret_cmdlist_state < 0)) {
		HISI_FB_ERR("compose timeout, GLB_CPU_OFF_CAM_INTS=0x%x, ret=%d, ret_rch_state=%d, diff =%d usecs!\n",
			inp32(hisifd->dss_base + GLB_CPU_OFF_CAM_INTS), ret, ret_cmdlist_state, hisifb_timestamp_diff(&tv2, &tv3));

		ret = -ETIMEDOUT;
		reset = true;

		if (g_debug_ovl_copybit_composer_hold || g_debug_ovl_block_composer)
			debug = true;
	} else {
		/* remove mctl ch & ov */
		hisi_remove_mctl_mutex(hisifd, mctl_idx, cmdlist_idxs);

		ret = 0;
	}

	hisi_copybit_clear(hisifd, pov_req, cmdlist_idxs, reset, debug);
	up(&(hisifd->copybit_info->copybit_sem));

	hisi_dss_copybit_composer_off(hisifd);

	if (g_debug_ovl_copybit_composer_timediff) {
		hisifb_get_timestamp(&tv1);
		timediff = hisifb_timestamp_diff(&tv0, &tv1);
		if (timediff >= g_debug_ovl_copybit_composer_time_threshold)
			HISI_FB_ERR("COPYBIT_COMPOSER_TIMEDIFF is %u us!\n", timediff);
	}

	if (g_debug_ovl_copybit_composer)
		HISI_FB_INFO("-.\n");

	return ret;

err_return_sem0:
	if (g_debug_ovl_copybit_composer_hold || g_debug_ovl_block_composer) {
		debug = true;
	}

	reset = true;

	if (pov_req)
		hisi_copybit_clear(hisifd, pov_req, cmdlist_idxs, reset, debug);

	up(&(hisifd->cmdlist_info->cmdlist_wb_common_sem));

	hisi_dss_copybit_composer_off(hisifd);

	if (g_debug_ovl_copybit_composer)
		HISI_FB_INFO("-.\n");

	return ret;
}
