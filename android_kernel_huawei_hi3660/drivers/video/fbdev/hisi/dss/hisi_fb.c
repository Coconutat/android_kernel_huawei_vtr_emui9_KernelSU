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

#include "hisi_fb.h"
#include "hisi_overlay_utils.h"
#include <huawei_platform/log/log_jank.h>
#include "hisi_display_effect.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include "tui.h"

#include "lcdkit_fb_util.h"


uint8_t color_temp_cal_buf[32] = {0};

static int hisi_fb_resource_initialized;
static struct platform_device *pdev_list[HISI_FB_MAX_DEV_LIST] = {0};

static int pdev_list_cnt;
struct fb_info *fbi_list[HISI_FB_MAX_FBI_LIST] = {0};
static int fbi_list_index;

struct hisi_fb_data_type *hisifd_list[HISI_FB_MAX_FBI_LIST] = {0};
static int hisifd_list_index;

#define HISI_FB_ION_CLIENT_NAME	"hisi_fb_ion"

uint32_t g_dts_resouce_ready = 0;
uint32_t g_fastboot_enable_flag = 0;
uint32_t g_fake_lcd_flag = 0;
uint32_t g_dss_base_phy = 0;
uint32_t g_dss_version_tag = 0;
uint32_t g_dss_module_resource_initialized = 0;
uint32_t g_logo_buffer_base = 0;
uint32_t g_logo_buffer_size = 0;

uint32_t g_fastboot_already_set = 0;

unsigned int g_esd_recover_disable = 0;

struct iommu_domain* g_hisi_domain = NULL;

static char __iomem *hisifd_dss_base;
static char __iomem *hisifd_peri_crg_base;
static char __iomem *hisifd_sctrl_base;
static char __iomem *hisifd_pctrl_base;
static char __iomem *hisifd_noc_dss_base;
static char __iomem *hisifd_mmbuf_crg_base;
static char __iomem *hisifd_mmbuf_asc0_base;
static char __iomem *hisifd_pmctrl_base = NULL;

static char __iomem *hisifd_media_crg_base = NULL;
static char __iomem *hisifd_media_common_base = NULL;
static char __iomem *hisifd_dp_base = NULL;

static uint32_t hisifd_irq_pdp;
static uint32_t hisifd_irq_sdp;
static uint32_t hisifd_irq_adp;
static uint32_t hisifd_irq_mdc;
static uint32_t hisifd_irq_dsi0;
static uint32_t hisifd_irq_dsi1;
static uint32_t hisifd_irq_mmbuf_asc0;
static uint32_t hisifd_irq_dptx = 0;

#define MAX_DPE_NUM	(3)
static struct regulator_bulk_data g_dpe_regulator[MAX_DPE_NUM] =
	{{0}, {0}, {0}};

static const char *g_dss_axi_clk_name;
static const char *g_dss_pclk_dss_name;
static const char *g_dss_pri_clk_name;
static const char *g_dss_pxl0_clk_name;
static const char *g_dss_pxl1_clk_name;
static const char *g_dss_mmbuf_clk_name;
static const char *g_dss_pclk_mmbuf_clk_name;
static const char *g_dss_dphy0_ref_clk_name;
static const char *g_dss_dphy1_ref_clk_name;
static const char *g_dss_dphy0_cfg_clk_name;
static const char *g_dss_dphy1_cfg_clk_name;
static const char *g_dss_pclk_dsi0_name;
static const char *g_dss_pclk_dsi1_name;
static const char *g_dss_pclk_pctrl_name;
static const char *g_dss_clk_gate_dpctrl_16m_name;
static const char *g_dss_pclk_gate_dpctrl_name;
static const char *g_dss_aclk_dpctrl_name;

int g_primary_lcd_xres = 0;
int g_primary_lcd_yres = 0;
uint64_t g_pxl_clk_rate = 0;
uint8_t g_prefix_ce_support = 0;
uint8_t g_prefix_sharpness1D_support = 0;
uint8_t g_prefix_sharpness2D_support = 0;
int g_debug_enable_lcd_sleep_in = 0;
int g_err_status = 0;

struct hisi_fb_data_type *g_hisifd_fb0 = NULL;
struct fb_info *g_info_fb0 = NULL;

/* mmbuf gen pool */
struct gen_pool *g_mmbuf_gen_pool = NULL;


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/
static int hisi_fb_register(struct hisi_fb_data_type *hisifd);

static int hisi_fb_open(struct fb_info *info, int user);
static int hisi_fb_release(struct fb_info *info, int user);
static int hisi_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int hisi_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int hisi_fb_set_par(struct fb_info *info);
static int hisi_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
static int hisi_fb_mmap(struct fb_info *info, struct vm_area_struct * vma);

static int hisi_fb_suspend_sub(struct hisi_fb_data_type *hisifd);
static int hisi_fb_resume_sub(struct hisi_fb_data_type *hisifd);



/*******************************************************************************
**
*/
struct platform_device *hisi_fb_add_device(struct platform_device *pdev)
{
	struct hisi_fb_panel_data *pdata = NULL;
	struct platform_device *this_dev = NULL;
	struct fb_info *fbi = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t type = 0;
	uint32_t id = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return NULL;
	}
	pdata = dev_get_platdata(&pdev->dev);
	if (NULL == pdata) {
		HISI_FB_ERR("pdata is NULL");
		return NULL;
	}

	if (fbi_list_index >= HISI_FB_MAX_FBI_LIST) {
		HISI_FB_ERR("no more framebuffer info list!\n");
		return NULL;
	}

	id = pdev->id;//lint !e732 !e838
	type = pdata->panel_info->type;
	/* alloc panel device data */
	this_dev = hisi_fb_device_alloc(pdata, type, id);
	if (!this_dev) {
		HISI_FB_ERR("failed to hisi_fb_device_alloc!\n");
		return NULL;
	}

	/* alloc framebuffer info + par data */
	fbi = framebuffer_alloc(sizeof(struct hisi_fb_data_type), NULL);
	if (fbi == NULL) {
		HISI_FB_ERR("can't alloc framebuffer info data!\n");
		platform_device_put(this_dev);
		return NULL;
	}

	hisifd = (struct hisi_fb_data_type *)fbi->par;
	memset(hisifd, 0, sizeof(struct hisi_fb_data_type));
	hisifd->fbi = fbi;

	hisifd->fb_imgType = HISI_FB_PIXEL_FORMAT_BGRA_8888;
	hisifd->index = fbi_list_index;
	hisifd->dss_base = hisifd_dss_base;
	hisifd->peri_crg_base = hisifd_peri_crg_base;
	hisifd->sctrl_base = hisifd_sctrl_base;
	hisifd->pctrl_base = hisifd_pctrl_base;
	hisifd->noc_dss_base = hisifd_noc_dss_base;
	hisifd->mmbuf_crg_base = hisifd_mmbuf_crg_base;
	hisifd->mmbuf_asc0_base = hisifd_mmbuf_asc0_base;
	hisifd->pmctrl_base = hisifd_pmctrl_base;

	hisifd->media_crg_base = hisifd_media_crg_base;
	hisifd->media_common_base = hisifd_media_common_base;
	hisifd->dp_base = hisifd_dp_base;

	hisifd->mipi_dsi0_base = hisifd->dss_base + DSS_MIPI_DSI0_OFFSET;
	hisifd->mipi_dsi1_base = hisifd->dss_base + DSS_MIPI_DSI1_OFFSET;

	hisifd->dss_base_phy = g_dss_base_phy;

	hisifd->dss_axi_clk_name = g_dss_axi_clk_name; // only enable
	hisifd->dss_pclk_dss_name = g_dss_pclk_dss_name; // only enable
	hisifd->dss_pri_clk_name = g_dss_pri_clk_name;
	hisifd->dss_pxl0_clk_name = g_dss_pxl0_clk_name;
	hisifd->dss_pxl1_clk_name = g_dss_pxl1_clk_name;
	hisifd->dss_mmbuf_clk_name = g_dss_mmbuf_clk_name;
	hisifd->dss_pclk_mmbuf_name = g_dss_pclk_mmbuf_clk_name;
	hisifd->dss_dphy0_ref_clk_name = g_dss_dphy0_ref_clk_name;
	hisifd->dss_dphy1_ref_clk_name = g_dss_dphy1_ref_clk_name;
	hisifd->dss_dphy0_cfg_clk_name = g_dss_dphy0_cfg_clk_name;
	hisifd->dss_dphy1_cfg_clk_name = g_dss_dphy1_cfg_clk_name;
	hisifd->dss_pclk_dsi0_name = g_dss_pclk_dsi0_name;
	hisifd->dss_pclk_dsi1_name = g_dss_pclk_dsi1_name;
	hisifd->dss_pclk_pctrl_name = g_dss_pclk_pctrl_name;
	hisifd->dss_auxclk_dpctrl_name = g_dss_clk_gate_dpctrl_16m_name;
	hisifd->dss_pclk_dpctrl_name = g_dss_pclk_gate_dpctrl_name;
	hisifd->dss_aclk_dpctrl_name = g_dss_aclk_dpctrl_name;

	hisifd->dsi0_irq = hisifd_irq_dsi0;
	hisifd->dsi1_irq = hisifd_irq_dsi1;
	hisifd->dp_irq = hisifd_irq_dptx;
	hisifd->mmbuf_asc0_irq = hisifd_irq_mmbuf_asc0;
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisifd->fb_num = HISI_FB0_NUM;
		hisifd->dpe_irq = hisifd_irq_pdp;
		hisifd->dpe_regulator = &(g_dpe_regulator[0]);
		hisifd->mmbuf_regulator = &(g_dpe_regulator[1]);
		hisifd->mediacrg_regulator = &(g_dpe_regulator[2]);
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		hisifd->fb_num = HISI_FB1_NUM;
		hisifd->dpe_irq = hisifd_irq_sdp;

		hisifd->dpe_regulator = &(g_dpe_regulator[0]);
		hisifd->mmbuf_regulator = &(g_dpe_regulator[1]);
		hisifd->mediacrg_regulator = &(g_dpe_regulator[2]);
	} else if (hisifd->index == AUXILIARY_PANEL_IDX) {
		hisifd->fb_num = HISI_FB2_NUM;
		hisifd->dpe_irq = hisifd_irq_adp;
		hisifd->dpe_regulator = &(g_dpe_regulator[0]);
		hisifd->mmbuf_regulator = &(g_dpe_regulator[1]);
		hisifd->mediacrg_regulator = &(g_dpe_regulator[2]);
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		hisifd->fb_num = HISI_FB3_NUM;
		hisifd->dpe_irq = hisifd_irq_mdc;
		hisifd->dpe_regulator = &(g_dpe_regulator[0]);
		hisifd->mmbuf_regulator = &(g_dpe_regulator[1]);
		hisifd->mediacrg_regulator = &(g_dpe_regulator[2]);
	} else {
		HISI_FB_ERR("fb%d not support now!\n", hisifd->index);
		platform_device_put(this_dev);
		framebuffer_release(fbi);
		return NULL;
	}

	/* link to the latest pdev */
	hisifd->pdev = this_dev;

	hisifd_list[hisifd_list_index++] = hisifd;
	fbi_list[fbi_list_index++] = fbi;

	 /* get/set panel info */
	memcpy(&hisifd->panel_info, pdata->panel_info, sizeof(struct hisi_panel_info));

	/* set driver data */
	platform_set_drvdata(this_dev, hisifd);

	if (platform_device_add(this_dev)) {
		HISI_FB_ERR("failed to platform_device_add!\n");
		framebuffer_release(fbi);
		platform_device_put(this_dev);
		hisifd_list_index--;
		fbi_list_index--;
		return NULL;
	}

	return this_dev;
}


