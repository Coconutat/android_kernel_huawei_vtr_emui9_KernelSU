/* Copyright (c) 2013-2016, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include "lcdkit_panel.h"
extern struct lcdkit_esd_error_info g_esd_error_info;


#define HISI_ESD_RECOVER_MAX_COUNT   (10)
#define HISI_ESD_CHECK_MAX_COUNT     (3)

extern unsigned int g_esd_recover_disable;

static void hisifb_frame_refresh_for_esd(struct hisi_fb_data_type *hisifd)
{
	char *envp[2];
	char buf[64];
	snprintf(buf, sizeof(buf), "Refresh=1");
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&(hisifd->fbi->dev->kobj), KOBJ_CHANGE, envp);

	HISI_FB_INFO("ESD_HAPPENDED=1!\n");
}

static void hisifb_esd_recover(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;
	uint32_t bl_level_cur = 0;

	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is null");
		return ;
	}

	if (g_esd_recover_disable) {
		HISI_FB_ERR("g_esd_recover_disable is disable");
		return;
	}

	down(&hisifd->brightness_esd_sem);
	bl_level_cur = hisifd->bl_level;
	hisifb_set_backlight(hisifd, 0, false);
	up(&hisifd->brightness_esd_sem);

	/*lcd panel off*/
	ret = hisi_fb_blank_sub(FB_BLANK_POWERDOWN, hisifd->fbi);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, blank_mode(%d) failed!\n", hisifd->index, FB_BLANK_POWERDOWN);
	}
	/*lcd panel on*/
	ret = hisi_fb_blank_sub(FB_BLANK_UNBLANK, hisifd->fbi);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, blank_mode(%d) failed!\n", hisifd->index, FB_BLANK_UNBLANK);
	}
	hisifb_frame_refresh_for_esd(hisifd);
	/*backlight on*/
	msleep(100);
	down(&hisifd->brightness_esd_sem);
	hisifb_set_backlight(hisifd, bl_level_cur? bl_level_cur:hisifd->bl_level, false);
	up(&hisifd->brightness_esd_sem);
	return ;
}

static void dsm_client_record_esd_err(void)
{
	int i=0;

	if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
		dsm_client_record(lcd_dclient, "lcd esd register status error:");
		for (i = 0; i < g_esd_error_info.esd_error_reg_num; i++)
		{
			dsm_client_record(lcd_dclient, "read_reg_val[%d]=0x%x, expect_reg_val[%d]=0x%x",
				g_esd_error_info.esd_reg_index[i],
				g_esd_error_info.esd_error_reg_val[i],
				g_esd_error_info.esd_reg_index[i],
				g_esd_error_info.esd_expect_reg_val[i]);
		}
		dsm_client_record(lcd_dclient, "\n");
		dsm_client_notify(lcd_dclient, DSM_LCD_ESD_STATUS_ERROR_NO);
	}

	return;
}

