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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

#include "hisi_fb.h"

/*
** /sys/class/graphics/fb0/vsync_event
*/
#define VSYNC_CTRL_EXPIRE_COUNT	(4)
#define MASKLAYER_BACKLIGHT_WAIT_VSYNC_COUNT  (2)

extern void mali_kbase_pm_report_vsync(int);
extern int mipi_dsi_ulps_cfg(struct hisi_fb_data_type *hisifd, int enable);
extern bool hisi_dss_check_reg_reload_status(struct hisi_fb_data_type *hisifd);

void hisifb_masklayer_backlight_flag_config(struct hisi_fb_data_type *hisifd,
	bool masklayer_backlight_flag)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (masklayer_backlight_flag == true) {
			hisifd->masklayer_maxbacklight_flag = true;
		}
	}
}

static void hisifb_masklayer_backlight_notify(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (hisifd->masklayer_backlight_notify_wq) {
			queue_work(hisifd->masklayer_backlight_notify_wq, &hisifd->masklayer_backlight_notify_work);
		}
	}
}

void hisifb_masklayer_backlight_notify_handler(struct work_struct *work)
{
	static uint32_t vsync_count = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == work) {
		HISI_FB_ERR("work is NULL.\n");
		return;
	}

	hisifd = container_of(work, struct hisi_fb_data_type, masklayer_backlight_notify_work);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL.\n");
		return;
	}

	if ((hisifd->index == PRIMARY_PANEL_IDX) && (g_dss_version_tag == FB_ACCEL_DSSV501)) {
		if (g_debug_online_vsync) {
			HISI_FB_DEBUG("flag=%d, vsync_count=%d.\n", hisifd->masklayer_maxbacklight_flag, vsync_count);
		}

		if (hisifd->masklayer_maxbacklight_flag == true) {
			vsync_count += 1;
			if (vsync_count == MASKLAYER_BACKLIGHT_WAIT_VSYNC_COUNT) {
				HISI_FB_INFO("mask layer max backlight done notify.\n");
				vsync_count = 0;
				hisifd->masklayer_maxbacklight_flag = false;
			}
		} else {
			vsync_count = 0;
		}
	}
}

void hisifb_frame_updated(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	if (hisifd->vsync_ctrl.vsync_report_fnc) {
		atomic_inc(&(hisifd->vsync_ctrl.buffer_updated));
	}
}

void hisifb_vsync_isr_handler(struct hisi_fb_data_type *hisifd)
{
	struct hisifb_vsync *vsync_ctrl = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	int buffer_updated = 0;
	ktime_t pre_vsync_timestamp;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}

	pre_vsync_timestamp = vsync_ctrl->vsync_timestamp;
	vsync_ctrl->vsync_timestamp = ktime_get();

	hisifb_masklayer_backlight_notify(hisifd);

	if (hisifd->vsync_ctrl.vsync_enabled) {
		wake_up_interruptible_all(&(vsync_ctrl->vsync_wait));
	}

	if (hisifd->panel_info.vsync_ctrl_type != VSYNC_CTRL_NONE) {
		spin_lock(&vsync_ctrl->spin_lock);
		if (vsync_ctrl->vsync_ctrl_expire_count) {
			vsync_ctrl->vsync_ctrl_expire_count--;

			if (vsync_ctrl->vsync_ctrl_expire_count == 1) {
				if (hisifd->panel_info.fps_updt_support
					&& (!hisifd->panel_info.fps_updt_panel_only)
					&& pdata->lcd_fps_scence_handle) {
						pdata->lcd_fps_scence_handle(hisifd->pdev, LCD_FPS_SCENCE_IDLE);
					}
			}

			if (vsync_ctrl->vsync_ctrl_expire_count == 0)
				schedule_work(&vsync_ctrl->vsync_ctrl_work);
		}
		spin_unlock(&vsync_ctrl->spin_lock);
	}

	if (vsync_ctrl->vsync_report_fnc) {
		if (hisifd->vsync_ctrl.vsync_enabled) {
			buffer_updated = atomic_dec_return(&(vsync_ctrl->buffer_updated));
		} else {
			buffer_updated = 1;
		}

		if (buffer_updated < 0) {
			atomic_cmpxchg(&(vsync_ctrl->buffer_updated), buffer_updated, 1);
		} else if (is_mipi_video_panel(hisifd)){
			vsync_ctrl->vsync_report_fnc(0);
		} else {
			vsync_ctrl->vsync_report_fnc(buffer_updated);
		}
	}

	if (g_debug_online_vsync) {
		HISI_FB_INFO("fb%d, VSYNC=%llu, time_diff=%llu.\n", hisifd->index,
			ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp),
			(ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp) - ktime_to_ns(pre_vsync_timestamp)));
	}
}