int hisi_fb_blank_sub(int blank_mode, struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;
	int curr_pwr_state = 0;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	down(&hisifd->blank_sem);
	down(&hisifd->blank_sem0);
	down(&hisifd->blank_sem_effect);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (!hisifd->panel_power_on) {
			ret = hisifd->on_fnc(hisifd);
			if (ret == 0) {
				hisifd->panel_power_on = true;
			}
		}
		break;

	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		if (hisifd->panel_power_on) {
			/* 1. if tui is running, dss should not powerdown,
			 *    because register will be writen in tui mode.
			 * 2. if tui is enable, but not running, then tui should not be ok,
			 *    send the msg that tui config fail.
			 */
			if (hisifd->secure_ctrl.secure_status == DSS_SEC_RUNNING) {
				hisifd->secure_ctrl.secure_blank_flag = 1;
				tui_poweroff_work_start();
				HISI_FB_INFO("wait for tui quit.\n");
				break;
			} else if (hisifd->secure_ctrl.secure_event == DSS_SEC_ENABLE) {
				hisifd->secure_ctrl.secure_event = DSS_SEC_DISABLE;
				send_tui_msg_config(TUI_POLL_CFG_FAIL, 0, "DSS");
				HISI_FB_INFO("In power down, secure event will not be handled.\n");
			}

			curr_pwr_state = hisifd->panel_power_on;
			down(&hisifd->power_esd_sem);
			hisifd->panel_power_on = false;
			up(&hisifd->power_esd_sem);

			hisifd->mask_layer_xcc_flag = 0;

			if (hisifd->bl_cancel) {
				hisifd->bl_cancel(hisifd);
			}

			ret = hisifd->off_fnc(hisifd);
			if (ret)
				hisifd->panel_power_on = curr_pwr_state;

			if (hisifd->buf_sync_suspend)
				hisifd->buf_sync_suspend(hisifd);
		}
		break;
	}
	up(&hisifd->blank_sem_effect);
	up(&hisifd->blank_sem0);
	up(&hisifd->blank_sem);

	return ret;
}

int fastboot_set_needed = 0;

static bool hisi_fb_set_fastboot_needed(struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL");
		return false;
	}
	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return false;
	}

	if (fastboot_set_needed == 1) {
		hisifb_ctrl_fastboot(hisifd);

		hisifd->panel_power_on = true;
		if (info->screen_base && (info->fix.smem_len > 0))
			memset(info->screen_base, 0x0, info->fix.smem_len);

		fastboot_set_needed = 0;
		return true;
	}

	return false;
}

static int hisi_fb_open_sub(struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;
	bool needed = false;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	if (hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external) {
		if (hisi_cmdlist_init(hisifd)) {
			HISI_FB_ERR("fb%d hisi_cmdlist_init failed!\n", hisifd->index);
			return -EINVAL;
		}
	}

	if (hisifd->set_fastboot_fnc) {
		needed = hisifd->set_fastboot_fnc(info);
	}

	if (!needed) {
		ret = hisi_fb_blank_sub(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			if (hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external) {
				hisi_cmdlist_deinit(hisifd);
			}
			HISI_FB_ERR("can't turn on display!\n");
			return ret;
		}
	}

	return 0;
}

static int hisi_fb_release_sub(struct fb_info *info)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;//lint !e838

	if (NULL == info) {
		HISI_FB_ERR("info is NULL");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	ret = hisi_fb_blank_sub(FB_BLANK_POWERDOWN, info);
	if (ret != 0) {
		HISI_FB_ERR("can't turn off display!\n");
		return ret;
	}

	if (hisifd->index == EXTERNAL_PANEL_IDX && !hisifd->panel_info.fake_external) {
		hisi_cmdlist_deinit(hisifd);
	}

	return 0;
}

static void hisi_fb_displayeffect_update(struct hisi_fb_data_type *hisifd) {
	if (hisifd == NULL) {
		return;
	}

	if (g_dss_version_tag == FB_ACCEL_HI366x && hisifd->panel_info.colormode_support == 1) {
		hisifd->effect_updated_flag.gmp_effect_updated = true;
		hisifd->effect_updated_flag.igm_effect_updated = true;
		hisifd->effect_updated_flag.xcc_effect_updated = true;
		hisifd->effect_updated_flag.gamma_effect_updated = true;
		hisifd->effect_updated_flag.acm_effect_updated = true;
	}

}

/*******************************************************************************
**
*/

void hisi_aod_dec_atomic(struct hisi_fb_data_type *hisifd)
{
	int temp = 0;
	if (hisifd == NULL)
		return;
	temp = atomic_read(&(hisifd->atomic_v));
	HISI_FB_INFO("atomic_v = %d.\n", temp);
	if (temp <= 0) {
		return;
	}
	atomic_dec(&(hisifd->atomic_v));
}

int hisi_aod_inc_atomic(struct hisi_fb_data_type *hisifd)
{
	int temp = 0;
	if (hisifd == NULL)
		return 0;
	temp = atomic_inc_return(&(hisifd->atomic_v));
	HISI_FB_INFO("atomic_v increased to %d.\n", temp);
	if (temp == 1) {
		return 1;
	}

	HISI_FB_INFO("no need reget dss, atomic_v = %d\n", temp);
	hisi_aod_dec_atomic(hisifd);
	return 0;
}

static int hisi_fb_unblank_wq_handle(struct work_struct *work)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;//lint !e838
	if (!g_info_fb0) {
		HISI_FB_ERR("g_info_fb0 is NULL\n");
	}
	struct fb_info *info = g_info_fb0;

	hisifd = container_of(work, struct hisi_fb_data_type, aod_ud_fast_unblank_work);

	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (hisifd->panel_info.fake_external && (hisifd->index == EXTERNAL_PANEL_IDX)) {
		HISI_FB_INFO("it is fake, blank it fail \n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d +.\n", hisifd->index);

	down(&hisifd->fast_unblank_sem);
	if ((hisifd->panel_power_on)
	    && (hisifd->secure_ctrl.secure_blank_flag)) {
		// wait for blank
		HISI_FB_INFO(" wait for tui blank!\n");
		while (hisifd->panel_power_on) mdelay(1);
	}

	if ((hisifd->index != AUXILIARY_PANEL_IDX)) {
		hisi_fb_displayeffect_update(hisifd);
	}

	if (hisifd->dp_device_srs) {
		hisifd->dp_device_srs(hisifd, true);
	} else {
		ret = hisi_fb_blank_sub(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) failed!\n", hisifd->index, FB_BLANK_UNBLANK);
			up(&hisifd->fast_unblank_sem);
			return ret;
		}

		{
			char *envp[2];
			char buf[64];
			snprintf(buf, sizeof(buf), "Refresh=1");
			envp[0] = buf;
			envp[1] = NULL;
			kobject_uevent_env(&(hisifd->fbi->dev->kobj), KOBJ_CHANGE, envp);

			HISI_FB_INFO("fb0_unblank_wq refresh!\n");
		}

		ret = hisifb_ce_service_blank(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) hisifb_ce_service_blank() failed!\n", hisifd->index, FB_BLANK_UNBLANK);
			up(&hisifd->fast_unblank_sem);
			return ret;
		}

		ret = hisifb_display_engine_blank(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) hisifb_display_engine_blank() failed!\n", hisifd->index, FB_BLANK_UNBLANK);
			up(&hisifd->fast_unblank_sem);
			return ret;
		}
	}

	HISI_FB_INFO("fb%d -.\n", hisifd->index);

	hisifd->enable_fast_unblank = FALSE;
	up(&hisifd->fast_unblank_sem);

	return 0;
}

void hisi_aod_schedule_wq(void)
{
	struct hisi_fb_data_type *hisifd = NULL;//lint !e838
	hisifd = g_hisifd_fb0;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return;
	}
	hisifd->enable_fast_unblank = TRUE;
	queue_work(hisifd->aod_ud_fast_unblank_workqueue, &hisifd->aod_ud_fast_unblank_work);
}

static int hisi_fb_blank(int blank_mode, struct fb_info *info)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;//lint !e838


	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (hisifd->panel_info.fake_external && (hisifd->index == EXTERNAL_PANEL_IDX)) {
		HISI_FB_INFO("it is fake, blank it fail \n");
		return -EINVAL;
	}
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		g_info_fb0 = info;
	}


	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		HISI_FB_DEBUG("fb%d, blank_mode(%d) +.\n", hisifd->index, blank_mode);
	} else {
		HISI_FB_INFO("fb%d, blank_mode(%d) +!\n", hisifd->index, blank_mode);
	}


	if (FB_BLANK_UNBLANK == blank_mode) {
		if ((hisifd->panel_power_on)
		    && (hisifd->secure_ctrl.secure_blank_flag)) {
			// wait for blank
			HISI_FB_INFO(" wait for tui blank!\n");
			while (hisifd->panel_power_on) mdelay(1);
		}
		while (hisifd->enable_fast_unblank) mdelay(1);
	}

	if ((hisifd->index != AUXILIARY_PANEL_IDX) && (FB_BLANK_UNBLANK == blank_mode)) {
		hisi_fb_displayeffect_update(hisifd);
	}

	if (hisifd->dp_device_srs) {
		hisifd->dp_device_srs(hisifd, (blank_mode == FB_BLANK_UNBLANK) ? true: false);
	} else {
		ret = hisi_fb_blank_sub(blank_mode, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) failed!\n", hisifd->index, blank_mode);
			goto sensorhub_aod_hw_unlock;
		}

		ret = hisifb_ce_service_blank(blank_mode, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) hisifb_ce_service_blank() failed!\n", hisifd->index, blank_mode);
			goto sensorhub_aod_hw_unlock;
		}

		ret = hisifb_display_engine_blank(blank_mode, info);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, blank_mode(%d) hisifb_display_engine_blank() failed!\n", hisifd->index, blank_mode);
			goto sensorhub_aod_hw_unlock;
		}
	}

	if (hisifd->index == AUXILIARY_PANEL_IDX) {
		HISI_FB_DEBUG("fb%d, blank_mode(%d) -.\n", hisifd->index, blank_mode);
	} else {
		HISI_FB_INFO("fb%d, blank_mode(%d) -!\n", hisifd->index, blank_mode);
	}


	return 0;

