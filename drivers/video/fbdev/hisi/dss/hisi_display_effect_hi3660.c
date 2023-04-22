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
#include "hisi_fb.h"
#include <linux/fb.h>
#include "global_ddr_map.h"
#include <linux/printk.h>

//lint -e747, -e838

#define COUNT_LIMIT_TO_PRINT_DELAY			(200)
#define EFFECT_GRADUAL_REFRESH_FRAMES		(30)
#define METADATA_INIT_LENGTH			100
#define GALLERY_OFF 		0x02
#define GALLERY_CTRL_BIT			2
/*#define LCD_CE_MODE_GALLERY 		lce_ce_mode_gallery
#define LCD_CE_MODE_DEFAULT 		lce_ce_mode_default
#define LCD_CE_MODE_VIDEO			lce_ce_mode_video*/
#define EFFECT_CE_GALLERY			effect_ce_gallery
#define ERR_RET 		-1

typedef struct time_interval {
	long start; 				// microsecond
	long stop;
} time_interval_t;

typedef struct delay_record {
	const char *name;
	long max;
	long min;
	long sum;
	int count;
} delay_record_t;

static int hisifb_ce_do_contrast(struct hisi_fb_data_type *hisifd);
static void hisifb_ce_service_init(void);
static void hisifb_ce_service_deinit(void);

#define _EFFECT_DEBUG_LOG(message, ...) \
	do { pr_info("%s: "message, __func__, ## __VA_ARGS__); } while (0)
#define EFFECT_DEBUG_LOG(feature, message, ...) \
	do { if (g_debug_effect & feature) \
		pr_info("%s: "message, __func__, ## __VA_ARGS__); } while (0)

#define DEFINE_DELAY_RECORD(var, name)		static delay_record_t var = {name, 0, 0xFFFFFFFF, 0, 0}

#define ce_service_wait_event(wq, condition)		/*lint -save -e* */						\
({																								\
	long _wait_ret = 0; 																		\
	do {																						\
		_wait_ret = wait_event_interruptible_timeout(wq, condition, msecs_to_jiffies(100000));	\
	} while(!_wait_ret);																		\
	_wait_ret;																					\
})		/*lint -restore */																		\

#define acm_set_hue(acm_dst, pinfo)\
	acm_dst->hue_r0_hh = pinfo->r0_hh;\
	acm_dst->hue_r0_lh = pinfo->r0_lh;\
	acm_dst->hue_r1_hh = pinfo->r1_hh;\
	acm_dst->hue_r1_lh = pinfo->r1_lh;\
	acm_dst->hue_r2_hh = pinfo->r2_hh;\
	acm_dst->hue_r2_lh = pinfo->r2_lh;\
	acm_dst->hue_r3_hh = pinfo->r3_hh;\
	acm_dst->hue_r3_lh = pinfo->r3_lh;\
	acm_dst->hue_r4_hh = pinfo->r4_hh;\
	acm_dst->hue_r4_lh = pinfo->r4_lh;\
	acm_dst->hue_r5_hh = pinfo->r5_hh;\
	acm_dst->hue_r5_lh = pinfo->r5_lh;\
	acm_dst->hue_r6_hh = pinfo->r6_hh;\
	acm_dst->hue_r6_lh = pinfo->r6_lh;

static bool g_is_effect_init = false;
static bool g_is_ce_service_init = false;
static struct mutex g_ce_service_lock;
static ce_service_t g_hiace_service;

static bool g_is_effect_lock_init = false;
static spinlock_t g_gmp_effect_lock;
static spinlock_t g_igm_effect_lock;
static spinlock_t g_xcc_effect_lock;
static spinlock_t g_gamma_effect_lock;
static spinlock_t g_acm_effect_lock;
static time_interval_t interval_wait_hist = {0};
static time_interval_t interval_wait_lut = {0};
static time_interval_t interval_algorithm = {0};
DEFINE_DELAY_RECORD(delay_wait_hist, "event hist waiting");
DEFINE_DELAY_RECORD(delay_wait_lut, "event lut waiting");
DEFINE_DELAY_RECORD(delay_algorithm, "algorithm processing");

/*static char lce_ce_mode_gallery[2] = {'0','\0'};
static char lce_ce_mode_default[2] = {'1','\0'};
static char lce_ce_mode_video[2] = {'2','\0'};*/

int g_enable_effect = ENABLE_EFFECT_HIACE | ENABLE_EFFECT_BL;
int g_debug_effect = 0;
static bool g_gmp_backup_enable = true;

static inline long get_timestamp_in_us(void)
{
	struct timespec ts;
	long timestamp;
	ktime_get_ts(&ts);
	timestamp = ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;
	return timestamp;
}

static inline void count_delay(delay_record_t *record, long delay)
{
	record->count++;
	record->sum += delay;
	if (delay > record->max) record->max = delay;
	if (delay < record->min) record->min = delay;
	if (!(record->count % COUNT_LIMIT_TO_PRINT_DELAY)) {
		_EFFECT_DEBUG_LOG("[effect] Delay(us/%4d) | average:%5ld | min:%5ld | max:%8ld | %s\n", record->count, record->sum / record->count, record->min, record->max, record->name);
		record->count = 0;
		record->sum = 0;
		record->max = 0;
		record->min = 0xFFFFFFFF;
	}
}

static inline uint32_t get_fixed_point_offset(uint32_t half_block_size)
{
	uint32_t num = 2;
	uint32_t len = 2;
	while (len < half_block_size) {
		num++;
		len <<= 1;
	}
	return num;
}

bool check_underflow_clear_ack(struct hisi_fb_data_type *hisifd)
{
	char __iomem *mctl_base = NULL;
	uint32_t mctl_status = 0;

	if(NULL == hisifd){
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return false;
	}

	mctl_base = hisifd->dss_module.mctl_base[DSS_MCTL0];
	if (mctl_base) {
		mctl_status = inp32(mctl_base + MCTL_CTL_STATUS);
		if ((mctl_status & 0x8) == 0x8) {
			HISI_FB_ERR("UNDERFLOW ACK TIMEOUT!\n");
			return false;
		}
	}
	return true;
}

static inline void enable_hiace(struct hisi_fb_data_type *hisifd, bool enable)
{
	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on) {
		if (hisifd->hiace_info.hiace_enable != enable) { //lint !e731
			char __iomem *hiace_base = hisifd->dss_base + DSS_HI_ACE_OFFSET;

			hisifb_activate_vsync(hisifd);
			if(!check_underflow_clear_ack(hisifd)){
				HISI_FB_ERR("[effect] fb%d, clear_ack is not return!\n", hisifd->index);
				return;
			}

			if (enable) {
				// enable hiace
				set_reg(hiace_base + HIACE_BYPASS_ACE, 0x0, 1, 0);
			} else {
				// disable hiace
				set_reg(hiace_base + HIACE_BYPASS_ACE, 0x1, 1, 0);
			}
			hisifb_deactivate_vsync(hisifd);

			hisifd->hiace_info.hiace_enable = enable;
			EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] hiace:%d\n", (int)enable);
		}
	} else {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] fb%d, panel power off!\n", hisifd->index);
	}
	up(&hisifd->blank_sem);
}

int hisifb_ce_service_enable_hiace(struct fb_info *info, void __user *argp)
{
	struct hisi_panel_info *pinfo = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;
	dss_ce_info_t *ce_info = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_metadata_t *metadata_ctrl = NULL;
	int enable;
	int ret;
	int mode;
	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
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
	pinfo = &(hisifd->panel_info);
	if (pinfo->hiace_support == 0) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] HIACE is not supported!\n");
		return -1;
	}
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ce_ctrl = &(hisifd->ce_ctrl);
		ce_info = &(hisifd->hiace_info);
		metadata_ctrl = &(hisifd->metadata_ctrl);
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!", hisifd->index);
		return -1;
	}
	ret = (int)copy_from_user(&enable, argp, sizeof(enable));
	if (ret) {
		HISI_FB_ERR("[effect] arg is invalid");
		return -EINVAL;
	}
	mode = enable;
	if (mode < 0) {
		mode = 0;
	} else if (mode >= CE_MODE_COUNT) {
		mode = CE_MODE_COUNT - 1;
	}

	if (mode != ce_ctrl->ctrl_ce_mode) {
		mutex_lock(&(ce_ctrl->ctrl_lock));
		ce_ctrl->ctrl_ce_mode = mode;
		mutex_unlock(&(ce_ctrl->ctrl_lock));
		if (mode == CE_MODE_DISABLE && hisifd->panel_power_on) {
			ce_info->gradual_frames = EFFECT_GRADUAL_REFRESH_FRAMES;
			ce_info->to_stop_hdr = true;
		}
		enable_hiace(hisifd, mode);
	}
	return 0;
}
int hisifb_get_reg_val(struct fb_info *info, void __user *argp) {
	struct hisi_fb_data_type *hisifd = NULL;
	struct dss_reg reg;
	uint32_t addr;
	int ret = 0;
	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
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
	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] step in\n");
	ret = (int)copy_from_user(&reg, argp, sizeof(struct dss_reg));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		return -2;
	}

	switch(reg.tag) {
		case TAG_ARSR_1P_ENABLE:
			addr = DSS_POST_SCF_OFFSET + ARSR1P_MODE;
			break;
		case TAG_LCP_XCC_ENABLE:
			addr = DSS_DPP_LCP_OFFSET + LCP_XCC_BYPASS_EN;
			break;
		case TAG_LCP_GMP_ENABLE:
			addr = DSS_DPP_LCP_OFFSET + LCP_GMP_BYPASS_EN;
			break;
		case TAG_LCP_IGM_ENABLE:
			addr = DSS_DPP_LCP_OFFSET + LCP_DEGAMA_EN;
			break;
		case TAG_GAMMA_ENABLE:
			addr = DSS_DPP_GAMA_OFFSET + GAMA_EN;
			break;
		//case HIACE_LHIST_SFT:
		//	addr = DSS_HI_ACE_OFFSET + DPE_LHIST_SFT;
		//	break;
		default:
			HISI_FB_ERR("[effect] invalid tag : %u", reg.tag);
			return -EINVAL;
	}

	down(&hisifd->blank_sem);
	if (hisifd->panel_power_on == false) {
		HISI_FB_ERR("[effect] panel power off\n");
		up(&hisifd->blank_sem);
		return -EINVAL;
	}
	hisifb_activate_vsync(hisifd);

	reg.value = (uint32_t)inp32(hisifd->dss_base + addr);

	hisifb_deactivate_vsync(hisifd);
	up(&hisifd->blank_sem);

	ret = (int)copy_to_user(argp, &reg, sizeof(struct dss_reg));
	if (ret) {
		HISI_FB_ERR("[effect] copy_to_user failed(param)! ret=%d.\n", ret);
		ret = -EINVAL;
	}

	return 0;
}

static inline void enable_blc(struct hisi_fb_data_type *hisifd, bool enable)
{
	dss_display_effect_bl_enable_t *bl_enable_ctrl = &(hisifd->bl_enable_ctrl);
	int bl_enable = enable ? 1 : 0;

	mutex_lock(&(bl_enable_ctrl->ctrl_lock));
	if (bl_enable != bl_enable_ctrl->ctrl_bl_enable) {
		bl_enable_ctrl->ctrl_bl_enable = bl_enable;
	}
	mutex_unlock(&(bl_enable_ctrl->ctrl_lock));
	EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] bl_enable change to %d\n", bl_enable_ctrl->ctrl_bl_enable);
}

static int hisifb_ce_do_contrast(struct hisi_fb_data_type *hisifd)
{
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	long wait_ret = 0;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	if (g_is_ce_service_init) {
		service->new_hist = true;
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_wait_hist.start = get_timestamp_in_us();
		}

		if (g_is_effect_init) {
			wake_up_interruptible(&service->wq_hist);
			wait_ret = ce_service_wait_event(service->wq_lut, service->new_lut || service->stop);
			service->stop = false;
		} else {
			HISI_FB_ERR("[effect] wq_lut uninit\n");
			return -EINVAL;
		}

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_wait_lut.stop = get_timestamp_in_us();
			count_delay(&delay_wait_lut, interval_wait_lut.stop - interval_wait_lut.start);
		}
	}

	if (service->new_lut) {
		service->new_lut = false;
		ret = hisifd->hiace_info.algorithm_result;
	} else {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
		ret = -3;
	}

	return ret;
}

void hisi_effect_init(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	dss_ce_info_t *ce_info = NULL;
	hiace_alg_parameter_t *param = NULL;
	if (!g_is_effect_lock_init) {
		spin_lock_init(&g_gmp_effect_lock);
		spin_lock_init(&g_igm_effect_lock);
		spin_lock_init(&g_xcc_effect_lock);
		spin_lock_init(&g_gamma_effect_lock);
		spin_lock_init(&g_acm_effect_lock);
		g_is_effect_lock_init = true;
	}

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ce_info = &hisifd->hiace_info;
		param = &pinfo->hiace_param;
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!", hisifd->index);
		return;
	}

	if (!g_is_effect_init) {
		mutex_init(&g_ce_service_lock);
		mutex_init(&(hisifd->al_ctrl.ctrl_lock));
		mutex_init(&(hisifd->ce_ctrl.ctrl_lock));
		mutex_init(&(hisifd->bl_ctrl.ctrl_lock));
		mutex_init(&(hisifd->bl_enable_ctrl.ctrl_lock));
		mutex_init(&(hisifd->sre_ctrl.ctrl_lock));
		mutex_init(&(hisifd->metadata_ctrl.ctrl_lock));
		hisifd->al_ctrl.ctrl_al_value = -1;

		memset(ce_info, 0, sizeof(dss_ce_info_t));
		memset(hisifd->metadata_ctrl.metadata, 0, METADATA_SIZE);
		hisifd->metadata_ctrl.count = METADATA_INIT_LENGTH;
		hisifd->metadata_ctrl.metadata[GALLERY_CTRL_BIT] = GALLERY_OFF;
		hisifd->user_scene_mode = SCENE_MODE_DEFAULT;

		ce_info->algorithm_result = 1;
		mutex_init(&(ce_info->hist_lock));
		mutex_init(&(ce_info->lut_lock));

		param->iWidth = (int)pinfo->xres;
		param->iHeight = (int)pinfo->yres;
		param->iMode = 0;
		param->bitWidth = 10;
		param->iMinBackLight = (int)hisifd->panel_info.bl_min;
		param->iMaxBackLight = (int)hisifd->panel_info.bl_max;

		memset(&g_hiace_service, 0, sizeof(g_hiace_service));
		init_waitqueue_head(&g_hiace_service.wq_hist);
		init_waitqueue_head(&g_hiace_service.wq_lut);

		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] width:%d, height:%d, minbl:%d, maxbl:%d\n", param->iWidth, param->iHeight, param->iMinBackLight, param->iMaxBackLight);

		g_is_effect_init = true;
	} else {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] bypass\n");
	}
}

void hisi_effect_deinit(struct hisi_fb_data_type *hisifd)
{
	dss_ce_info_t *ce_info = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ce_info = &hisifd->hiace_info;
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!", hisifd->index);
		return;
	}

	if (g_is_effect_init) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] step in\n");
		g_is_effect_init = false;

		mutex_destroy(&(ce_info->hist_lock));
		mutex_destroy(&(ce_info->lut_lock));

		mutex_destroy(&(hisifd->al_ctrl.ctrl_lock));
		mutex_destroy(&(hisifd->ce_ctrl.ctrl_lock));
		mutex_destroy(&(hisifd->bl_ctrl.ctrl_lock));
		mutex_destroy(&(hisifd->bl_enable_ctrl.ctrl_lock));
		mutex_destroy(&(hisifd->sre_ctrl.ctrl_lock));
		mutex_destroy(&(hisifd->metadata_ctrl.ctrl_lock));

		mutex_destroy(&g_ce_service_lock);
	} else {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] bypass\n");
	}
}

static void hisifb_ce_service_init(void)
{
	mutex_lock(&g_ce_service_lock);
	if (!g_is_ce_service_init) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] step in\n");

		g_hiace_service.is_ready = true;
		g_hiace_service.stop = false;

		g_is_ce_service_init = true;
	}
	mutex_unlock(&g_ce_service_lock);
}

static void hisifb_ce_service_deinit(void)
{
	mutex_lock(&g_ce_service_lock);
	if (g_is_ce_service_init) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] step in\n");

		g_is_ce_service_init = false;

		g_hiace_service.is_ready = false;
		g_hiace_service.stop = true;
		wake_up_interruptible(&g_hiace_service.wq_hist);
		wake_up_interruptible(&g_hiace_service.wq_lut);
	}
	mutex_unlock(&g_ce_service_lock);
}

int hisifb_ce_service_blank(int blank_mode, struct fb_info *info)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		if (pinfo->hiace_support) {
			EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] blank_mode is %d\n", blank_mode);
			if (blank_mode == FB_BLANK_UNBLANK) {
				hisifb_ce_service_init();
				enable_blc(hisifd, true);
				if (hisifb_display_effect_is_need_ace(hisifd)) {
					enable_hiace(hisifd, true);
				}
			} else {
				if (hisifb_display_effect_is_need_ace(hisifd)) {
					g_hiace_service.use_last_value = true;
				}
				hisifd->hiace_info.gradual_frames = 0;
				hisifd->hiace_info.hiace_enable = false;
				hisifd->hiace_info.to_stop_hdr = false;
				hisifd->hiace_info.to_stop_sre = false;
				g_hiace_service.blc_used = false;
				// Since java has no destruct function and Gallery will refresh metadata when power on, always close HDR for Gallery when power off.
				if (hisifd->ce_ctrl.ctrl_ce_mode == CE_MODE_IMAGE) {
					hisifd->ce_ctrl.ctrl_ce_mode = CE_MODE_DISABLE;
				}
				enable_blc(hisifd, false);
				hisifb_ce_service_deinit();
			}
		}
	}
	return 0;
}

int hisifb_ce_service_get_support(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	unsigned int support = 0;
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

	pinfo = &(hisifd->panel_info);
	if (pinfo->hiace_support) {
		support = 1;
	}
	EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] support:%d\n", support);

	ret = (int)copy_to_user(argp, &support, sizeof(support));
	if (ret) {
		HISI_FB_ERR("[effect] copy_to_user failed! ret=%d.\n", ret);
		return ret;
	}

	return ret;
}

