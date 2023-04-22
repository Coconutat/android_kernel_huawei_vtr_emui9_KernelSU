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
#ifndef HISI_OVERLAY_ONLINE_WRITEBACK_H
#define HISI_OVERLAY_ONLINE_WRITEBACK_H

struct hisifb_writeback {
	int online_wb_created;

	bool mmu_enable;
	bool afbc_enable;
	bool compress_enable;
	bool wb_clear_config;

	uint32_t ovl_idx;
	uint32_t wch_idx;
	uint32_t wdma_format;

	uint32_t wb_hsize;
	uint32_t wb_vsize;

	uint32_t wb_pad_num;
	uint32_t wb_pack_hsize;

	uint32_t wdfc_pad_hsize;
	uint32_t wdfc_pad_num;

	uint32_t wb_buffer_size;
	struct ion_handle *wb_handle;
	char __iomem *wb_buffer_base;
	ion_phys_addr_t wb_phys_addr;

	bool buffer_alloced;

	struct mutex wb_ctrl_lock;
	struct work_struct wb_ctrl_work;

	void (* wb_init)(struct hisi_fb_data_type *hisifd);
	int (* wb_alloc_buffer) (struct hisi_fb_data_type *hisifd);
	int (* wb_free_buffer) (struct hisi_fb_data_type *hisifd);
	void (* wb_save_buffer) (struct hisi_fb_data_type *hisifd);

	struct hisi_fb_data_type *hisifd;
};

void hisifb_ovl_online_wb_config(struct hisi_fb_data_type *hisifd);
void hisifb_ovl_online_wb_register(struct platform_device *pdev);
void hisifb_ovl_online_wb_unregister(struct platform_device *pdev);

#endif  /* HISI_OVERLAY_ONLINE_WRITEBACK_H */