static int vsync_timestamp_changed(struct hisi_fb_data_type *hisifd,
	ktime_t prev_timestamp)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	return !ktime_equal(prev_timestamp, hisifd->vsync_ctrl.vsync_timestamp);
}

static ssize_t vsync_show_event(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = -1;
	int vsync_flag = 0;
	bool report_flag = false;
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_secure *secure_ctrl = NULL;
	ktime_t prev_timestamp;

	if (NULL == dev) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (NULL == fbi) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	secure_ctrl = &(hisifd->secure_ctrl);
	prev_timestamp = hisifd->vsync_ctrl.vsync_timestamp;

	/*lint -e666*/
	ret = wait_event_interruptible(hisifd->vsync_ctrl.vsync_wait,
		(vsync_timestamp_changed(hisifd, prev_timestamp) && hisifd->vsync_ctrl.vsync_enabled)
		|| (!!secure_ctrl->tui_need_switch));
	/*lint +e666*/

	vsync_flag = (vsync_timestamp_changed(hisifd, prev_timestamp) &&
						hisifd->vsync_ctrl.vsync_enabled);

	report_flag = !!secure_ctrl->tui_need_switch;

	if (vsync_flag && report_flag) {
		HISI_FB_INFO("report (secure_event = %d) to hwc with vsync at (frame_no = %d).\n",
						secure_ctrl->secure_event, hisifd->ov_req.frame_no);

		ret = snprintf(buf, PAGE_SIZE, "VSYNC=%llu, SecureEvent=%d \n",
			ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp), secure_ctrl->secure_event);
		buf[strlen(buf) + 1] = '\0';
		if (secure_ctrl->secure_event == DSS_SEC_DISABLE) {
			secure_ctrl->tui_need_switch = 0;
		}

	} else if (vsync_flag) {
		ret = snprintf(buf, PAGE_SIZE, "VSYNC=%llu, xxxxxxEvent=x \n",
			ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp));
		buf[strlen(buf) + 1] = '\0';
		secure_ctrl->tui_need_skip_report = 0;

	} else if (report_flag && !secure_ctrl->tui_need_skip_report) {
		HISI_FB_INFO("report (secure_event = %d) to hwc at (frame_no = %d).\n",
						secure_ctrl->secure_event, hisifd->ov_req.frame_no);

		ret = snprintf(buf, PAGE_SIZE, "xxxxx=%llu, SecureEvent=%d \n",
			ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp), secure_ctrl->secure_event);
		buf[strlen(buf) + 1] = '\0';
		secure_ctrl->tui_need_skip_report = 1;
		if (secure_ctrl->secure_event == DSS_SEC_DISABLE) {
			secure_ctrl->tui_need_switch = 0;
		}

	} else {
		return -1;
	}

	return ret;
}

static ssize_t vsync_timestamp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = -1;
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == dev) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	fbi = dev_get_drvdata(dev);
	if (NULL == fbi) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer.\n");
		return -1;
	}

	ret = snprintf(buf, PAGE_SIZE, "%llu \n",
		ktime_to_ns(hisifd->vsync_ctrl.vsync_timestamp));
	buf[strlen(buf) + 1] = '\0';

	return ret;
}