int hisifb_ce_service_get_limit(struct fb_info *info, void __user *argp)
{
	int limit = 0;
	int ret = 0;

	if (NULL == argp) {
		HISI_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	ret = (int)copy_to_user(argp, &limit, sizeof(limit));
	if (ret) {
		HISI_FB_ERR("copy_to_user failed! ret=%d.\n", ret);
		return ret;
	}

	return ret;
}

int hisifb_ce_service_get_param(struct fb_info *info, void __user *argp)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;
	dss_display_effect_al_t *al_ctrl = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;
	dss_display_effect_metadata_t *metadata_ctrl = NULL;
	dss_ce_info_t *ce_info = NULL;
	int mode = 0;
	struct timespec ts;

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);
	if (!pinfo->hiace_support) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}

	ce_ctrl = &(hisifd->ce_ctrl);
	al_ctrl = &(hisifd->al_ctrl);
	sre_ctrl = &(hisifd->sre_ctrl);
	metadata_ctrl = &(hisifd->metadata_ctrl);
	ce_info = &(hisifd->hiace_info);
	mode = ce_ctrl->ctrl_ce_mode;
	if (mode != CE_MODE_DISABLE) {
		pinfo->hiace_param.iDoLCE = 1;
		pinfo->hiace_param.iDoAPLC = (mode == CE_MODE_VIDEO && hisifd->bl_level > 0 && al_ctrl->ctrl_al_value >= 0) ? 1 : 0;
	} else {
		if (ce_info->gradual_frames > 0) {
			pinfo->hiace_param.iDoLCE = -1;
			if (pinfo->hiace_param.iDoAPLC == 1) {
				pinfo->hiace_param.iDoAPLC = -1;
			}
		} else {
			pinfo->hiace_param.iDoLCE = 0;
			pinfo->hiace_param.iDoAPLC = 0;
			ce_info->to_stop_hdr = false;
		}
	}
	if (hisifd->bl_level == 0) {
		pinfo->hiace_param.iDoAPLC = 0;
	}
	pinfo->hiace_param.iDoSRE = sre_ctrl->ctrl_sre_enable;
	if (pinfo->hiace_param.iDoSRE == 0) {
		ce_info->to_stop_sre = false;
	}
	pinfo->hiace_param.iLevel = (mode == CE_MODE_VIDEO) ? 0 : 1;
	pinfo->hiace_param.iAmbientLight = (sre_ctrl->ctrl_sre_enable == 1) ? sre_ctrl->ctrl_sre_al : al_ctrl->ctrl_al_value;
	pinfo->hiace_param.iBackLight = (int)hisifd->bl_level;
	pinfo->hiace_param.iLaSensorSREOnTH = sre_ctrl->ctrl_sre_al_threshold;
	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] BLC:%d, bl:%d, panel_on:%d\n", pinfo->hiace_param.iDoAPLC, pinfo->hiace_param.iBackLight, hisifd->panel_power_on);
	ktime_get_ts(&ts);
	pinfo->hiace_param.lTimestamp = ts.tv_sec * MSEC_PER_SEC + ts.tv_nsec / NSEC_PER_MSEC;
	if (metadata_ctrl->count <= META_DATA_SIZE) {
		memcpy(pinfo->hiace_param.Classifieresult, metadata_ctrl->metadata, (size_t)metadata_ctrl->count);
		pinfo->hiace_param.iResultLen = metadata_ctrl->count;
	}

	ret = (int)copy_to_user(argp, &pinfo->hiace_param, sizeof(pinfo->hiace_param));
	if (ret) {
		HISI_FB_ERR("[effect] copy_to_user(hiace_param) failed! ret=%d.\n", ret);
		return ret;
	}

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] iLevel:%d, iAmbientLight:%d, iBackLight:%d, lTimestamp:%ld(ms)\n",
					 pinfo->hiace_param.iLevel, pinfo->hiace_param.iAmbientLight, pinfo->hiace_param.iBackLight, pinfo->hiace_param.lTimestamp);

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_algorithm.start = get_timestamp_in_us();
	}

	return ret;
}

int hisifb_ce_service_get_hiace_param(struct fb_info *info, void __user *argp){
	(void)info, (void)argp;
	return 0;
}

int hisifb_ce_service_set_param(struct fb_info *info, void __user *argp) {
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;
	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
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
	pinfo = &(hisifd->panel_info);
	if (!pinfo->hiace_support) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}
	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] step in\n");
	ret = (int)copy_from_user(&(hisifd->effect_info.hiace), argp, sizeof(struct hiace_info));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		return -2;
	}

	hisifd->effect_updated_flag.hiace_effect_updated = true;
	return ret;
}
int hisifb_ce_service_get_hist(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	long wait_ret = 0;

	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL\n");
		return -EINVAL;
	}

	if (hisifd->index != PRIMARY_PANEL_IDX) {
		HISI_FB_ERR("fb%d is not supported!\n", hisifd->index);
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_FB_ERR("[effect] argp is NULL\n");
		return -EINVAL;
	}

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] wait hist\n");

	if (g_is_effect_init) {
		unlock_fb_info(info);
		wait_ret = ce_service_wait_event(service->wq_hist, service->new_hist || service->stop);
		lock_fb_info(info);
		service->stop = false;
	}
	if (!g_is_effect_init) {
		HISI_FB_ERR("[effect] wq_hist uninit.\n");
		return -EINVAL;
	}

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_wait_hist.stop = get_timestamp_in_us();
		count_delay(&delay_wait_hist, interval_wait_hist.stop - interval_wait_hist.start);
	}

	if (service->new_hist) {
		time_interval_t interval_copy_hist = {0};
		DEFINE_DELAY_RECORD(delay_copy_hist, "hist copy");

		service->new_hist = false;

		mutex_lock(&hisifd->hiace_info.hist_lock);
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_copy_hist.start = get_timestamp_in_us();
		}
		ret = (int)copy_to_user(argp, hisifd->hiace_info.histogram, sizeof(hisifd->hiace_info.histogram));
		if (ret) {
			HISI_FB_ERR("[effect] copy_to_user failed(param)! ret=%d.\n", ret);
			ret = -1;
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_copy_hist.stop = get_timestamp_in_us();
			count_delay(&delay_copy_hist, interval_copy_hist.stop - interval_copy_hist.start);
		}
		mutex_unlock(&hisifd->hiace_info.hist_lock);
	} else {
		if (hisifd->panel_power_on) {
			EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
			ret = 3;
		} else {
			EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] Panel off! wait_event_interruptible_timeout() return %ld, -ERESTARTSYS:%d\n", wait_ret, -ERESTARTSYS);
			ret = 1;
		}
	}

	return ret;
}

int hisifb_ce_service_set_lut(struct fb_info *info, void __user *argp)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_display_effect_bl_t *bl_ctrl = NULL;
	hiace_interface_set_t hiace_set_interface;
	ce_service_t *service = &g_hiace_service;
	int ret = 0;
	time_interval_t interval_copy_lut = {0};
	DEFINE_DELAY_RECORD(delay_copy_lut, "lut copy");

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_algorithm.stop = get_timestamp_in_us();
		count_delay(&delay_algorithm, interval_algorithm.stop - interval_algorithm.start);
	}

	if (NULL == info) {
		HISI_FB_ERR("info is NULL\n");
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

	pinfo = &(hisifd->panel_info);
	if (!pinfo->hiace_support) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] Don't support HIACE\n");
		return -EINVAL;
	}

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	bl_ctrl = &(hisifd->bl_ctrl);

	ret = (int)copy_from_user(&hiace_set_interface, argp, sizeof(hiace_interface_set_t));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(param) failed! ret=%d.\n", ret);
		return -2;
	}

	hisifd->hiace_info.thminv = hiace_set_interface.thminv;

	mutex_lock(&hisifd->hiace_info.lut_lock);
	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_copy_lut.start = get_timestamp_in_us();
	}
	ret = (int)copy_from_user(hisifd->hiace_info.lut_table, hiace_set_interface.lut, sizeof(hisifd->hiace_info.lut_table));
	if (ret) {
		HISI_FB_ERR("[effect] copy_from_user(lut_table) failed! ret=%d.\n", ret);
		ret = -2;
	}
	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_copy_lut.stop = get_timestamp_in_us();
		count_delay(&delay_copy_lut, interval_copy_lut.stop - interval_copy_lut.start);
	}
	mutex_unlock(&hisifd->hiace_info.lut_lock);
	hisifd->hiace_info.algorithm_result = 0;

	service->new_lut = true;
	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_wait_lut.start = get_timestamp_in_us();
	}
	wake_up_interruptible(&service->wq_lut);

	return ret;
}

ssize_t hisifb_display_effect_al_ctrl_show(struct fb_info *info, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_al_t *al_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	al_ctrl = &(hisifd->al_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", al_ctrl->ctrl_al_value);
}

ssize_t hisifb_display_effect_al_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_al_t *al_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	al_ctrl = &(hisifd->al_ctrl);

	mutex_lock(&(al_ctrl->ctrl_lock));
	al_ctrl->ctrl_al_value = (int)simple_strtoul(buf, NULL, 0);
	if (al_ctrl->ctrl_al_value < 0) {
		al_ctrl->ctrl_al_value = 0;
	}
	mutex_unlock(&(al_ctrl->ctrl_lock));

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] ambient light is %d\n", al_ctrl->ctrl_al_value);

	return (ssize_t)count;
}

ssize_t hisifb_display_effect_ce_ctrl_show(struct fb_info *info, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_ce_t *ce_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	ce_ctrl = &(hisifd->ce_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", ce_ctrl->ctrl_ce_mode);
}

ssize_t hisifb_display_effect_ce_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	(void)info, (void)buf;

	return (ssize_t)count;
}

ssize_t hisifb_display_effect_bl_ctrl_show(struct fb_info *info, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_bl_t *bl_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	bl_ctrl = &(hisifd->bl_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", bl_ctrl->ctrl_bl_delta);
}

ssize_t hisifb_display_effect_bl_enable_ctrl_show(struct fb_info *info, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_bl_enable_t *bl_enable_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	bl_enable_ctrl = &(hisifd->bl_enable_ctrl);

	return snprintf(buf, PAGE_SIZE, "%d\n", bl_enable_ctrl->ctrl_bl_enable);
}

ssize_t hisifb_display_effect_bl_enable_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == info) {
		HISI_FB_ERR("[effect] info is NULL\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return -1;
	}

	enable_blc(hisifd, simple_strtoul(buf, NULL, 0) == 1);

	return (ssize_t)count;
}

ssize_t hisifb_display_effect_sre_ctrl_show(struct fb_info *info, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	sre_ctrl = &(hisifd->sre_ctrl);

	return snprintf(buf, PAGE_SIZE, "sre_enable:%d, sre_al_threshold:%d, sre_al:%d\n", sre_ctrl->ctrl_sre_enable, sre_ctrl->ctrl_sre_al_threshold, sre_ctrl->ctrl_sre_al);
}

ssize_t hisifb_display_effect_sre_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_display_effect_sre_t *sre_ctrl = NULL;
	dss_ce_info_t *ce_info = NULL;
	uint32_t value = (uint32_t)simple_strtoul(buf, NULL, 0);
	int last_enable = 0;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	pinfo = &(hisifd->panel_info);
	if (pinfo->hiace_support == 0) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] HIACE is not supported!\n");
		return -1;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		sre_ctrl = &(hisifd->sre_ctrl);
		ce_info = &(hisifd->hiace_info);
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!", hisifd->index);
		return -1;
	}

	last_enable = sre_ctrl->ctrl_sre_enable;
	mutex_lock(&(sre_ctrl->ctrl_lock));
	sre_ctrl->ctrl_sre_enable = (int)((value >> 28) & 0x1);
	sre_ctrl->ctrl_sre_al_threshold = (int)((value >> 16) & 0xFFF);
	sre_ctrl->ctrl_sre_al = (int)(value & 0xFFFF);
	mutex_unlock(&(sre_ctrl->ctrl_lock));
	if (last_enable != sre_ctrl->ctrl_sre_enable) {
		if (sre_ctrl->ctrl_sre_enable != 1) {
			ce_info->to_stop_sre = true;
		}
		enable_hiace(hisifd, hisifb_display_effect_is_need_ace(hisifd));
	}

	return (ssize_t)count;
}

ssize_t hisifb_display_effect_metadata_ctrl_show(struct fb_info *info, char *buf)
{
	int ret = 0;
	int i = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	dss_display_effect_metadata_t *metadata_ctrl = NULL;

	if (NULL == info) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	hisifd = (struct hisi_fb_data_type *)info->par;
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -1;
	}

	metadata_ctrl = &(hisifd -> metadata_ctrl);
	for(i = 0; i<metadata_ctrl->count ; i++) {
		ret += snprintf(buf+i, PAGE_SIZE, "%x", metadata_ctrl -> metadata[i]);
	}
	ret += snprintf(buf+ret, PAGE_SIZE, "\n");
	return ret;
}

ssize_t hisifb_display_effect_metadata_ctrl_store(struct fb_info *info, const char *buf, size_t count)
{
	(void)info, (void)buf;

	return (ssize_t)count;
}

void hisifb_display_effect_func_switch(struct hisi_fb_data_type *hisifd, const char *command)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}
	if (NULL == command) {
		HISI_FB_ERR("[effect] command is NULL\n");
		return;
	}

	if (!strncmp("hiace:", command, strlen("hiace:"))) {
		if('0' == command[strlen("hiace:")]) {
			hisifd->panel_info.hiace_support = 0;
			HISI_FB_INFO("[effect] hiace disable\n");
		} else {
			hisifd->panel_info.hiace_support = 1;
			HISI_FB_INFO("[effect] hiace enable\n");
		}
	}
	if (!strncmp("effect_enable:", command, strlen("effect_enable:"))) {
		g_enable_effect = (int)simple_strtoul(&command[strlen("effect_enable:")], NULL, 0);
		HISI_FB_INFO("[effect] effect_enable changed to %d\n", g_enable_effect);
	}
	if (!strncmp("effect_debug:", command, strlen("effect_debug:"))) {
		g_debug_effect = (int)simple_strtoul(&command[strlen("effect_debug:")], NULL, 0);
		HISI_FB_INFO("[effect] effect_debug changed to %d\n", g_debug_effect);
	}
}

bool hisifb_display_effect_is_need_ace(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return false;
	}
	//HISI_FB_DEBUG("[effect] ctrl_ce_mode=%d, ctrl_sre_enable=%d, to_stop_hdr=%d, to_stop_sre=%d\n", hisifd->ce_ctrl.ctrl_ce_mode, hisifd->sre_ctrl.ctrl_sre_enable, hisifd->hiace_info.to_stop_hdr, hisifd->hiace_info.to_stop_sre);
	return (g_enable_effect & ENABLE_EFFECT_HIACE) && (hisifd->ce_ctrl.ctrl_ce_mode > 0 || hisifd->sre_ctrl.ctrl_sre_enable == 1 || hisifd->hiace_info.to_stop_hdr || hisifd->hiace_info.to_stop_sre);
}

bool hisifb_display_effect_is_need_blc(struct hisi_fb_data_type *hisifd)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return false;
	}

	return (g_enable_effect & ENABLE_EFFECT_BL && hisifd->bl_enable_ctrl.ctrl_bl_enable == 1 &&
		((hisifd->ce_ctrl.ctrl_ce_mode == CE_MODE_VIDEO && hisifd->bl_level > 0 && hisifd->al_ctrl.ctrl_al_value >= 0) ||
		hisifd->panel_info.hiace_param.iDoAPLC != 0)) || hisifd->de_info.blc_enable;
}

bool hisifb_display_effect_check_bl_value(int curr, int last) {
	return false;
}

bool hisifb_display_effect_check_bl_delta(int curr, int last) {
	return false;
}

bool hisifb_display_effect_fine_tune_backlight(struct hisi_fb_data_type *hisifd, int backlight_in, int *backlight_out)
{
	bool changed = false;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return false;
	}

	if (NULL == backlight_out) {
		HISI_FB_ERR("[effect] backlight_out is NULL\n");
		return false;
	}

	if (hisifd->bl_level > 0) {
		if (hisifb_display_effect_is_need_blc(hisifd)) {
			int delta = hisifd->de_info.blc_enable ? hisifd->de_info.blc_delta : hisifd->bl_ctrl.ctrl_bl_delta;
			int bl = MIN((int)hisifd->panel_info.bl_max, MAX((int)hisifd->panel_info.bl_min, backlight_in + delta));
			if (*backlight_out != bl) {
				HISI_FB_DEBUG("[effect] delta:%d bl:%d(%d)->%d\n", hisifd->bl_ctrl.ctrl_bl_delta, backlight_in, hisifd->bl_level, bl);
				*backlight_out = bl;
				g_hiace_service.blc_used = true;
				changed = true;
			}
		} else {
			if (g_hiace_service.blc_used) {
				// Ensure the backlight brightness is correct after close BLC.
				if (*backlight_out != backlight_in) {
					HISI_FB_DEBUG("[effect] bl:%d->%d\n", *backlight_out, backlight_in);
					*backlight_out = backlight_in;
					changed = true;
				}
				g_hiace_service.blc_used = false;
			}
		}
	}

	return changed;
}

int hisifb_display_effect_blc_cabc_update(struct hisi_fb_data_type *hisifd)
{
	if (hisifd == NULL) {
		HISI_FB_ERR("hisifd is NULL\n");
		return -1;
	}

	if (hisifd->pdev == NULL) {
		HISI_FB_ERR("hisifd->pdev is NULL\n");
		return -1;
	}

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		static int bl_level = 0;
		struct hisi_fb_panel_data *pdata = NULL;

		pdata = dev_get_platdata(&hisifd->pdev->dev);
		if (pdata == NULL) {
			HISI_FB_ERR("pdata is NULL\n");
			return -1;
		}

		down(&hisifd->blank_sem);
		if (!hisifd->panel_power_on) {
			up(&hisifd->blank_sem);
			return 0;
		}

		if (hisifb_display_effect_fine_tune_backlight(hisifd, (int)hisifd->bl_level, &bl_level)) {
			if (pdata->set_blc_brightness) {
				hisifb_set_vsync_activate_state(hisifd, true);
				hisifb_activate_vsync(hisifd);
				pdata->set_blc_brightness(hisifd->pdev,(uint32_t)bl_level);
				hisifb_set_vsync_activate_state(hisifd, false);
				hisifb_deactivate_vsync(hisifd);
			}
		}

		up(&hisifd->blank_sem);
	} else if ((hisifd->panel_info.blpwm_input_ena || hisifb_display_effect_is_need_blc(hisifd)) && hisifd->cabc_update) {
		hisifd->cabc_update(hisifd);
	}

	return 0;
}