sensorhub_aod_hw_unlock:

	return ret;
}

static int hisi_fb_open(struct fb_info *info, int user)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (hisifd->panel_info.fake_external && (hisifd->index == EXTERNAL_PANEL_IDX)) {
		HISI_FB_INFO("fb%d, is fake, open it fail \n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->ref_cnt) {
		HISI_FB_DEBUG("fb%d, +!\n", hisifd->index);
		if (hisifd->open_sub_fnc) {
			LOG_JANK_D(JLID_KERNEL_LCD_OPEN, "%s", "JL_KERNEL_LCD_OPEN 3650");
			ret = hisifd->open_sub_fnc(info);
		}
		HISI_FB_DEBUG("fb%d, -!\n", hisifd->index);
	}

	hisifd->ref_cnt++;

	return ret;
}//lint !e715

static int hisi_fb_release(struct fb_info *info, int user)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (hisifd->panel_info.fake_external && (hisifd->index == EXTERNAL_PANEL_IDX)) {
		HISI_FB_INFO("fb%d, is fake, release it fail \n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->ref_cnt) {
		HISI_FB_INFO("try to close unopened fb%d!\n", hisifd->index);
		return -EINVAL;
	}

	hisifd->ref_cnt--;

	if (!hisifd->ref_cnt) {
		HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
		if (hisifd->release_sub_fnc) {
			ret = hisifd->release_sub_fnc(info);
		}
		HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

		if (hisifd->index == PRIMARY_PANEL_IDX) {
			if (hisifd->fb_mem_free_flag)
				hisifb_free_fb_buffer(hisifd);
			if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
				HISI_FB_INFO("fb%d, ref_cnt = %d\n", hisifd->index, hisifd->ref_cnt);
				dsm_client_record(lcd_dclient, "No fb0 device can use\n");
				dsm_client_notify(lcd_dclient, DSM_LCD_FB0_CLOSE_ERROR_NO);
			}
		}
	}

//	if (hisifd->pm_runtime_put)
//		hisifd->pm_runtime_put(hisifd);

	return ret;
}//lint !e715

static int hisi_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == var) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (var->rotate != FB_ROTATE_UR) {
		HISI_FB_ERR("error rotate %d!\n", var->rotate);
		return -EINVAL;
	}

	if (var->grayscale != info->var.grayscale) {
		HISI_FB_DEBUG("error grayscale %d!\n", var->grayscale);
		return -EINVAL;
	}

	if ((var->xres_virtual <= 0) || (var->yres_virtual <= 0)) {
		HISI_FB_ERR("xres_virtual=%d yres_virtual=%d out of range!",
			var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}


	if ((var->xres == 0) || (var->yres == 0)) {
		HISI_FB_ERR("xres=%d, yres=%d is invalid!\n", var->xres, var->yres);
		return -EINVAL;
	}

	if (var->xoffset > (var->xres_virtual - var->xres)) {
		HISI_FB_ERR("xoffset=%d(xres_virtual=%d, xres=%d) out of range!\n",
			var->xoffset, var->xres_virtual, var->xres);
		return -EINVAL;
	}

	if (var->yoffset > (var->yres_virtual - var->yres)) {
		HISI_FB_ERR("yoffset=%d(yres_virtual=%d, yres=%d) out of range!\n",
			var->yoffset, var->yres_virtual, var->yres);
		return -EINVAL;
	}

	return 0;
}