static void hisifb_esd_check_wq_handler(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_esd *esd_ctrl = NULL;
	int ret = 0;
	int recover_count = 0;
	int esd_check_count = 0;

	esd_ctrl = container_of(work, struct hisifb_esd, esd_check_work);
	if (NULL == esd_ctrl) {
		HISI_FB_ERR("esd_ctrl is NULL");
		return;
	}
	hisifd = esd_ctrl->hisifd;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (!hisifd->panel_info.esd_enable || g_esd_recover_disable) {
		if (g_esd_recover_disable) {
			HISI_FB_INFO("esd_enable=%d, g_esd_recover_disable=%d",
				hisifd->panel_info.esd_enable, g_esd_recover_disable);
		}
		return ;
	}
	while (recover_count < HISI_ESD_RECOVER_MAX_COUNT) {
		if (esd_check_count < HISI_ESD_CHECK_MAX_COUNT) {
			if (DSS_SEC_RUNNING == hisifd->secure_ctrl.secure_status)
				break;

			if ((DSS_SEC_IDLE == hisifd->secure_ctrl.secure_status)
				&& (DSS_SEC_ENABLE == hisifd->secure_ctrl.secure_event))
				break;

			ret = hisifb_ctrl_esd(hisifd);
			if (ret || (ESD_RECOVER_STATE_START == hisifd->esd_recover_state)) {
				esd_check_count++;
				hisifd->esd_happened = 1;
				HISI_FB_INFO("esd check abnormal, esd_check_count:%d!\n", esd_check_count);
			} else {
				hisifd->esd_happened = 0;
				break;
			}
		}

		if ((esd_check_count >= HISI_ESD_CHECK_MAX_COUNT) || (ESD_RECOVER_STATE_START == hisifd->esd_recover_state)) {
			HISI_FB_ERR("esd recover panel, recover_count:%d!\n",recover_count);
			dsm_client_record_esd_err();
			hisifb_esd_recover(hisifd);
			hisifd->esd_recover_state = ESD_RECOVER_STATE_COMPLETE;
			esd_check_count = 0;
			recover_count++;
		}
	}

	// recover count equate 5, we disable esd check function
	if (recover_count >= HISI_ESD_RECOVER_MAX_COUNT) {
		hrtimer_cancel(&esd_ctrl->esd_hrtimer);
		hisifd->panel_info.esd_enable = 0;
		HISI_FB_ERR("esd recover %d count, disable esd function\n", HISI_ESD_RECOVER_MAX_COUNT);
	}
}

static enum hrtimer_restart hisifb_esd_hrtimer_fnc(struct hrtimer *timer)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_esd *esd_ctrl = NULL;

	esd_ctrl = container_of(timer, struct hisifb_esd, esd_hrtimer);
	if (NULL == esd_ctrl) {
		HISI_FB_ERR("esd_ctrl is NULL");
		return HRTIMER_NORESTART;
	}
	hisifd = esd_ctrl->hisifd;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return HRTIMER_NORESTART;
	}

	if (hisifd->panel_info.esd_enable) {
		if (esd_ctrl->esd_check_wq) {
			queue_work(esd_ctrl->esd_check_wq, &(esd_ctrl->esd_check_work));
		}
	}
	hrtimer_start(&esd_ctrl->esd_hrtimer, ktime_set(ESD_CHECK_TIME_PERIOD / 1000,
		(ESD_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}


void hisifb_esd_register(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_esd *esd_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}
	esd_ctrl = &(hisifd->esd_ctrl);
	if (NULL == esd_ctrl) {
		dev_err(&pdev->dev, "esd_ctrl is NULL");
		return;
	}

	if (esd_ctrl->esd_inited) {
		return;
	}

	if (hisifd->panel_info.esd_enable) {
		hisifd->esd_happened = 0;
		hisifd->esd_recover_state = ESD_RECOVER_STATE_NONE;
		esd_ctrl->hisifd = hisifd;

		esd_ctrl->esd_check_wq = create_singlethread_workqueue("esd_check");
		if (!esd_ctrl->esd_check_wq) {
			dev_err(&pdev->dev, "create esd_check_wq failed\n");
		}

		INIT_WORK(&esd_ctrl->esd_check_work, hisifb_esd_check_wq_handler);

		/* hrtimer for ESD timing */
		hrtimer_init(&esd_ctrl->esd_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		esd_ctrl->esd_hrtimer.function = hisifb_esd_hrtimer_fnc;
		hrtimer_start(&esd_ctrl->esd_hrtimer, ktime_set(ESD_CHECK_TIME_PERIOD / 1000,
			(ESD_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);

		esd_ctrl->esd_inited = 1;
	}
}

void hisifb_esd_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_esd *esd_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}
	esd_ctrl = &(hisifd->esd_ctrl);
	if (NULL == esd_ctrl) {
		dev_err(&pdev->dev, "esd_ctrl is NULL");
		return;
	}

	if (!esd_ctrl->esd_inited)
		return;

	if (hisifd->panel_info.esd_enable) {
		hrtimer_cancel(&esd_ctrl->esd_hrtimer);
	}

	esd_ctrl->esd_inited = 0;
}