void init_hiace(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *hiace_base = NULL;

	unsigned long dw_jiffies = 0;
	uint32_t tmp = 0;
	bool is_ready = false;

	uint32_t global_hist_ab_work;
	uint32_t global_hist_ab_shadow;
	uint32_t gamma_ab_work;
	uint32_t gamma_ab_shadow;
	uint32_t width;
	uint32_t height;
	uint32_t half_block_w;
	uint32_t half_block_h;
	uint32_t lhist_sft;
	uint32_t slop;
	uint32_t th_max;
	uint32_t th_min;
	uint32_t up_thres;
	uint32_t low_thres;
	uint32_t fixbit_x;
	uint32_t fixbit_y;
	uint32_t reciprocal_x;
	uint32_t reciprocal_y;

	uint32_t block_pixel_num;
	uint32_t max_lhist_block_pixel_num;
	uint32_t max_lhist_bin_reg_num;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	pinfo = &(hisifd->panel_info);
	if (pinfo->hiace_support == 0) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] HIACE is not supported!\n");
		return;
	}

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hiace_base = hisifd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!", hisifd->index);
		return;
	}

	if(!check_underflow_clear_ack(hisifd)){
		HISI_FB_ERR("[effect] fb%d, clear_ack is not return!\n", hisifd->index);
		return;
	}

	set_reg(hiace_base + HIACE_BYPASS_ACE, 0x1, 1, 0);
	set_reg(hiace_base + HIACE_INIT_GAMMA, 0x1, 1, 0);
	set_reg(hiace_base + HIACE_UPDATE_LOCAL, 0x1, 1, 0);

	//parameters
	width = hisifd->panel_info.xres & 0x1fff;
	height = hisifd->panel_info.yres & 0x1fff;
	set_reg(hiace_base + HIACE_IMAGE_INFO, (height << 16) | width, 32, 0);

	half_block_w = (width / 12) & 0x1ff;
	half_block_h = ((height + 11) / 12) & 0x1ff;
	set_reg(hiace_base + HIACE_HALF_BLOCK_H_W,
		(half_block_h << 16) | half_block_w, 32, 0);

	block_pixel_num = (half_block_w * half_block_h) << 2;
	max_lhist_block_pixel_num = block_pixel_num << 2;
	max_lhist_bin_reg_num = (1 << 16) - 1; // each local hist bin 20bit -> 16bit
	if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 1)) {
		lhist_sft = 0;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 2)) {
		lhist_sft = 1;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 3)) {
		lhist_sft = 2;
	} else if (max_lhist_block_pixel_num < (max_lhist_bin_reg_num << 4)) {
		lhist_sft = 3;
	} else {
		lhist_sft = 4;
	}
	set_reg(hiace_base + HIACE_LHIST_SFT, lhist_sft, 3, 0);
	pinfo->hiace_param.ilhist_sft = (int)lhist_sft;

	slop = 68 & 0xff;
	th_min = 0 & 0x1ff;
	th_max = 30 & 0x1ff;
	set_reg(hiace_base + HIACE_HUE, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	slop = 68 & 0xff;
	th_min = 80 & 0xff;
	th_max = 140 & 0xff;
	set_reg(hiace_base + HIACE_SATURATION, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	slop = 68 & 0xff;
	th_min = 100 & 0xff;
	th_max = 255 & 0xff;
	set_reg(hiace_base + HIACE_VALUE, (slop << 24) | (th_max << 12) | th_min, 32, 0);

	set_reg(hiace_base + HIACE_SKIN_GAIN, 128, 8, 0);

	up_thres = 248 & 0xff;
	low_thres = 8 & 0xff;
	set_reg(hiace_base + HIACE_UP_LOW_TH, (up_thres << 8) | low_thres, 32, 0);

	fixbit_x = get_fixed_point_offset(half_block_w) & 0x1f;
	fixbit_y = get_fixed_point_offset(half_block_h) & 0x1f;
	reciprocal_x = (1U << (fixbit_x + 8)) / (2 * MAX(half_block_w, 1)) & 0x3ff;
	reciprocal_y = (1U << (fixbit_y + 8)) / (2 * MAX(half_block_h, 1)) & 0x3ff;
	set_reg(hiace_base + HIACE_XYWEIGHT, (fixbit_y << 26) | (reciprocal_y << 16)
		| (fixbit_x << 10) | reciprocal_x, 32, 0);

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] half_block_w:%d, half_block_h:%d, fixbit_x:%d, fixbit_y:%d, reciprocal_x:%d, reciprocal_y:%d, lhist_sft:%d\n",
					 half_block_w, half_block_h, fixbit_x, fixbit_y, reciprocal_x, reciprocal_y, lhist_sft);

	//wait for gamma init finishing
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(hiace_base + HIACE_INIT_GAMMA);
		if ((tmp & 0x1) != 0x1) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies)); //lint !e550

	if (!is_ready) {
		HISI_FB_INFO("[effect] fb%d, HIACE_INIT_GAMMA is not ready! HIACE_INIT_GAMMA=0x%08X.\n",
					 hisifd->index, tmp);
	}

	global_hist_ab_work = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_WORK);
	global_hist_ab_shadow = !global_hist_ab_work;

	gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);
	gamma_ab_shadow = !gamma_ab_work;

	set_reg(hiace_base + HIACE_GLOBAL_HIST_AB_SHADOW, global_hist_ab_shadow, 1, 0);
	set_reg(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow, 1, 0);

}

void hisi_dss_dpp_hiace_set_reg(struct hisi_fb_data_type *hisifd)
{
	char __iomem *hiace_base = NULL;
	dss_ce_info_t *ce_info = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	if (NULL == hisifd->dss_base) {
		HISI_FB_ERR("[effect] dss_base is NULL\n");
		return;
	}

	if (hisifd->panel_info.hiace_support == 0) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] HIACE is not support!\n");
		return;
	}

	if (PRIMARY_PANEL_IDX == hisifd->index) {
		hiace_base = hisifd->dss_base + DSS_HI_ACE_OFFSET;
		ce_info = &(hisifd->hiace_info);
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!\n", hisifd->index);
		return;
	}

	if (!hisifb_display_effect_is_need_ace(hisifd)) {
		g_hiace_service.use_last_value = false;
		return;
	}

	if(!check_underflow_clear_ack(hisifd)){
		HISI_FB_ERR("[effect] fb%d, clear_ack is not return!\n", hisifd->index);
		return;
	}

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	//lint -e{438}
	if (g_hiace_service.use_last_value) {
		int gamma_ab_shadow = inp32(hiace_base + HIACE_GAMMA_AB_SHADOW);
		int gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);

		set_reg(hiace_base + HIACE_VALUE, (uint32_t)ce_info->thminv, 12, 0);

		if (gamma_ab_shadow == gamma_ab_work) {
			int i = 0;

			//write gamma lut
			set_reg(hiace_base + HIACE_GAMMA_EN, 1, 1, 31);

			mutex_lock(&ce_info->lut_lock);
			for (i = 0; i < (6 * 6 * 11); i++) {
				// cppcheck-suppress *
				outp32(hiace_base + HIACE_GAMMA_VxHy_3z2_3z1_3z_W, hisifd->hiace_info.lut_table[i]);
			}
			mutex_unlock(&ce_info->lut_lock);

			set_reg(hiace_base + HIACE_GAMMA_EN, 0, 1, 31);

			gamma_ab_shadow ^= 1;
			outp32(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow);
		}

		g_hiace_service.use_last_value = false;
	}
}

void hisi_dpp_hiace_end_handle_func(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *hiace_base = NULL;
	uint32_t * global_hist_ptr = NULL;
	uint32_t * local_hist_ptr = NULL;
	dss_ce_info_t *ce_info = NULL;

	int i = 0;
	int global_hist_ab_shadow = 0;
	int global_hist_ab_work = 0;
	int local_valid = 0;

	time_interval_t interval_total = {0};
	time_interval_t interval_hist_global = {0};
	time_interval_t interval_hist_local = {0};
	DEFINE_DELAY_RECORD(delay_total, "interrupt handling");
	DEFINE_DELAY_RECORD(delay_hist_global, "global hist reading");
	DEFINE_DELAY_RECORD(delay_hist_local, "local hist reading");

	hisifd = container_of(work, struct hisi_fb_data_type, hiace_end_work);
	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] hisifd is NULL\n");
		return;
	}

	if (hisifd->panel_info.hiace_support == 0) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] HIACE is not support!\n");
		return;
	}

	if (PRIMARY_PANEL_IDX == hisifd->index) {
		ce_info = &(hisifd->hiace_info);
		hiace_base = hisifd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		HISI_FB_ERR("[effect] fb%d, not support!\n", hisifd->index);
		return;
	}

	if (!hisifb_display_effect_is_need_ace(hisifd)) {
		enable_hiace(hisifd, false);
		return;
	}

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] panel power off!\n");
		goto ERR_OUT;
	}

	if (g_hiace_service.is_ready) {
		g_hiace_service.is_ready = false;
	} else {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] service is not ready!\n");
		goto ERR_OUT;
	}

	EFFECT_DEBUG_LOG(DEBUG_EFFECT_FRAME, "[effect] step in\n");

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_total.start = get_timestamp_in_us();
	}

	hisifb_activate_vsync(hisifd);

	down(&hisifd->hiace_clear_sem);

	mutex_lock(&ce_info->hist_lock);

	if(!check_underflow_clear_ack(hisifd)){
		HISI_FB_ERR("[effect] fb%d, clear_ack is not return!\n", hisifd->index);

		mutex_unlock(&ce_info->hist_lock);

		up(&hisifd->hiace_clear_sem);

		hisifb_deactivate_vsync(hisifd);

		goto ERR_OUT;
	}

	local_valid = inp32(hiace_base + HIACE_LOCAL_VALID);
	if (local_valid == 1) {
		//read local hist
		local_hist_ptr = &hisifd->hiace_info.histogram[HIACE_GHIST_RANK];
		outp32(hiace_base + HIACE_LHIST_EN, (1 << 31));

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_local.start = get_timestamp_in_us();
		}
		for (i = 0; i < (6 * 6 * 16); i++) {//H  L
			local_hist_ptr[i] = inp32(hiace_base + HIACE_LOCAL_HIST_VxHy_2z_2z1);
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_local.stop = get_timestamp_in_us();
			count_delay(&delay_hist_local, interval_hist_local.stop - interval_hist_local.start);
		}

		outp32(hiace_base + HIACE_LHIST_EN, (0 << 31));
		outp32(hiace_base + HIACE_UPDATE_LOCAL, 1);
	}

	global_hist_ab_shadow = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_SHADOW);
	global_hist_ab_work = inp32(hiace_base + HIACE_GLOBAL_HIST_AB_WORK);
	if (global_hist_ab_shadow == global_hist_ab_work) {
		//read global hist
		global_hist_ptr = &hisifd->hiace_info.histogram[0];//HIACE_GHIST_RANK

		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_global.start = get_timestamp_in_us();
		}
		for (i = 0; i < 32; i++) {
			global_hist_ptr[i] = inp32(hiace_base + HIACE_GLOBAL_HIST_LUT_ADDR + i * 4);
		}
		if (g_debug_effect & DEBUG_EFFECT_DELAY) {
			interval_hist_global.stop = get_timestamp_in_us();
			count_delay(&delay_hist_global, interval_hist_global.stop - interval_hist_global.start);
		}

		outp32(hiace_base +  HIACE_GLOBAL_HIST_AB_SHADOW, global_hist_ab_shadow ^ 1);
	}

	mutex_unlock(&ce_info->hist_lock);

	up(&hisifd->hiace_clear_sem);

	hisifb_deactivate_vsync(hisifd);

	up(&hisifd->blank_sem);

	if ((local_valid == 1) || (global_hist_ab_shadow == global_hist_ab_work)) { // global or local hist is updated
		ce_info->algorithm_result = hisifb_ce_do_contrast(hisifd);
	}
	g_hiace_service.is_ready = true;

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		EFFECT_DEBUG_LOG(DEBUG_EFFECT_ENTRY, "[effect] panel power off!\n");
		goto ERR_OUT;
	}

	hisifb_activate_vsync(hisifd);

	down(&hisifd->hiace_clear_sem);

	//lint -e{438}
	if (ce_info->algorithm_result == 0) {
		int gamma_ab_shadow = inp32(hiace_base + HIACE_GAMMA_AB_SHADOW);
		int gamma_ab_work = inp32(hiace_base + HIACE_GAMMA_AB_WORK);
		time_interval_t interval_lut = {0};
		DEFINE_DELAY_RECORD(delay_lut, "lut writing");

		set_reg(hiace_base + HIACE_VALUE, (uint32_t)ce_info->thminv, 12, 0);

		if (gamma_ab_shadow == gamma_ab_work) {
			//write gamma lut
			set_reg(hiace_base + HIACE_GAMMA_EN, 1, 1, 31);

			if (g_debug_effect & DEBUG_EFFECT_DELAY) {
				interval_lut.start = get_timestamp_in_us();
			}
			mutex_lock(&ce_info->lut_lock);
			for (i = 0; i < (6 * 6 * 11); i++) {
				// cppcheck-suppress *
				outp32(hiace_base + HIACE_GAMMA_VxHy_3z2_3z1_3z_W, hisifd->hiace_info.lut_table[i]);
			}
			mutex_unlock(&ce_info->lut_lock);
			if (g_debug_effect & DEBUG_EFFECT_DELAY) {
				interval_lut.stop = get_timestamp_in_us();
				count_delay(&delay_lut, interval_lut.stop - interval_lut.start);
			}

			set_reg(hiace_base + HIACE_GAMMA_EN, 0, 1, 31);

			gamma_ab_shadow ^= 1;
			outp32(hiace_base + HIACE_GAMMA_AB_SHADOW, gamma_ab_shadow);
		}

		ce_info->algorithm_result = 1;
	}


	up(&hisifd->hiace_clear_sem);

	hisifb_deactivate_vsync(hisifd);

	if (ce_info->gradual_frames > 0) {
		ce_info->gradual_frames--;
	}

	if (g_debug_effect & DEBUG_EFFECT_DELAY) {
		interval_total.stop = get_timestamp_in_us();
		count_delay(&delay_total, interval_total.stop - interval_total.start);
	}

ERR_OUT:
	up(&hisifd->blank_sem);
} //lint !e550

////////////////////////////////////////////////////////////////////////////////
//ACE
static u32 ace_orginal_lut[] = {
	0x50002,	0x90007,	0xe000c,	0x130011,	0x180016,	0x1e001b,	0x230020,	0x290026,
	0x2f002c,	0x350032,	0x3b0038,	0x42003e,	0x480045,	0x4f004c,	0x560053,	0x5e005a,
	0x650061,	0x6d0069,	0x750071,	0x7d0079,	0x850081,	0x8e0089,	0x960092,	0x9f009a,
	0xa700a3,	0xb000ac,	0xb900b5,	0xc200bd,	0xcb00c6,	0xd400cf,	0xdd00d9,	0xe600e2,
	0xef00eb,	0xf900f4,	0x10200fd,	0x10b0107,	0x1150110,	0x11e0119,	0x1280123,	0x131012d,
	0x13b0136,	0x1450140,	0x14e0149,	0x1580153,	0x162015d,	0x16c0167,	0x1760171,	0x180017b,
	0x18a0185,	0x194018f,	0x19e0199,	0x1a801a3,	0x1b201ad,	0x1bc01b7,	0x1c601c1,	0x1d001cb,
	0x1da01d5,	0x1e401df,	0x1ee01e9,	0x1f801f3,	0x20101fd,	0x20b0206,	0x2150210,	0x21f021a,
	0x2290224,	0x232022d,	0x23c0237,	0x2450241,	0x24f024a,	0x2580253,	0x261025d,	0x26a0266,
	0x273026f,	0x27c0278,	0x2850281,	0x28e028a,	0x2970293,	0x2a0029b,	0x2a802a4,	0x2b102ad,
	0x2ba02b6,	0x2c302be,	0x2cc02c7,	0x2d502d0,	0x2de02d9,	0x2e702e2,	0x2ef02eb,	0x2f802f4,
	0x30102fd,	0x30a0306,	0x313030e,	0x31b0317,	0x3240320,	0x32c0328,	0x3350331,	0x33d0339,
	0x3450341,	0x34d0349,	0x3540350,	0x35c0358,	0x3630360,	0x36b0367,	0x372036f,	0x37a0376,
	0x381037d,	0x3880384,	0x38f038c,	0x3960393,	0x39e039a,	0x3a503a1,	0x3ab03a8,	0x3b203af,
	0x3b903b6,	0x3bf03bc,	0x3c603c3,	0x3cc03c9,	0x3d203cf,	0x3d703d4,	0x3dc03da,	0x3e203df,
	0x3e603e4,	0x3eb03e8,	0x3ef03ed,	0x3f203f1,	0x3f603f4,	0x3f903f7,	0x3fc03fa,	0x3fe03fd,
};