static DEVICE_ATTR(vsync_event, S_IRUGO, vsync_show_event, NULL);
static DEVICE_ATTR(vsync_timestamp, S_IRUGO, vsync_timestamp_show, NULL);


static void hisifb_vsync_ctrl_workqueue_handler(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	unsigned long flags = 0;

	vsync_ctrl = container_of(work, typeof(*vsync_ctrl), vsync_ctrl_work);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return;
	}
	hisifd = vsync_ctrl->hisifd;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}

	down(&(hisifd->blank_sem));

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel is power off!", hisifd->index);
		up(&(hisifd->blank_sem));
		return;
	}

	mutex_lock(&(vsync_ctrl->vsync_lock));
	if (vsync_ctrl->vsync_ctrl_disabled_set &&
		!vsync_ctrl->vsync_disable_enter_idle &&
		(vsync_ctrl->vsync_ctrl_expire_count == 0) &&
		vsync_ctrl->vsync_ctrl_enabled &&
		!vsync_ctrl->vsync_enabled && !vsync_ctrl->vsync_ctrl_offline_enabled) {
		HISI_FB_DEBUG("fb%d, dss clk off!\n", hisifd->index);

		spin_lock_irqsave(&(vsync_ctrl->spin_lock), flags);
		if (pdata->vsync_ctrl) {
			pdata->vsync_ctrl(hisifd->pdev, 0);
		} else {
			HISI_FB_ERR("fb%d, vsync_ctrl not supported!\n", hisifd->index);
		}
		vsync_ctrl->vsync_ctrl_enabled = 0;
		vsync_ctrl->vsync_ctrl_disabled_set = 0;
		spin_unlock_irqrestore(&(vsync_ctrl->spin_lock), flags);

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_MIPI_ULPS) {
			mipi_dsi_ulps_cfg(hisifd, 0);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_VCC_OFF) {
			if (hisifd->lp_fnc)
				hisifd->lp_fnc(hisifd, true);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_CLK_OFF) {
			dpe_inner_clk_disable(hisifd);
			dpe_common_clk_disable(hisifd);
			mipi_dsi_clk_disable(hisifd);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_VCC_OFF) {
			dpe_regulator_disable(hisifd);
		}
		//hisifb_buf_sync_suspend(hisifd);
	}
	mutex_unlock(&(vsync_ctrl->vsync_lock));

	if (vsync_ctrl->vsync_report_fnc) {
		if (is_mipi_video_panel(hisifd)) {
			vsync_ctrl->vsync_report_fnc(0);
		} else {
			vsync_ctrl->vsync_report_fnc(1);
		}
	}

	up(&(hisifd->blank_sem));
}

void hisifb_vsync_register(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		dev_err(&pdev->dev, "vsync_ctrl is NULL");
		return;
	}

	if (vsync_ctrl->vsync_created)
		return;

	vsync_ctrl->hisifd = hisifd;
	vsync_ctrl->vsync_infinite = 0;
	vsync_ctrl->vsync_enabled = 0;
	vsync_ctrl->vsync_ctrl_offline_enabled = 0;
	vsync_ctrl->vsync_timestamp = ktime_get();
	vsync_ctrl->vactive_timestamp = ktime_get();
	init_waitqueue_head(&(vsync_ctrl->vsync_wait));
	spin_lock_init(&(vsync_ctrl->spin_lock));
	INIT_WORK(&vsync_ctrl->vsync_ctrl_work, hisifb_vsync_ctrl_workqueue_handler);

	mutex_init(&(vsync_ctrl->vsync_lock));

	atomic_set(&(vsync_ctrl->buffer_updated), 1);
	vsync_ctrl->vsync_report_fnc = mali_kbase_pm_report_vsync;

	if (hisifd->sysfs_attrs_append_fnc) {
		hisifd->sysfs_attrs_append_fnc(hisifd, &dev_attr_vsync_event.attr);
		hisifd->sysfs_attrs_append_fnc(hisifd, &dev_attr_vsync_timestamp.attr);
	}

	vsync_ctrl->vsync_created = 1;
}