static int hisi_fb_set_par(struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct fb_var_screeninfo *var = NULL;

	if (NULL == info) {
		HISI_FB_ERR("set par info NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("set par hisifd NULL Pointer\n");
		return -EINVAL;
	}

	var = &info->var;

	hisifd->fbi->fix.line_length = hisifb_line_length(hisifd->index, var->xres_virtual,
		var->bits_per_pixel >> 3);

	return 0;
}

static int hisi_fb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == var || NULL == info) {
		HISI_FB_ERR("pan display var or info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL ==  hisifd) {
		HISI_FB_ERR("pan display hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	down(&hisifd->blank_sem);

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("fb%d, panel power off!\n", hisifd->index);
		ret = -EPERM;
		goto err_out;
	}

	if (var->xoffset > (info->var.xres_virtual - info->var.xres)) {
		ret = -EINVAL;
		goto err_out;
	}

	if (var->yoffset > (info->var.yres_virtual - info->var.yres)) {
		ret = -EINVAL;
		goto err_out;
	}

	if (info->fix.xpanstep)
		info->var.xoffset =
		(var->xoffset / info->fix.xpanstep) * info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset =
		(var->yoffset / info->fix.ypanstep) * info->fix.ypanstep;

	if (hisifd->pan_display_fnc)
		hisifd->pan_display_fnc(hisifd);
	else
		HISI_FB_ERR("fb%d pan_display_fnc not set!\n", hisifd->index);

	up(&hisifd->blank_sem);

	if (hisifd->bl_update) {
		hisifd->bl_update(hisifd);
	}

	return ret;

err_out:
	up(&hisifd->blank_sem);
	return 0;
}

static int hisifb_lcd_dirty_region_info_get(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("dirty region info get info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("dirty region info get hisifdNULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("dirty region info get argp NULL Pointer!\n");
		return -EINVAL;
	}

	if (copy_to_user(argp, &(hisifd->panel_info.dirty_region_info),
		sizeof(struct lcd_dirty_region_info))) {
		HISI_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int hisifb_dirty_region_updt_set(struct fb_info *info, void __user *argp)
{
	int enable = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("dirty region updt set info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("dirty region updt set hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_ERR("fb%d, not supported!\n", hisifd->index);
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("dirty region updt set argp NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd->dirty_region_updt_enable = 0;

	if (g_enable_dirty_region_updt
		&& hisifd->panel_info.dirty_region_updt_support
		&& !hisifd->sbl_enable
		&& !hisifd->color_temperature_flag
		&& !hisifd->display_effect_flag
		&& !hisifb_display_effect_is_need_ace(hisifd)
		&& !hisifd->esd_happened
		&& (DSS_SEC_DISABLE == hisifd->secure_ctrl.secure_event)
		&& !hisifd->aod_mode
		&& !hisifd->vr_mode) {
		enable = 1;
		hisifd->dirty_region_updt_enable = 1;
	}

	if (copy_to_user(argp, &enable, sizeof(enable))) {
		HISI_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int hisifb_video_idle_ctrl_get(struct fb_info *info, void __user *argp)
{
	int is_video_idle = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		return -EINVAL;
	}

	if (NULL == argp) {
		return -EINVAL;
	}

	is_video_idle = 0;
	if (g_enable_video_idle_l3cache
		&& is_video_idle_ctrl_mode(hisifd)) {
		is_video_idle = 1;
	}

	if (copy_to_user(argp, &is_video_idle, sizeof(is_video_idle))) {
		HISI_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int hisifb_idle_is_allowed(struct fb_info *info, void __user *argp)
{
	int is_allowed = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("idle is allowed info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("idle is allowed hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("idle is allowed argp NULL Pointer!\n");
		return -EINVAL;
	}

	is_allowed = (hisifd->frame_update_flag == 1) ? 0 : 1;//lint !e838

	if (copy_to_user(argp, &is_allowed, sizeof(is_allowed))) {
		HISI_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}


static int hisifb_debug_check_fence_timeline(struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_buf_sync *buf_sync_ctrl = NULL;
	unsigned long flags;
	int val = 0;

	if (NULL == info) {
		HISI_FB_ERR("timeline info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("timeline hisifd NULL Pointer!\n");
		return -EINVAL;
	}
	buf_sync_ctrl = &hisifd->buf_sync_ctrl;
	if (NULL == buf_sync_ctrl->timeline) {
		HISI_FB_ERR("timeline NULL Pointer!\n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d frame_no(%d) timeline_max(%d), TL(Nxt %d , Crnt %d)!\n",
		hisifd->index, hisifd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
		buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);

	if ((buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value) > 0) {
		val = buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value;
	}

	hisi_dss_resync_timeline(buf_sync_ctrl->timeline);
	hisi_dss_resync_timeline(buf_sync_ctrl->timeline_retire);

	buf_sync_ctrl->timeline_max += val;
	buf_sync_ctrl->refresh = 0;

	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);

	HISI_FB_INFO("fb%d frame_no(%d) timeline_max(%d), TL(Nxt %d , Crnt %d)!\n",
		hisifd->index, hisifd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
		buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);

	return 0;
}

static int hisifb_dss_mmbuf_alloc(struct fb_info *info, void __user *argp)
{
	int ret = 0;
	uint32_t mmbuf_size_max = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	dss_mmbuf_t mmbuf_info;

	if (NULL == info) {
		HISI_FB_ERR("dss mmbuf alloc info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("dss mmbuf alloc hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("dss mmbuf alloc argp NULL Pointer!\n");
		return -EINVAL;
	}

	ret = copy_from_user(&mmbuf_info, argp, sizeof(dss_mmbuf_t));
	if (ret) {
		HISI_FB_ERR("fb%d, copy for user failed!ret=%d.\n", hisifd->index, ret);
		ret = -EINVAL;
		goto err_out;
	}
	mmbuf_size_max = MMBUF_SIZE_MAX;

	/*lint -e574 -e737*/
	if ((mmbuf_info.size <= 0) || (mmbuf_info.size > mmbuf_size_max) || (mmbuf_info.size & (MMBUF_ADDR_ALIGN - 1))) {
		HISI_FB_ERR("fb%d, mmbuf size is invalid, size=%d!\n", hisifd->index, mmbuf_info.size);
		ret = -EINVAL;
		goto err_out;
	}
	/*lint +e574 +e737*/

	if (g_mmbuf_addr_test > 0) {
		if (g_mmbuf_addr_test >= (mmbuf_size_max + 0x40)) {
			HISI_FB_ERR("g_mmbuf_addr_test(0x%x) is overflow max mmbuf size + 0x40(0x%x)\n", g_mmbuf_addr_test, mmbuf_size_max + 0x40);
			HISI_FB_ERR("remain buff size if %d \n", (mmbuf_size_max + 0x40) - (g_mmbuf_addr_test - mmbuf_info.size));
			g_mmbuf_addr_test = 0;
		} else {
			mmbuf_info.addr = g_mmbuf_addr_test;
			g_mmbuf_addr_test += mmbuf_info.size;
		}

		HISI_FB_INFO("addr = 0x%x, size =%d,g_mmbuf_addr_test = 0x%x, MAX_SIZE= 0x%x \n", mmbuf_info.addr, mmbuf_info.size, g_mmbuf_addr_test, mmbuf_size_max + 0x40);
	}

	if (0 == g_mmbuf_addr_test) {
		mmbuf_info.addr = hisi_dss_mmbuf_alloc(hisifd->mmbuf_gen_pool, mmbuf_info.size);
		if (mmbuf_info.addr < MMBUF_BASE) {
			ret = -EINVAL;
			goto err_out;
		}
	}

	ret = copy_to_user(argp, &mmbuf_info, sizeof(dss_mmbuf_t));
	if (ret) {
		HISI_FB_ERR("fb%d, copy to user failed!ret=%d.", hisifd->index, ret);
		hisi_dss_mmbuf_free(hisifd->mmbuf_gen_pool, mmbuf_info.addr, mmbuf_info.size);
		ret = -EFAULT;
		goto err_out;
	}

	return 0;

err_out:
	return ret;
}

static int hisifb_dss_mmbuf_free(struct fb_info *info, void __user *argp)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_fb_panel_data *pdata = NULL;
	dss_mmbuf_t mmbuf_info;

	if (NULL == info) {
		HISI_FB_ERR("dss mmbuf free info NULL Pointer!\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("dss mmbuf free  hisifd NULL Pointer!\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&hisifd->pdev->dev);//lint !e838
	if (NULL == pdata) {
		HISI_FB_ERR("dss mmbuf free  pdata NULL Pointer!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("dss mmbuf free  argp NULL Pointer!\n");
		return -EINVAL;
	}

	 //lint -save -e775 -e732
	ret = copy_from_user(&mmbuf_info, argp, sizeof(dss_mmbuf_t));
	if (ret) {
		HISI_FB_ERR("fb%d, copy for user failed!ret=%d.", hisifd->index, ret);
		ret = -EINVAL;
		goto err_out;
	}

	if ((mmbuf_info.addr <= 0) || (mmbuf_info.size <= 0)) {
		HISI_FB_ERR("fb%d, addr=0x%x, size=%d is invalid!\n",
			hisifd->index, mmbuf_info.addr, mmbuf_info.size);
		ret = -EINVAL;
		goto err_out;
	}

	hisi_dss_mmbuf_free(hisifd->mmbuf_gen_pool, mmbuf_info.addr, mmbuf_info.size);
	//lint -restore

	return 0;

err_out:
	return ret;
}

static int hisifb_dss_get_platform_type(struct fb_info *info, void __user *argp)
{
	int type;
	int ret = 0;

	//lint -save -e712 -e838
	type = HISIFB_DSS_PLATFORM_TYPE;//lint !e713 !e737
	if (g_fpga_flag == 1) {
		type = HISIFB_DSS_PLATFORM_TYPE | FB_ACCEL_PLATFORM_TYPE_FPGA;
	}

	if (NULL == argp) {
		HISI_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}
	ret = copy_to_user(argp, &type, sizeof(type));
	if (ret) {
		HISI_FB_ERR("copy to user failed! ret=%d.", ret);
		ret = -EFAULT;
	}
	//lint -restore

	return ret;
}

static int hisi_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = -ENOSYS;
	struct hisi_fb_data_type *hisifd = NULL;
	void __user *argp = (void __user *)arg;
	//sigset_t setmask;
	//sigset_t oldmask;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer");
		return -EINVAL;
	}

	//sigemptyset(&setmask);
	//sigaddset(&setmask, SIGSTOP);
	//sigprocmask(SIG_SETMASK, &setmask, &oldmask);

	switch (cmd) { //lint -e30, -e142
	case HISIFB_VSYNC_CTRL:
		if (hisifd->vsync_ctrl_fnc) {
			ret = hisifd->vsync_ctrl_fnc(info, argp);
		}
		break;

	case HISIFB_DEBUG_CHECK_FENCE_TIMELINE:
		ret = hisifb_debug_check_fence_timeline(info);
		break;

	case HISIFB_IDLE_IS_ALLOWED:
		ret = hisifb_idle_is_allowed(info, argp);
		break;

	case HISIFB_DSS_VOLTAGE_GET:
		ret = hisifb_ctrl_dss_voltage_get(info, argp);
		break;

	case HISIFB_DSS_VOLTAGE_SET:
		ret = hisifb_ctrl_dss_voltage_set(info, argp);
		break;

	case HISIFB_DSS_VOTE_CMD_SET:
		ret = hisifb_ctrl_dss_vote_cmd_set(info, argp);
		break;
	case HISIFB_LCD_DIRTY_REGION_INFO_GET:
		ret = hisifb_lcd_dirty_region_info_get(info, argp);
		break;
	case HISIFB_DIRTY_REGION_UPDT_SET:
		ret = hisifb_dirty_region_updt_set(info, argp);
		break;
	case HISIFB_VIDEO_IDLE_CTRL:
		ret = hisifb_video_idle_ctrl_get(info, argp);
		break;

	case HISIFB_DSS_MMBUF_ALLOC:
		ret = hisifb_dss_mmbuf_alloc(info, argp);
		break;
	case HISIFB_DSS_MMBUF_FREE:
		ret = hisifb_dss_mmbuf_free(info, argp);
		break;
	case HISIFB_PLATFORM_TYPE_GET:
		ret = hisifb_dss_get_platform_type(info, argp);
		break;

	case HISIFB_MDC_CHANNEL_INFO_REQUEST:
		ret = hisi_mdc_chn_request(info, argp);
		break;

	case HISIFB_MDC_CHANNEL_INFO_RELEASE:
		ret = hisi_mdc_chn_release(info, argp);
		break;
	case HISIFB_CE_ENABLE:
		ret = hisifb_ce_service_enable_hiace(info, argp);
		break;
	case HISIFB_CE_SUPPORT_GET:
		ret = hisifb_ce_service_get_support(info, argp);
		break;
	case HISIFB_CE_SERVICE_LIMIT_GET:
		ret = hisifb_ce_service_get_limit(info, argp);
		break;
	case HISIFB_CE_PARAM_GET:
		ret = hisifb_ce_service_get_param(info, argp);
		break;
	case HISIFB_HIACE_PARAM_GET:
		ret = hisifb_ce_service_get_hiace_param(info, argp);
		break;
	case HISIFB_CE_PARAM_SET:
		ret = hisifb_ce_service_set_param(info, argp);
		break;
	case HISIFB_GET_REG_VAL:
		ret = hisifb_get_reg_val(info, argp);
		break;
	case HISIFB_CE_HIST_GET:
		ret = hisifb_ce_service_get_hist(info, argp);
		break;
	case HISIFB_CE_LUT_SET:
		ret = hisifb_ce_service_set_lut(info, argp);
		break;
	case HISIFB_DISPLAY_ENGINE_INIT:
		ret = hisifb_display_engine_init(info, argp);
		break;
	case HISIFB_DISPLAY_ENGINE_DEINIT:
		ret = hisifb_display_engine_deinit(info, argp);
		break;
	case HISIFB_DISPLAY_ENGINE_PARAM_GET:
		ret = hisifb_display_engine_param_get(info, argp);
		break;
	case HISIFB_DISPLAY_ENGINE_PARAM_SET:
		ret = hisifb_display_engine_param_set(info, argp);
		break;
	case HISIFB_EFFECT_MODULE_INIT:
	case HISIFB_EFFECT_MODULE_DEINIT:
	case HISIFB_EFFECT_INFO_GET:
	case HISIFB_EFFECT_INFO_SET:
		if (hisifd->display_effect_ioctl_handler)
			ret = hisifd->display_effect_ioctl_handler(hisifd, cmd, argp);
		break;
	case HISIFB_DPTX_GET_COLOR_BIT_MODE:
		if (hisifd->dp_get_color_bit_mode)
			ret = hisifd->dp_get_color_bit_mode(hisifd, argp);
		break;
	case HISIFB_DPTX_GET_SOURCE_MODE:
		if (hisifd->dp_get_source_mode)
			ret = hisifd->dp_get_source_mode(hisifd, argp);
		break;
	case HISIFB_GRALLOC_GET_PHYS:
		ret = hisi_ion_get_phys(info, argp);
		break;
	default:
		if (hisifd->ov_ioctl_handler)
			ret = hisifd->ov_ioctl_handler(hisifd, cmd, argp);
		break;
	} //lint +e30, +e142

	//sigprocmask(SIG_SETMASK, &oldmask, NULL);

	if (ret == -ENOSYS)
		HISI_FB_ERR("unsupported ioctl (%x)\n", cmd);

	return ret;
}

static int hisi_fb_mmap(struct fb_info *info, struct vm_area_struct * vma)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct sg_table *table = NULL;
	struct scatterlist *sg = NULL;
	struct page *page = NULL;
	unsigned long remainder = 0;
	unsigned long len = 0;
	unsigned long addr = 0;
	unsigned long offset = 0;
	unsigned long size = 0;
	int i = 0;
	int ret = 0;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd || !(hisifd->pdev)) {
		HISI_FB_ERR("NULL Pointer");
		return -EINVAL;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (hisifd->fb_mem_free_flag) {
			if (!hisifb_alloc_fb_buffer(hisifd)) {
				HISI_FB_ERR("fb%d, hisifb_alloc_buffer failed!\n", hisifd->index);
				return -ENOMEM;
			}
		}
	} else {
		HISI_FB_ERR("fb%d, no fb buffer!\n", hisifd->index);
		return -EFAULT;
	}

	table = hisifb_ion_sg_table(hisifd->ion_client, hisifd->ion_handle, &(hisifd->pdev->dev));
	if ((table == NULL) || (vma == NULL)) {
		HISI_FB_ERR("fb%d, table or vma is NULL!\n", hisifd->index);
		return -EFAULT;
	}

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	addr = vma->vm_start;
	offset = vma->vm_pgoff * PAGE_SIZE;
	size = vma->vm_end - vma->vm_start;

	if (size > info->fix.smem_len) {
		HISI_FB_ERR("fb%d, size=%lu is out of range(%u)!\n", hisifd->index, size, info->fix.smem_len);
		return -EFAULT;
	}

	for_each_sg(table->sgl, sg, table->nents, i) {
		page = sg_page(sg);
		remainder = vma->vm_end - addr;
		len = sg->length;

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		} else if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}
		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr, page_to_pfn(page), len,
			vma->vm_page_prot);
		if (ret != 0) {
			HISI_FB_ERR("fb%d, failed to remap_pfn_range! ret=%d\n", hisifd->index, ret);
		}

		addr += len;
		if (addr >= vma->vm_end) {
			return 0;
        }
	}

	return 0;
}

unsigned long hisifb_alloc_fb_buffer(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;
	struct ion_client *client = NULL;
	struct ion_handle *handle = NULL;
	size_t buf_len = 0;
	unsigned long buf_addr = 0;

	if (NULL == hisifd || !(hisifd->pdev)) {
		HISI_FB_ERR("hisifd is NULL");
		return EINVAL;
	}
	fbi = hisifd->fbi;
	if (NULL == fbi) {
		HISI_FB_ERR("fbi is NULL");
		return EINVAL;
	}

	if (hisifd->ion_handle != NULL)
		return fbi->fix.smem_start;

	client = hisifd->ion_client;
	if (IS_ERR_OR_NULL(client)) {
		HISI_FB_ERR("failed to create ion client!\n");
		goto err_return;
	}

	buf_len = fbi->fix.smem_len;

	handle = ion_alloc(client, buf_len, PAGE_SIZE, ION_HEAP(ION_SYSTEM_HEAP_ID), 0);
	if (IS_ERR_OR_NULL(handle)) {
		HISI_FB_ERR("failed to ion_alloc!\n");
		goto err_return;
	}

	fbi->screen_base = ion_map_kernel(client, handle);
	if (!fbi->screen_base) {
		HISI_FB_ERR("failed to ion_map_kernel!\n");
		goto err_ion_map;
	}

	if (ion_map_iommu(client, handle, &(hisifd->iommu_format))) {
		HISI_FB_ERR("failed to ion_map_iommu!\n");
		goto err_ion_get_addr;
	}

	buf_addr = hisifd->iommu_format.iova_start;

	fbi->fix.smem_start = buf_addr;
	fbi->screen_size = fbi->fix.smem_len;
	//memset(fbi->screen_base, 0xFF, fbi->screen_size);

	hisifd->ion_handle = handle;

	return buf_addr;

err_ion_get_addr:
	ion_unmap_kernel(hisifd->ion_client, handle);
err_ion_map:
	ion_free(hisifd->ion_client, handle);
err_return:
	return 0;
}

void hisifb_free_fb_buffer(struct hisi_fb_data_type *hisifd)
{
	struct fb_info *fbi = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	fbi = hisifd->fbi;
	if (NULL == fbi) {
		HISI_FB_ERR("fbi is NULL");
		return;
	}

	if (hisifd->ion_client != NULL &&
		hisifd->ion_handle != NULL) {
		ion_unmap_iommu(hisifd->ion_client, hisifd->ion_handle);
		ion_unmap_kernel(hisifd->ion_client, hisifd->ion_handle);
		ion_free(hisifd->ion_client, hisifd->ion_handle);
		hisifd->ion_handle = NULL;

		fbi->screen_base = 0;
		fbi->fix.smem_start = 0;
	}
}

void hisifb_free_logo_buffer(struct hisi_fb_data_type *hisifd)
{
	int i;
	struct fb_info *fbi = NULL;
	uint32_t logo_buffer_base_temp = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	fbi = hisifd->fbi;//lint !e838
	if (NULL == fbi) {
		HISI_FB_ERR("fbi is NULL");
		return;
	}

	logo_buffer_base_temp = g_logo_buffer_base;
	for (i = 0; i < (g_logo_buffer_size / PAGE_SIZE); i++) {
		free_reserved_page(phys_to_page(logo_buffer_base_temp));
		logo_buffer_base_temp += PAGE_SIZE;
	}
	memblock_free(g_logo_buffer_base, g_logo_buffer_size);

	g_logo_buffer_size = 0;
	g_logo_buffer_base = 0;
}

/*******************************************************************************
** fb sys fs
*/
static void hisifb_sysfs_init(struct hisi_fb_data_type *hisifd)
{
	int i = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	hisifd->sysfs_index = 0;
	for (i = 0; i < HISI_FB_SYSFS_ATTRS_NUM; i++) {
		hisifd->sysfs_attrs[i] = NULL;
	}
	hisifd->sysfs_attr_group.attrs = hisifd->sysfs_attrs;
}

static void hisifb_sysfs_attrs_append(struct hisi_fb_data_type *hisifd, struct attribute *attr)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == attr) {
		HISI_FB_ERR("attr is NULL");
		return;
	}

	if (hisifd->sysfs_index >= HISI_FB_SYSFS_ATTRS_NUM) {
		HISI_FB_ERR("fb%d, sysfs_atts_num(%d) is out of range(%d)!\n",
			hisifd->index, hisifd->sysfs_index, HISI_FB_SYSFS_ATTRS_NUM);
		return ;
	}

	hisifd->sysfs_attrs[hisifd->sysfs_index] = attr;
	hisifd->sysfs_index++;
}

static int hisifb_sysfs_create(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);//lint !e838
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	ret = sysfs_create_group(&hisifd->fbi->dev->kobj, &(hisifd->sysfs_attr_group));
	if (ret) {
		HISI_FB_ERR("fb%d sysfs group creation failed, error=%d!\n",
			hisifd->index, ret);
	}


	if (get_lcdkit_support()) {
		HISI_FB_INFO("lcdkit is support!\n");
		lcdkit_fb_create_sysfs(&hisifd->fbi->dev->kobj);
	}

	return ret;
}

static void hisifb_sysfs_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	sysfs_remove_group(&hisifd->fbi->dev->kobj, &(hisifd->sysfs_attr_group));

	if (get_lcdkit_support()) {
		lcdkit_fb_remove_sysfs(&hisifd->fbi->dev->kobj);
	}

	hisifb_sysfs_init(hisifd);
}

/*******************************************************************************
**
*/
static struct fb_ops hisi_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = hisi_fb_open,
	.fb_release = hisi_fb_release,
	.fb_read = NULL,
	.fb_write = NULL,
	.fb_cursor = NULL,
	.fb_check_var = hisi_fb_check_var,
	.fb_set_par = hisi_fb_set_par,
	.fb_setcolreg = NULL,
	.fb_blank = hisi_fb_blank,
	.fb_pan_display = hisi_fb_pan_display,
	.fb_fillrect = NULL,
	.fb_copyarea = NULL,
	.fb_imageblit = NULL,
	.fb_sync = NULL,
	.fb_ioctl = hisi_fb_ioctl,
	.fb_compat_ioctl = hisi_fb_ioctl,
	.fb_mmap = hisi_fb_mmap,
};