void init_acm_ce(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *ace_base = NULL;

	if (hisifd == NULL) {
		HISI_FB_ERR("[effect] init_ace hisifd is NULL!\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ace_base = hisifd->dss_base + DSS_DPP_ACE_OFFSET;
	} else {
		HISI_FB_INFO("[effect] fb%d, not support!", hisifd->index);
		return;
	}

	if (!pinfo->acm_ce_support) {
		// ace memory shutdown
		outp32(ace_base + ACE_MEM_CTRL, 0x44);
		return;
	}

	set_reg(ace_base + ACE_SKIN_CFG, 0x3, 2, 0);

	set_reg(ace_base + ACE_R2Y, 0x36, 8, 0);
	set_reg(ace_base + ACE_G2Y, 0xB8, 8, 0);
	set_reg(ace_base + ACE_B2Y, 0x12, 8, 0);

	set_reg(ace_base + ACE_Y_OFFSET, 0x0, 8, 0);

	set_reg(ace_base + ACE_Y_CEN, 0x91, 8, 0);
	set_reg(ace_base + ACE_U_CEN, 0x64, 8, 0);
	set_reg(ace_base + ACE_V_CEN, 0xAA, 8, 0);

	set_reg(ace_base + ACE_Y_EXT, 0x64, 8, 0);
	set_reg(ace_base + ACE_U_EXT, 0x0C, 7, 0);
	set_reg(ace_base + ACE_V_EXT, 0x0F, 7, 0);

	set_reg(ace_base + ACE_Y_ATTENU, 0x29, 12, 0);
	set_reg(ace_base + ACE_U_ATTENU, 0x155, 12, 0);
	set_reg(ace_base + ACE_V_ATTENU, 0x111, 12, 0);

	set_reg(ace_base + ACE_ROTA, 0x3FF, 11, 0);
	set_reg(ace_base + ACE_ROTB, 0x0, 11, 0);

	set_reg(ace_base + ACE_Y_CORE, 0x1E, 8, 0);
	set_reg(ace_base + ACE_U_CORE, 0x6, 7, 0);
	set_reg(ace_base + ACE_V_CORE, 0xA, 7, 0);

	set_reg(ace_base + ACE_LUT_SEL, 0x1, 2, 0);
	set_reg(ace_base + ACE_EN, 0x3, 2, 0);
}

void hisi_dss_dpp_ace_set_reg(struct hisi_fb_data_type *hisifd)
{
	struct hisi_panel_info *pinfo = NULL;
	dss_ce_info_t *ace_info = NULL;
	char __iomem *ace_base = NULL;
	char __iomem *ace_lut_base = NULL;
	uint32_t i = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] NULL Pointer\n");
		return;
	}

	pinfo = &(hisifd->panel_info);
	ace_info = &(hisifd->acm_ce_info);
	ace_base = hisifd->dss_base + DSS_DPP_ACE_OFFSET;
	ace_lut_base = hisifd->dss_base + DSS_DPP_ACE_LUT_OFFSET;

	if (pinfo->acm_ce_support == 0)
		return;

	if (0 == (0x1 & inp32(ace_base + ACE_LUT_SEL))) { //lint !e587
		ace_info->lut_base = ace_lut_base + ACE_LUT0;
		for (i = 0; i < 128; i++) {
			//output lut0
			outp32(ace_info->lut_base + i * 4, ace_orginal_lut[i]);
		}
		ace_info->lut_sel = 1;
	} else {
		ace_info->lut_base = ace_lut_base + ACE_LUT1;
		for (i = 0; i < 128; i++) {
			//output lut1
			outp32(ace_info->lut_base + i * 4, ace_orginal_lut[i]);
		}
		ace_info->lut_sel = 0;
	}

	hisifd->set_reg(hisifd, ace_base + ACE_LUT_SEL, ace_info->lut_sel, 32, 0);
}

void hisi_dpp_ace_end_handle_func(struct work_struct *work)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_ce_info_t *ace_info = NULL;

	char __iomem *ace_base = NULL;
	char __iomem *ace_lut_base = NULL;
	char __iomem *ace_hist_prt_base = NULL;
	int i = 0;

	hisifd = container_of(work, struct hisi_fb_data_type, dpp_ce_end_work);

	if (NULL == hisifd) {
		HISI_FB_ERR("[effect] NULL Pointer\n");
		return;
	}

	pinfo = &(hisifd->panel_info);
	ace_info = &(hisifd->acm_ce_info);

	ace_base = hisifd->dss_base + DSS_DPP_ACE_OFFSET;
	ace_lut_base = hisifd->dss_base + DSS_DPP_ACE_LUT_OFFSET;

	if (pinfo->acm_ce_support == 0)
		return;

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		up(&hisifd->blank_sem);
		return;
	}

	hisifb_activate_vsync(hisifd);
	if (0 == (0x1 & inp32(ace_base + ACE_HIST_IND))) { //lint !e587
		ace_hist_prt_base = ace_lut_base + ACE_HIST0;
		for (i = 0; i < 256; i++) {
			ace_info->histogram[i] = inp32(ace_hist_prt_base + i * 4);
		}
	} else {
		ace_hist_prt_base = ace_lut_base + ACE_HIST1;
		for (i = 0; i < 256; i++) {
			ace_info->histogram[i] = inp32(ace_hist_prt_base + i * 4);
		}
	}
	hisifb_deactivate_vsync(hisifd);

	up(&hisifd->blank_sem);
}

/*******************************************************************************
** GM IGM
*/
#define GM_LUT_LEN 257
#define GM_LUT_MHLEN 254
static uint16_t degm_gm_lut[GM_LUT_LEN *6];

int hisifb_use_dynamic_gamma(struct hisi_fb_data_type *hisifd, char __iomem *dpp_base)
{
	uint32_t i = 0;
	uint32_t index = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (hisifd == NULL) {
		return -1;
	}

	if (dpp_base == NULL) {
		return -1;
	}

	pinfo = &(hisifd->panel_info);

	if(hisifd->dynamic_gamma_info.use_dynamic_gm_init == 1) {
		uint16_t* gm_lut_r =  degm_gm_lut;
		uint16_t* gm_lut_g =  gm_lut_r + GM_LUT_LEN;
		uint16_t* gm_lut_b =  gm_lut_g + GM_LUT_LEN;

		for (i = 0; i < pinfo->gamma_lut_table_len / 2; i++) {
			index = i << 1;
			if (index >= GM_LUT_MHLEN)
			{
				index = GM_LUT_MHLEN;
			}
			outp32(dpp_base + (U_GAMA_R_COEF + i * 4), gm_lut_r[index] | gm_lut_r[index+1] << 16);
			outp32(dpp_base + (U_GAMA_G_COEF + i * 4), gm_lut_g[index] | gm_lut_g[index+1] << 16);
			outp32(dpp_base + (U_GAMA_B_COEF + i * 4), gm_lut_b[index] | gm_lut_b[index+1] << 16);
		}
		if (pinfo->gamma_lut_table_len <= GM_LUT_LEN) {
			outp32(dpp_base + U_GAMA_R_LAST_COEF, gm_lut_r[pinfo->gamma_lut_table_len - 1]);
			outp32(dpp_base + U_GAMA_G_LAST_COEF, gm_lut_g[pinfo->gamma_lut_table_len - 1]);
			outp32(dpp_base + U_GAMA_B_LAST_COEF, gm_lut_b[pinfo->gamma_lut_table_len - 1]);
		}

		return 1;
	}

	return 0;

}

int hisifb_use_dynamic_degamma(struct hisi_fb_data_type *hisifd, char __iomem *dpp_base)
{
	uint32_t i = 0;
	uint32_t index = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (hisifd == NULL) {
		return -1;
	}

	if (dpp_base == NULL) {
		return -1;
	}

	pinfo = &(hisifd->panel_info);

	if(hisifd->dynamic_gamma_info.use_dynamic_gm_init == 1) {
		uint16_t* degm_lut_r = degm_gm_lut + GM_LUT_LEN * 3;
		uint16_t* degm_lut_g = degm_lut_r + GM_LUT_LEN;
		uint16_t* degm_lut_b = degm_lut_g + GM_LUT_LEN;

		for (i = 0; i < pinfo->igm_lut_table_len / 2; i++) {
			index = i << 1;
			if(index >= GM_LUT_MHLEN)
			{
				index = GM_LUT_MHLEN;
			}
			outp32(dpp_base + (LCP_U_DEGAMA_R_COEF +  i * 4), degm_lut_r[index] | degm_lut_r[index+1] << 16);
			outp32(dpp_base + (LCP_U_DEGAMA_G_COEF +  i * 4), degm_lut_g[index] | degm_lut_g[index+1] << 16);
			outp32(dpp_base + (LCP_U_DEGAMA_B_COEF +  i * 4), degm_lut_b[index] | degm_lut_b[index+1] << 16);
		}
		if (pinfo->igm_lut_table_len <= GM_LUT_LEN) {
			outp32(dpp_base + LCP_U_DEGAMA_R_LAST_COEF, degm_lut_r[pinfo->igm_lut_table_len - 1]);
			outp32(dpp_base + LCP_U_DEGAMA_G_LAST_COEF, degm_lut_g[pinfo->igm_lut_table_len - 1]);
			outp32(dpp_base + LCP_U_DEGAMA_B_LAST_COEF, degm_lut_b[pinfo->igm_lut_table_len - 1]);
		}

		return 1;
	}

	return 0;

}

void hisifb_update_dynamic_gamma(struct hisi_fb_data_type *hisifd, const char* buffer, size_t len)
{
	struct hisi_panel_info *pinfo = NULL;
	if (hisifd == NULL) {
		return;
	}

	pinfo = &(hisifd->panel_info);
	if (pinfo == NULL) {
		return;
	}

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_GAMA)){
		return;
	}

	hisifd->dynamic_gamma_info.use_dynamic_gm = 0;
	hisifd->dynamic_gamma_info.use_dynamic_gm_init = 0;

	if (pinfo->gamma_support == 1) {
		if ((len > 0) && (len <= (int)sizeof(degm_gm_lut))) {
			memcpy((char*)degm_gm_lut, buffer, (size_t)len);
			hisifd->dynamic_gamma_info.use_dynamic_gm = 1;
			hisifd->dynamic_gamma_info.use_dynamic_gm_init = 1;
		}
	}

}

void hisifb_update_gm_from_reserved_mem(uint32_t *gm_r, uint32_t *gm_g, uint32_t *gm_b,
	uint32_t *igm_r, uint32_t *igm_g, uint32_t *igm_b)
{
	int i = 0;
	int len = 0;
	uint16_t *u16_gm_r = NULL;
	uint16_t *u16_gm_g = NULL;
	uint16_t *u16_gm_b = NULL;
	uint16_t *u16_igm_r = NULL;
	uint16_t *u16_igm_g = NULL;
	uint16_t *u16_igm_b = NULL;
	void *mem = NULL;
	unsigned long gm_addr = 0;
	unsigned long gm_size = 0;

	if (gm_r == NULL || gm_g == NULL || gm_b == NULL
		|| igm_r == NULL || igm_g == NULL || igm_b == NULL) {
		return;
	}

	gm_addr = HISI_SUB_RESERVED_LCD_GAMMA_MEM_PHYMEM_BASE;
	gm_size = HISI_SUB_RESERVED_LCD_GAMMA_MEM_PHYMEM_SIZE;

	HISI_FB_INFO("gamma kernel gm_addr = 0x%lx gm_size = 0x%lx \n", gm_addr, gm_size);

	mem = (void *)ioremap_wc(gm_addr, gm_size);
	if (mem == NULL) {
		HISI_FB_ERR("mem ioremap error ! \n");
		return;
	}
	memcpy(&len, mem, 4UL);
	HISI_FB_INFO("gamma read len = %d \n", len);
	if (len != GM_IGM_LEN) {
		HISI_FB_INFO("gamma read len error ! \n");
		iounmap(mem);
		return;
	}

	u16_gm_r = (uint16_t *)(mem + 4);
	u16_gm_g = u16_gm_r + GM_LUT_LEN;
	u16_gm_b = u16_gm_g + GM_LUT_LEN;

	u16_igm_r =  u16_gm_b + GM_LUT_LEN;
	u16_igm_g = u16_igm_r + GM_LUT_LEN;
	u16_igm_b = u16_igm_g + GM_LUT_LEN;

	for (i = 0; i < GM_LUT_LEN; i++) {
		gm_r[i] = u16_gm_r[i];
		gm_g[i] = u16_gm_g[i];
		gm_b[i] = u16_gm_b[i];

		igm_r[i]  = u16_igm_r[i];
		igm_g[i] = u16_igm_g[i];
		igm_b[i] = u16_igm_b[i];
	}
	iounmap(mem);
	return;
}

/*lint -e571, -e573, -e737, -e732, -e850, -e730, -e713, -e529, -e574, -e679, -e732, -e845, -e570, -e774*/

#define ACM_HUE_LUT_LENGTH ((uint32_t)256)
#define ACM_SATA_LUT_LENGTH ((uint32_t)256)
#define ACM_SATR_LUT_LENGTH ((uint32_t)64)

#define LCP_GMP_LUT_LENGTH	((uint32_t)9*9*9)
#define LCP_XCC_LUT_LENGTH	((uint32_t)12)

#define IGM_LUT_LEN ((uint32_t)257)
#define GAMMA_LUT_LEN	((uint32_t)257)

#define BYTES_PER_TABLE_ELEMENT 4

static int hisi_effect_copy_to_user(uint32_t *table_dst, uint32_t *table_src, uint32_t table_length)
{
	unsigned long table_size = 0;

	if ((NULL == table_dst) || (NULL == table_src) || (table_length == 0)) {
		HISI_FB_ERR("invalid input parameters.\n");
		return -EINVAL;
	}

	table_size = (unsigned long)table_length * BYTES_PER_TABLE_ELEMENT;

	if (copy_to_user(table_dst, table_src, table_size)) {
		HISI_FB_ERR("failed to copy table to user.\n");
		return -EINVAL;
	}

	return 0;
}

static int hisi_effect_alloc_and_copy(uint32_t **table_dst, uint32_t *table_src,
	uint32_t lut_table_length, bool copy_user)
{
	uint32_t *table_new = NULL;
	unsigned long table_size = 0;

	if ((NULL == table_dst) ||(NULL == table_src) ||  (lut_table_length == 0)) {
		HISI_FB_ERR("invalid input parameter");
		return -EINVAL;
	}

	table_size = (unsigned long)lut_table_length * BYTES_PER_TABLE_ELEMENT;

	if (*table_dst == NULL) {
		table_new = (uint32_t *)kmalloc(table_size, GFP_ATOMIC);
		if (table_new) {
			memset(table_new, 0, table_size);
			*table_dst = table_new;
		} else {
			HISI_FB_ERR("failed to kmalloc lut_table!\n");
			return -EINVAL;
		}
	}

	if (copy_user) {
		if (copy_from_user(*table_dst, table_src, table_size)) {
			HISI_FB_ERR("failed to copy table from user\n");
			if (table_new)
				kfree(table_new);
				*table_dst = NULL;
				return -EINVAL;
			}
	} else {
		memcpy(*table_dst, table_src, table_size);
	}

	return 0;
}

static void hisi_effect_kfree(uint32_t **free_table)
{
	if (*free_table) {
		kfree((uint32_t *) *free_table);
		*free_table = NULL;
	}
}

static void free_acm_table(struct acm_info *acm)
{
	if(acm == NULL){
		HISI_FB_ERR("acm is NULL!\n");
		return;
	}

	hisi_effect_kfree(&acm->hue_table);
	hisi_effect_kfree(&acm->sata_table);
	hisi_effect_kfree(&acm->satr0_table);
	hisi_effect_kfree(&acm->satr1_table);
	hisi_effect_kfree(&acm->satr2_table);
	hisi_effect_kfree(&acm->satr3_table);
	hisi_effect_kfree(&acm->satr4_table);
	hisi_effect_kfree(&acm->satr5_table);
	hisi_effect_kfree(&acm->satr6_table);
	hisi_effect_kfree(&acm->satr7_table);
}

static void free_lcp_table(struct lcp_info *lcp)
{
	if(lcp == NULL){
		HISI_FB_ERR("lcp is NULL!\n");
		return;
	}

	hisi_effect_kfree(&lcp->gmp_table_low32);
	hisi_effect_kfree(&lcp->gmp_table_high4);
	hisi_effect_kfree(&lcp->igm_r_table);
	hisi_effect_kfree(&lcp->igm_g_table);
	hisi_effect_kfree(&lcp->igm_b_table);
	hisi_effect_kfree(&lcp->xcc_table);
}

static void free_gamma_table(struct gamma_info *gamma)
{
	if(gamma == NULL){
		HISI_FB_ERR("gamma is NULL!\n");
		return;
	}

	hisi_effect_kfree(&gamma->gamma_r_table);
	hisi_effect_kfree(&gamma->gamma_g_table);
	hisi_effect_kfree(&gamma->gamma_b_table);
}

int hisi_effect_arsr2p_info_get(struct hisi_fb_data_type *hisifd, struct arsr2p_info *arsr2p)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == arsr2p) {
		HISI_FB_ERR("fb%d, arsr2p is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.arsr2p_sharp_support) {
		HISI_FB_INFO("fb%d, arsr2p is not supported!\n", hisifd->index);
		return 0;
	}

	memcpy(&arsr2p[0], &(hisifd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect), sizeof(struct arsr2p_info));
	memcpy(&arsr2p[1], &(hisifd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect_scale_up), sizeof(struct arsr2p_info));
	memcpy(&arsr2p[2], &(hisifd->dss_module_default.arsr2p[DSS_RCHN_V0].arsr2p_effect_scale_down), sizeof(struct arsr2p_info));
	arsr2p[0].sharp_enable = hisifd->panel_info.prefix_sharpness2D_support;
	arsr2p[0].skin_enable  = arsr2p[0].sharp_enable;
	arsr2p[0].shoot_enable = arsr2p[0].sharp_enable;
	arsr2p[1].sharp_enable = hisifd->panel_info.prefix_sharpness2D_support;
	arsr2p[1].skin_enable  = arsr2p[1].sharp_enable;
	arsr2p[1].shoot_enable = arsr2p[1].sharp_enable;
	arsr2p[2].sharp_enable = hisifd->panel_info.prefix_sharpness2D_support;
	arsr2p[2].skin_enable  = arsr2p[2].sharp_enable;
	arsr2p[2].shoot_enable = arsr2p[2].sharp_enable;

	return 0;
}

