/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "hisi_display_effect.h"
#include "hisi_overlay_utils.h"
#include "hisi_dpe_utils.h"
#include <linux/hisi/hw_cmdline_parse.h>


int g_factory_gamma_enable = 0;
struct mutex g_rgbw_lock;


/*lint -e838, -e778, -e845, -e712, -e527, -e30, -e142, -e715, -e655, -e550*/
static void hisi_effect_module_support (struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	struct dss_effect *effect_ctrl = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	pinfo = &(hisifd->panel_info);
	effect_ctrl = &(hisifd->effect_ctl);

	memset(effect_ctrl, 0, sizeof(struct dss_effect));

	effect_ctrl->acm_support = (pinfo->acm_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACM) != 0));

	effect_ctrl->ace_support = (pinfo->acm_ce_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE) != 0));

	effect_ctrl->dither_support = (pinfo->dither_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_DITHER) != 0));

	effect_ctrl->lcp_xcc_support = (pinfo->xcc_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_XCC) != 0));

	effect_ctrl->lcp_gmp_support = (pinfo->gmp_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_GMP) != 0));

	effect_ctrl->lcp_igm_support = (pinfo->gamma_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_LCP_IGM) !=0));

	effect_ctrl->gamma_support = (pinfo->gamma_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA) != 0));

	effect_ctrl->sbl_support = (pinfo->sbl_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_SBL) !=0 ));

	effect_ctrl->hiace_support =	(pinfo->hiace_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_HIACE) != 0));

	effect_ctrl->arsr1p_sharp_support = (pinfo->arsr1p_sharpness_support
		&& (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF) != 0));

	effect_ctrl->arsr2p_sharp_support = (pinfo->prefix_sharpness2D_support);

	effect_ctrl->dss_ready = true;
}