int hisifb_esd_recover_disable(int value)
{
	struct hisi_fb_panel_data *pdata = NULL;
	/* primary panel */
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];
	if (hisifd == NULL) {
		HISI_FB_INFO("esd_recover_disable fail\n");
		return 0;
	}

	pdata = (struct hisi_fb_panel_data *)hisifd->pdev->dev.platform_data;
	if (pdata && pdata->panel_info) {
		if (pdata->panel_info->esd_enable) {
			HISI_FB_INFO("esd_recover_disable=%d\n", value);
			g_esd_recover_disable = value;
		}
	}
	return 0;
}
EXPORT_SYMBOL(hisifb_esd_recover_disable);

static int hisifb_check_ldi_porch(struct hisi_panel_info *pinfo)
{
	int vertical_porch_min_time = 0;
	int pxl_clk_rate_div = 0;
	int ifbc_v_porch_div = 1;

	pxl_clk_rate_div = (pinfo->pxl_clk_rate_div == 0 ? 1 : pinfo->pxl_clk_rate_div);
	if (pinfo->ifbc_type == IFBC_TYPE_RSP3X){
		pxl_clk_rate_div *= 3;
		pxl_clk_rate_div /= 2;
		ifbc_v_porch_div = 2;
	}

	if (g_fpga_flag == 1)
		return 0;   //do not check ldi porch in fpga version

	/* hbp+hfp+hsw time should be longer than 30 pixel clock cycel */
	if (pxl_clk_rate_div * (pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch
		+ pinfo->ldi.h_pulse_width) <= 30) {
		HISI_FB_ERR("ldi hbp+hfp+hsw is not larger than 30, return!\n");
		return -1;
	}

	/* check vbp+vsw */
	if (pinfo->xres * pinfo->yres >= RES_4K_PAD) {
		vertical_porch_min_time = 50;
	} else if (pinfo->xres * pinfo->yres >= RES_1600P) {
		vertical_porch_min_time = 45;
	} else if (pinfo->xres * pinfo->yres >= RES_1440P) {
		vertical_porch_min_time = 40;
	} else if (pinfo->xres * pinfo->yres >= RES_1200P) {
		vertical_porch_min_time = 45;
	} else {
		vertical_porch_min_time = 35;
	}

	if ((uint32_t)ifbc_v_porch_div * (pinfo->ldi.v_back_porch + pinfo->ldi.v_pulse_width) < vertical_porch_min_time) {
		if (1 == pinfo->non_check_ldi_porch) {
			HISI_FB_INFO("panel IC specification do not match this rule(vbp+vsw >= %d),"
				"we must skip it, Otherwise it will display unnormal!\n", vertical_porch_min_time);
		} else {
			HISI_FB_ERR("ldi vbp+vsw is less than %d, return!\n", vertical_porch_min_time);
			return -1;
		}
	}

	if (pinfo->sbl_support && (pinfo->ldi.v_front_porch * pxl_clk_rate_div * (pinfo->xres
		+ pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch + pinfo->ldi.v_pulse_width) < 14000)) {
		HISI_FB_ERR("vfp * hsize not larger than 14000, return!\n");
		return -1;
	}

	return 0;
}