static void arsr1p_rog_init(struct arsr1p_info *arsr1p_rog)
{
	if (arsr1p_rog == NULL) {
		return;
	}
	arsr1p_rog->skin_thres_y = 512<<16 | 83<<8 | 75;	   /* 0x200534b */
	arsr1p_rog->skin_thres_u = 819<<16 | 10<<8 | 5; 	   /* 0x3330a05 */
	arsr1p_rog->skin_thres_v = 682<<16 | 12<<8 | 6; 	   /* 0x2aa0c06 */
	arsr1p_rog->skin_expected = 145<<16 | 113<<8 | 150;    /* 0x917196 */
	arsr1p_rog->skin_cfg = 3<<16 | 10<<8 | 6;			   /* 0x30a06 */
	arsr1p_rog->shoot_cfg1 = 2<<24 | 20;				   /* 0x2000014 */
	arsr1p_rog->shoot_cfg2 = 512<<18 | 16<<9 | 0;		   /* 0x8002000 */ /* which is differrent from 512<<18 | 0<<9 | (-16 & 0x1ff); */
	arsr1p_rog->sharp_cfg1 = 64<<24 | 48<<16 | 7<<8 | 3;   /* 0x40300703 */
	arsr1p_rog->sharp_cfg2 = 64<<24 | 48<<16 | 9<<8 | 5;   /* 0x40300905 */ /* which is differrent from 64<<24 | 48<<16 | 7<<8 | 3; */
	arsr1p_rog->sharp_cfg3 = 100<<16 | 100; 			   /* 0x640064 */ /* which is differrent from 40<<16 | 0; */
	arsr1p_rog->sharp_cfg4 = 140<<16 | 0;				   /* 0x8c0000 */ /* which is differrent from 0<<16 | 0; */
	arsr1p_rog->sharp_cfg5 = 140<<16 | 0;				   /* 0x8c0000 */ /* which is differrent from 80<<16 | 0; */
	arsr1p_rog->sharp_cfg6 = 36<<24 | 20<<16 | 10<<8 | 3;  /* 0x24140a03 */ /* which is differrent from 40<<24 | 24<<16 | 16<<8 | 6; */
	arsr1p_rog->sharp_cfg7 = 2<<24 | 50<<16 | 1<<9 | 0<<8 | 8; /* 0x2320208 */ /* which is differrent from 1<<24 | 80<<16 | 1<<9 | 0<<8 | 16; */
	arsr1p_rog->sharp_cfg8 = 3<<21 | 640;				   /* 0x600280 */ /* which is differrent from 4<<21 | 640; */
	arsr1p_rog->sharp_cfg9 = 1<<24 | 8960;				   /* 0x1002300 */ /* which is differrent from 1<<24 | 5120; */
	arsr1p_rog->sharp_cfg10 = 640;						   /* 0x280 */ /* which is differrent from 0; */
	arsr1p_rog->sharp_cfg11 = 8960; 					   /* 0x2300 */ /* which is differrent from 0; */
	arsr1p_rog->diff_ctrl = 20<<8 | 15; 				   /* 0x140f */
	arsr1p_rog->lsc_cfg1 = 15<<26 | 780<<13 | 1040; 	   /* 0x3c618410 */
	arsr1p_rog->lsc_cfg2 = 0;
	arsr1p_rog->lsc_cfg3 = 128<<16 | 1536;				   /* 0x800600 */
}
int hisi_effect_arsr1p_info_get(struct hisi_fb_data_type *hisifd, struct arsr1p_info *arsr1p)
{
	struct arsr1p_info *arsr1p_param = NULL;
	struct arsr1p_info *arsr1p_rog = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == arsr1p) {
		HISI_FB_ERR("fb%d, arsr1p is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.arsr1p_sharp_support) {
		HISI_FB_INFO("fb%d, arsr1p sharp is not supported!\n", hisifd->index);
		return 0;
	}

	arsr1p_param = &(hisifd->effect_info.arsr1p[0]);

	arsr1p_param->sharp_enable = hisifd->panel_info.arsr1p_sharpness_support;
	arsr1p_param->skin_enable = arsr1p_param->sharp_enable;
	arsr1p_param->shoot_enable = arsr1p_param->skin_enable;

	arsr1p_param->skin_thres_y = 512<<16 | 83<<8 | 75;
	arsr1p_param->skin_thres_u = 819<<16 | 10<<8 | 5;
	arsr1p_param->skin_thres_v = 682<<16 | 12<<8 | 6;
	arsr1p_param->skin_expected = 145<<16 | 113<<8 | 150;
	arsr1p_param->skin_cfg = 3<<16 | 10<<8 | 6;

	arsr1p_param->shoot_cfg1 = 2<<24 | 20;
	arsr1p_param->shoot_cfg2 = 512<<18 | 0<<9 | (-16 & 0x1ff);

	arsr1p_param->sharp_cfg1 = 64<<24 | 48<<16 | 7<<8 | 3;
	arsr1p_param->sharp_cfg2 = 64<<24 | 48<<16 | 7<<8 | 3;
	arsr1p_param->sharp_cfg3 = 40<<16 | 0;
	arsr1p_param->sharp_cfg4 = 0<<16 | 0;
	arsr1p_param->sharp_cfg5 = 80<<16 | 0;
	arsr1p_param->sharp_cfg6 = 40<<24 | 24<<16 | 16<<8 | 6;
	arsr1p_param->sharp_cfg7 = 1<<24 | 80<<16 | 1<<9 | 0<<8 | 16;
	arsr1p_param->sharp_cfg8 = 4<<21 | 640;
	arsr1p_param->sharp_cfg9 = 1<<24 | 5120;
	arsr1p_param->sharp_cfg10 = 0;
	arsr1p_param->sharp_cfg11 = 0;

	arsr1p_param->diff_ctrl = 20<<8 | 15;

	arsr1p_param->lsc_cfg1 = 15<<26 | 780<<13 | 1040;
	arsr1p_param->lsc_cfg2 = 0;
	arsr1p_param->lsc_cfg3 = 128<<16 | 1536;

	arsr1p_rog = &(hisifd->effect_info.arsr1p[1]);
	arsr1p_rog->sharp_enable = hisifd->panel_info.arsr1p_sharpness_support;
	arsr1p_rog->skin_enable = arsr1p_param->sharp_enable;
	arsr1p_rog->shoot_enable = arsr1p_param->skin_enable;
	if (!hisifd->effect_info.arsr1p_rog_initialized) {
		arsr1p_rog_init(arsr1p_rog);
		hisifd->effect_info.arsr1p_rog_initialized = true;
	}
	memcpy(&(arsr1p[0]), arsr1p_param, sizeof(struct arsr1p_info));
	memcpy(&(arsr1p[1]), arsr1p_rog  , sizeof(struct arsr1p_info));

	return 0;
}

int hisi_effect_acm_info_get(struct hisi_fb_data_type *hisifd, struct acm_info *acm_dst)
{
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == acm_dst) {
		HISI_FB_ERR("fb%d, acm is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.acm_support) {
		HISI_FB_INFO("fb%d, acm is not supported!\n", hisifd->index);
		return 0;
	}

	pinfo = &hisifd->panel_info;

	acm_set_hue(acm_dst, pinfo);
	acm_dst->enable = pinfo->acm_ce_support;

	if (hisi_effect_copy_to_user(acm_dst->hue_table, pinfo->acm_lut_hue_table, ACM_HUE_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm hue table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->sata_table, pinfo->acm_lut_sata_table, ACM_SATA_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm sata table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr0_table, pinfo->acm_lut_satr0_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr0 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr1_table, pinfo->acm_lut_satr1_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr1 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr2_table, pinfo->acm_lut_satr2_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr2 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr3_table, pinfo->acm_lut_satr3_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr3 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr4_table, pinfo->acm_lut_satr4_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr4 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr5_table, pinfo->acm_lut_satr5_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr5 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr6_table, pinfo->acm_lut_satr6_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr6 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	if (hisi_effect_copy_to_user(acm_dst->satr7_table, pinfo->acm_lut_satr7_table, ACM_SATR_LUT_LENGTH)) {
		HISI_FB_ERR("fb%d, failed to copy acm satr7 table to user!\n", hisifd->index);
		return -EINVAL;
	}

	return 0;
}
int hisi_effect_hiace_info_get(struct hisi_fb_data_type *hisifd, struct hiace_info *hiace) {
	(void)hisifd, (void)hiace;
	return 0;
}

int hisi_effect_lcp_info_get(struct hisi_fb_data_type *hisifd, struct lcp_info *lcp)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == lcp) {
		HISI_FB_ERR("fb%d, lcp is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);

	if (hisifd->effect_ctl.lcp_gmp_support) {
		ret = hisi_effect_copy_to_user(lcp->gmp_table_low32, pinfo->gmp_lut_table_low32bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy gmp_table_low32 to user!\n", hisifd->index);
			goto err_ret;
		}

		ret = hisi_effect_copy_to_user(lcp->gmp_table_high4, pinfo->gmp_lut_table_high4bit, LCP_GMP_LUT_LENGTH);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy gmp_table_high4 to user!\n", hisifd->index);
			goto err_ret;
		}
	}

	if (hisifd->effect_ctl.lcp_xcc_support) {
		ret = hisi_effect_copy_to_user(lcp->xcc_table, pinfo->xcc_table, LCP_XCC_LUT_LENGTH);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy xcc_table to user!\n", hisifd->index);
			goto err_ret;
		}
	}

	if (hisifd->effect_ctl.lcp_igm_support) {
		ret = hisi_effect_copy_to_user(lcp->igm_r_table, pinfo->igm_lut_table_R, IGM_LUT_LEN);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy igm_r_table to user!\n", hisifd->index);
			goto err_ret;
		}

		ret = hisi_effect_copy_to_user(lcp->igm_g_table, pinfo->igm_lut_table_G, IGM_LUT_LEN);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy igm_g_table to user!\n", hisifd->index);
			goto err_ret;
		}

		ret = hisi_effect_copy_to_user(lcp->igm_b_table, pinfo->igm_lut_table_B, IGM_LUT_LEN);
		if (ret) {
			HISI_FB_ERR("fb%d, failed to copy igm_b_table to user!\n", hisifd->index);
			goto err_ret;
		}
	}

err_ret:
	return ret;
}

int hisi_effect_gamma_info_get(struct hisi_fb_data_type *hisifd, struct gamma_info *gamma)
{
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == gamma) {
		HISI_FB_ERR("fb%d, gamma is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.gamma_support) {
		HISI_FB_INFO("fb%d, gamma is not supported!\n", hisifd->index);
		return 0;
	}

	pinfo = &(hisifd->panel_info);

	gamma->para_mode = 0;

	ret = hisi_effect_copy_to_user(gamma->gamma_r_table, pinfo->gamma_lut_table_R, GAMMA_LUT_LEN);
	if (ret) {
		HISI_FB_ERR("fb%d, failed to copy gamma_r_table to user!\n", hisifd->index);
	}

	ret = hisi_effect_copy_to_user(gamma->gamma_g_table, pinfo->gamma_lut_table_G, GAMMA_LUT_LEN);
	if (ret) {
		HISI_FB_ERR("fb%d, failed to copy gamma_g_table to user!\n", hisifd->index);
	}

	ret = hisi_effect_copy_to_user(gamma->gamma_b_table, pinfo->gamma_lut_table_B, GAMMA_LUT_LEN);
	if (ret) {
		HISI_FB_ERR("fb%d, failed to copy gamma_b_table to user!\n", hisifd->index);
	}

	return ret;
}

#define ARSR2P_MAX_NUM 3

int hisi_effect_arsr2p_info_set(struct hisi_fb_data_type *hisifd, struct arsr2p_info *arsr2p)
{
	uint32_t i;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == arsr2p) {
		HISI_FB_ERR("fb%d, arsr2p is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.arsr2p_sharp_support) {
		HISI_FB_INFO("fb%d, arsr2p sharp is not supported!\n", hisifd->index);
		return 0;
	}

	for (i = 0; i < ARSR2P_MAX_NUM; i++) {
		if (arsr2p[i].update == 1) {
			memcpy(&(hisifd->effect_info.arsr2p[i]), &(arsr2p[i]), sizeof(struct arsr2p_info));
		}
	}

	hisifd->effect_updated_flag.arsr2p_effect_updated = true;

	return 0;
}

int hisi_effect_arsr1p_info_set(struct hisi_fb_data_type *hisifd, struct arsr1p_info *arsr1p)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == arsr1p) {
		HISI_FB_ERR("fb%d, arsr1p is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	if (!hisifd->effect_ctl.arsr1p_sharp_support) {
		HISI_FB_INFO("fb%d, arsr1p sharp is not supported!\n", hisifd->index);
		return 0;
	}

	memcpy(hisifd->effect_info.arsr1p, arsr1p, sizeof(struct arsr1p_info)*2);
	hisifd->effect_updated_flag.arsr1p_effect_updated = true;

	if (arsr1p[1].update == 1) {
		hisifd->effect_info.arsr1p_rog_initialized = true;
	}

	return 0;
}

