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

#include "hisi_mipi_dsi.h"

static int mipi_dsi_lp_ctrl(struct platform_device *pdev, bool lp_enter)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = panel_next_remove(pdev);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (hisifd->dss_dphy0_ref_clk) {
			clk_put(hisifd->dss_dphy0_ref_clk);
			hisifd->dss_dphy0_ref_clk = NULL;
		}

		if (hisifd->dss_dphy0_cfg_clk) {
			clk_put(hisifd->dss_dphy0_cfg_clk);
			hisifd->dss_dphy0_cfg_clk = NULL;
		}

		if (is_dual_mipi_panel(hisifd)) {
			if (hisifd->dss_dphy1_ref_clk) {
				clk_put(hisifd->dss_dphy1_ref_clk);
				hisifd->dss_dphy1_ref_clk = NULL;
			}

			if (hisifd->dss_dphy1_cfg_clk) {
				clk_put(hisifd->dss_dphy1_cfg_clk);
				hisifd->dss_dphy1_cfg_clk = NULL;
			}
		}
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		if (hisifd->dss_dphy1_ref_clk) {
			clk_put(hisifd->dss_dphy1_ref_clk);
			hisifd->dss_dphy1_ref_clk = NULL;
		}

		if (hisifd->dss_dphy1_cfg_clk) {
			clk_put(hisifd->dss_dphy1_cfg_clk);
			hisifd->dss_dphy1_cfg_clk = NULL;
		}
	} else {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = panel_next_set_backlight(pdev, bl_level);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_vsync_ctrl(struct platform_device *pdev, int enable)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = panel_next_vsync_ctrl(pdev, enable);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_lcd_fps_scence_handle(struct platform_device *pdev, uint32_t scence)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = panel_next_lcd_fps_scence_handle(pdev, scence);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_lcd_fps_updt_handle(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);

	ret = panel_next_lcd_fps_updt_handle(pdev);

	HISI_FB_DEBUG("fb%d, -!\n", hisifd->index);

	return ret;
}

static int mipi_dsi_esd_handle(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	bool is_timeout = true;
	uint32_t tmp = 0;
	uint32_t cmp_stopstate_val = 0;
	uint32_t try_times = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;
	pinfo = &(hisifd->panel_info);

	HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);

	if (pinfo->esd_skip_mipi_check == 1)
		goto panel_check;

	if (hisifd->panel_info.mipi.lane_nums >= DSI_4_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9) | BIT(11));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_3_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7) | BIT(9));
	} else if (hisifd->panel_info.mipi.lane_nums >= DSI_2_LANES) {
		cmp_stopstate_val = (BIT(4) | BIT(7));
	} else {
		cmp_stopstate_val = (BIT(4));
	}

	// check DPHY data and clock lane stopstate
	try_times = 0;
	tmp = inp32(mipi_dsi0_base + MIPIDSI_PHY_STATUS_OFFSET);
	while ((tmp & cmp_stopstate_val) != cmp_stopstate_val) {
		udelay(10);
		if (++try_times > 100) {
			is_timeout = false;
			break;
		}

		tmp = inp32(mipi_dsi0_base + MIPIDSI_PHY_STATUS_OFFSET);
	}

	if (is_timeout) {
		HISI_FB_ERR("fb%d, check DPHY data lane status failed! MIPIDSI_PHY_STATUS=0x%x.\n",
			hisifd->index, tmp);
		ret = -EINVAL;
		goto error;
	}

panel_check:
	// disable generate High Speed clock
	//set_reg(mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
	// check panel power status
	ret = panel_next_esd_handle(pdev);
	// enable generate High Speed clock
	//set_reg(mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x1, 1, 0);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;

error:
	return ret;
}