static int hisi_fb_register(struct hisi_fb_data_type *hisifd)
{
	int bpp = 0;
	struct hisi_panel_info *panel_info = NULL;
	struct fb_info *fbi = NULL;
	struct fb_fix_screeninfo *fix = NULL;
	struct fb_var_screeninfo *var = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	panel_info = &hisifd->panel_info;//lint !e838
	if (NULL == panel_info) {
		HISI_FB_ERR("panel_info is NULL");
		return -EINVAL;
	}

	/*
	 * fb info initialization
	 */
	fbi = hisifd->fbi;//lint !e838
	fix = &fbi->fix;//lint !e838
	var = &fbi->var;//lint !e838

	fix->type_aux = 0;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;

	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	var->nonstd = 0;
	var->activate = FB_ACTIVATE_VBL;
	var->height = panel_info->height;
	var->width = panel_info->width;
	var->accel_flags = 0;
	var->sync = 0;
	var->rotate = 0;

	switch (hisifd->fb_imgType) {
	case HISI_FB_PIXEL_FORMAT_BGR_565:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 11;
		var->transp.offset = 0;

		var->blue.length = 5;
		var->green.length = 6;
		var->red.length = 5;
		var->transp.length = 0;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		break;

	case HISI_FB_PIXEL_FORMAT_BGRX_4444:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 4;
		var->red.offset = 8;
		var->transp.offset = 0;

		var->blue.length = 4;
		var->green.length = 4;
		var->red.length = 4;
		var->transp.length = 0;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		break;

	case HISI_FB_PIXEL_FORMAT_BGRA_4444:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 4;
		var->red.offset = 8;
		var->transp.offset = 12;

		var->blue.length = 4;
		var->green.length = 4;
		var->red.length = 4;
		var->transp.length = 4;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		break;

	case HISI_FB_PIXEL_FORMAT_BGRX_5551:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 10;
		var->transp.offset = 0;

		var->blue.length = 5;
		var->green.length = 5;
		var->red.length = 5;
		var->transp.length = 0;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		break;

	case HISI_FB_PIXEL_FORMAT_BGRA_5551:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 10;
		var->transp.offset = 15;

		var->blue.length = 5;
		var->green.length = 5;
		var->red.length = 5;
		var->transp.length = 1;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;
		bpp = 2;
		break;

	case HISI_FB_PIXEL_FORMAT_BGRA_8888:
		fix->type = FB_TYPE_PACKED_PIXELS;
		fix->xpanstep = 1;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		var->blue.offset = 0;
		var->green.offset = 8;
		var->red.offset = 16;
		var->transp.offset = 24;

		var->blue.length = 8;
		var->green.length = 8;
		var->red.length = 8;
		var->transp.length = 8;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;

		bpp = 4;
		break;

	case HISI_FB_PIXEL_FORMAT_YUV_422_I:
		fix->type = FB_TYPE_INTERLEAVED_PLANES;
		fix->xpanstep = 2;
		fix->ypanstep = 1;
		var->vmode = FB_VMODE_NONINTERLACED;

		/* FIXME: R/G/B offset? */
		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.offset = 11;
		var->transp.offset = 0;

		var->blue.length = 5;
		var->green.length = 6;
		var->red.length = 5;
		var->transp.length = 0;

		var->blue.msb_right = 0;
		var->green.msb_right = 0;
		var->red.msb_right = 0;
		var->transp.msb_right = 0;

		bpp = 2;
		break;

	default:
		HISI_FB_ERR("fb%d, unkown image type!\n", hisifd->index);
		return -EINVAL;
	}

	//for resolution update
	memset(&(hisifd->resolution_rect), 0, sizeof(dss_rect_t));

	var->xres = panel_info->xres;
	var->yres = panel_info->yres;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * hisifd->fb_num;
	var->bits_per_pixel = bpp * 8;

	snprintf(fix->id, sizeof(fix->id), "hisifb%d", hisifd->index);
	fix->line_length = hisifb_line_length(hisifd->index, var->xres_virtual, bpp);
	fix->smem_len = roundup(fix->line_length * var->yres_virtual, PAGE_SIZE);
	fix->smem_start = 0;

	fbi->screen_base = 0;
	fbi->fbops = &hisi_fb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = NULL;

	fix->reserved[0] = is_mipi_cmd_panel(hisifd) ? 1 : 0;

	hisifd->ion_client = hisi_ion_client_create(HISI_FB_ION_CLIENT_NAME);
	if (IS_ERR_OR_NULL(hisifd->ion_client)) {
		HISI_FB_ERR("failed to create ion client!\n");
		return -ENOMEM;
	}
	hisifd->ion_handle = NULL;
	memset(&hisifd->iommu_format, 0, sizeof(struct iommu_map_format));

	if (fix->smem_len > 0) {
		if (!hisifb_alloc_fb_buffer(hisifd)) {
			HISI_FB_ERR("hisifb_alloc_buffer failed!\n");
			return -ENOMEM;
		}
	}

	hisifd->ref_cnt = 0;
	hisifd->panel_power_on = false;
	hisifd->aod_function = 0;
	hisifd->aod_mode = 0;
	hisifd->vr_mode = 0;
	hisifd->mask_layer_xcc_flag = 0;
	hisifd->enable_fast_unblank = FALSE;
	atomic_set(&(hisifd->atomic_v), 0);
	sema_init(&hisifd->blank_sem, 1);
	sema_init(&hisifd->blank_sem0, 1);
	sema_init(&hisifd->blank_sem_effect, 1);
	sema_init(&hisifd->brightness_esd_sem, 1);
	sema_init(&hisifd->power_esd_sem, 1);
	sema_init(&hisifd->fast_unblank_sem, 1);
	sema_init(&hisifd->hiace_clear_sem, 1);



	hisifb_sysfs_init(hisifd);

	hisifd->on_fnc = hisifb_ctrl_on;
	hisifd->off_fnc = hisifb_ctrl_off;

	hisifd->hisi_domain = g_hisi_domain;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisifd->fb_mem_free_flag = false;

		//for offline composer
		g_primary_lcd_xres = var->xres;
		g_primary_lcd_yres = var->yres;//lint !e713
		g_pxl_clk_rate = panel_info->pxl_clk_rate;
		g_prefix_ce_support = panel_info->prefix_ce_support;
		g_prefix_sharpness1D_support = panel_info->prefix_sharpness1D_support;
		g_prefix_sharpness2D_support = panel_info->prefix_sharpness2D_support;

		if (g_fastboot_enable_flag == 1) {
			hisifd->set_fastboot_fnc = hisi_fb_set_fastboot_needed;
			fastboot_set_needed = 1;
		} else {
			hisifd->set_fastboot_fnc = NULL;
		}
		hisifd->open_sub_fnc = hisi_fb_open_sub;
		hisifd->release_sub_fnc = hisi_fb_release_sub;
		hisifd->hpd_open_sub_fnc = NULL;
		hisifd->hpd_release_sub_fnc = NULL;
		hisifd->lp_fnc = hisifb_ctrl_lp;
		hisifd->esd_fnc = hisifb_ctrl_esd;
		hisifd->sbl_ctrl_fnc = hisifb_ctrl_sbl;
		hisifd->fps_upt_isr_handler = hisifb_fps_upt_isr_handler;
		hisifd->mipi_dsi_bit_clk_upt_isr_handler = mipi_dsi_bit_clk_upt_isr_handler;
		hisifd->panel_mode_switch_isr_handler = panel_mode_switch_isr_handler;
		hisifd->sysfs_attrs_add_fnc = hisifb_sysfs_attrs_add;
		hisifd->sysfs_attrs_append_fnc = hisifb_sysfs_attrs_append;
		hisifd->sysfs_create_fnc = hisifb_sysfs_create;
		hisifd->sysfs_remove_fnc = hisifb_sysfs_remove;

		hisifd->pm_runtime_register = NULL;
		hisifd->pm_runtime_unregister = NULL;
		hisifd->pm_runtime_get = NULL;
		hisifd->pm_runtime_put = NULL;
		hisifd->bl_register = hisifb_backlight_register;
		hisifd->bl_unregister = hisifb_backlight_unregister;
		hisifd->bl_update = hisifb_backlight_update;
		hisifd->bl_cancel = hisifb_backlight_cancel;
		hisifd->vsync_register = hisifb_vsync_register;
		hisifd->vsync_unregister = hisifb_vsync_unregister;
		hisifd->vsync_ctrl_fnc = hisifb_vsync_ctrl;
		hisifd->vsync_isr_handler = hisifb_vsync_isr_handler;
		hisifd->buf_sync_register = hisifb_buf_sync_register;
		hisifd->buf_sync_unregister = hisifb_buf_sync_unregister;
		hisifd->buf_sync_signal = hisifb_buf_sync_signal;
		hisifd->buf_sync_suspend = hisifb_buf_sync_suspend;
		hisifd->secure_register = hisifb_secure_register;
		hisifd->secure_unregister = hisifb_secure_unregister;
		hisifd->esd_register = hisifb_esd_register;
		hisifd->esd_unregister = hisifb_esd_unregister;
		hisifd->debug_register = hisifb_debug_register;
		hisifd->debug_unregister = hisifb_debug_unregister;
		hisifd->cabc_update = updateCabcPwm;
		hisifd->video_idle_ctrl_register = NULL;
		hisifd->video_idle_ctrl_unregister = NULL;
		hisifd->pipe_clk_updt_isr_handler = NULL;
		hisifd->overlay_online_wb_register = NULL;
		hisifd->overlay_online_wb_unregister = NULL;

		if (hisifb_check_ldi_porch(panel_info)) {
			HISI_FB_ERR("check ldi porch failed, return!\n");
			return -EINVAL;
		}
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		hisifd->fb_mem_free_flag = true;

		hisifd->set_fastboot_fnc = NULL;
		hisifd->open_sub_fnc = NULL;
		hisifd->release_sub_fnc = NULL;
		hisifd->hpd_open_sub_fnc = hisi_fb_open_sub;
		hisifd->hpd_release_sub_fnc = hisi_fb_release_sub;
		hisifd->lp_fnc = NULL;
		hisifd->esd_fnc = NULL;
		hisifd->sbl_ctrl_fnc = NULL;
		hisifd->fps_upt_isr_handler = NULL;
		hisifd->mipi_dsi_bit_clk_upt_isr_handler = NULL;
		hisifd->sysfs_attrs_add_fnc = NULL;

		hisifd->pm_runtime_register = NULL;
		hisifd->pm_runtime_unregister = NULL;
		hisifd->pm_runtime_get = NULL;
		hisifd->pm_runtime_put = NULL;
		hisifd->bl_register = hisifb_backlight_register;
		hisifd->bl_unregister = hisifb_backlight_unregister;
		hisifd->bl_update = hisifb_backlight_update;
		hisifd->bl_cancel = hisifb_backlight_cancel;
		hisifd->vsync_register = hisifb_vsync_register;
		hisifd->vsync_unregister = hisifb_vsync_unregister;
		hisifd->vsync_ctrl_fnc = hisifb_vsync_ctrl;
		hisifd->vsync_isr_handler = hisifb_vsync_isr_handler;
		hisifd->buf_sync_register = hisifb_buf_sync_register;
		hisifd->buf_sync_unregister = hisifb_buf_sync_unregister;
		hisifd->buf_sync_signal = hisifb_buf_sync_signal;
		hisifd->buf_sync_suspend = hisifb_buf_sync_suspend;
		hisifd->secure_register = NULL;
		hisifd->secure_unregister = NULL;
		hisifd->overlay_online_wb_register = NULL;
		hisifd->overlay_online_wb_unregister = NULL;
		hisifd->video_idle_ctrl_register = NULL;
		hisifd->video_idle_ctrl_unregister = NULL;
		hisifd->pipe_clk_updt_isr_handler = NULL;
		hisifd->esd_register = NULL;
		hisifd->esd_unregister = NULL;
		hisifd->debug_register = hisifb_debug_register;
		hisifd->debug_unregister = NULL;
	} else if (hisifd->index == MEDIACOMMON_PANEL_IDX) {
		sema_init(&hisifd->media_common_composer_sem, 1);
		sema_init(&hisifd->media_common_sr_sem, 1);
		hisifd->media_common_composer_sr_refcount = 0;

		hisifd->fb_mem_free_flag = true;

		hisifd->set_fastboot_fnc = NULL;
		hisifd->open_sub_fnc = NULL;
		hisifd->release_sub_fnc = NULL;
		hisifd->hpd_open_sub_fnc = NULL;
		hisifd->hpd_release_sub_fnc = NULL;
		hisifd->lp_fnc = NULL;
		hisifd->esd_fnc = NULL;
		hisifd->sbl_ctrl_fnc = NULL;
		hisifd->fps_upt_isr_handler = NULL;
		hisifd->mipi_dsi_bit_clk_upt_isr_handler = NULL;
		hisifd->sysfs_attrs_add_fnc = NULL;
		hisifd->sysfs_attrs_append_fnc = NULL;
		hisifd->sysfs_create_fnc = NULL;
		hisifd->sysfs_remove_fnc = NULL;

		hisifd->pm_runtime_register = NULL;
		hisifd->pm_runtime_unregister = NULL;
		hisifd->pm_runtime_get = NULL;
		hisifd->pm_runtime_put = NULL;
		hisifd->bl_register = NULL;
		hisifd->bl_unregister = NULL;
		hisifd->bl_update = NULL;
		hisifd->bl_cancel = NULL;
		hisifd->vsync_register = NULL;
		hisifd->vsync_unregister = NULL;
		hisifd->vsync_ctrl_fnc = NULL;
		hisifd->vsync_isr_handler = NULL;
		hisifd->buf_sync_register = NULL;
		hisifd->buf_sync_unregister = NULL;
		hisifd->buf_sync_signal = NULL;
		hisifd->buf_sync_suspend = NULL;
		hisifd->secure_register = NULL;
		hisifd->secure_unregister = NULL;
		hisifd->esd_register = NULL;
		hisifd->esd_unregister = NULL;
		hisifd->debug_register = NULL;
		hisifd->debug_unregister = NULL;
		hisifd->video_idle_ctrl_register = NULL;
		hisifd->video_idle_ctrl_unregister = NULL;
		hisifd->pipe_clk_updt_isr_handler = NULL;
		hisifd->overlay_online_wb_register = NULL;
		hisifd->overlay_online_wb_unregister = NULL;
	} else {
		sema_init(&hisifd->offline_composer_sr_sem, 1);
		hisifd->offline_composer_sr_refcount = 0;

		hisifd->fb_mem_free_flag = true;

		hisifd->set_fastboot_fnc = NULL;
		hisifd->open_sub_fnc = NULL;
		hisifd->release_sub_fnc = NULL;
		hisifd->hpd_open_sub_fnc = NULL;
		hisifd->hpd_release_sub_fnc = NULL;
		hisifd->lp_fnc = NULL;
		hisifd->esd_fnc = NULL;
		hisifd->sbl_ctrl_fnc = NULL;
		hisifd->fps_upt_isr_handler = NULL;
		hisifd->mipi_dsi_bit_clk_upt_isr_handler = NULL;
		hisifd->sysfs_attrs_add_fnc = NULL;
		hisifd->sysfs_attrs_append_fnc = NULL;
		hisifd->sysfs_create_fnc = NULL;
		hisifd->sysfs_remove_fnc = NULL;

		hisifd->pm_runtime_register = NULL;
		hisifd->pm_runtime_unregister = NULL;
		hisifd->pm_runtime_get = NULL;
		hisifd->pm_runtime_put = NULL;
		hisifd->bl_register = NULL;
		hisifd->bl_unregister = NULL;
		hisifd->bl_update = NULL;
		hisifd->bl_cancel = NULL;
		hisifd->vsync_register = NULL;
		hisifd->vsync_unregister = NULL;
		hisifd->vsync_ctrl_fnc = NULL;
		hisifd->vsync_isr_handler = NULL;
		hisifd->buf_sync_register = NULL;
		hisifd->buf_sync_unregister = NULL;
		hisifd->buf_sync_signal = NULL;
		hisifd->buf_sync_suspend = NULL;
		hisifd->secure_register = NULL;
		hisifd->secure_unregister = NULL;
		hisifd->esd_register = NULL;
		hisifd->esd_unregister = NULL;
		hisifd->debug_register = NULL;
		hisifd->debug_unregister = NULL;
		hisifd->video_idle_ctrl_register = NULL;
		hisifd->video_idle_ctrl_unregister = NULL;
		hisifd->pipe_clk_updt_isr_handler = NULL;
		hisifd->overlay_online_wb_register = NULL;
		hisifd->overlay_online_wb_unregister = NULL;
	}


	if (hisi_overlay_init(hisifd)) {
		HISI_FB_ERR("unable to init overlay!\n");
		return -EPERM;
	}

	hisi_display_effect_init(hisifd);

	if (register_framebuffer(fbi) < 0) {
		HISI_FB_ERR("fb%d failed to register_framebuffer!", hisifd->index);
		return -EPERM;
	}

	if (hisifd->sysfs_attrs_add_fnc) {
		hisifd->sysfs_attrs_add_fnc(hisifd);
	}

	/* debug register */
	if (hisifd->debug_register)
		hisifd->debug_register(hisifd->pdev);
	/* backlight register */
	if (hisifd->bl_register)
		hisifd->bl_register(hisifd->pdev);
	/* vsync register */
	if (hisifd->vsync_register)
		hisifd->vsync_register(hisifd->pdev);
	/* secure register */
	if (hisifd->secure_register)
		hisifd->secure_register(hisifd->pdev);
	/* buf_sync register */
	if (hisifd->buf_sync_register)
		hisifd->buf_sync_register(hisifd->pdev);
	/* pm runtime register */
	if (hisifd->pm_runtime_register)
		hisifd->pm_runtime_register(hisifd->pdev);
	/* fb sysfs create */
	if (hisifd->sysfs_create_fnc)
		hisifd->sysfs_create_fnc(hisifd->pdev);
	/* lcd check esd register */
	if (hisifd->esd_register)
		hisifd->esd_register(hisifd->pdev);
	/* register video idle crtl */
	if (hisifd->video_idle_ctrl_register)
		hisifd->video_idle_ctrl_register(hisifd->pdev);
	if (hisifd->overlay_online_wb_register)
		hisifd->overlay_online_wb_register(hisifd->pdev);

	HISI_FB_INFO("FrameBuffer[%d] %dx%d size=%d bytes"
		"is registered successfully!\n",
		hisifd->index, var->xres, var->yres, fbi->fix.smem_len);

	return 0;
}