static int set_acm_normal_param(struct acm_info *acm_dst, struct hisi_panel_info *pinfo)
{
	if (acm_dst == NULL) {
		HISI_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (pinfo == NULL) {
		HISI_FB_DEBUG("pinfo is NULL!\n");
		return -1;
	}

	acm_set_hue(acm_dst, pinfo);

	/* malloc acm_dst lut table memory*/
	if (hisi_effect_alloc_and_copy(&acm_dst->hue_table, pinfo->acm_lut_hue_table,
		ACM_HUE_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm hut table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->sata_table, pinfo->acm_lut_sata_table,
		ACM_SATA_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm sata table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr0_table, pinfo->acm_lut_satr0_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr0 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr1_table, pinfo->acm_lut_satr1_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr1 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr2_table, pinfo->acm_lut_satr2_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr2 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr3_table, pinfo->acm_lut_satr3_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr3 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr4_table, pinfo->acm_lut_satr4_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr4 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr5_table, pinfo->acm_lut_satr5_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr5 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr6_table, pinfo->acm_lut_satr6_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr6 table from panel\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr7_table, pinfo->acm_lut_satr7_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr7 table from panel\n");
		return -EINVAL;
	}

	return 0;
}

static int set_acm_cinema_param(struct acm_info *acm_dst, struct hisi_panel_info *pinfo)
{
	if (acm_dst == NULL) {
		HISI_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (pinfo == NULL) {
		HISI_FB_DEBUG("pinfo is NULL!\n");
		return -1;
	}

	acm_dst->hue_r0_hh = pinfo->cinema_r0_hh;
	acm_dst->hue_r0_lh = pinfo->cinema_r0_lh;
	acm_dst->hue_r1_hh = pinfo->cinema_r1_hh;
	acm_dst->hue_r1_lh = pinfo->cinema_r1_lh;
	acm_dst->hue_r2_hh = pinfo->cinema_r2_hh;
	acm_dst->hue_r2_lh = pinfo->cinema_r2_lh;
	acm_dst->hue_r3_hh = pinfo->cinema_r3_hh;
	acm_dst->hue_r3_lh = pinfo->cinema_r3_lh;
	acm_dst->hue_r4_hh = pinfo->cinema_r4_hh;
	acm_dst->hue_r4_lh = pinfo->cinema_r4_lh;
	acm_dst->hue_r5_hh = pinfo->cinema_r5_hh;
	acm_dst->hue_r5_lh = pinfo->cinema_r5_lh;
	acm_dst->hue_r6_hh = pinfo->cinema_r6_hh;
	acm_dst->hue_r6_lh = pinfo->cinema_r6_lh;

	/* malloc acm_dst lut table memory*/
	if (hisi_effect_alloc_and_copy(&acm_dst->hue_table, pinfo->cinema_acm_lut_hue_table,
		ACM_HUE_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm hut table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->sata_table, pinfo->cinema_acm_lut_sata_table,
		ACM_SATA_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm sata table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr0_table, pinfo->cinema_acm_lut_satr0_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr0 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr1_table, pinfo->cinema_acm_lut_satr1_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr1 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr2_table, pinfo->cinema_acm_lut_satr2_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr2 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr3_table, pinfo->cinema_acm_lut_satr3_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr3 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr4_table, pinfo->cinema_acm_lut_satr4_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr4 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr5_table, pinfo->cinema_acm_lut_satr5_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr5 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr6_table, pinfo->cinema_acm_lut_satr6_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr6 table from panel cinema mode\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr7_table, pinfo->cinema_acm_lut_satr7_table,
		ACM_SATR_LUT_LENGTH, false)) {
		HISI_FB_ERR("failed to set acm satr7 table from panel cinema mode\n");
		return -EINVAL;
	}

	return 0;
}

static int set_acm_user_param(struct acm_info *acm_dst, struct acm_info *acm_src)
{
	if (acm_dst == NULL) {
		HISI_FB_DEBUG("acm_dst is NULL!\n");
		return -1;
	}

	if (acm_src == NULL) {
		HISI_FB_DEBUG("acm_src is NULL!\n");
		return -1;
	}

	acm_dst->hue_r0_hh = acm_src->hue_r0_hh;
	acm_dst->hue_r0_lh = acm_src->hue_r0_lh;
	acm_dst->hue_r1_hh = acm_src->hue_r1_hh;
	acm_dst->hue_r1_lh = acm_src->hue_r1_lh;
	acm_dst->hue_r2_hh = acm_src->hue_r2_hh;
	acm_dst->hue_r2_lh = acm_src->hue_r2_lh;
	acm_dst->hue_r3_hh = acm_src->hue_r3_hh;
	acm_dst->hue_r3_lh = acm_src->hue_r3_lh;
	acm_dst->hue_r4_hh = acm_src->hue_r4_hh;
	acm_dst->hue_r4_lh = acm_src->hue_r4_lh;
	acm_dst->hue_r5_hh = acm_src->hue_r5_hh;
	acm_dst->hue_r5_lh = acm_src->hue_r5_lh;
	acm_dst->hue_r6_hh = acm_src->hue_r6_hh;
	acm_dst->hue_r6_lh = acm_src->hue_r6_lh;

	/* malloc acm_dst lut table memory*/
	if (hisi_effect_alloc_and_copy(&acm_dst->hue_table, acm_src->hue_table,
		ACM_HUE_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm hut table from user\n");
		return -EINVAL;
	}

	if (hisi_effect_alloc_and_copy(&acm_dst->sata_table, acm_src->sata_table,
		ACM_SATA_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm sata table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr0_table, acm_src->satr0_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr0 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr1_table, acm_src->satr1_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr1 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr2_table, acm_src->satr2_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr2 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr3_table, acm_src->satr3_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr3 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr4_table, acm_src->satr4_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr4 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr5_table, acm_src->satr5_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr5 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr6_table, acm_src->satr6_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr6 table from user\n");
		return -EINVAL;
	}
	if (hisi_effect_alloc_and_copy(&acm_dst->satr7_table, acm_src->satr7_table,
		ACM_SATR_LUT_LENGTH, true)) {
		HISI_FB_ERR("failed to copy acm satr7 table from user\n");
		return -EINVAL;
	}

	return 0;
}

int hisi_effect_acm_info_set(struct hisi_fb_data_type *hisifd, struct acm_info *acm_src)
{
	struct acm_info *acm_dst = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == acm_src) {
		HISI_FB_ERR("fb%d, acm_src is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	acm_dst = &(hisifd->effect_info.acm);
	pinfo = &(hisifd->panel_info);

	if (!hisifd->effect_ctl.acm_support) {
		HISI_FB_INFO("fb%d, acm is not supported!\n", hisifd->index);
		return 0;
	}

	/*set acm info*/
	spin_lock(&g_acm_effect_lock);
	acm_dst->enable = acm_src->enable;
	if (acm_src->para_mode == 0) {
		if (set_acm_normal_param(acm_dst, pinfo)) {
			HISI_FB_ERR("fb%d, failed to set acm normal mode parameters\n", hisifd->index);
			goto err_ret;
		}
	} else if (acm_src->para_mode == 1) {
		if (set_acm_cinema_param(acm_dst, pinfo)) {
			HISI_FB_ERR("fb%d, failed to set acm cinema mode parameters\n", hisifd->index);
			goto err_ret;
		}
	} else if (acm_src->para_mode == 2) {
		if (set_acm_user_param(acm_dst, acm_src)) {
			HISI_FB_ERR("fb%d, failed to set acm cinema mode parameters\n", hisifd->index);
			goto err_ret;
		}
	} else {
		HISI_FB_ERR("fb%d, invalid acm para mode!\n", hisifd->index);
		spin_unlock(&g_acm_effect_lock);
		return -EINVAL;
	}

	hisifd->effect_updated_flag.acm_effect_updated = true;
	spin_unlock(&g_acm_effect_lock);
	return 0;

err_ret:
	free_acm_table(acm_dst);
	spin_unlock(&g_acm_effect_lock);
	return -EINVAL;
}

int hisi_effect_gmp_info_set(struct hisi_fb_data_type *hisifd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}
	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		HISI_FB_ERR("fb%d, lcp_src is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	lcp_dst = &(hisifd->effect_info.lcp);
	effect = &(hisifd->effect_ctl);

	if (!effect->lcp_gmp_support) {
		HISI_FB_INFO("fb%d, lcp gmp is not supported!\n", hisifd->index);
		return 0;
	}

	spin_lock(&g_gmp_effect_lock);
	lcp_dst->gmp_enable = lcp_src->gmp_enable;
	if(g_gmp_backup_enable){
		if (hisi_effect_alloc_and_copy(&lcp_dst->gmp_table_high4, lcp_src->gmp_table_high4,
			LCP_GMP_LUT_LENGTH, true)) {
			HISI_FB_ERR("fb%d, failed to set gmp_table_high4!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&lcp_dst->gmp_table_low32, lcp_src->gmp_table_low32,
			LCP_GMP_LUT_LENGTH, true)) {
			HISI_FB_ERR("fb%d, failed to set gmp_lut_table_low32bit!\n", hisifd->index);
			goto err_ret;
		}
	}

	hisifd->effect_updated_flag.gmp_effect_updated = true;
	spin_unlock(&g_gmp_effect_lock);
	return 0;

err_ret:
	hisi_effect_kfree(&lcp_dst->gmp_table_high4);
	hisi_effect_kfree(&lcp_dst->gmp_table_low32);

	spin_unlock(&g_gmp_effect_lock);
	return -EINVAL;
}

int hisi_effect_igm_info_set(struct hisi_fb_data_type *hisifd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}
	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		HISI_FB_ERR("fb%d, lcp_src is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	lcp_dst = &(hisifd->effect_info.lcp);
	effect = &(hisifd->effect_ctl);

	if (!effect->lcp_igm_support) {
		HISI_FB_INFO("fb%d, lcp degamma is not supported!\n", hisifd->index);
		return 0;
	}

	spin_lock(&g_igm_effect_lock);
	lcp_dst->igm_enable = lcp_src->igm_enable;

	if (hisi_effect_alloc_and_copy(&lcp_dst->igm_r_table, lcp_src->igm_r_table,
		IGM_LUT_LEN, true)) {
		HISI_FB_ERR("fb%d, failed to set igm_r_table!\n", hisifd->index);
		goto err_ret;
	}

	if (hisi_effect_alloc_and_copy(&lcp_dst->igm_g_table, lcp_src->igm_g_table,
		IGM_LUT_LEN, true)) {
		HISI_FB_ERR("fb%d, failed to set igm_g_table!\n", hisifd->index);
		goto err_ret;
	}

	if (hisi_effect_alloc_and_copy(&lcp_dst->igm_b_table, lcp_src->igm_b_table,
		IGM_LUT_LEN, true)) {
		HISI_FB_ERR("fb%d, failed to set igm_b_table!\n", hisifd->index);
		goto err_ret;
	}

	spin_unlock(&g_igm_effect_lock);
	hisifd->effect_updated_flag.igm_effect_updated = true;
	return 0;

err_ret:
	hisi_effect_kfree(&lcp_dst->igm_r_table);
	hisi_effect_kfree(&lcp_dst->igm_g_table);
	hisi_effect_kfree(&lcp_dst->igm_b_table);

	spin_unlock(&g_igm_effect_lock);
	return -EINVAL;
}

int hisi_effect_xcc_info_set(struct hisi_fb_data_type *hisifd, struct lcp_info *lcp_src){
	struct lcp_info *lcp_dst = NULL;
	struct dss_effect *effect = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}
	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == lcp_src) {
		HISI_FB_ERR("fb%d, lcp_src is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	lcp_dst = &(hisifd->effect_info.lcp);
	effect = &(hisifd->effect_ctl);

	if (!effect->lcp_xcc_support) {
		HISI_FB_INFO("fb%d, lcp xcc are not supported!\n", hisifd->index);
		return 0;
	}

	spin_lock(&g_xcc_effect_lock);
	lcp_dst->xcc_enable = lcp_src->xcc_enable;

	if (hisi_effect_alloc_and_copy(&lcp_dst->xcc_table, lcp_src->xcc_table,
		LCP_XCC_LUT_LENGTH, true)) {
		HISI_FB_ERR("fb%d, failed to set xcc_table!\n", hisifd->index);
		goto err_ret;
	}

	hisifd->effect_updated_flag.xcc_effect_updated = true;
	spin_unlock(&g_xcc_effect_lock);
	return 0;

err_ret:
	hisi_effect_kfree(&lcp_dst->xcc_table);
	spin_unlock(&g_xcc_effect_lock);
	return -EINVAL;
}

int hisi_effect_gamma_info_set(struct hisi_fb_data_type *hisifd, struct gamma_info *gamma_src)
{
	struct gamma_info *gamma_dst = NULL;
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}
	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return -EINVAL;
	}

	if (NULL == gamma_src) {
		HISI_FB_ERR("fb%d, gamma_src is NULL!\n", hisifd->index);
		return -EINVAL;
	}

	gamma_dst = &(hisifd->effect_info.gamma);
	pinfo = &(hisifd->panel_info);

	if (!hisifd->effect_ctl.gamma_support) {
		HISI_FB_INFO("fb%d, gamma is not supported!\n", hisifd->index);
		return 0;
	}

	spin_lock(&g_gamma_effect_lock);
	gamma_dst->enable = gamma_src->enable;
	gamma_dst->para_mode = gamma_src->para_mode;
	if (gamma_src->para_mode == 0) {
		//Normal mode
		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_r_table, pinfo->gamma_lut_table_R,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_r_table!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_g_table, pinfo->gamma_lut_table_G,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_g_table!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_b_table, pinfo->gamma_lut_table_B,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_b_table!\n", hisifd->index);
			goto err_ret;
		}
	} else if (gamma_src->para_mode == 1) {
		//Cinema mode
		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_r_table, pinfo->cinema_gamma_lut_table_R,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_r_table!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_g_table, pinfo->cinema_gamma_lut_table_G,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_g_table!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_b_table, pinfo->cinema_gamma_lut_table_B,
			GAMMA_LUT_LEN, false)) {
			HISI_FB_ERR("fb%d, failed to set gamma_b_table!\n", hisifd->index);
			goto err_ret;
		}
	} else if (gamma_src->para_mode == 2) {
		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_r_table, gamma_src->gamma_r_table,
			GAMMA_LUT_LEN, true)) {
			HISI_FB_ERR("fb%d, failed to copy gamma_r_table from user!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_g_table, gamma_src->gamma_g_table,
			GAMMA_LUT_LEN, true)) {
			HISI_FB_ERR("fb%d, failed to copy gamma_g_table from user!\n", hisifd->index);
			goto err_ret;
		}

		if (hisi_effect_alloc_and_copy(&gamma_dst->gamma_b_table, gamma_src->gamma_b_table,
			GAMMA_LUT_LEN, true)) {
			HISI_FB_ERR("fb%d, failed to copy gamma_b_table from user!\n", hisifd->index);
			goto err_ret;
		}
	} else {
		HISI_FB_ERR("fb%d, not supported gamma para_mode!\n", hisifd->index);
		spin_unlock(&g_gamma_effect_lock);
		return -EINVAL;
	}

	hisifd->effect_updated_flag.gamma_effect_updated = true;
	spin_unlock(&g_gamma_effect_lock);
	return 0;

err_ret:
	free_gamma_table(gamma_dst);

	spin_unlock(&g_gamma_effect_lock);
	return -EINVAL;
}

void hisi_effect_acm_set_reg(struct hisi_fb_data_type *hisifd)
{
	struct acm_info *acm_param = NULL;
	char __iomem *acm_base = NULL;
	char __iomem *acm_lut_base = NULL;
	//static uint32_t acm_config_flag = 0;
	uint32_t lut_sel;


	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return;
	}

	if (!hisifd->effect_ctl.acm_support) {
		return;
	}

	if (!hisifd->effect_updated_flag.acm_effect_updated) {
		return;
	}

	acm_base = hisifd->dss_base + DSS_DPP_ACM_OFFSET;
	acm_lut_base = hisifd->dss_base + DSS_DPP_ACM_LUT_OFFSET;

	acm_param = &(hisifd->effect_info.acm);
	if (!spin_can_lock(&g_acm_effect_lock)) {
		HISI_FB_INFO("gamma effect param is being updated, delay set reg to next frame!\n");
		return;
	}

	spin_lock(&g_acm_effect_lock);

	if (NULL == acm_param->hue_table) {
		HISI_FB_INFO("fb%d, invalid acm hue table param!\n", hisifd->index);
		goto err_ret;
	}

	if (NULL == acm_param->sata_table) {
		HISI_FB_INFO("fb%d, invalid acm sata table param!\n", hisifd->index);
		goto err_ret;
	}

	if ((NULL == acm_param->satr0_table) || (NULL == acm_param->satr1_table)
		|| (NULL == acm_param->satr2_table) || (NULL == acm_param->satr3_table)
		|| (NULL == acm_param->satr4_table) || (NULL == acm_param->satr5_table)
		|| (NULL == acm_param->satr6_table) || (NULL == acm_param->satr7_table)) {
		HISI_FB_INFO("fb%d, invalid acm satr table param!\n", hisifd->index);
		goto err_ret;
	}

	//start updating new acm info into register
	set_reg(acm_base + ACM_R0_H, ((acm_param->hue_r0_hh & 0x3ff) << 16) | (acm_param->hue_r0_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R1_H, ((acm_param->hue_r1_hh & 0x3ff) << 16) | (acm_param->hue_r1_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R2_H, ((acm_param->hue_r2_hh & 0x3ff) << 16) | (acm_param->hue_r2_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R3_H, ((acm_param->hue_r3_hh & 0x3ff) << 16) | (acm_param->hue_r3_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R4_H, ((acm_param->hue_r4_hh & 0x3ff) << 16) | (acm_param->hue_r4_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R5_H, ((acm_param->hue_r5_hh & 0x3ff) << 16) | (acm_param->hue_r5_lh & 0x3ff), 32, 0);
	set_reg(acm_base + ACM_R6_H, ((acm_param->hue_r6_hh & 0x3ff) << 16) | (acm_param->hue_r6_lh & 0x3ff), 32, 0);

	acm_set_lut_hue(acm_lut_base + ACM_U_H_COEF, acm_param->hue_table, ACM_HUE_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATA_COEF, acm_param->sata_table, ACM_SATA_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR0_COEF, acm_param->satr0_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR1_COEF, acm_param->satr1_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR2_COEF, acm_param->satr2_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR3_COEF, acm_param->satr3_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR4_COEF, acm_param->satr4_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR5_COEF, acm_param->satr5_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR6_COEF, acm_param->satr6_table, ACM_SATR_LUT_LENGTH);
	acm_set_lut(acm_lut_base + ACM_U_SATR7_COEF, acm_param->satr7_table, ACM_SATR_LUT_LENGTH);

	lut_sel = inp32(acm_base + ACM_LUT_SEL);
	set_reg(acm_base + ACM_LUT_SEL, (~lut_sel) & 0x3FF, 10, 0);
	set_reg(acm_base + ACM_EN, acm_param->enable, 1, 0);

err_ret:
	hisifd->effect_updated_flag.acm_effect_updated = false;
	spin_unlock(&g_acm_effect_lock);

	//free_acm_table(acm_param);
	//acm_config_flag = 0;
	return;
}

/*******************************************************************************
**ACM REG DIMMING
*/
/*lint -e866*/
static inline uint32_t cal_hue_value(uint32_t cur,uint32_t target,int count) {
	if (count <= 0) {
		return cur;
	}

	if (abs((int)(cur - target)) > HUE_REG_RANGE) {
		if (cur > target) {
			target += HUE_REG_OFFSET;
		} else {
			cur += HUE_REG_OFFSET;
		}
	}

	if (target > cur) {
		cur += (target - cur) / (uint32_t)count;
	} else if (cur > target) {
		cur -= (cur - target) / (uint32_t)count;
	}

	cur %= HUE_REG_OFFSET;

	return cur;
}

static inline uint32_t cal_sata_value(uint32_t cur,uint32_t target,int count) {
	if (count <= 0) {
		return cur;
	}

	if (target > cur) {
		cur += (target - cur) / (uint32_t)count;
	} else if (cur > target) {
		cur -= (cur - target) / (uint32_t)count;
	}

	return cur;
}

/*lint -e838*/
static inline int satr_unint32_to_int(uint32_t input) {
	int result = 0;

	if (input >= SATR_REG_RANGE) {
		result = (int)input - SATR_REG_OFFSET;
	} else {
		result = (int)input;
	}
	return result;
}

/*lint -e838*/
static inline uint32_t cal_satr_value(uint32_t cur,uint32_t target,int count) {
	int i_cur = 0;
	int i_target = 0;

	if (count <= 0) {
		return cur;
	}

	i_cur = satr_unint32_to_int(cur);
	i_target = satr_unint32_to_int(target);

	return (uint32_t)(i_cur + (i_target - i_cur) / count) % SATR_REG_OFFSET;
}

/*lint -e838*/
static inline void cal_cur_acm_reg(int count,acm_reg_t *cur_acm_reg,acm_reg_table_t *target_acm_reg) {
	int index = 0;

	if (NULL == cur_acm_reg || NULL == target_acm_reg) {
		return;
	}

	for (index = 0;index < ACM_HUE_SIZE;index++) {
		cur_acm_reg->acm_lut_hue_table[index] = cal_hue_value(cur_acm_reg->acm_lut_hue_table[index],target_acm_reg->acm_lut_hue_table[index],count);
	}
	for (index = 0;index < ACM_SATA_SIZE;index++) {
		cur_acm_reg->acm_lut_sata_table[index] = cal_sata_value(cur_acm_reg->acm_lut_sata_table[index],target_acm_reg->acm_lut_sata_table[index],count);
	}
	for (index = 0;index < ACM_SATR_SIZE;index++) {
		cur_acm_reg->acm_lut_satr0_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr0_table[index],target_acm_reg->acm_lut_satr0_table[index],count);
		cur_acm_reg->acm_lut_satr1_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr1_table[index],target_acm_reg->acm_lut_satr1_table[index],count);
		cur_acm_reg->acm_lut_satr2_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr2_table[index],target_acm_reg->acm_lut_satr2_table[index],count);
		cur_acm_reg->acm_lut_satr3_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr3_table[index],target_acm_reg->acm_lut_satr3_table[index],count);
		cur_acm_reg->acm_lut_satr4_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr4_table[index],target_acm_reg->acm_lut_satr4_table[index],count);
		cur_acm_reg->acm_lut_satr5_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr5_table[index],target_acm_reg->acm_lut_satr5_table[index],count);
		cur_acm_reg->acm_lut_satr6_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr6_table[index],target_acm_reg->acm_lut_satr6_table[index],count);
		cur_acm_reg->acm_lut_satr7_table[index] = cal_satr_value(cur_acm_reg->acm_lut_satr7_table[index],target_acm_reg->acm_lut_satr7_table[index],count);
	}
}

/*lint -e838*/
void hisi_effect_color_dimming_acm_reg_set(struct hisi_fb_data_type *hisifd, acm_reg_t *cur_acm_reg) {
	acm_reg_table_t target_acm_reg = {0};
	struct hisi_panel_info *pinfo = NULL;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!");
		return;
	}

	if (NULL == cur_acm_reg) {
		HISI_FB_ERR("cur_acm_reg is NULL!");
		return;
	}

	pinfo = &(hisifd->panel_info);

	if (SCENE_MODE_GALLERY == hisifd->user_scene_mode || SCENE_MODE_DEFAULT == hisifd->user_scene_mode) {
		target_acm_reg.acm_lut_hue_table = pinfo->acm_lut_hue_table;
		target_acm_reg.acm_lut_sata_table = pinfo->acm_lut_sata_table;
		target_acm_reg.acm_lut_satr0_table = pinfo->acm_lut_satr0_table;
		target_acm_reg.acm_lut_satr1_table = pinfo->acm_lut_satr1_table;
		target_acm_reg.acm_lut_satr2_table = pinfo->acm_lut_satr2_table;
		target_acm_reg.acm_lut_satr3_table = pinfo->acm_lut_satr3_table;
		target_acm_reg.acm_lut_satr4_table = pinfo->acm_lut_satr4_table;
		target_acm_reg.acm_lut_satr5_table = pinfo->acm_lut_satr5_table;
		target_acm_reg.acm_lut_satr6_table = pinfo->acm_lut_satr6_table;
		target_acm_reg.acm_lut_satr7_table = pinfo->acm_lut_satr7_table;
	} else {
		target_acm_reg.acm_lut_hue_table = pinfo->video_acm_lut_hue_table;
		target_acm_reg.acm_lut_sata_table = pinfo->video_acm_lut_sata_table;
		target_acm_reg.acm_lut_satr0_table = pinfo->video_acm_lut_satr0_table;
		target_acm_reg.acm_lut_satr1_table = pinfo->video_acm_lut_satr1_table;
		target_acm_reg.acm_lut_satr2_table = pinfo->video_acm_lut_satr2_table;
		target_acm_reg.acm_lut_satr3_table = pinfo->video_acm_lut_satr3_table;
		target_acm_reg.acm_lut_satr4_table = pinfo->video_acm_lut_satr4_table;
		target_acm_reg.acm_lut_satr5_table = pinfo->video_acm_lut_satr5_table;
		target_acm_reg.acm_lut_satr6_table = pinfo->video_acm_lut_satr6_table;
		target_acm_reg.acm_lut_satr7_table = pinfo->video_acm_lut_satr7_table;
	}

	cal_cur_acm_reg(hisifd->dimming_count,cur_acm_reg,&target_acm_reg);
}