static int mipi_dsi_set_display_region(struct platform_device *pdev, struct dss_rect *dirty)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (pdev == NULL || dirty == NULL) {
		HISI_FB_ERR("pdev or firty is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("index=%d, enter!\n", hisifd->index);

	ret = panel_next_set_display_region(pdev, dirty);

	HISI_FB_DEBUG("index=%d, exit!\n", hisifd->index);

	return ret;
}

static int mipi_dsi_get_lcd_id(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	return panel_next_get_lcd_id(pdev);
}

static ssize_t mipi_dsi_bit_clk_upt_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	uint32_t dsi_bit_clk_upt_tmp = 0;
	int n_str = 0;
	int i = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	for (i = 0; buf[i] != '\0' && buf[i] != '\n'; i++) {
		n_str++;
		if (n_str >= 6) {
			HISI_FB_ERR("invalid input parameter: n_str = %d, count = %ld\n", n_str, count);
			break;
		}
	}

	if (n_str != 5) {
		HISI_FB_ERR("invalid input parameter: n_str = %d, count = %ld\n", n_str, count);
		return count;
	}

	if (!hisifd->panel_info.dsi_bit_clk_upt_support) {
		HISI_FB_INFO("fb%d, not support!\n", hisifd->index);
		return count;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (!strncmp(buf, MIPI_DSI_BIT_CLK_STR1, n_str)) {
		dsi_bit_clk_upt_tmp = pinfo->mipi.dsi_bit_clk_val1;
	} else if (!strncmp(buf, MIPI_DSI_BIT_CLK_STR2, n_str)) {
		dsi_bit_clk_upt_tmp = pinfo->mipi.dsi_bit_clk_val2;
	} else if (!strncmp(buf, MIPI_DSI_BIT_CLK_STR3, n_str)) {
		dsi_bit_clk_upt_tmp = pinfo->mipi.dsi_bit_clk_val3;
	} else if (!strncmp(buf, MIPI_DSI_BIT_CLK_STR4, n_str)) {
		dsi_bit_clk_upt_tmp = pinfo->mipi.dsi_bit_clk_val4;
	} else if (!strncmp(buf, MIPI_DSI_BIT_CLK_STR5, n_str)) {
		dsi_bit_clk_upt_tmp = pinfo->mipi.dsi_bit_clk_val5;
	} else {
		HISI_FB_ERR("fb%d, unknown dsi_bit_clk_index!\n", hisifd->index);
	}

	if (dsi_bit_clk_upt_tmp == 0) {
		return count;
	}
	pinfo->mipi.dsi_bit_clk_upt = dsi_bit_clk_upt_tmp;

	panel_next_snd_mipi_clk_cmd_store(pdev, pinfo->mipi.dsi_bit_clk_upt);

	HISI_FB_INFO("switch mipi clk to %d.\n", pinfo->mipi.dsi_bit_clk_upt);

	return count;
}

static ssize_t mipi_dsi_bit_clk_upt_show(struct platform_device *pdev, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	ssize_t ret;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	pinfo = &(hisifd->panel_info);

	if (!hisifd->panel_info.dsi_bit_clk_upt_support) {
		HISI_FB_INFO("fb%d, panel_info.dsi_bit_clk_upt_support 0 !\n", hisifd->index);
		return -1;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d", pinfo->mipi.dsi_bit_clk_upt);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_model_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_model_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_check_reg(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_mipi_detect_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_mipi_detect(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_hkadc_debug_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_hkadc_debug_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_hkadc_debug_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_hkadc_debug_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_gram_check_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_gram_check_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_gram_check_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_gram_check_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_dynamic_sram_checksum_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_dynamic_sram_checksum_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_dynamic_sram_checksum_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_dynamic_sram_checksum_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}


static ssize_t mipi_dsi_lcd_voltage_enable_store(struct platform_device *pdev,	const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_voltage_enable_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_bist_check(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_bist_check(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_amoled_vr_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_amoled_vr_mode_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_amoled_vr_mode_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_amoled_vr_mode_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}
static ssize_t mipi_dsi_lcd_acl_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_acl_ctrl_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_acl_ctrl_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_acl_ctrl_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}


static ssize_t mipi_dsi_lcd_sleep_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_sleep_ctrl_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_sleep_ctrl_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_sleep_ctrl_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_test_config_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_test_config_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_test_config_store(struct platform_device *pdev,	const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_test_config_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_reg_read_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_reg_read_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_reg_read_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_reg_read_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_support_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_support_mode_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_support_mode_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_support_mode_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_support_checkmode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_support_checkmode_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_lp2hs_mipi_check_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_lp2hs_mipi_check_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_lp2hs_mipi_check_store(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_lp2hs_mipi_check_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_amoled_pcd_errflag_check(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	ret = panel_next_amoled_pcd_errflag_check(pdev, buf);
	return ret;
}

static ssize_t mipi_dsi_lcd_ic_color_enhancement_mode_store(struct platform_device *pdev,	const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("mipi dsi_lcd_ic_color_enhancement_mode_store pdev NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("mipi dsi_lcd_ic_color_enhancement_mode_store hisifd NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_ic_color_enhancement_mode_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_lcd_ic_color_enhancement_mode_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("mipi_dsi_lcd_ic_color_enhancement_mode_show pdev NULL Pointer");
		return 0;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("mipi_dsi_lcd_ic_color_enhancement_mode_show hisifd NULL Pointer");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_lcd_ic_color_enhancement_mode_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_sharpness2d_table_store(struct platform_device *pdev,	const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_sharpness2d_table_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_alpm_setting(struct platform_device *pdev, const char *buf, size_t count)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_alpm_setting_store(pdev, buf, count);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_sharpness2d_table_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_sharpness2d_table_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_dsi_panel_info_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_panel_info_show(pdev, buf);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_dsi_sbl_ctrl(struct platform_device *pdev, int enable)
{
	ssize_t ret = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = panel_next_sbl_ctrl(pdev, enable);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
	return ret;
}

static int mipi_dsi_clk_irq_setup(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisifd->dss_dphy0_ref_clk = devm_clk_get(&pdev->dev, hisifd->dss_dphy0_ref_clk_name);
		if (IS_ERR(hisifd->dss_dphy0_ref_clk)) {
			ret = PTR_ERR(hisifd->dss_dphy0_ref_clk);
			return ret;
		} else {
			ret = clk_set_rate(hisifd->dss_dphy0_ref_clk, DEFAULT_MIPI_CLK_RATE);
			if (ret < 0) {
				HISI_FB_ERR("fb%d dss_dphy0_ref_clk clk_set_rate(%lu) failed, error=%d!\n",
					hisifd->index, DEFAULT_MIPI_CLK_RATE, ret);
				return -EINVAL;
			}

			HISI_FB_INFO("dss_dphy0_ref_clk:[%lu]->[%lu].\n",
				DEFAULT_MIPI_CLK_RATE, clk_get_rate(hisifd->dss_dphy0_ref_clk));
		}

		hisifd->dss_dphy0_cfg_clk = devm_clk_get(&pdev->dev, hisifd->dss_dphy0_cfg_clk_name);
		if (IS_ERR(hisifd->dss_dphy0_cfg_clk)) {
			ret = PTR_ERR(hisifd->dss_dphy0_cfg_clk);
			return ret;
		} else {
			ret = clk_set_rate(hisifd->dss_dphy0_cfg_clk, DEFAULT_MIPI_CLK_RATE);
			if (ret < 0) {
				HISI_FB_ERR("fb%d dss_dphy0_cfg_clk clk_set_rate(%lu) failed, error=%d!\n",
					hisifd->index, DEFAULT_MIPI_CLK_RATE, ret);
				return -EINVAL;
			}

			HISI_FB_INFO("dss_dphy0_cfg_clk:[%lu]->[%lu].\n",
				DEFAULT_MIPI_CLK_RATE, clk_get_rate(hisifd->dss_dphy0_cfg_clk));
		}

		hisifd->dss_pclk_dsi0_clk = devm_clk_get(&pdev->dev, hisifd->dss_pclk_dsi0_name);
		if (IS_ERR(hisifd->dss_pclk_dsi0_clk)) {
			ret = PTR_ERR(hisifd->dss_pclk_dsi0_clk);
			return ret;
		} else {

			HISI_FB_INFO("dss_pclk_dsi0_clk:[%lu]->[%lu].\n",
				DEFAULT_PCLK_DSI_RATE, clk_get_rate(hisifd->dss_pclk_dsi0_clk));
		}
	}


	if (is_dual_mipi_panel(hisifd) || (hisifd->index == EXTERNAL_PANEL_IDX)) {
		hisifd->dss_dphy1_ref_clk = devm_clk_get(&pdev->dev, hisifd->dss_dphy1_ref_clk_name);
		if (IS_ERR(hisifd->dss_dphy1_ref_clk)) {
			ret = PTR_ERR(hisifd->dss_dphy1_ref_clk);
			return ret;
		} else {
			ret = clk_set_rate(hisifd->dss_dphy1_ref_clk, DEFAULT_MIPI_CLK_RATE);
			if (ret < 0) {
				HISI_FB_ERR("fb%d dss_dphy1_ref_clk clk_set_rate(%lu) failed, error=%d!\n",
					hisifd->index, DEFAULT_MIPI_CLK_RATE, ret);
				return -EINVAL;
			}

			HISI_FB_INFO("dss_dphy1_ref_clk:[%lu]->[%lu].\n",
				DEFAULT_MIPI_CLK_RATE, clk_get_rate(hisifd->dss_dphy1_ref_clk));
		}

		hisifd->dss_dphy1_cfg_clk = devm_clk_get(&pdev->dev, hisifd->dss_dphy1_cfg_clk_name);
		if (IS_ERR(hisifd->dss_dphy1_cfg_clk)) {
			ret = PTR_ERR(hisifd->dss_dphy1_cfg_clk);
			return ret;
		} else {
			ret = clk_set_rate(hisifd->dss_dphy1_cfg_clk, DEFAULT_MIPI_CLK_RATE);
			if (ret < 0) {
				HISI_FB_ERR("fb%d dss_dphy1_cfg_clk clk_set_rate(%lu) failed, error=%d!\n",
					hisifd->index, DEFAULT_MIPI_CLK_RATE, ret);
				return -EINVAL;
			}

			HISI_FB_INFO("dss_dphy1_cfg_clk:[%lu]->[%lu].\n",
				DEFAULT_MIPI_CLK_RATE, clk_get_rate(hisifd->dss_dphy1_cfg_clk));
		}

		hisifd->dss_pclk_dsi1_clk = devm_clk_get(&pdev->dev, hisifd->dss_pclk_dsi1_name);
		if (IS_ERR(hisifd->dss_pclk_dsi1_clk)) {
			ret = PTR_ERR(hisifd->dss_pclk_dsi1_clk);
			return ret;
		} else {

			HISI_FB_INFO("dss_pclk_dsi1_clk:[%lu]->[%lu].\n",
				DEFAULT_PCLK_DSI_RATE, clk_get_rate(hisifd->dss_pclk_dsi1_clk));
		}
	}

	return ret;
}

static int mipi_dsi_probe(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct platform_device *dpp_dev = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = mipi_dsi_clk_irq_setup(pdev);
	if (ret) {
		dev_err(&pdev->dev, "fb%d mipi_dsi_irq_clk_setup failed, error=%d!\n", hisifd->index, ret);
		goto err;
	}

	/* alloc device */
	dpp_dev = platform_device_alloc(DEV_NAME_DSS_DPE, pdev->id);
	if (!dpp_dev) {
		dev_err(&pdev->dev, "fb%d platform_device_alloc failed, error=%d!\n", hisifd->index, ret);
		ret = -ENOMEM;
		goto err_device_alloc;
	}

	/* link to the latest pdev */
	hisifd->pdev = dpp_dev;

	/* alloc panel device data */
	ret = platform_device_add_data(dpp_dev, dev_get_platdata(&pdev->dev),
		sizeof(struct hisi_fb_panel_data));
	if (ret) {
		dev_err(&pdev->dev, "fb%d platform_device_add_data failed error=%d!\n", hisifd->index, ret);
		goto err_device_put;
	}

	/* data chain */
	pdata = dev_get_platdata(&dpp_dev->dev);
	pdata->set_fastboot = mipi_dsi_set_fastboot;
	pdata->on = mipi_dsi_on;
	pdata->off = mipi_dsi_off;
	pdata->lp_ctrl = mipi_dsi_lp_ctrl;
	pdata->remove = mipi_dsi_remove;
	pdata->set_backlight = mipi_dsi_set_backlight;
	pdata->vsync_ctrl = mipi_dsi_vsync_ctrl;
	pdata->lcd_fps_scence_handle = mipi_dsi_lcd_fps_scence_handle;
	pdata->lcd_fps_updt_handle = mipi_dsi_lcd_fps_updt_handle;
	pdata->esd_handle = mipi_dsi_esd_handle;
	pdata->set_display_region= mipi_dsi_set_display_region;
	pdata->mipi_dsi_bit_clk_upt_store = mipi_dsi_bit_clk_upt_store;
	pdata->mipi_dsi_bit_clk_upt_show = mipi_dsi_bit_clk_upt_show;
	pdata->lcd_model_show = mipi_dsi_lcd_model_show;
	pdata->lcd_check_reg = mipi_dsi_lcd_check_reg_show;
	pdata->lcd_mipi_detect = mipi_dsi_lcd_mipi_detect_show;
	pdata->lcd_hkadc_debug_show = mipi_dsi_lcd_hkadc_debug_show;
	pdata->lcd_hkadc_debug_store = mipi_dsi_lcd_hkadc_debug_store;
	pdata->lcd_gram_check_show = mipi_dsi_lcd_gram_check_show;
	pdata->lcd_gram_check_store = mipi_dsi_lcd_gram_check_store;
	pdata->lcd_dynamic_sram_checksum_show = mipi_dsi_lcd_dynamic_sram_checksum_show;
	pdata->lcd_dynamic_sram_checksum_store = mipi_dsi_lcd_dynamic_sram_checksum_store;
	pdata->lcd_voltage_enable_store = mipi_dsi_lcd_voltage_enable_store;
	pdata->lcd_bist_check = mipi_dsi_lcd_bist_check;
	pdata->lcd_sleep_ctrl_show = mipi_dsi_lcd_sleep_ctrl_show;
	pdata->lcd_sleep_ctrl_store = mipi_dsi_lcd_sleep_ctrl_store;
	pdata->lcd_acl_ctrl_show = mipi_dsi_lcd_acl_ctrl_show;
	pdata->lcd_acl_ctrl_store = mipi_dsi_lcd_acl_ctrl_store;
	pdata->lcd_amoled_vr_mode_show = mipi_dsi_lcd_amoled_vr_mode_show;
	pdata->lcd_amoled_vr_mode_store = mipi_dsi_lcd_amoled_vr_mode_store;
	pdata->lcd_test_config_show = mipi_dsi_lcd_test_config_show;
	pdata->lcd_test_config_store = mipi_dsi_lcd_test_config_store;
	pdata->lcd_reg_read_show = mipi_dsi_lcd_reg_read_show;
	pdata->lcd_reg_read_store = mipi_dsi_lcd_reg_read_store;
	pdata->lcd_support_mode_show = mipi_dsi_lcd_support_mode_show;
	pdata->lcd_support_mode_store = mipi_dsi_lcd_support_mode_store;
	pdata->lcd_support_checkmode_show = mipi_dsi_lcd_support_checkmode_show;
	pdata->lcd_lp2hs_mipi_check_show = mipi_dsi_lcd_lp2hs_mipi_check_show;
	pdata->lcd_lp2hs_mipi_check_store = mipi_dsi_lcd_lp2hs_mipi_check_store;
	pdata->amoled_pcd_errflag_check = mipi_dsi_amoled_pcd_errflag_check;
	pdata->lcd_ic_color_enhancement_mode_show = mipi_dsi_lcd_ic_color_enhancement_mode_show;
	pdata->lcd_ic_color_enhancement_mode_store = mipi_dsi_lcd_ic_color_enhancement_mode_store;
	pdata->sharpness2d_table_store = mipi_dsi_sharpness2d_table_store;
	pdata->sharpness2d_table_show = mipi_dsi_sharpness2d_table_show;
	pdata->panel_info_show = mipi_dsi_panel_info_show;
	pdata->get_lcd_id = mipi_dsi_get_lcd_id;
	pdata->amoled_alpm_setting_store = mipi_dsi_alpm_setting;
	pdata->sbl_ctrl = mipi_dsi_sbl_ctrl;
	pdata->next = pdev;

	/* get/set panel info */
	memcpy(&hisifd->panel_info, pdata->panel_info, sizeof(struct hisi_panel_info));

	/* set driver data */
	platform_set_drvdata(dpp_dev, hisifd);
	/* device add */
	ret = platform_device_add(dpp_dev);
	if (ret) {
		dev_err(&pdev->dev, "fb%d platform_device_add failed, error=%d!\n", hisifd->index, ret);
		goto err_device_put;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;

err_device_put:
	platform_device_put(dpp_dev);
err_device_alloc:
err:
	return ret;
}

static struct platform_driver this_driver = {
	.probe = mipi_dsi_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = DEV_NAME_MIPIDSI,
	},
};

static int __init mipi_dsi_driver_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}
/*lint +e776 +e715 +e712 +e737 +e776 +e838*/
module_init(mipi_dsi_driver_init);