static int hisifb_effect_module_init_handler(void __user *argp)
{
	int ret;
	struct hisi_fb_data_type *hisifd_primary = NULL;

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if (NULL == hisifd_primary) {
		HISI_FB_ERR("fb0 is not existed, return!\n");
		//effect_ctrl.dss_ready = false; ????
		ret = -ENODEV;
		goto err_out;
	}

	if(argp == NULL){
		HISI_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_to_user(argp, &(hisifd_primary->effect_ctl), sizeof(struct dss_effect));
	if (ret) {
		HISI_FB_ERR("failed to copy result of ioctl to user space.\n");
		goto err_out;
	}

err_out:
	return ret;
}

static int hisifb_effect_module_deinit_handler(void __user *argp)
{
	int ret;
	struct dss_effect init_status;

	if(argp == NULL){
		HISI_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&init_status, argp, sizeof(struct dss_effect));
	if (ret) {
		HISI_FB_ERR("failed to copy data to kernel space.\n");
		goto err_out;
	}

err_out:
	return ret;
}

static int hisifb_effect_info_get_handler(void __user *argp)
{
	int ret = -EINVAL;
	struct dss_effect_info effect_info;
	struct hisi_fb_data_type *hisifd_primary = NULL;

	if(argp == NULL){
		HISI_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&effect_info, argp, sizeof(struct dss_effect_info));
	if (ret) {
		HISI_FB_ERR("failed to copy data from user.\n");
		goto err_out;
	}

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if (NULL == hisifd_primary) {
		HISI_FB_ERR("fb0 is not existed, return!\n");
		ret = -ENODEV;
		goto err_out;
	}

	if (!hisifd_primary->panel_power_on) {
		HISI_FB_ERR("panel is power down, return!\n");
		ret = -EBADRQC;
		goto err_out;
	}

	if (!hisifd_primary->effect_ctl.dss_ready) {
		HISI_FB_ERR("dss is not ready\n");
		ret = -EBADRQC;
		goto err_out;
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR2P) {
		ret = hisi_effect_arsr2p_info_get(hisifd_primary, effect_info.arsr2p);
		if (ret) {
			HISI_FB_ERR("failed to get arsr2p info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR1P) {
		ret = hisi_effect_arsr1p_info_get(hisifd_primary, effect_info.arsr1p);
		if (ret) {
			HISI_FB_ERR("failed to get arsr1p info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ACM) {
		ret = hisi_effect_acm_info_get(hisifd_primary, &effect_info.acm);
		if (ret) {
			HISI_FB_ERR("failed to get acm info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & (DSS_EFFECT_MODULE_LCP_GMP | DSS_EFFECT_MODULE_LCP_IGM | DSS_EFFECT_MODULE_LCP_XCC)) {
		ret = hisi_effect_lcp_info_get(hisifd_primary, &effect_info.lcp);
		if (ret) {
			HISI_FB_ERR("failed to get lcp info\n");
			goto err_out;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_GAMMA) {
		ret = hisi_effect_gamma_info_get(hisifd_primary, &effect_info.gamma);
		if (ret) {
			HISI_FB_ERR("failed to get gamma info\n");
			goto err_out;
		}
	}

	ret = copy_to_user(argp, &effect_info, sizeof(struct dss_effect_info));
	if (ret) {
		HISI_FB_ERR("failed to copy result of ioctl to user space.\n");
		goto err_out;
	}

err_out:
	return ret;;
}

static int hisifb_effect_info_set_handler(void __user *argp)
{
	int ret;
	struct dss_effect_info effect_info;
	struct hisi_fb_data_type *hisifd_primary = NULL;

	if(argp == NULL){
		HISI_FB_ERR("argp is null pointer\n");
		return -1;
	}

	ret = copy_from_user(&effect_info, argp, sizeof(struct dss_effect_info));
	if (ret) {
		HISI_FB_ERR("failed to copy data to kernel space.\n");
		goto err_out;
	}

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if (NULL == hisifd_primary) {
		HISI_FB_ERR("hisifd_primary is null or unexpected input fb\n");
		ret = -EBADRQC;
		goto err_out;
	}

	spin_lock(&hisifd_primary->effect_lock);

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR2P) {
		ret = hisi_effect_arsr2p_info_set(hisifd_primary, effect_info.arsr2p);
		if (ret) {
			HISI_FB_ERR("failed to set arsr2p info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ARSR1P) {
		ret = hisi_effect_arsr1p_info_set(hisifd_primary, effect_info.arsr1p);
		if (ret) {
			HISI_FB_ERR("failed to set arsr1p info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_ACM) {
		ret = hisi_effect_acm_info_set(hisifd_primary, &effect_info.acm);
		if (ret) {
			HISI_FB_ERR("failed to set acm info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_LCP_GMP) {
		ret = hisi_effect_gmp_info_set(hisifd_primary, &effect_info.lcp);
		if (ret) {
			HISI_FB_ERR("failed to set GMP info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_LCP_IGM) {
		ret = hisi_effect_igm_info_set(hisifd_primary, &effect_info.lcp);
		if (ret) {
			HISI_FB_ERR("failed to set IGM info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_LCP_XCC) {
		ret = hisi_effect_xcc_info_set(hisifd_primary, &effect_info.lcp);
		if (ret) {
			HISI_FB_ERR("failed to set XCC info\n");
			goto err_out_spin;
		}
	}

	if (effect_info.modules & DSS_EFFECT_MODULE_GAMMA) {
		ret = hisi_effect_gamma_info_set(hisifd_primary, &effect_info.gamma);
		if (ret) {
			HISI_FB_ERR("failed to set gama info\n");
			goto err_out_spin;
		}
	}
	hisifd_primary->display_effect_flag = 4;
err_out_spin:
	spin_unlock(&hisifd_primary->effect_lock);

	HISI_FB_DEBUG("fb%d, modules = 0x%x, -.\n", hisifd_primary->index, effect_info.modules);

err_out:
	return ret;;
}

static int hisi_display_effect_ioctl_handler(struct hisi_fb_data_type *hisifd, unsigned int cmd, void __user *argp)
{
	int ret = -EINVAL;

	if (NULL == argp || NULL == hisifd) {
		HISI_FB_ERR("NULL pointer of argp or hisifd.\n");
		goto err_out;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (runmode_is_factory()) {
		ret = 0;
		goto err_out;
	}

	switch (cmd) {
	case HISIFB_EFFECT_MODULE_INIT:
		ret = hisifb_effect_module_init_handler(argp);
		break;
	case HISIFB_EFFECT_MODULE_DEINIT:
		ret = hisifb_effect_module_deinit_handler(argp);
		break;
	case HISIFB_EFFECT_INFO_GET:
		ret = hisifb_effect_info_get_handler(argp);
		break;
	case HISIFB_EFFECT_INFO_SET:
		ret = hisifb_effect_info_set_handler(argp);
		break;
	default:
		HISI_FB_ERR("unknown cmd id.\n");
		ret = -ENOSYS;
		break;
	};

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

err_out:
	return ret;
}

void hisi_display_effect_init(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (PRIMARY_PANEL_IDX == hisifd->index) {
		hisifd->display_effect_ioctl_handler = NULL;
		memset(&hisifd->effect_updated_flag, 0, sizeof(struct dss_module_update));
		spin_lock_init(&hisifd->effect_lock);

		hisi_effect_module_support(hisifd);
	} else if (AUXILIARY_PANEL_IDX == hisifd->index) {
		hisifd->display_effect_ioctl_handler = hisi_display_effect_ioctl_handler;
	} else {
		hisifd->display_effect_ioctl_handler = NULL;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}
void hisi_dss_effect_set_reg(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return ;
	}

	if (PRIMARY_PANEL_IDX != hisifd->index) {
		return;
	}

	if (spin_can_lock(&hisifd->effect_lock)) {
		spin_lock(&hisifd->effect_lock);
		if (hisifd->panel_info.smart_color_mode_support == 0) {
			hisi_effect_acm_set_reg(hisifd);
			hisi_effect_lcp_set_reg(hisifd);
		}
		hisi_effect_gamma_set_reg(hisifd);
		spin_unlock(&hisifd->effect_lock);
	} else {
		HISI_FB_DEBUG("dss effect param is being updated, delay set reg to next frame!\n");
	}

	return;
}

static int display_engine_blc_param_get(struct hisi_fb_data_type *hisifd, display_engine_blc_param_t *param)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (NULL == param) {
		HISI_FB_ERR("[effect] param is NULL\n");
		return -EINVAL;
	}

	param->enable = hisifd->de_info.blc_enable ? 1 : 0;
	param->delta = hisifd->de_info.blc_delta;
	return 0;
}

static int display_engine_blc_param_set(struct hisi_fb_data_type *hisifd, display_engine_blc_param_t *param)
{
	struct hisi_fb_panel_data *pdata = NULL;
	ktime_t current_bl_timestamp = ktime_get();
	int bl_timeinterval = 16670000;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (NULL == param) {
		HISI_FB_ERR("[effect] param is NULL\n");
		return -EINVAL;
	}

	hisifd->de_info.blc_enable = (param->enable == 1) ? true : false;
	hisifd->de_info.blc_delta = param->delta;

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return -EINVAL;
	}
	down(&hisifd->blank_sem_effect);

	if (abs((int)(ktime_to_ns(hisifd->backlight.bl_timestamp) - ktime_to_ns(current_bl_timestamp))) > bl_timeinterval) {
		HISI_FB_DEBUG("[effect] delta:%d bl:%d enable:%d set\n", hisifd->de_info.blc_delta, hisifd->bl_level, hisifd->de_info.blc_enable);
		if (hisifd->bl_level > 0) {
			down(&hisifd->brightness_esd_sem);
			hisifb_set_backlight(hisifd, hisifd->bl_level, true);
			up(&hisifd->brightness_esd_sem);
		}
	} else {
		HISI_FB_DEBUG("[effect] delta:%d bl:%d enable:%d skip\n", hisifd->de_info.blc_delta, hisifd->bl_level, hisifd->de_info.blc_enable);
		ret = 2; // This delta has been skipped, and hal may resent the command.
	}

	up(&hisifd->blank_sem_effect);
	return ret;
}

int display_engine_ddic_color_param_get(struct hisi_fb_data_type *hisifd, display_engine_ddic_color_param_t *param)
{
	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	param->ddic_color_mode = hisifd->de_info.ddic_color_mode;

	return 0;
}

int display_engine_ddic_color_param_set(struct hisi_fb_data_type *hisifd, display_engine_ddic_color_param_t *param)
{
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (hisifd->pdev == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (pdata == NULL) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	hisifd->de_info.ddic_color_mode = param->ddic_color_mode;

	down(&hisifd->blank_sem);

	if (!hisifd->panel_power_on) {
		HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
		ret = -1;
		goto err_out;
	}

	if (pdata->lcd_ce_mode_store) {
		char buf[8];
		int count = 0;
		hisifb_activate_vsync(hisifd);
		count = snprintf(buf, sizeof(buf), "%d", param->ddic_color_mode);
		pdata->lcd_ce_mode_store(hisifd->pdev, buf, count);
		hisifb_deactivate_vsync(hisifd);
	}

err_out:
	up(&hisifd->blank_sem);

	return ret;
}

int display_engine_ddic_cabc_param_get(struct hisi_fb_data_type *hisifd, display_engine_ddic_cabc_param_t *param)
{
	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}
	param->ddic_cabc_mode = hisifd->de_info.ddic_cabc_mode;
	return 0;
}

int display_engine_ddic_cabc_param_set(struct hisi_fb_data_type *hisifd, display_engine_ddic_cabc_param_t *param)
{
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (hisifd->pdev == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}


	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	hisifd->de_info.ddic_cabc_mode = param->ddic_cabc_mode;
	down(&hisifd->blank_sem);

	if (!hisifd->panel_power_on) {
		HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
		ret = -1;
		goto err_out;
	}

	if (pdata->lcd_cabc_mode_store) {
		char buf[8];
		int count = 0;
		hisifb_activate_vsync(hisifd);
		count = snprintf(buf, sizeof(buf), "%d", param->ddic_cabc_mode);
		pdata->lcd_cabc_mode_store(hisifd->pdev, buf, count);
		hisifb_deactivate_vsync(hisifd);
	}

err_out:
	up(&hisifd->blank_sem);

	return ret;
}

int display_engine_ddic_rgbw_param_get(struct hisi_fb_data_type *hisifd, display_engine_ddic_rgbw_param_t *param)
{
	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	param->ddic_panel_id = hisifd->de_info.ddic_panel_id;
	param->ddic_rgbw_mode = hisifd->de_info.ddic_rgbw_mode;
	param->ddic_rgbw_backlight = hisifd->de_info.ddic_rgbw_backlight;
	param->rgbw_saturation_control = hisifd->de_info.rgbw_saturation_control;
	param->frame_gain_limit = hisifd->de_info.frame_gain_limit;
	param->frame_gain_speed = hisifd->de_info.frame_gain_speed;
	param->color_distortion_allowance = hisifd->de_info.color_distortion_allowance;
	param->pixel_gain_limit = hisifd->de_info.pixel_gain_limit;
	param->pixel_gain_speed = hisifd->de_info.pixel_gain_speed;
	param->pwm_duty_gain = hisifd->de_info.pwm_duty_gain;
	HISI_FB_DEBUG("[effect] display_engine_ddic_rgbw_param_get params\n");
	return 0;
}

int display_engine_ddic_rgbw_param_set(struct hisi_fb_data_type *hisifd, display_engine_ddic_rgbw_param_t *param)
{
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	mutex_lock(&g_rgbw_lock);
	hisifd->de_info.ddic_panel_id = param->ddic_panel_id;
	hisifd->de_info.ddic_rgbw_mode = param->ddic_rgbw_mode;
	hisifd->de_info.rgbw_saturation_control = param->rgbw_saturation_control;
	hisifd->de_info.frame_gain_speed = param->frame_gain_speed;
	hisifd->de_info.pixel_gain_speed = param->pixel_gain_speed;
	if((param->ddic_panel_id != LG_NT36772A_RGBW_ID) && (param->ddic_panel_id != LG_NT36772A_RGBW_ID_HMA) && (param->ddic_panel_id != BOE_HX83112E_RGBW_ID_HMA)) {
		hisifd->de_info.frame_gain_limit = param->frame_gain_limit;
		hisifd->de_info.pwm_duty_gain = param->pwm_duty_gain;
		hisifd->de_info.color_distortion_allowance = param->color_distortion_allowance;
		hisifd->de_info.pixel_gain_limit = param->pixel_gain_limit;
	}
	mutex_unlock(&g_rgbw_lock);
	return ret;
}

int display_engine_hbm_param_set(struct hisi_fb_data_type *hisifd, display_engine_hbm_param_t *param)
{
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if(NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}
	if(NULL == param) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if(NULL == pdata){
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	hisifd->de_info.hbm_level = param->level;
	hisifd->de_info.hbm_dimming = param->dimming == 1 ? true : false;
	down(&hisifd->blank_sem);
	if(!hisifd->panel_power_on) {
		HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
		ret = -1;
		goto err_out;
	}
	if(pdata->lcd_hbm_set_func) {
		hisifb_activate_vsync(hisifd);
		pdata->lcd_hbm_set_func(hisifd);
		hisifb_deactivate_vsync(hisifd);
	} else {
		HISI_FB_ERR("[effect] lcd_hbm_set_func is NULL\n");
		ret = -1;
		goto err_out;
	}

err_out:
	hisifd->de_info.last_hbm_level = hisifd->de_info.hbm_level;
	up(&hisifd->blank_sem);
	return ret;
}

int display_engine_amoled_algo_param_set(struct hisi_fb_data_type *hisifd, display_engine_amoled_param_t *param)
{
	int ret = 0;

	if(NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}
	if(NULL == param) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	down(&hisifd->blank_sem);
	hisifd->de_info.amoled_param.HBMEnable = param->HBMEnable;
	hisifd->de_info.amoled_param.AmoledDimingEnable = param->AmoledDimingEnable;
	hisifd->de_info.amoled_param.HBM_Threshold_BackLight = param->HBM_Threshold_BackLight;
	hisifd->de_info.amoled_param.HBM_Min_BackLight = param->HBM_Min_BackLight;
	hisifd->de_info.amoled_param.HBM_Max_BackLight = param->HBM_Max_BackLight;
	hisifd->de_info.amoled_param.HBM_MinLum_Regvalue = param->HBM_MinLum_Regvalue;
	hisifd->de_info.amoled_param.HBM_MaxLum_Regvalue = param->HBM_MaxLum_Regvalue;
	hisifd->de_info.amoled_param.Hiac_DBVThres = param->Hiac_DBVThres;
	hisifd->de_info.amoled_param.Hiac_DBV_XCCThres = param->Hiac_DBV_XCCThres;
	hisifd->de_info.amoled_param.Hiac_DBV_XCC_MinThres = param->Hiac_DBV_XCC_MinThres;
	hisifd->de_info.amoled_param.Lowac_DBVThres = param->Lowac_DBVThres;
	hisifd->de_info.amoled_param.Lowac_DBV_XCCThres = param->Lowac_DBV_XCCThres;
	hisifd->de_info.amoled_param.Lowac_DBV_XCC_MinThres = param->Lowac_DBV_XCC_MinThres;
	hisifd->de_info.amoled_param.Lowac_Fixed_DBVThres= param->Lowac_Fixed_DBVThres;
	up(&hisifd->blank_sem);
	HISI_FB_DEBUG("[effect] first screen on ! HBM_Max_BackLight:%d Hiac_DBVThres: %d HBMEnable: %d AmoledDimingEnable : %d\n",param->HBM_Max_BackLight,param->Hiac_DBVThres,param->HBMEnable,param->AmoledDimingEnable);
	HISI_FB_DEBUG("[effect] first screen on ! Lowac_DBVThres:%d Lowac_DBV_XCCThres: %d Lowac_DBV_XCC_MinThres: %d Lowac_Fixed_DBVThres : %d\n",param->Lowac_DBVThres,param->Lowac_DBV_XCCThres,param->Lowac_DBV_XCC_MinThres,param->Lowac_Fixed_DBVThres);
	return ret;
}

int display_engine_panel_info_get(struct hisi_fb_data_type *hisifd, display_engine_panel_info_param_t *param) {

	struct hisi_panel_info * pinfo = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	pinfo = &(hisifd->panel_info);
	param->width = pinfo->xres;
	param->height = pinfo->yres;
	param->maxluminance = pinfo->hiace_param.iMaxLcdLuminance;
	param->minluminance = pinfo->hiace_param.iMinLcdLuminance;
	param->maxbacklight = pinfo->bl_max;
	param->minbacklight = pinfo->bl_min;
	memcpy(param->lcd_panel_version, pinfo->lcd_panel_version, sizeof(pinfo->lcd_panel_version));
	param->factory_runmode = runmode_is_factory();

	if (pinfo->gamma_lut_table_len && g_factory_gamma_enable) {
		uint32_t i = 0;
		uint16_t *u16_gm_r = param->factory_gamma;
		uint16_t *u16_gm_g = u16_gm_r + pinfo->gamma_lut_table_len;
		uint16_t *u16_gm_b = u16_gm_g + pinfo->gamma_lut_table_len;

		for (i = 0; i < pinfo->gamma_lut_table_len; i++) {
			u16_gm_r[i] = (uint16_t)pinfo->gamma_lut_table_R[i];
			u16_gm_g[i] = (uint16_t)pinfo->gamma_lut_table_G[i];
			u16_gm_b[i] = (uint16_t)pinfo->gamma_lut_table_B[i];
		}
		param->factory_gamma_enable = g_factory_gamma_enable;
	} else {
		param->factory_gamma_enable = 0;
	}

	return 0;
}

int display_engine_color_rectify_param_get(struct hisi_fb_data_type *hisifd, display_engine_color_rectify_param_t *param) {
	int ret = 0;
	struct hisi_fb_panel_data *pdata = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL Pointer\n");
		return -1;
	}

	if (param == NULL) {
		HISI_FB_ERR("[effect] param is NULL Pointer\n");
		return -1;
	}

	if (hisifd->pdev == NULL) {
		HISI_FB_ERR("[effect] pdev is NULL Pointer\n");
		return -1;
	}

	memset(&(hisifd->de_info.lcd_color_oeminfo), 0, sizeof(struct disp_lcdbrightnesscoloroeminfo));

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	down(&hisifd->blank_sem);

	if (!hisifd->panel_power_on) {
		HISI_FB_DEBUG("fb%d, panel power off!\n", hisifd->index);
		ret = -1;
		goto err_out;
	}

	if (pdata->lcd_color_param_get_func) {
		hisifb_activate_vsync(hisifd);
		pdata->lcd_color_param_get_func(hisifd);
		hisifb_deactivate_vsync(hisifd);
	}
	param->lcd_color_oeminfo = hisifd->de_info.lcd_color_oeminfo;
	HISI_FB_DEBUG("[effect] display_engine_color_rectify_param_get params\n");
err_out:
	up(&hisifd->blank_sem);
	return ret;
}

int hisifb_display_engine_blank(int blank_mode, struct fb_info *info) {
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_fb_panel_data *pdata = NULL;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (blank_mode == FB_BLANK_UNBLANK) {
			if (pdata->lcd_cabc_mode_store && hisifd->de_info.ddic_cabc_mode) {
				char buf[8];
				int count = 0;
				hisifb_activate_vsync(hisifd);
				count = snprintf(buf, sizeof(buf), "%d", hisifd->de_info.ddic_cabc_mode);
				pdata->lcd_cabc_mode_store(hisifd->pdev, buf, count);
				hisifb_deactivate_vsync(hisifd);
			}
		}
	}
	return 0;
}

void hisifb_dbv_curve_mapped_init(struct hisi_fb_data_type *hisifd) {
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	if (hisifd->panel_info.dbv_curve_mapped_support && hisifd->panel_info.panel_name) {
		if (!strncmp(hisifd->panel_info.panel_name, "LG_EA9151 6.39' CMD TFT 1440 x 3120", strlen("LG_EA9151 6.39' CMD TFT 1440 x 3120"))) {
				if(!strncmp(hisifd->panel_info.lcd_panel_version, " VER:V4", strlen(" VER:V4"))) {
					hisifd->panel_info.dbv_map_index = 0;
					hisifd->panel_info.is_dbv_need_mapped = 1;
				} else if(!strncmp(hisifd->panel_info.lcd_panel_version, " VER:VN1", strlen(" VER:VN1"))) {
					hisifd->panel_info.dbv_map_index = 1;
					hisifd->panel_info.is_dbv_need_mapped = 1;
				} else if(!strncmp(hisifd->panel_info.lcd_panel_version, " VER:VN2", strlen(" VER:VN2"))) {
					hisifd->panel_info.dbv_map_index= 2;
					hisifd->panel_info.is_dbv_need_mapped = 1;
				}
				HISI_FB_INFO("[effect] hisifd index is %d need_map = %d\n",hisifd->panel_info.dbv_map_index,hisifd->panel_info.is_dbv_need_mapped);
		}
	}
}

int hisifb_display_engine_init(struct fb_info *info, void __user *argp) {
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	display_engine_t de;
	int ret = 0;
	const char* wq_name = "fb0_display_engine";

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("[effect] pdata is NULL Pointer\n");
		return -1;
	}

	if (!hisifd->de_info.is_ready) {
		memset(&hisifd->de_info, 0, sizeof(display_engine_info_t));
		mutex_init(&hisifd->de_info.param_lock);

		hisifd->display_engine_wq = create_singlethread_workqueue(wq_name);
		if (!hisifd->display_engine_wq) {
			HISI_FB_ERR("[effect] create display engine workqueue failed!\n");
			goto ERR_OUT;
		}
		INIT_WORK(&hisifd->display_engine_work, hisifb_display_engine_workqueue_handler);

		hisifd->de_info.is_ready = true;
	}

	hisifb_dbv_curve_mapped_init(hisifd);

	de.blc_support = 1;
	de.ddic_cabc_support = pdata->lcd_cabc_mode_store == NULL ? 0 : 1;
	de.ddic_rgbw_support = pdata->lcd_rgbw_set_func == NULL ? 0 : 1;
	de.ddic_hbm_support = pdata->lcd_hbm_set_func == NULL ? 0 : 1;
	de.ddic_color_support = pdata->lcd_ce_mode_store == NULL ? 0 : 1;
	de.ddic_color_rectify_support = pdata->lcd_color_param_get_func == NULL ? 0 : 1;

	ret = (int)copy_to_user(argp, &de, sizeof(display_engine_t));
	if (ret) {
		HISI_FB_ERR("[effect] copy_to_user(display_engine_t) failed! ret=%d.\n", ret);
		goto ERR_OUT;
	}

ERR_OUT:
	return ret;
}

int hisifb_display_engine_deinit(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	display_engine_t de;
	int ret = 0;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (hisifd->de_info.is_ready) {
		if (hisifd->display_engine_wq) {
			destroy_workqueue(hisifd->display_engine_wq);
			hisifd->display_engine_wq = NULL;
		}
		mutex_lock(&hisifd->de_info.param_lock);
		mutex_destroy(&hisifd->de_info.param_lock);

		ret = (int)copy_from_user(&de, argp, sizeof(display_engine_t));
		if (ret) {
			HISI_FB_ERR("[effect] copy_from_user(display_engine_t) failed! ret=%d.\n", ret);
			goto ERR_OUT;
		}
		hisifd->de_info.is_ready = false;
	}

ERR_OUT:
	return ret;
}

int hisifb_display_engine_param_get(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	display_engine_param_t de_param;
	int ret = 0;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (!hisifd->de_info.is_ready) {
		HISI_FB_ERR("[effect] display engine has not been initialized!\n");
		return -EINVAL;
	}

	ret = (int)copy_from_user(&de_param, argp, sizeof(display_engine_param_t));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		goto ERR_OUT;
	}

	if (de_param.modules & DISPLAY_ENGINE_BLC) {
		ret = display_engine_blc_param_get(hisifd, &de_param.blc);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get BLC, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_CABC) {
		ret = display_engine_ddic_cabc_param_get(hisifd, &de_param.ddic_cabc);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get BLC, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_COLOR) {
		ret = display_engine_ddic_color_param_get(hisifd, &de_param.ddic_color);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get BLC, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_RGBW) {
		ret = display_engine_ddic_rgbw_param_get(hisifd, &de_param.ddic_rgbw);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get rgbw, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_PANEL_INFO) {
		HISI_FB_INFO("[effect] DISPLAY_ENGINE_PANEL_INFO.\n");
		ret = display_engine_panel_info_get(hisifd, &de_param.panel_info);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get panel info, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_COLOR_RECTIFY) {
		HISI_FB_INFO("[effect] DISPLAY_ENGINE_COLOR_RECTIFY.\n");
		ret = display_engine_color_rectify_param_get(hisifd, &de_param.color_param);
		if (ret) {
			HISI_FB_ERR("[effect] failed to get color param, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	ret = (int)copy_to_user(argp, &de_param, sizeof(display_engine_param_t));
	if (ret) {
		HISI_FB_ERR("[effect] copy_to_user(param) failed! ret=%d.\n", ret);
		goto ERR_OUT;
	}

ERR_OUT:
	return ret;
}

int hisifb_param_check(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (!hisifd->de_info.is_ready) {
		HISI_FB_ERR("[effect] display engine has not been initialized!\n");
		return -EINVAL;
	}

	return 0;
}

int hisifb_runmode_check(display_engine_param_t* de_param)
{
	if(de_param == NULL){
		HISI_FB_ERR("de_param is null pointer\n");
		return -1;
	}

	if (de_param->modules & DISPLAY_ENGINE_DDIC_RGBW || de_param->modules & DISPLAY_ENGINE_HBM) {
		return 0;
	} else if (runmode_is_factory()) {
		return -EINVAL;
	}

	return 0;
}

int hisifb_display_engine_param_set(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	display_engine_param_t de_param;
	uint32_t hisifd_modules = 0;
	int ret = 0;

	if (hisifb_param_check(info, argp)) {
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if(hisifd == NULL){
		HISI_FB_ERR("hisifd is null pointer\n");
		return -1;
	}

	ret = (int)copy_from_user(&de_param, argp, sizeof(display_engine_param_t));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		goto ERR_OUT;
	}

	if (hisifb_runmode_check(&de_param)) {
		return -EINVAL;
	}

	mutex_lock(&hisifd->de_info.param_lock);
	hisifd_modules = hisifd->de_param.modules;
	hisifd->de_param = de_param;
	hisifd_modules |= de_param.modules;
	hisifd->de_param.modules = hisifd_modules;
	mutex_unlock(&hisifd->de_info.param_lock);
	if (hisifd->display_engine_wq) {
		queue_work(hisifd->display_engine_wq, &hisifd->display_engine_work);
	}

ERR_OUT:
	return ret;
}

void hisifb_display_engine_workqueue_handler(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	display_engine_param_t de_param;
	int ret = 0;

	if (NULL == work) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	hisifd = container_of(work, struct hisi_fb_data_type, display_engine_work);
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	mutex_lock(&hisifd->de_info.param_lock);
	de_param = hisifd->de_param;
	hisifd->de_param.modules = 0;
	mutex_unlock(&hisifd->de_info.param_lock);

	if (de_param.modules & DISPLAY_ENGINE_BLC) {
		ret = display_engine_blc_param_set(hisifd, &de_param.blc);
		if (ret < 0) {
			HISI_FB_ERR("[effect] failed to set BLC, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_CABC) {
		ret = display_engine_ddic_cabc_param_set(hisifd, &de_param.ddic_cabc);
		if (ret) {
			HISI_FB_ERR("[effect] failed to set DDIC_CABC, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_RGBW) {
		ret = display_engine_ddic_rgbw_param_set(hisifd, &de_param.ddic_rgbw);
		if (ret) {
			HISI_FB_ERR("[effect] failed to set DDIC_RGBW, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_DDIC_COLOR) {
		ret = display_engine_ddic_color_param_set(hisifd, &de_param.ddic_color);
		if (ret) {
			HISI_FB_ERR("[effect] failed to set DDIC_COLOR, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_HBM) {
		ret = display_engine_hbm_param_set(hisifd, &de_param.hbm);
		if (ret) {
			HISI_FB_ERR("[effect] failed to set HBM, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_AMOLED_ALGO) {
		HISI_FB_DEBUG("[effect] failed to set DISPLAY_ENGINE_AMOLED_ALGO, ret=%d\n", ret);
		ret = display_engine_amoled_algo_param_set(hisifd, &de_param.amoled_param);
		if (ret) {
			HISI_FB_ERR("[effect] failed to set Amoled, ret=%d\n", ret);
			goto ERR_OUT;
		}
	}

	if (de_param.modules & DISPLAY_ENGINE_FLICKER_DETECTOR) {
		bl_flicker_detector_init(de_param.flicker_detector_config);
	}

ERR_OUT:
	return;
}

ssize_t hisifb_display_effect_bl_ctrl_store(struct fb_info *info, const char *buf, size_t count) {
	struct hisi_fb_data_type *hisifd = NULL;
	ktime_t current_bl_timestamp = ktime_get();
	int bl_timeinterval = 16670000;
	ssize_t ret = (ssize_t)count;
	uint32_t len = 0;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	if (NULL == buf) {
		HISI_FB_ERR("[effect] buf is NULL\n");
		return -1;
	}

	len = (uint32_t)strlen(buf);
	if (len < 3) {
		HISI_FB_ERR("[effect] invalid input buf, length=%d\n", len);
		return -1;
	}
	if (buf[1] != ':') {
		HISI_FB_ERR("[effect] invalid input buf:%s\n", buf);
		return -1;
	}

	down(&hisifd->blank_sem_effect);

	hisifd->de_info.blc_enable = (buf[0] == '1') ? true : false;
	hisifd->de_info.blc_delta = (int)simple_strtol(buf + 2, NULL, 0);

	if (abs((int)(ktime_to_ns(hisifd->backlight.bl_timestamp) - ktime_to_ns(current_bl_timestamp))) > bl_timeinterval) {
		HISI_FB_DEBUG("[effect] delta:%d bl:%d enable:%d set, return %d\n", hisifd->de_info.blc_delta, hisifd->bl_level, hisifd->de_info.blc_enable, (uint32_t)ret);
		if (hisifd->bl_level > 0) {
			down(&hisifd->brightness_esd_sem);
			hisifb_set_backlight(hisifd, hisifd->bl_level, true);
			up(&hisifd->brightness_esd_sem);
		}
	} else {
		HISI_FB_DEBUG("[effect] delta:%d bl:%d enable:%d skip, return 0\n", hisifd->de_info.blc_delta, hisifd->bl_level, hisifd->de_info.blc_enable);
		ret = 0; // This delta has been skipped, and hal may resent the command.
	}

	up(&hisifd->blank_sem_effect);
	return ret;
}

/*lint +e838, +e778, +e845, +e712, +e527, +e30, +e142, +e715, +e655, +e550*/