/*lint -e838*/
void hisi_effect_color_dimming_acm_reg_init(struct hisi_fb_data_type *hisifd) {//add dimming reg init
	struct hisi_panel_info *pinfo = NULL;
	acm_reg_t *cur_acm_reg = NULL;
	int index = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd, null pointer warning.\n");
		return;
	}

	pinfo = &(hisifd->panel_info);

	cur_acm_reg = &(hisifd->acm_reg);

	if (SCENE_MODE_GALLERY == hisifd->user_scene_mode || SCENE_MODE_DEFAULT == hisifd->user_scene_mode) {
		for (index = 0;index < ACM_HUE_SIZE;index++) {
			cur_acm_reg->acm_lut_hue_table[index] = pinfo->acm_lut_hue_table[index];
		}
		for (index = 0;index < ACM_SATA_SIZE;index++) {
			cur_acm_reg->acm_lut_sata_table[index] = pinfo->acm_lut_sata_table[index];
		}
		for (index = 0;index < ACM_SATR_SIZE;index++) {
			cur_acm_reg->acm_lut_satr0_table[index] = pinfo->acm_lut_satr0_table[index];
			cur_acm_reg->acm_lut_satr1_table[index] = pinfo->acm_lut_satr1_table[index];
			cur_acm_reg->acm_lut_satr2_table[index] = pinfo->acm_lut_satr2_table[index];
			cur_acm_reg->acm_lut_satr3_table[index] = pinfo->acm_lut_satr3_table[index];
			cur_acm_reg->acm_lut_satr4_table[index] = pinfo->acm_lut_satr4_table[index];
			cur_acm_reg->acm_lut_satr5_table[index] = pinfo->acm_lut_satr5_table[index];
			cur_acm_reg->acm_lut_satr6_table[index] = pinfo->acm_lut_satr6_table[index];
			cur_acm_reg->acm_lut_satr7_table[index] = pinfo->acm_lut_satr7_table[index];
		}
	} else {
		for (index = 0;index < ACM_HUE_SIZE;index++) {
			cur_acm_reg->acm_lut_hue_table[index] = pinfo->video_acm_lut_hue_table[index];
		}
		for (index = 0;index < ACM_SATA_SIZE;index++) {
			cur_acm_reg->acm_lut_sata_table[index] = pinfo->video_acm_lut_sata_table[index];
		}
		for (index = 0;index < ACM_SATR_SIZE;index++) {
			cur_acm_reg->acm_lut_satr0_table[index] = pinfo->video_acm_lut_satr0_table[index];
			cur_acm_reg->acm_lut_satr1_table[index] = pinfo->video_acm_lut_satr1_table[index];
			cur_acm_reg->acm_lut_satr2_table[index] = pinfo->video_acm_lut_satr2_table[index];
			cur_acm_reg->acm_lut_satr3_table[index] = pinfo->video_acm_lut_satr3_table[index];
			cur_acm_reg->acm_lut_satr4_table[index] = pinfo->video_acm_lut_satr4_table[index];
			cur_acm_reg->acm_lut_satr5_table[index] = pinfo->video_acm_lut_satr5_table[index];
			cur_acm_reg->acm_lut_satr6_table[index] = pinfo->video_acm_lut_satr6_table[index];
			cur_acm_reg->acm_lut_satr7_table[index] = pinfo->video_acm_lut_satr7_table[index];
		}
	}
}

#define XCC_COEF_LEN	12
#define GMP_BLOCK_SIZE	137
#define GMP_CNT_NUM 18

