/* Copyright (c) 2017-2018, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
*/
#ifndef HISI_FB_VIDEO_IDLE_H
#define HISI_FB_VIDEO_IDLE_H
#ifdef CONFIG_HISI_L3CACHE_SHARE
#include <linux/hisi/hisi_l3share.h>
#endif
#include <linux/hisi/ion-iommu.h>
#include "hisi_fb.h"

#define CACHE_WB_SIZE (1080*2244*4)
extern uint32_t g_dss_chn_sid_num[DSS_CHN_MAX_DEFINE];
extern uint32_t g_dss_smmu_smrx_idx[DSS_CHN_MAX_DEFINE];

struct idle_rb_closed_reg {
	bool need_recovery;
	uint32_t dsc_en;
	uint32_t dpp_clip_en;
	uint32_t dither_ctl0;
	uint32_t gama_en;
	uint32_t xcc_en;
	uint32_t degama_en;
	uint32_t gmp_en;
	uint32_t arsr_post_bypass;
	uint32_t ifbc_en;
	uint32_t hiace_bypass;
	uint32_t nr_bypass;
	uint32_t sbl_en;
};

struct hisifb_video_idle_ctrl {
	int idle_ctrl_created;

	bool mmu_enable;
	bool afbc_enable;
	bool compress_enable;
	bool video_idle_wb_status;
	bool video_idle_rb_status;

	uint32_t wb_irq;
	uint32_t rch_idx;
	uint32_t wch_idx;
	uint32_t ovl_idx;
	uint32_t wdma_format;

	uint32_t wb_hsize;
	uint32_t wb_vsize;

	uint32_t wb_pad_num;
	uint32_t wb_pack_hsize;

	uint32_t wdfc_pad_hsize;
	uint32_t wdfc_pad_num;

	uint32_t l3cache_size;

	struct idle_rb_closed_reg rb_closed_reg;

	uint32_t wb_buffer_size;
	struct ion_handle *wb_handle;
	char __iomem *wb_buffer_base;
	ion_phys_addr_t wb_phys_addr;
	uint64_t wb_vir_addr;

	bool buffer_alloced;
#ifdef CONFIG_HISI_L3CACHE_SHARE
	struct l3_cache_request_params request_params;
	struct l3_cache_release_params release_params;
#endif
	struct workqueue_struct *idle_wb_err_wq;
	struct work_struct idle_wb_err_work;
	struct work_struct wb_err_work;

	struct mutex video_idle_ctrl_lock;
	struct work_struct video_idle_ctrl_work;

	struct hisi_fb_data_type *hisifd;
};

int hisifb_video_idle_check_enable(struct hisi_fb_data_type *hisifd, uint32_t video_idle_status);
void hisifb_hisync_disp_sync_enable(struct hisi_fb_data_type *hisifd);
int hisifb_hisync_disp_sync_config(struct hisi_fb_data_type *hisifd);
void hisifb_video_idle_buffer_free(struct hisi_fb_data_type *hisifd);

#endif /* HISI_FB_VIDEO_IDLE_H */