/*******************************************************************************
**
*/
static int hisi_fb_enable_iommu(struct platform_device *pdev)
{
	struct iommu_domain *hisi_domain = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}

	/* create iommu domain */
	hisi_domain = hisi_ion_enable_iommu(pdev);
	if (!hisi_domain) {
		HISI_FB_ERR("iommu_domain_alloc failed!\n");
		return -EINVAL;
	}

	g_hisi_domain = hisi_domain;

	return 0;
}

static void hisi_create_aod_wq(struct hisi_fb_data_type *hisifd)
{
	if (NULL ==  hisifd) {
		HISI_FB_ERR("hisifd NULL Pointer!\n");
		return;
	}
	/*creat aod workqueue*/
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hisifd->aod_ud_fast_unblank_workqueue= create_singlethread_workqueue("aod_ud_fast_unblank");
		if (!hisifd->aod_ud_fast_unblank_workqueue) {
			HISI_FB_ERR("creat aod work queue failed!\n");
			return;
		}
		INIT_WORK(&hisifd->aod_ud_fast_unblank_work, hisi_fb_unblank_wq_handle);

		g_hisifd_fb0 = hisifd;
	}
}

static int hisi_fb_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct device_node *np = NULL;
	struct device *dev = NULL;
	static struct workqueue_struct *queue = NULL;
	static struct work_struct work;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}
	dev = &pdev->dev;

	if (!hisi_fb_resource_initialized) {
		dev_dbg(dev, "initialized=%d, +.\n", hisi_fb_resource_initialized);

		pdev->id = 0;

		np = of_find_compatible_node(NULL, NULL, DTS_COMP_FB_NAME);
		if (!np) {
			dev_err(dev, "NOT FOUND device node %s!\n", DTS_COMP_FB_NAME);
			return -ENXIO;
		}

		ret = of_property_read_u32(np, "fpga_flag", &g_fpga_flag);
		if (ret) {
			dev_err(dev, "failed to get fpga_flag resource.\n");
			return -ENXIO;
		}
		dev_info(dev, "g_fpga_flag=%d.\n", g_fpga_flag);

		ret = of_property_read_u32(np, "fastboot_enable_flag", &g_fastboot_enable_flag);
		if (ret) {
			dev_err(dev, "failed to get fastboot_display_flag resource.\n");
			return -ENXIO;
		}
		dev_info(dev, "g_fastboot_enable_flag=%d.\n", g_fastboot_enable_flag);

		ret = of_property_read_u32(np, "fake_lcd_flag", &g_fake_lcd_flag);
		if (ret) {
			dev_err(dev, "failed to get fake_lcd_flag resource.\n");
			return -ENXIO;
		}
		dev_info(dev, "g_fake_lcd_flag=%d.\n", g_fake_lcd_flag);

		ret = of_property_read_u32(np, "dss_base_phy", &g_dss_base_phy);
		if (ret) {
			dev_err(dev, "failed to get dss_base_phy.\n");
			return -ENXIO;
		}
		dev_info(dev, "g_dss_base_phy=0x%x.\n", g_dss_base_phy);

		ret = of_property_read_u32(np, "dss_version_tag", &g_dss_version_tag);
		if (ret) {
			dev_err(dev, "failed to get g_dss_version_tag.\n");
		}
		dev_info(dev, "g_dss_version_tag=0x%x.\n", g_dss_version_tag);

		/* get irq no */
		hisifd_irq_pdp = irq_of_parse_and_map(np, 0);
		if (!hisifd_irq_pdp) {
			dev_err(dev, "failed to get hisifd_irq_pdp resource.\n");
			return -ENXIO;
		}

		hisifd_irq_sdp = irq_of_parse_and_map(np, 1);
		if (!hisifd_irq_sdp) {
			dev_err(dev, "failed to get hisifd_irq_sdp resource.\n");
			return -ENXIO;
		}

		hisifd_irq_adp = irq_of_parse_and_map(np, 2);
		if (!hisifd_irq_adp) {
			dev_err(dev, "failed to get hisifd_irq_adp resource.\n");
			return -ENXIO;
		}

		hisifd_irq_dsi0 = irq_of_parse_and_map(np, 3);
		if (!hisifd_irq_dsi0) {
			dev_err(dev, "failed to get hisifd_irq_dsi0 resource.\n");
			return -ENXIO;
		}

		hisifd_irq_dsi1 = irq_of_parse_and_map(np, 4);
		if (!hisifd_irq_dsi1) {
			dev_err(dev, "failed to get hisifd_irq_dsi1 resource.\n");
			return -ENXIO;
		}



		/* get dss reg base */
		hisifd_dss_base = of_iomap(np, 0);
		if (!hisifd_dss_base) {
			dev_err(dev, "failed to get hisifd_dss_base resource.\n");
			return -ENXIO;
		}

		hisifd_peri_crg_base = of_iomap(np, 1);
		if (!hisifd_peri_crg_base) {
			dev_err(dev, "failed to get hisifd_peri_crg_base resource.\n");
			return -ENXIO;
		}

		hisifd_sctrl_base = of_iomap(np, 2);
		if (!hisifd_sctrl_base) {
			dev_err(dev, "failed to get hisifd_sctrl_base resource.\n");
			return -ENXIO;
		}

		hisifd_pctrl_base = of_iomap(np, 3);
		if (!hisifd_pctrl_base) {
			dev_err(dev, "failed to get hisifd_pctrl_base resource.\n");
			return -ENXIO;
		}

		hisifd_noc_dss_base = of_iomap(np, 4);
		if (!hisifd_noc_dss_base) {
			dev_err(dev, "failed to get hisifd_noc_dss_base resource.\n");
			return -ENXIO;
		}

		hisifd_mmbuf_crg_base = of_iomap(np, 5);
		if (!hisifd_mmbuf_crg_base) {
			dev_err(dev, "failed to get hisifd_mmbuf_crg_base resource.\n");
			return -ENXIO;
		}

		hisifd_pmctrl_base = of_iomap(np, 6);
		if (!hisifd_pmctrl_base) {
			dev_err(dev, "failed to get hisifd_pmctrl_base resource.\n");
			return -ENXIO;
		}

		/* get regulator resource */
		g_dpe_regulator[0].supply = REGULATOR_PDP_NAME;
		g_dpe_regulator[1].supply = REGULATOR_MMBUF;
		g_dpe_regulator[2].supply = REGULATOR_MEDIA_NAME;
		ret = devm_regulator_bulk_get(&(pdev->dev),
			ARRAY_SIZE(g_dpe_regulator), g_dpe_regulator);
		if (ret) {
			dev_err(dev, "failed to get regulator resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		/* get dss clk resource */
		ret = of_property_read_string_index(np, "clock-names", 0, &g_dss_axi_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get axi_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 1, &g_dss_pclk_dss_name);
		if (ret != 0) {
			dev_err(dev, "failed to get pclk_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 2, &g_dss_pri_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get pri_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 3, &g_dss_pxl0_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get pxl0_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 4, &g_dss_pxl1_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get pxl1_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}

		ret = of_property_read_string_index(np, "clock-names", 5, &g_dss_mmbuf_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get mmbuf_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 6, &g_dss_pclk_mmbuf_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get pclk_mmbuf_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}

		ret = of_property_read_string_index(np, "clock-names", 7, &g_dss_dphy0_ref_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dphy0_ref_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 8, &g_dss_dphy1_ref_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dphy1_ref_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 9, &g_dss_dphy0_cfg_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dphy0_cfg_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 10, &g_dss_dphy1_cfg_clk_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dphy1_cfg_clk resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 11, &g_dss_pclk_dsi0_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dss_pclk_dsi0 resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 12, &g_dss_pclk_dsi1_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dss_pclk_dsi1 resource! ret=%d.\n", ret);
			return -ENXIO;
		}
		ret = of_property_read_string_index(np, "clock-names", 13, &g_dss_pclk_pctrl_name);
		if (ret != 0) {
			dev_err(dev, "failed to get dss_pclk_pctrl resource! ret=%d.\n", ret);
			return -ENXIO;
		}

		ret = hisi_fb_enable_iommu(pdev);
		if (ret != 0) {
			dev_err(dev, "failed to hisi_fb_enable_iommu! ret=%d.\n", ret);
			return -ENXIO;
		}

		/* find and get logo-buffer base */
		np = of_find_node_by_path(DTS_PATH_LOGO_BUFFER);
		if (!np) {
			dev_err(dev, "NOT FOUND dts path: %s!\n", DTS_PATH_LOGO_BUFFER);
			//return -ENXIO;
		}

		if (g_fastboot_enable_flag == 1) {
			ret = of_property_read_u32_index(np, "reg", 1, &g_logo_buffer_base);
			if (ret != 0) {
				dev_err(dev, "failed to get g_logo_buffer_base resource! ret=%d.\n", ret);
				g_logo_buffer_base = 0;
			}
			ret = of_property_read_u32_index(np, "reg", 3, &g_logo_buffer_size);
			if (ret != 0) {
				dev_err(dev, "failed to get g_logo_buffer_size resource! ret=%d.\n", ret);
				g_logo_buffer_size = 0;
			}

			dev_info(dev, "g_logo_buffer_base = 0x%x, g_logo_buffer_size = 0x%x. \n", g_logo_buffer_base, g_logo_buffer_size);
		}

		hisi_fb_resource_initialized = 1;

		hisi_fb_device_set_status0(DTS_FB_RESOURCE_INIT_READY);

		dev_dbg(dev, "initialized=%d, -.\n", hisi_fb_resource_initialized);
		return 0;
	}

	if (pdev->id < 0) {
		dev_err(dev, "WARNING: id=%d, name=%s!\n", pdev->id, pdev->name);
		return 0;
	}

	if (!hisi_fb_resource_initialized) {
		dev_err(dev, "fb resource not initialized!\n");
		return -EPERM;
	}

	if (pdev_list_cnt >= HISI_FB_MAX_DEV_LIST) {
		dev_err(dev, "too many fb devices, num=%d!\n", pdev_list_cnt);
		return -ENOMEM;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(dev, "hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = hisi_fb_register(hisifd);
	if (ret) {
		dev_err(dev, "fb%d hisi_fb_register failed, error=%d!\n", hisifd->index, ret);
		return ret;
	}

	/* config earlysuspend */

	pdev_list[pdev_list_cnt++] = pdev;

	/* set device probe status */
	hisi_fb_device_set_status1(hisifd);

	hisi_create_aod_wq(hisifd);


	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int hisi_fb_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	/* pm_runtime unregister */
	if (hisifd->pm_runtime_unregister)
		hisifd->pm_runtime_unregister(pdev);

	/* stop the device */
	if (hisi_fb_suspend_sub(hisifd) != 0)
		HISI_FB_ERR("fb%d hisi_fb_suspend_sub failed!\n", hisifd->index);

	/* overlay destroy */
	hisi_overlay_deinit(hisifd);

	/* free framebuffer */
	hisifb_free_fb_buffer(hisifd);
	if (hisifd->ion_client) {
		ion_client_destroy(hisifd->ion_client);
		hisifd->ion_client = NULL;
	}

	/* remove /dev/fb* */
	unregister_framebuffer(hisifd->fbi);

	/* unregister buf_sync */
	if (hisifd->buf_sync_unregister)
		hisifd->buf_sync_unregister(pdev);
	/* unregister vsync */
	if (hisifd->vsync_unregister)
		hisifd->vsync_unregister(pdev);
	/* unregister backlight */
	if (hisifd->bl_unregister)
		hisifd->bl_unregister(pdev);
	/* fb sysfs remove */
	if (hisifd->sysfs_remove_fnc)
		hisifd->sysfs_remove_fnc(hisifd->pdev);
	/* lcd check esd remove */
	if (hisifd->esd_unregister)
		hisifd->esd_unregister(hisifd->pdev);
	/* unregister debug */
	if (hisifd->debug_unregister)
		hisifd->debug_unregister(hisifd->pdev);
	/* remove video idle ctrl */
	if (hisifd->video_idle_ctrl_unregister)
		hisifd->video_idle_ctrl_unregister(hisifd->pdev);
	/*remove overlay online wirteback*/
	if (hisifd->overlay_online_wb_unregister)
		hisifd->overlay_online_wb_unregister(hisifd->pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int hisi_fb_suspend_sub(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	ret = hisi_fb_blank_sub(FB_BLANK_POWERDOWN, hisifd->fbi);
	if (ret) {
		HISI_FB_ERR("fb%d can't turn off display, error=%d!\n", hisifd->index, ret);
		return ret;
	}

	return 0;
}

static int hisi_fb_resume_sub(struct hisi_fb_data_type *hisifd)
{
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	ret = hisi_fb_blank_sub(FB_BLANK_UNBLANK, hisifd->fbi);
	if (ret) {
		HISI_FB_ERR("fb%d can't turn on display, error=%d!\n", hisifd->index, ret);
	}

	return ret;
}


static int hisi_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	console_lock();
	fb_set_suspend(hisifd->fbi, FBINFO_STATE_SUSPENDED);
	ret = hisi_fb_suspend_sub(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("fb%d hisi_fb_suspend_sub failed, error=%d!\n", hisifd->index, ret);
		fb_set_suspend(hisifd->fbi, FBINFO_STATE_RUNNING);
	} else {
		pdev->dev.power.power_state = state;
	}
	console_unlock();

	HISI_FB_INFO("fb%d, -.\n", hisifd->index);

	return ret;
}

static int hisi_fb_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	console_lock();
	ret = hisi_fb_resume_sub(hisifd);
	pdev->dev.power.power_state = PMSG_ON;
	fb_set_suspend(hisifd->fbi, FBINFO_STATE_RUNNING);
	console_unlock();

	HISI_FB_INFO("fb%d, -.\n", hisifd->index);

	return ret;
}


/*******************************************************************************
** pm_runtime
*/

static int hisi_fb_pm_suspend(struct device *dev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

	if (NULL == dev) {
		HISI_FB_ERR("NULL Poniter\n");
		return 0;
	}

	hisifd = dev_get_drvdata(dev);
	if (!hisifd)
		return 0;

	if (hisifd->index == EXTERNAL_PANEL_IDX || hisifd->index == AUXILIARY_PANEL_IDX)
		return 0;

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	ret = hisi_fb_suspend_sub(hisifd);
	if (ret != 0) {
		HISI_FB_ERR("fb%d, failed to hisi_fb_suspend_sub! ret=%d\n", hisifd->index, ret);
	}

	HISI_FB_INFO("fb%d, -.\n", hisifd->index);

	return 0;
}


static void hisi_fb_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL Pointer\n");
		return;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		if (pdev->id) {
			HISI_FB_ERR("hisifd NULL Pointer,pdev->id=%d\n", pdev->id);
		}
		return;
	}

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_DEBUG("fb%d do not shutdown\n", hisifd->index);
		return;
	}

	HISI_FB_INFO("fb%d shutdown +\n", hisifd->index);
	hisifd->fb_shutdown = true;

	ret = hisi_fb_blank_sub(FB_BLANK_POWERDOWN, hisifd->fbi);
	if (ret) {
		HISI_FB_ERR("fb%d can't turn off display, error=%d!\n", hisifd->index, ret);
	}

	HISI_FB_INFO("fb%d shutdown -\n", hisifd->index);
}


/*******************************************************************************
**
*/
static struct dev_pm_ops hisi_fb_dev_pm_ops = {
	.suspend = hisi_fb_pm_suspend,
	.resume = NULL,
};

static const struct of_device_id hisi_fb_match_table[] = {
	{
		.compatible = DTS_COMP_FB_NAME,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_fb_match_table);

static struct platform_driver hisi_fb_driver = {
	.probe = hisi_fb_probe,
	.remove = hisi_fb_remove,
	.suspend = hisi_fb_suspend,
	.resume = hisi_fb_resume,
	.shutdown = hisi_fb_shutdown,
	.driver = {
		.name = DEV_NAME_FB,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_fb_match_table),
		.pm = &hisi_fb_dev_pm_ops,
	},
};

static int __init hisi_fb_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&hisi_fb_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(hisi_fb_init);

MODULE_DESCRIPTION("Hisilicon Framebuffer Driver");
MODULE_LICENSE("GPL v2");
/*lint -restore*/