static bool lcp_igm_set_reg(char __iomem *lcp_lut_base, struct lcp_info *lcp_param)
{
	int cnt;

	if (lcp_lut_base == NULL) {
		HISI_FB_DEBUG("lcp_lut_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		HISI_FB_DEBUG("lcp_param is NULL!\n");
		return false;
	}
	if (lcp_param->igm_r_table == NULL || lcp_param->igm_g_table == NULL || lcp_param->igm_b_table == NULL) {
		HISI_FB_DEBUG("igm_r_table or igm_g_table or igm_b_table is NULL!\n");
		return false;
	}

	for (cnt = 0; cnt < IGM_LUT_LEN; cnt = cnt + 2) {
		set_reg(lcp_lut_base + (LCP_U_DEGAMA_R_COEF + cnt * 2), lcp_param->igm_r_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(lcp_lut_base + (LCP_U_DEGAMA_R_COEF + cnt * 2), lcp_param->igm_r_table[cnt+1], 12,16);

		set_reg(lcp_lut_base + (LCP_U_DEGAMA_G_COEF + cnt * 2), lcp_param->igm_g_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(lcp_lut_base + (LCP_U_DEGAMA_G_COEF + cnt * 2), lcp_param->igm_g_table[cnt+1], 12,16);

		set_reg(lcp_lut_base + (LCP_U_DEGAMA_B_COEF + cnt * 2), lcp_param->igm_b_table[cnt], 12,0);
		if(cnt != IGM_LUT_LEN-1)
			set_reg(lcp_lut_base + (LCP_U_DEGAMA_B_COEF + cnt * 2), lcp_param->igm_b_table[cnt+1], 12,16);
	}
	return true;
}

static bool lcp_xcc_set_reg(char __iomem *lcp_base, struct lcp_info *lcp_param)
{
	int cnt;

	if (lcp_base == NULL) {
		HISI_FB_DEBUG("lcp_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		HISI_FB_DEBUG("lcp_param is NULL!\n");
		return false;
	}

	if (lcp_param->xcc_table == NULL) {
		HISI_FB_DEBUG("xcc_table is NULL!\n");
		return false;
	}
	for (cnt = 0; cnt < XCC_COEF_LEN; cnt++) {
		set_reg(lcp_base + LCP_XCC_COEF_00 +cnt * 4,  lcp_param->xcc_table[cnt], 17, 0);
	}
	return true;
}

static bool lcp_gmp_set_reg(char __iomem *lcp_lut_base, struct lcp_info *lcp_param)
{
	int cnt;
	int block;
	int valBlock = 0;
	static uint32_t gmp_lut_low32[LCP_GMP_LUT_LENGTH];
	static uint32_t gmp_lut_high4[LCP_GMP_LUT_LENGTH];

	if (lcp_lut_base == NULL) {
		HISI_FB_DEBUG("lcp_lut_base is NULL!\n");
		return false;
	}

	if (lcp_param == NULL) {
		HISI_FB_DEBUG("lcp_param is NULL!\n");
		return false;
	}

	if(g_gmp_backup_enable){
		if (lcp_param->gmp_table_low32 == NULL || lcp_param->gmp_table_high4 == NULL) {
			HISI_FB_DEBUG("gmp_table_low32 or gmp_table_high4 is NULL!\n");
			return false;
		}
	}

	if(g_gmp_backup_enable){
		for (block = 0; block < GMP_BLOCK_SIZE; block ++) {
			for (cnt = 0; cnt < GMP_CNT_NUM; cnt ++) {
				if (cnt % 2 == 0) {
					set_reg(lcp_lut_base + block * 0x80 + cnt * 4, lcp_param->gmp_table_low32[valBlock*9+cnt/2], 32, 0);
					gmp_lut_low32[valBlock*9+cnt/2] = lcp_param->gmp_table_low32[valBlock*9+cnt/2];
				} else {
					set_reg(lcp_lut_base + block * 0x80 + cnt * 4, lcp_param->gmp_table_high4[valBlock*9+cnt/2], 4, 0);
					gmp_lut_high4[valBlock*9+cnt/2] = lcp_param->gmp_table_high4[valBlock*9+cnt/2];
				}
			}

			valBlock ++;
			//9*9 blocks, each block includes 9 values
			if (valBlock == 81) {
				valBlock =0;
			}

			if ((block % 8 == 0) && (((block / 8) % 2) != 0)) {
				block += 7;
			}
		}
		g_gmp_backup_enable = false;
	}else{
		for (block = 0; block < GMP_BLOCK_SIZE; block ++) {
			for (cnt = 0; cnt < GMP_CNT_NUM; cnt ++) {
				if (cnt % 2 == 0) {
					set_reg(lcp_lut_base + block * 0x80 + cnt * 4, gmp_lut_low32[valBlock*9+cnt/2], 32, 0);
			    } else {
					set_reg(lcp_lut_base + block * 0x80 + cnt * 4, gmp_lut_high4[valBlock*9+cnt/2], 4, 0);
				}
			}
			valBlock ++;
			if (valBlock == 81) {
				valBlock =0;
			}
			if ((block % 8 == 0) && (((block / 8) % 2) != 0)) {
				block += 7;
			}
		}
	}
	return true;
}

void hisi_effect_lcp_set_reg(struct hisi_fb_data_type *hisifd)
{
	struct dss_effect *effect = NULL;
	struct lcp_info *lcp_param = NULL;
	char __iomem *lcp_base = NULL;
	char __iomem *lcp_lut_base = NULL;
	static uint32_t lcp_config_flag = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return;
	}

	effect = &hisifd->effect_ctl;
	if ((!effect->lcp_igm_support) &&
		(!effect->lcp_gmp_support) &&
		(!effect->lcp_xcc_support)) {
		HISI_FB_DEBUG("lcp is not support!");
		return;
	}
	if (!hisifd->effect_updated_flag.gmp_effect_updated &&
		!hisifd->effect_updated_flag.igm_effect_updated &&
		!hisifd->effect_updated_flag.xcc_effect_updated) {
		HISI_FB_DEBUG("lcp param is not update!");
		return;
	}
	lcp_base = hisifd->dss_base + DSS_DPP_LCP_OFFSET;
	lcp_lut_base = hisifd->dss_base + DSS_DPP_LCP_LUT_OFFSET;

	if (lcp_config_flag == 0 && hisifd->effect_updated_flag.gmp_effect_updated) {
		//Disable GMP
		if(spin_can_lock(&g_gmp_effect_lock)){
			spin_lock(&g_gmp_effect_lock);
			set_reg(lcp_base + LCP_GMP_BYPASS_EN,  0x1, 1, 0);
			lcp_config_flag = 1;
			spin_unlock(&g_gmp_effect_lock);
		}
		return;
	}

	lcp_param = &(hisifd->effect_info.lcp);

	//Update De-Gamma LUT
	if (effect->lcp_igm_support && hisifd->effect_updated_flag.igm_effect_updated) {
		if(spin_can_lock(&g_igm_effect_lock)){
			spin_lock(&g_igm_effect_lock);
			bool ret = lcp_igm_set_reg(lcp_lut_base, lcp_param);
			//Enable De-Gamma
			if (ret) {
				set_reg(lcp_base + LCP_DEGAMA_EN,  lcp_param->igm_enable, 1, 0);
			}
			hisifd->effect_updated_flag.igm_effect_updated = false;
			spin_unlock(&g_igm_effect_lock);
		}else{
			HISI_FB_INFO("igm effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//Update XCC Coef
	if (effect->lcp_xcc_support && hisifd->effect_updated_flag.xcc_effect_updated) {
		if(spin_can_lock(&g_xcc_effect_lock)){
			spin_lock(&g_xcc_effect_lock);
			bool ret = lcp_xcc_set_reg(lcp_base, lcp_param);
			//Enable XCC
			if (ret) {
				set_reg(lcp_base + LCP_XCC_BYPASS_EN,  (!lcp_param->xcc_enable), 1, 0);
			}
			hisifd->effect_updated_flag.xcc_effect_updated = false;
			spin_unlock(&g_xcc_effect_lock);
		}else{
			HISI_FB_INFO("xcc effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//Update GMP LUT
	if (effect->lcp_gmp_support && hisifd->effect_updated_flag.gmp_effect_updated) {
		if (spin_can_lock(&g_gmp_effect_lock)){
			spin_lock(&g_gmp_effect_lock);
			bool ret;
			if(lcp_param->gmp_enable==1){
				ret = lcp_gmp_set_reg(lcp_lut_base, lcp_param);
			}
			//Enable GMP
			if (lcp_param->gmp_enable==0||ret) {
				set_reg(lcp_base + LCP_GMP_BYPASS_EN,  (!lcp_param->gmp_enable), 1, 0);
			}
			hisifd->effect_updated_flag.gmp_effect_updated = false;
			spin_unlock(&g_gmp_effect_lock);
		}else{
			HISI_FB_INFO("gmp effect param is being updated, delay set reg to next frame!\n");
		}
	}

	//free_lcp_table(lcp_param);
	lcp_config_flag = 0;
	return;
}

void hisi_effect_gamma_set_reg(struct hisi_fb_data_type *hisifd)
{
	struct gamma_info *gamma_param = NULL;
	char __iomem *gamma_base = NULL;
	char __iomem *gamma_lut_base = NULL;
	static uint32_t gamma_config_flag = 0;
	int cnt = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!");
		return;
	}

	if (!g_is_effect_lock_init) {
		HISI_FB_INFO("display effect lock is not init!\n");
		return;
	}

	if (!hisifd->effect_ctl.gamma_support) {
		return;
	}

	if (!hisifd->effect_updated_flag.gamma_effect_updated) {
		return;
	}

	gamma_base = hisifd->dss_base + DSS_DPP_GAMA_OFFSET;
	gamma_lut_base = hisifd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;

	if(gamma_config_flag == 0) {
		//Disable Gamma
		set_reg(gamma_base + GAMA_EN,  0x0, 1, 0);
		gamma_config_flag = 1;
		return;
	}

	gamma_param = &(hisifd->effect_info.gamma);
	if (!spin_can_lock(&g_gamma_effect_lock)) {
		HISI_FB_INFO("gamma effect param is being updated, delay set reg to next frame!\n");
		return;
	}
	spin_lock(&g_gamma_effect_lock);

	if ((NULL == gamma_param->gamma_r_table) ||
		(NULL == gamma_param->gamma_g_table) ||
		(NULL == gamma_param->gamma_b_table)) {
		HISI_FB_INFO("fb%d, gamma table is null!\n", hisifd->index);
		goto err_ret;
	}

	//Update Gamma LUT
	for (cnt = 0; cnt < GAMMA_LUT_LEN; cnt = cnt + 2) {
		set_reg(gamma_lut_base + (U_GAMA_R_COEF + cnt * 2), gamma_param->gamma_r_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN-1)
			set_reg(gamma_lut_base + (U_GAMA_R_COEF + cnt * 2), gamma_param->gamma_r_table[cnt+1], 12,16);

		set_reg(gamma_lut_base + (U_GAMA_G_COEF + cnt * 2), gamma_param->gamma_g_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN-1)
			set_reg(gamma_lut_base + (U_GAMA_G_COEF + cnt * 2), gamma_param->gamma_g_table[cnt+1], 12,16);

		set_reg(gamma_lut_base + (U_GAMA_B_COEF + cnt * 2), gamma_param->gamma_b_table[cnt], 12,0);
		if (cnt != GAMMA_LUT_LEN-1)
			set_reg(gamma_lut_base + (U_GAMA_B_COEF + cnt * 2), gamma_param->gamma_b_table[cnt+1], 12,16);
	}

	//Enable Gamma
	set_reg(gamma_base + GAMA_EN,  gamma_param->enable, 1, 0);

err_ret:
	hisifd->effect_updated_flag.gamma_effect_updated = false;
	gamma_config_flag = 0;
	//free_gamma_table(gamma_param);

	spin_unlock(&g_gamma_effect_lock);
	return;
}


#define ARSR1P_MIN_SIZE 16
#define ARSR1P_MAX_SIZE 8192
#define ARSR1P_MAX_SRC_WIDTH_SIZE 3840
static int set_hiace_param(struct hisi_fb_data_type *hisifd) {
	char __iomem *hiace_base = NULL;
	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return -1;
	}
	if (hisifd->index == PRIMARY_PANEL_IDX) {
		hiace_base = hisifd->dss_base + DSS_HI_ACE_OFFSET;
	} else {
		HISI_FB_DEBUG("[effect] fb%d, not support!", hisifd->index);
		return 0;
	}

	if(!check_underflow_clear_ack(hisifd)){
		HISI_FB_ERR("[effect] fb%d, clear_ack is not return!\n", hisifd->index);
		return -1;
	}

	if (hisifd->effect_updated_flag.hiace_effect_updated) {
		set_reg(hiace_base + HIACE_IMAGE_INFO, hisifd->effect_info.hiace.image_info, 32, 0);
		set_reg(hiace_base + HIACE_HALF_BLOCK_H_W, hisifd->effect_info.hiace.half_block_info, 32, 0);
		set_reg(hiace_base + HIACE_XYWEIGHT, hisifd->effect_info.hiace.xyweight, 32, 0);
		set_reg(hiace_base + HIACE_LHIST_SFT, hisifd->effect_info.hiace.lhist_sft, 32, 0);
		set_reg(hiace_base + HIACE_HUE, hisifd->effect_info.hiace.hue, 32, 0);
		set_reg(hiace_base + HIACE_SATURATION, hisifd->effect_info.hiace.saturation, 32, 0);
		set_reg(hiace_base + HIACE_VALUE, hisifd->effect_info.hiace.value, 32, 0);
		set_reg(hiace_base + HIACE_SKIN_GAIN, hisifd->effect_info.hiace.skin_gain, 32, 0);
		set_reg(hiace_base + HIACE_UP_LOW_TH, hisifd->effect_info.hiace.up_low_th, 32, 0);
		set_reg(hiace_base + HIACE_UP_CNT, hisifd->effect_info.hiace.up_cnt, 32, 0);
		set_reg(hiace_base + HIACE_LOW_CNT, hisifd->effect_info.hiace.low_cnt, 32, 0);
		set_reg(hiace_base + HIACE_GAMMA_EN, hisifd->effect_info.hiace.gamma_w, 32, 0);
		set_reg(hiace_base + HIACE_GAMMA_EN_HV_R, hisifd->effect_info.hiace.gamma_r, 32, 0);
	}
	hisifd->effect_updated_flag.hiace_effect_updated = false;
	return 0;
}

static void hisi_effect_update_arsr1p_param(dss_arsr1p_t *post_scf, struct arsr1p_info *arsr1p_param) {
	if (NULL == post_scf || NULL == arsr1p_param) {
		HISI_FB_ERR("invalid param!!!");
		return;
	}

	post_scf->mode = set_bits32(post_scf->mode, ~arsr1p_param->enable, 1, 0);
	post_scf->mode = set_bits32(post_scf->mode, arsr1p_param->sharp_enable, 1, 1);
	post_scf->mode = set_bits32(post_scf->mode, arsr1p_param->shoot_enable, 1, 2);
	post_scf->mode = set_bits32(post_scf->mode, arsr1p_param->skin_enable, 1, 3);
	post_scf->skin_thres_y = arsr1p_param->skin_thres_y;
	post_scf->skin_thres_u = arsr1p_param->skin_thres_u;
	post_scf->skin_thres_v = arsr1p_param->skin_thres_v;
	post_scf->skin_expected = arsr1p_param->skin_expected;
	post_scf->skin_cfg = arsr1p_param->skin_cfg;
	post_scf->shoot_cfg1 = arsr1p_param->shoot_cfg1;
	post_scf->shoot_cfg2 = arsr1p_param->shoot_cfg2;
	post_scf->sharp_cfg1 = arsr1p_param->sharp_cfg1;
	post_scf->sharp_cfg2 = arsr1p_param->sharp_cfg2;
	post_scf->sharp_cfg3 = arsr1p_param->sharp_cfg3;
	post_scf->sharp_cfg4 = arsr1p_param->sharp_cfg4;
	post_scf->sharp_cfg5 = arsr1p_param->sharp_cfg5;
	post_scf->sharp_cfg6 = arsr1p_param->sharp_cfg6;
	post_scf->sharp_cfg7 = arsr1p_param->sharp_cfg7;
	post_scf->sharp_cfg8 = arsr1p_param->sharp_cfg8;
	post_scf->sharp_cfg9 = arsr1p_param->sharp_cfg9;
	post_scf->sharp_cfg10 = arsr1p_param->sharp_cfg10;
	post_scf->sharp_cfg11 = arsr1p_param->sharp_cfg11;

	post_scf->diff_ctrl = arsr1p_param->diff_ctrl;
	post_scf->lsc_cfg1 = arsr1p_param->lsc_cfg1;
	post_scf->lsc_cfg2 = arsr1p_param->lsc_cfg2;
	post_scf->lsc_cfg3 = arsr1p_param->lsc_cfg3;

	return;
}

static int set_arsr1p_param(struct hisi_fb_data_type *hisifd, dss_arsr1p_t *post_scf, struct arsr1p_info *arsr1p_param,
	dss_rect_t src_rect, dss_rect_t dst_rect)
{
	int32_t ihinc = 0;
	int32_t ivinc = 0;
	int32_t ihleft;
	int32_t ihright;
	int32_t ihleft1;
	int32_t ihright1;
	int32_t ivtop;
	int32_t ivbottom;
	int32_t extraw;
	int32_t extraw_left;
	int32_t extraw_right;

	ihinc = (int32_t)(ARSR1P_INC_FACTOR * (uint64_t)src_rect.w / dst_rect.w);
	ivinc = (int32_t)(ARSR1P_INC_FACTOR * (uint64_t)src_rect.h / dst_rect.h);

	if (hisifd == NULL) {
		HISI_FB_DEBUG("hisifd is NULL!\n");
		return -1;
	}

	if (post_scf == NULL) {
		HISI_FB_DEBUG("post_scf is NULL!\n");
		return -1;
	}

	if (arsr1p_param == NULL) {
		HISI_FB_DEBUG("arsr1p_param is NULL!\n");
		return -1;
	}

	// 0x2000<=ihinc<=ARSR1P_INC_FACTOR; 0x2000<=ivinc<=ARSR1P_INC_FACTOR;
	if ((ihinc < 0x2000) || (ihinc > ARSR1P_INC_FACTOR)
		|| (ivinc < 0x2000) || (ivinc > ARSR1P_INC_FACTOR)) {
		HISI_FB_ERR("invalid ihinc(0x%x), ivinc(0x%x)!\n", ihinc, ivinc);
		return -EINVAL;
	}

	extraw = (8 * ARSR1P_INC_FACTOR) / ihinc;
	extraw_left = (extraw % 2) ? (extraw + 1) : (extraw);
	extraw = (2 * ARSR1P_INC_FACTOR) / ihinc;
	extraw_right = (extraw % 2) ? (extraw + 1) : (extraw);

	//ihleft1 = (startX_o * ihinc) - (ov_startX0 << 16)
	ihleft1 = dst_rect.x * ihinc - src_rect.x * ARSR1P_INC_FACTOR;
	if (ihleft1 < 0) ihleft1 = 0;
	//ihleft = ihleft1 - even(8 * 65536 / ihinc) * ihinc;
	ihleft = ihleft1 - extraw_left * ihinc;
	if (ihleft < 0) ihleft = 0;

	//ihright1 = ihleft1 + (oww-1) * ihinc
	ihright1 = ihleft1 + (dst_rect.w - 1) * ihinc;
	//ihright = ihright1 + even(2 * 65536/ihinc) * ihinc
	ihright = ihright1 + extraw_right * ihinc;
	//ihright >= img_width * ihinc
	if (ihright >= src_rect.w * ARSR1P_INC_FACTOR)
		ihright = src_rect.w * ARSR1P_INC_FACTOR - 1;

	//ivtop = (startY_o * ivinc) - (ov_startY0<<16)
	ivtop = dst_rect.y * ivinc - src_rect.y * ARSR1P_INC_FACTOR;
	if (ivtop < 0) ivtop = 0;
	//ivbottom = ivtop + (ohh - 1) * ivinc
	ivbottom = ivtop + (dst_rect.h - 1) * ivinc;
	//ivbottom >= img_height * ivinc
	if (ivbottom >= src_rect.h * ARSR1P_INC_FACTOR)
		ivbottom = src_rect.h * ARSR1P_INC_FACTOR - 1;

	//(ihleft1 - ihleft) % (ihinc) == 0;
	if ((ihleft1 - ihleft) % (ihinc)) {
		HISI_FB_ERR("(ihleft1(%d)-ihleft(%d))  ihinc(%d) != 0, invalid!\n",
			ihleft1, ihleft, ihinc);
		post_scf->mode = 0x1;
		return -EINVAL;
	}

	//(ihright1 - ihleft1) % ihinc == 0;
	if ((ihright1 - ihleft1) % ihinc) {
		HISI_FB_ERR("(ihright1(%d)-ihleft1(%d))  ihinc(%d) != 0, invalid!\n",
			ihright1, ihleft1, ihinc);
		post_scf->mode = 0x1;
		return -EINVAL;
	}

	post_scf->mode &= 0xFFFFFFFE; //cancel arsr1p bypass
	post_scf->mode |= 0xc;//skinctrl, shootdetect
	post_scf->mode |= 0x20;//enable direction

	if ((ihinc < ARSR1P_INC_FACTOR) || (ivinc < ARSR1P_INC_FACTOR)) {
		post_scf->mode |= 0x10;//enable diintplen
	} else {
		post_scf->mode |= 0x40;//only sharp, enable nointplen
	}

	hisifd->dss_module.post_scf_used = 1;

	post_scf->ihleft = set_bits32(post_scf->ihleft, ihleft, 32, 0);
	post_scf->ihright = set_bits32(post_scf->ihright, ihright, 32, 0);
	post_scf->ihleft1 = set_bits32(post_scf->ihleft1, ihleft1, 32, 0);
	post_scf->ihright1 = set_bits32(post_scf->ihright1, ihright1, 32, 0);
	post_scf->ivtop = set_bits32(post_scf->ivtop, ivtop, 32, 0);
	post_scf->ivbottom = set_bits32(post_scf->ivbottom, ivbottom, 32, 0);
	post_scf->ihinc = set_bits32(post_scf->ihinc, ihinc, 32, 0);
	post_scf->ivinc = set_bits32(post_scf->ivinc, ivinc, 32, 0);

	hisi_effect_update_arsr1p_param(post_scf, arsr1p_param);

	post_scf->dpp_img_size_bef_sr = set_bits32(post_scf->dpp_img_size_bef_sr,
		(DSS_HEIGHT((uint32_t)src_rect.h) << 16) | DSS_WIDTH((uint32_t)src_rect.w), 32, 0);
	post_scf->dpp_img_size_aft_sr = set_bits32(post_scf->dpp_img_size_aft_sr,
		(DSS_HEIGHT((uint32_t)dst_rect.h) << 16) | DSS_WIDTH((uint32_t)dst_rect.w), 32, 0);
	post_scf->dpp_used = 1;

	return 0;
}

static bool hisi_effect_update_rog_param(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req) {
	struct hisi_panel_info *pinfo = NULL;
	dss_arsr1p_t *post_scf = NULL;

	if ((NULL == hisifd) || (NULL == pov_req)) {
		HISI_FB_ERR("hisifd or pov_req is NULL!");
		return false;
	}

	pinfo = &(hisifd->panel_info);
	post_scf = &(hisifd->dss_module.post_scf);

	if ((pov_req->res_updt_rect.w != pinfo->xres)
		||(pov_req->res_updt_rect.h != pinfo->yres)) {
		struct arsr1p_info *arsr1p_rog = &(hisifd->effect_info.arsr1p[1]);
		if (!hisifd->effect_info.arsr1p_rog_initialized) {
			arsr1p_rog_init(arsr1p_rog);
			hisifd->effect_info.arsr1p_rog_initialized = true;
		}
		post_scf->skin_thres_y = arsr1p_rog->skin_thres_y;
		post_scf->skin_thres_u = arsr1p_rog->skin_thres_u;
		post_scf->skin_thres_v = arsr1p_rog->skin_thres_v;
		post_scf->skin_expected = arsr1p_rog->skin_expected;
		post_scf->skin_cfg = arsr1p_rog->skin_cfg;

		post_scf->shoot_cfg1 = arsr1p_rog->shoot_cfg1;
		post_scf->shoot_cfg2 = arsr1p_rog->shoot_cfg2;

		post_scf->sharp_cfg1 = arsr1p_rog->sharp_cfg1;
		post_scf->sharp_cfg2 = arsr1p_rog->sharp_cfg2;
		post_scf->sharp_cfg3 = arsr1p_rog->sharp_cfg3;
		post_scf->sharp_cfg4 = arsr1p_rog->sharp_cfg4;
		post_scf->sharp_cfg5 = arsr1p_rog->sharp_cfg5;
		post_scf->sharp_cfg6 = arsr1p_rog->sharp_cfg6;
		post_scf->sharp_cfg7 = arsr1p_rog->sharp_cfg7;
		post_scf->sharp_cfg8 = arsr1p_rog->sharp_cfg8;
		post_scf->sharp_cfg9 = arsr1p_rog->sharp_cfg9;
		post_scf->sharp_cfg10 = arsr1p_rog->sharp_cfg10;
		post_scf->sharp_cfg11 = arsr1p_rog->sharp_cfg11;

		post_scf->diff_ctrl = arsr1p_rog->diff_ctrl;
		post_scf->lsc_cfg1 = arsr1p_rog->lsc_cfg1;
		post_scf->lsc_cfg2 = arsr1p_rog->lsc_cfg2;
		post_scf->lsc_cfg3 = arsr1p_rog->lsc_cfg3;

		return true;
	}

	return false;
}

int hisi_effect_arsr1p_config(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	struct arsr1p_info *arsr1p_param = NULL;
	struct hisi_panel_info *pinfo = NULL;
	dss_rect_t src_rect = {0};
	dss_rect_t dst_rect = {0};
	dss_arsr1p_t *post_scf = NULL;
	int ret = 0;

	if ((NULL == hisifd) || (NULL == pov_req)) {
		HISI_FB_ERR("hisifd or pov_req is NULL!");
		return 0;
	}

	if (PRIMARY_PANEL_IDX != hisifd->index) {
		HISI_FB_DEBUG("hisifd %d not surpport!",hisifd->index);
		return 0;
	}

	if ((pov_req->res_updt_rect.w < 0) || (pov_req->res_updt_rect.h < 0)) {
		HISI_FB_ERR("fb%d, res_updt_rect[%d,%d, %d,%d] is invalid!\n", hisifd->index,
			pov_req->res_updt_rect.x, pov_req->res_updt_rect.y,
			pov_req->res_updt_rect.w, pov_req->res_updt_rect.h);
		return -EINVAL;
	}

	if (!spin_can_lock(&hisifd->effect_lock)) {
		HISI_FB_DEBUG("fb%d, dss effect param is being updated, delay set reg to next frame!\n", hisifd->index);
		return 0;
	}

	spin_lock(&hisifd->effect_lock);

	pinfo = &(hisifd->panel_info);

	if (!hisifd->effect_ctl.arsr1p_sharp_support) {
		goto err_return;
	}

	if ((!hisifd->effect_updated_flag.arsr1p_effect_updated) &&
		((hisifd->panel_info.dirty_region_updt_support == 0) ||
		((pov_req->dirty_rect.x == hisifd->dirty_region_updt.x)
		&& (pov_req->dirty_rect.y == hisifd->dirty_region_updt.y)
		&& (pov_req->dirty_rect.w == hisifd->dirty_region_updt.w)
		&& (pov_req->dirty_rect.h == hisifd->dirty_region_updt.h)))) {
		goto err_return;
	}

	post_scf = &(hisifd->dss_module.post_scf);

	if (pov_req->dirty_rect.x == 0 && pov_req->dirty_rect.y == 0 &&
		pov_req->dirty_rect.w == 0 && pov_req->dirty_rect.h == 0) {
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.w = pinfo->xres;
		dst_rect.h = pinfo->yres;
	} else {
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.w = pov_req->dirty_rect.w;
		dst_rect.h = pov_req->dirty_rect.h;
	}

	if (((pov_req->dirty_rect.w > 0) && (pov_req->dirty_rect.h > 0)) ||
		((pov_req->res_updt_rect.w == 0) || (pov_req->res_updt_rect.h == 0))) {
		src_rect = dst_rect;
	} else {
		src_rect = pov_req->res_updt_rect;
	}

	if ((src_rect.w < ARSR1P_MIN_SIZE) || (src_rect.h < ARSR1P_MIN_SIZE)
		|| (src_rect.w > ARSR1P_MAX_SRC_WIDTH_SIZE) || (src_rect.h > ARSR1P_MAX_SIZE)
		|| (dst_rect.w > ARSR1P_MAX_SIZE) || (dst_rect.h > ARSR1P_MAX_SIZE)) {
		HISI_FB_ERR("fb%d, invalid input size: src_rect(%d,%d,%d,%d) should be larger than 16*16, less than 3840*8192!\n"
			"invalid output size: dst_rect(%d,%d,%d,%d) should be less than 8192*8192!\n",
			hisifd->index,
			src_rect.x, src_rect.y, src_rect.w, src_rect.h,
			dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
		// bypass
		post_scf->mode = 0x1;
		ret = -EINVAL;
		goto err_return;
	}

	post_scf->mode |= 0x2;

	//rog param
	if (hisi_effect_update_rog_param(hisifd, pov_req)) {
		spin_unlock(&hisifd->effect_lock);
		return 0;
	}

	//normal param
	arsr1p_param = &(hisifd->effect_info.arsr1p[0]);
	if (set_arsr1p_param(hisifd, post_scf, arsr1p_param, src_rect, dst_rect)) {
		HISI_FB_ERR("fb%d, failed to set arsr1p param.\n", hisifd->index);
		ret = -EINVAL;
		goto err_return;
	}

	hisifd->effect_updated_flag.arsr1p_effect_updated = false;

err_return:
	spin_unlock(&hisifd->effect_lock);
	return ret;
}

static void hisi_effect_arsr2p_update(struct hisi_fb_data_type *hisifd, struct dss_arsr2p *arsr2p) {
	if (hisifd == NULL || arsr2p == NULL) {
		HISI_FB_ERR("hisifd or arsr2p is null. return\n");
		return;
	}

	if (hisifd->effect_info.arsr2p[0].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect), &(hisifd->effect_info.arsr2p[0]), sizeof(struct arsr2p_info));
		hisifd->effect_info.arsr2p[0].update = 0;
	}

	if (hisifd->effect_info.arsr2p[1].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_up), &(hisifd->effect_info.arsr2p[1]), sizeof(struct arsr2p_info));
		hisifd->effect_info.arsr2p[1].update = 0;
	}

	if (hisifd->effect_info.arsr2p[2].update == 1) {
		memcpy(&(arsr2p->arsr2p_effect_scale_down), &(hisifd->effect_info.arsr2p[2]), sizeof(struct arsr2p_info));
		hisifd->effect_info.arsr2p[2].update = 0;
	}
}


int hisi_effect_arsr2p_config(struct arsr2p_info *arsr2p_effect_dst, int ih_inc, int iv_inc)
{
	struct hisi_fb_data_type *hisifd_primary = NULL;
	struct dss_arsr2p *arsr2p = NULL;

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if (NULL == hisifd_primary) {
		HISI_FB_ERR("hisifd_primary is NULL pointer, return!\n");
		return -EINVAL;
	}

	if (NULL == arsr2p_effect_dst) {
		HISI_FB_ERR("arsr2p_effect_dst is NULL pointer, return!\n");
		return -EINVAL;
	}

	arsr2p = &(hisifd_primary->dss_module_default.arsr2p[DSS_RCHN_V0]);
	hisi_effect_arsr2p_update(hisifd_primary, arsr2p);

	if ((ih_inc == ARSR2P_INC_FACTOR) && (iv_inc == ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect), sizeof(struct arsr2p_info));
	} else if ((ih_inc < ARSR2P_INC_FACTOR) || (iv_inc < ARSR2P_INC_FACTOR)) {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect_scale_up), sizeof(struct arsr2p_info));
	} else {
		memcpy(arsr2p_effect_dst, &(arsr2p->arsr2p_effect_scale_down), sizeof(struct arsr2p_info));
	}

	return 0;
}
int hisi_effect_hiace_config(struct hisi_fb_data_type *hisifd) {
	return set_hiace_param(hisifd);
}
/*lint +e571, +e573, +e737, +e732, +e850, +e730, +e713, +e529, +e574, +e679, +e732, +e845, +e570, +e774*/
//lint +e747, +e838