void hisifb_vsync_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		dev_err(&pdev->dev, "vsync_ctrl is NULL");
		return;
	}

	if (!vsync_ctrl->vsync_created)
		return;

	vsync_ctrl->vsync_created = 0;
}

void hisifb_set_vsync_activate_state(struct hisi_fb_data_type *hisifd, bool infinite)
{
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return;
	}

	if (hisifd->panel_info.vsync_ctrl_type == VSYNC_CTRL_NONE)
		return;

	mutex_lock(&(vsync_ctrl->vsync_lock));

	if (infinite) {
		vsync_ctrl->vsync_infinite_count += 1;
	} else {
		vsync_ctrl->vsync_infinite_count -= 1;
	}

	if (vsync_ctrl->vsync_infinite_count >= 1) {
		vsync_ctrl->vsync_infinite = 1;
	}

	if (vsync_ctrl->vsync_infinite_count == 0) {
		vsync_ctrl->vsync_infinite = 0;
	}

	mutex_unlock(&(vsync_ctrl->vsync_lock));
}

void hisifb_activate_vsync(struct hisi_fb_data_type *hisifd)
{
	struct hisi_fb_panel_data *pdata = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	unsigned long flags = 0;
	int clk_enabled = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return;
	}

	if (hisifd->panel_info.vsync_ctrl_type == VSYNC_CTRL_NONE)
		return;

	mutex_lock(&(vsync_ctrl->vsync_lock));

	if (vsync_ctrl->vsync_ctrl_enabled == 0) {
		HISI_FB_DEBUG("fb%d, dss clk on!\n", hisifd->index);

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_VCC_OFF) {
			dpe_regulator_enable(hisifd);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_CLK_OFF) {
			mipi_dsi_clk_enable(hisifd);
			dpe_common_clk_enable(hisifd);
			dpe_inner_clk_enable(hisifd);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_VCC_OFF) {
			if (hisifd->lp_fnc)
				hisifd->lp_fnc(hisifd, false);
		}

		if (hisifd->panel_info.vsync_ctrl_type & VSYNC_CTRL_MIPI_ULPS) {
			mipi_dsi_ulps_cfg(hisifd, 1);
		}

		vsync_ctrl->vsync_ctrl_enabled = 1;
		clk_enabled = 1;
	} else if (vsync_ctrl->vsync_ctrl_isr_enabled) {
		clk_enabled = 1;
		vsync_ctrl->vsync_ctrl_isr_enabled = 0;
	} else {
		;
	}

	spin_lock_irqsave(&(vsync_ctrl->spin_lock), flags);
	vsync_ctrl->vsync_ctrl_disabled_set = 0;
	vsync_ctrl->vsync_ctrl_expire_count = 0;

	if (hisifd->panel_info.fps_updt_support
		&& (!hisifd->panel_info.fps_updt_panel_only)
		&& pdata->lcd_fps_scence_handle) {
		pdata->lcd_fps_scence_handle(hisifd->pdev, LCD_FPS_SCENCE_NORMAL);
	}

	if (clk_enabled) {
		if (pdata->vsync_ctrl) {
			pdata->vsync_ctrl(hisifd->pdev, 1);
		} else {
			HISI_FB_ERR("fb%d, vsync_ctrl not supported!\n", hisifd->index);
		}
	}
	spin_unlock_irqrestore(&(vsync_ctrl->spin_lock), flags);

	mutex_unlock(&(vsync_ctrl->vsync_lock));
}

void hisifb_deactivate_vsync(struct hisi_fb_data_type *hisifd)
{
	struct hisi_fb_panel_data *pdata = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	unsigned long flags = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return;
	}

	if (hisifd->panel_info.vsync_ctrl_type == VSYNC_CTRL_NONE)
		return;

	if (hisifd->secure_ctrl.secure_event == DSS_SEC_ENABLE) {
		return;
	}

	mutex_lock(&(vsync_ctrl->vsync_lock));

	spin_lock_irqsave(&(vsync_ctrl->spin_lock), flags);
	if (vsync_ctrl->vsync_infinite == 0)
		vsync_ctrl->vsync_ctrl_disabled_set = 1;

	if (vsync_ctrl->vsync_ctrl_enabled)
		vsync_ctrl->vsync_ctrl_expire_count = VSYNC_CTRL_EXPIRE_COUNT;
	spin_unlock_irqrestore(&(vsync_ctrl->spin_lock), flags);

	mutex_unlock(&(vsync_ctrl->vsync_lock));
}

int hisifb_vsync_ctrl(struct fb_info *info, void __user *argp)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	struct hisifb_vsync *vsync_ctrl = NULL;
	int enable = 0;

	if (NULL == info) {
		HISI_FB_ERR("vsync ctrl info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("vsync ctrl hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	if ((hisifd->index != PRIMARY_PANEL_IDX)
		&& (hisifd->index != EXTERNAL_PANEL_IDX)) {
		HISI_FB_ERR("fb%d, vsync ctrl not supported!\n", hisifd->index);
		return -EINVAL;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}

	ret = copy_from_user(&enable, argp, sizeof(enable));
	if (ret) {
		HISI_FB_ERR("hisifb_vsync_ctrl ioctl failed!\n");
		return ret;
	}

	enable = (enable) ? 1 : 0;

	mutex_lock(&(vsync_ctrl->vsync_lock));

	if (vsync_ctrl->vsync_enabled == enable) {
		mutex_unlock(&(vsync_ctrl->vsync_lock));
		return 0;
	}

	if (g_debug_online_vsync)
		HISI_FB_INFO("fb%d, enable=%d!\n", hisifd->index, enable);

	vsync_ctrl->vsync_enabled = enable;

	mutex_unlock(&(vsync_ctrl->vsync_lock));

	down(&hisifd->blank_sem);

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel is power off!", hisifd->index);
		up(&hisifd->blank_sem);
		return 0;
	}

	if (enable) {
		hisifb_activate_vsync(hisifd);
	} else {
		hisifb_deactivate_vsync(hisifd);
	}

	up(&hisifd->blank_sem);

	return 0;
}

void hisifb_vsync_disable_enter_idle(struct hisi_fb_data_type *hisifd, bool disable)
{
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return;
	}

	if (hisifd->panel_info.vsync_ctrl_type == VSYNC_CTRL_NONE)
		return;

	mutex_lock(&(vsync_ctrl->vsync_lock));
	if (disable) {
		vsync_ctrl->vsync_disable_enter_idle = 1;
	} else {
		vsync_ctrl->vsync_disable_enter_idle = 0;
	}
	mutex_unlock(&(vsync_ctrl->vsync_lock));
}

int hisifb_vsync_resume(struct hisi_fb_data_type *hisifd)
{
	struct hisifb_vsync *vsync_ctrl = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	vsync_ctrl = &(hisifd->vsync_ctrl);
	if (NULL == vsync_ctrl) {
		HISI_FB_ERR("vsync_ctrl is NULL");
		return -EINVAL;
	}

	vsync_ctrl->vsync_ctrl_expire_count = 0;
	vsync_ctrl->vsync_ctrl_disabled_set = 0;
	vsync_ctrl->vsync_ctrl_enabled = 1;
	vsync_ctrl->vsync_ctrl_isr_enabled = 1;
	//vsync_ctrl->vsync_infinite = 0;

	atomic_set(&(vsync_ctrl->buffer_updated), 1);


	return 0;
}

int hisifb_vsync_suspend(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	hisifd->vsync_ctrl.vsync_enabled = 0;
	return 0;
}
#pragma GCC diagnostic pop
