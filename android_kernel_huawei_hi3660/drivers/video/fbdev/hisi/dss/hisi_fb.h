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
#ifndef HISI_FB_H
#define HISI_FB_H

/*lint -e551 -e551*/
#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/raid/pq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/backlight.h>
#include <linux/pwm.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/syscalls.h>

#include <linux/spi/spi.h>

#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>
#include <linux/hisi/hisi-iommu.h>

#include <linux/atomic.h>

/*lint +e551 +e551*/
//#include <linux/huawei/hisi_irq_affinity.h>
#include "hisifb_ion.h"
#include "hisi_fb_def.h"
#include "hisi_fb_panel.h"
#include "hisi_fb_debug.h"
#include "hisi_dss.h"
#include "hisi_mipi_dsi.h"
#include <soc_dss_interface.h>
#include "hisi_overlay_cmdlist_utils.h"
#include "hisi_display_effect.h"

#if defined (CONFIG_HISI_FB_3650)
#include "hisi_overlay_utils_hi3650.h"
#elif defined(CONFIG_HISI_FB_6250)
#include "hisi_overlay_utils_hi6250.h"
#elif defined(CONFIG_HISI_FB_3660)
#include "hisi_overlay_utils_hi3660.h"
#elif defined(CONFIG_HISI_FB_970)
#include "hisi_overlay_utils_kirin970.h"
#elif defined(CONFIG_HISI_FB_V501)
#include "hisi_overlay_utils_dssv501.h"
#include "hisi_dpe_pipe_clk_utils.h"
#elif defined(CONFIG_HISI_FB_V320)
#include "hisi_overlay_utils_dssv320.h"
#elif defined(CONFIG_HISI_FB_V510)
#include "hisi_overlay_utils_dssv510.h"
#elif defined(CONFIG_HISI_FB_V330)
#include "hisi_overlay_utils_dssv330.h"
#endif

#include "hisi_dpe_utils.h"
#include "hisi_overlay_utils.h"
#include "hisi_fb_video_idle.h"
#include "hisi_ovl_online_wb.h"
#include "hisi_dss_sync.h"

#ifndef CONFIG_SYNC_FILE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
#include "sync.h"
#include "sw_sync.h"
#else
#include <linux/sync.h>
#include <linux/sw_sync.h>
#endif
#endif

//#define CONFIG_HISI_FB_COLORBAR_USED
//#define CONFIG_HISI_FB_DPP_COLORBAR_USED
//#define CONFIG_HISI_FB_LDI_COLORBAR_USED
//#define CONFIG_HISI_FB_OV_BASE_USED
//#define CONFIG_FPGA_SDP_TEST

#define CONFIG_HISI_FB_BACKLIGHT_DELAY
//#define CONFIG_HISI_FB_HEAP_CARVEOUT_USED
//#define CONFIG_FB_DEBUG_USED

#define CONFIG_BACKLIGHT_2048

#define HISI_DSS_COMPOSER_HOLD_TIME	(1000 * 3600 * 24 * 7)

#define HISI_FB0_NUM	(3)
#define HISI_FB1_NUM	(0)
#define HISI_FB2_NUM	(0)
#define HISI_FB3_NUM	(0)

#define HISI_FB_SYSFS_ATTRS_NUM	(64)

#define HISI_FB_MAX_DEV_LIST (32)
#define HISI_FB_MAX_FBI_LIST (32)

#define HISI_DSS_OFFLINE_MAX_NUM	(2)
#define HISI_DSS_OFFLINE_MAX_LIST	(128)
#define ONLINE_PLAY_LOG_PRINTF   (10)
#define BACKLIGHT_LOG_PRINTF   (16)


//esd check period-->5000ms
#define ESD_CHECK_TIME_PERIOD	(5000)

#define DSM_CREATE_FENCE_FAIL_EXPIRE_COUNT (6)

struct hisifb_vsync {
	wait_queue_head_t vsync_wait;
	ktime_t vsync_timestamp;
	ktime_t vsync_timestamp_prev;
	ktime_t vactive_timestamp;
	int vsync_created;
	/* flag for soft vsync signal synchronizing with TE*/
	int vsync_enabled;
	int vsync_infinite;
	int vsync_infinite_count;

	int vsync_ctrl_expire_count;
	int vsync_ctrl_enabled;
	int vsync_ctrl_disabled_set;
	int vsync_ctrl_isr_enabled;
	int vsync_ctrl_offline_enabled;
	int vsync_disable_enter_idle;
	struct work_struct vsync_ctrl_work;
	spinlock_t spin_lock;

	struct mutex vsync_lock;

	atomic_t buffer_updated;
	void (*vsync_report_fnc) (int buffer_updated);

	struct hisi_fb_data_type *hisifd;
};

struct dss_comm_mmbuf_info {
    int ov_idx;
    dss_mmbuf_t mmbuf;
};

enum dss_sec_event {
	DSS_SEC_DISABLE = 0,
	DSS_SEC_ENABLE,
};

enum dss_sec_status {
	DSS_SEC_IDLE = 0,
	DSS_SEC_RUNNING,
};

enum bl_control_mode {
	REG_ONLY_MODE = 1,
	PWM_ONLY_MODE,
	MUTI_THEN_RAMP_MODE,
	RAMP_THEN_MUTI_MODE,
	I2C_ONLY_MODE = 6,
	BLPWM_AND_CABC_MODE,
	COMMON_IC_MODE = 8,
	AMOLED_NO_BL_IC_MODE = 9,
	BLPWM_MODE = 10,
};

enum ESD_RECOVER_STATE {
	ESD_RECOVER_STATE_NONE = 0,
	ESD_RECOVER_STATE_START = 1,
	ESD_RECOVER_STATE_COMPLETE = 2,
};

struct hisifb_secure {
	uint32_t secure_created;
	uint32_t secure_status;
	uint32_t secure_event;
	uint32_t secure_blank_flag;
	uint32_t tui_need_switch;
	uint32_t tui_need_skip_report;
	bool have_set_backlight;

	struct work_struct secure_ctrl_work;

	void (*secure_layer_config) (struct hisi_fb_data_type *hisifd, int32_t chn_idx);
	void (*secure_layer_deconfig) (struct hisi_fb_data_type *hisifd, int32_t chn_idx);
	void (*notify_secure_switch) (struct hisi_fb_data_type *hisifd);
	void (*set_reg) (uint32_t addr, uint32_t val, uint8_t bw, uint8_t bs);
#if defined(CONFIG_HISI_FB_970) || defined (CONFIG_HISI_FB_V501) || defined (CONFIG_HISI_FB_V510)
	void (*hdcp13_enable)(uint32_t en);
	void (*hdcp22_enable)(uint32_t en);
	void (*hdcp13_encrypt_enable)(uint32_t en);
	void (*hdcp_dpc_sec_en)(void);
	void (*hdcp_obs_set)(uint32_t reg);
	void (*hdcp_int_clr)(uint32_t reg);
	void (*hdcp_int_mask)(uint32_t reg);
	void (*hdcp_cp_irq)(void);
	int (*hdcp_reg_get)(uint32_t addr);
	void (*hdcp_enc_mode)(uint32_t en);
#endif
	struct hisi_fb_data_type *hisifd;
};



/*******************************************************************************
**  mediacomm channel manager struct config
*/
#define MAX_MDC_CHANNEL  (3)
enum {
	FREE = 0,
	HWC_USED = 1,
	MDC_USED = 2,
};

typedef struct mdc_chn_info {
	unsigned int status;
	unsigned int drm_used;
	unsigned int cap_available;
	unsigned int rch_idx;
	unsigned int wch_idx;
	unsigned int ovl_idx;
	unsigned int wb_composer_type;
} mdc_chn_info_t;

typedef struct mdc_func_ops {
	unsigned int chan_num;
	struct semaphore mdc_req_sem;
	struct semaphore mdc_rel_sem;
	struct task_struct *refresh_handle_thread;
	wait_queue_head_t refresh_handle_wait;

	mdc_chn_info_t mdc_channel[MAX_MDC_CHANNEL];

	int (* chn_request_handle)(struct hisi_fb_data_type *hisifd, mdc_ch_info_t *chn_info);
	int (* chn_release_handle)(struct hisi_fb_data_type *hisifd, mdc_ch_info_t *chn_info);
} mdc_func_ops_t;

int hisi_mdc_resource_init(struct hisi_fb_data_type *hisifd, unsigned int platform);
int hisi_mdc_chn_request(struct fb_info *info, void __user *argp);
int hisi_mdc_chn_release(struct fb_info *info, void __user *argp);

#if defined (CONFIG_HISI_FB_970) || defined (CONFIG_HISI_FB_V501) || defined (CONFIG_HISI_FB_V510)
#define MDC_RCHN_V  (DSS_RCHN_V0)
#define MDC_WCHN_W  (DSS_WCHN_W1)
#define MDC_OVL  (DSS_OVL3)
int hisi_mdc_power_ctrl(struct fb_info *info, void __user *argp);
void hisi_mdc_mif_on(struct hisi_fb_data_type *hisifd);
int hisi_mdc_scl_coef_on(struct hisi_fb_data_type *hisifd, bool enable_cmdlist, int coef_lut_idx);
int hisi_ov_media_common_play(struct hisi_fb_data_type *hisifd, void __user *argp);
#endif
/*******************************************************************************/

/* esd func define */
struct hisifb_esd {
	int esd_inited;
	struct hrtimer esd_hrtimer;
	struct workqueue_struct *esd_check_wq;
	struct work_struct esd_check_work;

	struct hisi_fb_data_type *hisifd;
};

struct hisifb_pipe_clk {
	uint64_t pxl0_ppll_rate;//need config in panel init.
	uint64_t pipe_clk_rate;
	uint32_t pipe_clk_updt_hporch[3];
	uint32_t fps_updt_hporch[3];
	uint32_t hporch_pre_set[3];
	uint32_t pipe_clk_rate_div;
	uint32_t div_pre_set;
	uint8_t  pipe_clk_updt_state;
	uint8_t  pipe_clk_updt_times;
	uint8_t inited;
	uint8_t underflow_int;
	uint8_t dirty_region_updt_disable;
	uint8_t fullhdplus;
	uint8_t reserved[2];

	struct workqueue_struct *pipe_clk_handle_wq;
	struct work_struct pipe_clk_handle_work;
	struct hisi_fb_data_type *hisifd;
};

struct hisifb_buf_sync {
	char *fence_name;
	struct hisi_dss_timeline *timeline;
	struct hisi_dss_timeline *timeline_retire;

	int timeline_max;
	int threshold;
	int retire_threshold;
	int refresh;
	spinlock_t refresh_lock;

	struct workqueue_struct *free_layerbuf_queue;
	struct work_struct free_layerbuf_work;
	struct list_head layerbuf_list;
	bool layerbuf_flushed;
	spinlock_t layerbuf_spinlock;
	struct semaphore layerbuf_sem;
};

struct hisifb_layerbuf {
	struct ion_handle *ion_handle;
	struct list_head list_node;
	int timeline;
	bool has_map_iommu;

	int32_t shared_fd;
	uint32_t frame_no;
	dss_mmbuf_t mmbuf;
	uint64_t vir_addr;
	int32_t chn_idx;
};

struct hisifb_backlight {
#ifdef CONFIG_HISI_FB_BACKLIGHT_DELAY
	struct delayed_work bl_worker;
#endif
	struct semaphore bl_sem;
	int bl_updated;
	int bl_level_old;
	int frame_updated;

	struct workqueue_struct *sbl_queue;
	struct work_struct sbl_work;
	ktime_t bl_timestamp;
};

struct hisi_fb_data_type {
	uint32_t index;
	uint32_t ref_cnt;
	uint32_t fb_num;
	uint32_t fb_imgType;
	uint32_t bl_level;

	char __iomem *dss_base;
	char __iomem *peri_crg_base;
	char __iomem *sctrl_base;
	char __iomem *pctrl_base;
	char __iomem *noc_dss_base;
	char __iomem *mmbuf_crg_base;
	char __iomem *pmctrl_base;

	char __iomem *media_crg_base;
	char __iomem *media_common_base;
	char __iomem *dp_base;

	char __iomem *mmbuf_asc0_base;
	char __iomem *mipi_dsi0_base;
	char __iomem *mipi_dsi1_base;
	uint32_t dss_base_phy;

	uint32_t dpe_irq;
	uint32_t dsi0_irq;
	uint32_t dsi1_irq;
	uint32_t dp_irq;
	uint32_t mmbuf_asc0_irq;

	struct regulator_bulk_data *dpe_regulator;
	struct regulator_bulk_data *mediacrg_regulator;
	struct regulator_bulk_data *mmbuf_regulator;

	const char *dss_axi_clk_name;
	const char *dss_pclk_dss_name;
	const char *dss_pri_clk_name;
	const char *dss_pxl0_clk_name;
	const char *dss_pxl1_clk_name;
	const char *dss_mmbuf_clk_name;
	const char *dss_pclk_mmbuf_name;
	const char *dss_dphy0_ref_clk_name;
	const char *dss_dphy1_ref_clk_name;
	const char *dss_dphy0_cfg_clk_name;
	const char *dss_dphy1_cfg_clk_name;
	const char *dss_pclk_dsi0_name;
	const char *dss_pclk_dsi1_name;
	const char *dss_pclk_pctrl_name;
	const char *dss_auxclk_dpctrl_name;
	const char *dss_pclk_dpctrl_name;
	const char *dss_aclk_dpctrl_name;

	struct clk *dss_axi_clk;
	struct clk *dss_pclk_dss_clk;
	struct clk *dss_pri_clk;
	struct clk *dss_pxl0_clk;
	struct clk *dss_pxl1_clk;
	struct clk *dss_mmbuf_clk;
	struct clk *dss_pclk_mmbuf_clk;
	struct clk *dss_dphy0_ref_clk;
	struct clk *dss_dphy1_ref_clk;
	struct clk *dss_dphy0_cfg_clk;
	struct clk *dss_dphy1_cfg_clk;
	struct clk *dss_pclk_dsi0_clk;
	struct clk *dss_pclk_dsi1_clk;
	struct clk *dss_pclk_pctrl_clk;
	struct clk *dss_auxclk_dpctrl_clk;
	struct clk *dss_pclk_dpctrl_clk;
	struct clk *dss_aclk_dpctrl_clk;
	struct clk *dss_clk_media_common_clk;

	struct hisi_panel_info panel_info;
	bool panel_power_on;
	bool fb_shutdown;
	bool need_tuning_clk;
	bool lcd_self_testing;
	bool enable_fast_unblank;
	unsigned int aod_function;
	unsigned int aod_mode;
	unsigned int vr_mode;
	unsigned int mask_layer_xcc_flag;
	atomic_t atomic_v;

	struct semaphore blank_sem;
	struct semaphore blank_sem0;
	struct semaphore blank_sem_effect;
	struct semaphore brightness_esd_sem;
	struct semaphore power_esd_sem;
	struct semaphore offline_composer_sr_sem;
	struct semaphore fast_unblank_sem;
#if defined(CONFIG_HISI_FB_3660) || defined (CONFIG_HISI_FB_V320)
	struct semaphore hiace_clear_sem;
#endif
#if defined (CONFIG_HISI_FB_V320) || defined (CONFIG_HISI_FB_970) || defined (CONFIG_HISI_FB_V501) || defined (CONFIG_HISI_FB_V510)
	struct semaphore hiace_hist_lock_sem;
#endif

	uint32_t offline_composer_sr_refcount;

	void (*sysfs_attrs_append_fnc) (struct hisi_fb_data_type *hisifd, struct attribute *attr);
	int (*sysfs_create_fnc) (struct platform_device *pdev);
	void (*sysfs_remove_fnc) (struct platform_device *pdev);
	void (*pm_runtime_register) (struct platform_device *pdev);
	void (*pm_runtime_unregister) (struct platform_device *pdev);
	void (*pm_runtime_get) (struct hisi_fb_data_type *hisifd);
	void (*pm_runtime_put) (struct hisi_fb_data_type *hisifd);
	void (*bl_register) (struct platform_device *pdev);
	void (*bl_unregister) (struct platform_device *pdev);
	void (*bl_update) (struct hisi_fb_data_type *hisifd);
	void (*bl_cancel) (struct hisi_fb_data_type *hisifd);
	void (*vsync_register) (struct platform_device *pdev);
	void (*vsync_unregister) (struct platform_device *pdev);
	int (*vsync_ctrl_fnc) (struct fb_info *info, void __user *argp);
	void (*vsync_isr_handler) (struct hisi_fb_data_type *hisifd);
	void (*secure_register) (struct platform_device *pdev);
	void (*secure_unregister) (struct platform_device *pdev);
	void (*buf_sync_register) (struct platform_device *pdev);
	void (*buf_sync_unregister) (struct platform_device *pdev);
	void (*buf_sync_signal) (struct hisi_fb_data_type *hisifd);
	void (*buf_sync_suspend) (struct hisi_fb_data_type *hisifd);
	void (*esd_register) (struct platform_device *pdev);
	void (*esd_unregister) (struct platform_device *pdev);
	void (*debug_register) (struct platform_device *pdev);
	void (*debug_unregister) (struct platform_device *pdev);
	int (*cabc_update) (struct hisi_fb_data_type *hisifd);
	void (*pipe_clk_updt_isr_handler) (struct hisi_fb_data_type *hisifd);

	bool (*set_fastboot_fnc) (struct fb_info *info);
	int (*open_sub_fnc) (struct fb_info *info);
	int (*release_sub_fnc) (struct fb_info *info);
	int (*hpd_open_sub_fnc) (struct fb_info *info);
	int (*hpd_release_sub_fnc) (struct fb_info *info);
	int (*on_fnc) (struct hisi_fb_data_type *hisifd);
	int (*off_fnc) (struct hisi_fb_data_type *hisifd);
	int (*lp_fnc) (struct hisi_fb_data_type *hisifd, bool lp_enter);
	int (*esd_fnc) (struct hisi_fb_data_type *hisifd);
	int (*sbl_ctrl_fnc) (struct fb_info *info, int value);
	void (*sbl_isr_handler)(struct hisi_fb_data_type *hisifd);
	int (*fps_upt_isr_handler) (struct hisi_fb_data_type *hisifd);
	int (*mipi_dsi_bit_clk_upt_isr_handler) (struct hisi_fb_data_type *hisifd);
	void (*panel_mode_switch_isr_handler) (struct hisi_fb_data_type *hisifd, uint8_t mode_switch_to);
	void (*crc_isr_handler)(struct hisi_fb_data_type *hisifd);
	void (*ov_ldi_underflow_isr_handle)(struct hisi_fb_data_type *hisifd);

	int (*pan_display_fnc) (struct hisi_fb_data_type *hisifd);
	int (*ov_ioctl_handler) (struct hisi_fb_data_type *hisifd, uint32_t cmd, void __user *argp);
	int (*display_effect_ioctl_handler) (struct hisi_fb_data_type *hisifd, unsigned int cmd, void __user *argp);
	int (*ov_online_play) (struct hisi_fb_data_type *hisifd, void __user *argp);
	int (*ov_offline_play) (struct hisi_fb_data_type *hisifd, void __user *argp);
	int (*ov_copybit_play) (struct hisi_fb_data_type *hisifd, void __user *argp);
	int (*ov_media_common_play) (struct hisi_fb_data_type *hisifd, void __user *argp);
	void (*ov_wb_isr_handler) (struct hisi_fb_data_type *hisifd);
	void (*ov_vactive0_start_isr_handler) (struct hisi_fb_data_type *hisifd);
	void (*set_reg) (struct hisi_fb_data_type *hisifd,
		char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);

	void (*sysfs_attrs_add_fnc)(struct hisi_fb_data_type*hisifd);

	int (*dp_device_srs)(struct hisi_fb_data_type *hisifd, bool blank);
	int (*dp_get_color_bit_mode) (struct hisi_fb_data_type *hisifd, void __user *argp);
	int (*dp_get_source_mode) (struct hisi_fb_data_type *hisifd, void __user *argp);
	int (*dp_pxl_ppll7_init)(struct hisi_fb_data_type *hisifd, u64 pixel_clock);

	void (*video_idle_ctrl_register) (struct platform_device *pdev);
	void (*video_idle_ctrl_unregister) (struct platform_device *pdev);

	void (*overlay_online_wb_register) (struct platform_device *pdev);
	void (*overlay_online_wb_unregister) (struct platform_device *pdev);

	int (*dp_wakeup)(struct hisi_fb_data_type *hisifd);

	struct hisifb_backlight backlight;
	int sbl_enable;
	uint32_t sbl_lsensor_value;
	int sbl_level;
	dss_sbl_t sbl;
	int sre_enable;

	int color_temperature_flag;
	int display_effect_flag;
	int xcc_coef_set;
	dss_display_effect_al_t al_ctrl;
	dss_display_effect_ce_t ce_ctrl;
	dss_display_effect_bl_t bl_ctrl;
	dss_display_effect_bl_enable_t bl_enable_ctrl;
	dss_display_effect_sre_t sre_ctrl;
	dss_display_effect_metadata_t metadata_ctrl;
	dss_ce_info_t acm_ce_info;
	dss_ce_info_t prefix_ce_info[DSS_CHN_MAX_DEFINE];
	display_engine_info_t de_info;
	display_engine_param_t de_param;
	int user_scene_mode;
	int dimming_count;
	acm_reg_t acm_reg;
#if !defined(CONFIG_HISI_FB_3650) && !defined (CONFIG_HISI_FB_6250)
	dss_ce_info_t hiace_info;
	dss_gm_t dynamic_gamma_info;
	char blc_last_bl_level;
	char reserved[3];
#endif
	bool dirty_region_updt_enable;
	uint32_t  online_play_count;

	spinlock_t effect_lock;
	struct dss_module_update effect_updated_flag;
	struct dss_effect effect_ctl;
	struct dss_effect_info effect_info;
#if defined (CONFIG_HISI_FB_V501) || defined (CONFIG_HISI_FB_V510)
	bool effect_gmp_update_flag;
#endif

	int sysfs_index;
	struct attribute *sysfs_attrs[HISI_FB_SYSFS_ATTRS_NUM];
	struct attribute_group sysfs_attr_group;

	struct hisifb_vsync vsync_ctrl;
	struct hisifb_buf_sync buf_sync_ctrl;
	struct hisifb_video_idle_ctrl video_idle_ctrl;
	struct dss_vote_cmd dss_vote_cmd;
	struct hisifb_writeback wb_ctrl;
	struct hisifb_secure secure_ctrl;
	struct hisifb_esd esd_ctrl;
	struct hisifb_pipe_clk pipe_clk_ctrl;
	struct dp_ctrl dp;
	bool need_refresh;
	mdc_func_ops_t mdc_ops;

	dss_module_reg_t dss_module;
	dss_overlay_t ov_req;
	dss_overlay_block_t ov_block_infos[HISI_DSS_OV_BLOCK_NUMS];
	dss_overlay_t ov_req_prev;
	dss_overlay_block_t ov_block_infos_prev[HISI_DSS_OV_BLOCK_NUMS];
	dss_overlay_t ov_req_prev_prev;
	dss_overlay_block_t ov_block_infos_prev_prev[HISI_DSS_OV_BLOCK_NUMS];

	dss_rect_t *ov_block_rects[HISI_DSS_CMDLIST_BLOCK_MAX];
	dss_wb_info_t wb_info;

	dss_cmdlist_data_t *cmdlist_data_tmp[HISI_DSS_CMDLIST_DATA_MAX];
	dss_cmdlist_data_t *cmdlist_data;
	dss_cmdlist_info_t *cmdlist_info;
	int32_t cmdlist_idx;

	dss_copybit_info_t *copybit_info;

	struct semaphore media_common_sr_sem;
	struct semaphore media_common_composer_sem;
	int32_t media_common_composer_sr_refcount;
	dss_cmdlist_data_t *media_common_cmdlist_data;
	dss_media_common_info_t *media_common_info;

	struct gen_pool *mmbuf_gen_pool;
	dss_mmbuf_info_t mmbuf_infos[HISI_DSS_CMDLIST_DATA_MAX];
	dss_mmbuf_info_t *mmbuf_info;
	struct list_head *mmbuf_list;

	bool dss_module_resource_initialized;
	dss_module_reg_t dss_module_default;
	dss_module_reg_t dss_mdc_module_default;

	struct dss_rect dirty_region_updt;
	uint32_t esd_happened;
	uint32_t esd_recover_state;

	struct ion_client *ion_client;
	struct ion_handle *ion_handle;
	struct iommu_map_format iommu_format;
	struct iommu_domain* hisi_domain;

	struct gen_pool *cmdlist_pool;
	struct ion_handle *cmdlist_pool_ion_handle;
	void *cmdlist_pool_vir_addr;
	ion_phys_addr_t cmdlist_pool_phy_addr;
	size_t sum_cmdlist_pool_size;

	struct fb_info *fbi;
	struct platform_device *pdev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

	wait_queue_head_t vactive0_start_wq;
	uint32_t vactive0_start_flag;
	uint32_t vactive0_end_flag;
	uint32_t ldi_data_gate_en;

	wait_queue_head_t crc_wq;
	uint32_t crc_flag;
	struct workqueue_struct *dss_debug_wq;
	struct work_struct dss_debug_work;

	struct workqueue_struct *aod_ud_fast_unblank_workqueue;
	struct work_struct aod_ud_fast_unblank_work;

	struct workqueue_struct *ldi_underflow_wq;
	struct work_struct ldi_underflow_work;
	struct workqueue_struct *rch2_ce_end_wq;
	struct work_struct rch2_ce_end_work;
	struct workqueue_struct *rch4_ce_end_wq;
	struct work_struct rch4_ce_end_work;
	struct workqueue_struct *dpp_ce_end_wq;
	struct work_struct dpp_ce_end_work;
#if !defined(CONFIG_HISI_FB_3650) && !defined (CONFIG_HISI_FB_6250)
	struct workqueue_struct *hiace_end_wq;
	struct work_struct hiace_end_work;
#endif
	struct workqueue_struct *display_engine_wq;
	struct work_struct display_engine_work;
#if defined (CONFIG_HUAWEI_DSM)
	struct workqueue_struct *dss_underflow_debug_workqueue;
	struct work_struct dss_underflow_debug_work;
#endif
#if defined (CONFIG_HISI_FB_V501) || defined (CONFIG_HISI_FB_V510) || defined (CONFIG_HISI_FB_V330)
	struct workqueue_struct *gmp_lut_wq;
	struct work_struct gmp_lut_work;
#endif

	struct workqueue_struct *masklayer_backlight_notify_wq;
	struct work_struct masklayer_backlight_notify_work;

	dss_rect_t resolution_rect;

	uint32_t frame_count;
	uint32_t frame_update_flag;
	bool fb_mem_free_flag;

	uint8_t core_clk_upt_support;

	uint32_t vactive_start_event;

	uint32_t vsync_ctrl_type;
	struct notifier_block nb;
	struct notifier_block lcd_int_nb; //for clear lcd ocp interrupt

	/*sensorhub aod */
	bool masklayer_maxbacklight_flag;
	struct semaphore sh_aod_blank_sem;
};


/******************************************************************************
** FUNCTIONS PROTOTYPES
*/
extern int g_primary_lcd_xres;
extern int g_primary_lcd_yres;
extern uint64_t g_pxl_clk_rate;

extern uint8_t g_prefix_ce_support;
extern uint8_t g_prefix_sharpness1D_support;
extern uint8_t g_prefix_sharpness2D_support;

extern uint32_t g_online_cmdlist_idxs;
extern uint32_t g_offline_cmdlist_idxs;

extern uint32_t g_fpga_flag;
extern uint32_t g_fastboot_enable_flag;
extern uint32_t g_fake_lcd_flag;
extern uint32_t g_dss_version_tag;
extern uint32_t g_dss_module_resource_initialized;
extern uint32_t g_logo_buffer_base;
extern uint32_t g_logo_buffer_size;
extern uint32_t g_underflow_stop_perf_stat;

extern uint32_t g_fastboot_already_set;

extern int g_debug_online_vactive;

extern struct gen_pool *g_mmbuf_gen_pool;

extern struct fb_info *fbi_list[HISI_FB_MAX_FBI_LIST];
extern struct hisi_fb_data_type *hisifd_list[HISI_FB_MAX_FBI_LIST];

uint32_t get_panel_xres(struct hisi_fb_data_type *hisifd);
uint32_t get_panel_yres(struct hisi_fb_data_type *hisifd);

bool is_fastboot_display_enable(void);
bool is_dss_idle_enable(void);


/* fb buffer */
unsigned long hisifb_alloc_fb_buffer(struct hisi_fb_data_type *hisifd);
void hisifb_free_fb_buffer(struct hisi_fb_data_type *hisifd);
void hisifb_free_logo_buffer(struct hisi_fb_data_type *hisifd);

/* dss secure */
void hisifb_secure_register(struct platform_device *pdev);
void hisifb_secure_unregister(struct platform_device *pdev);
int hisi_fb_blank_sub(int blank_mode, struct fb_info *info);

/* backlight */
void hisifb_backlight_update(struct hisi_fb_data_type *hisifd);
void hisifb_backlight_cancel(struct hisi_fb_data_type *hisifd);
void hisifb_backlight_register(struct platform_device *pdev);
void hisifb_backlight_unregister(struct platform_device *pdev);
void hisifb_sbl_isr_handler(struct hisi_fb_data_type *hisifd);
void hisifb_set_backlight(struct hisi_fb_data_type *hisifd, uint32_t bkl_lvl, bool enforce);
int updateCabcPwm(struct hisi_fb_data_type *hisifd);

/* backlight flicker detector*/
void bl_flicker_detector_init(display_engine_flicker_detector_config_t config);
void bl_flicker_detector_collect_upper_bl(int level);
void bl_flicker_detector_collect_algo_delta_bl(int level);
void bl_flicker_detector_collect_device_bl(int level);

/* vsync */
void hisifb_frame_updated(struct hisi_fb_data_type *hisifd);
void hisifb_set_vsync_activate_state(struct hisi_fb_data_type *hisifd, bool infinite);
void hisifb_activate_vsync(struct hisi_fb_data_type *hisifd);
void hisifb_deactivate_vsync(struct hisi_fb_data_type *hisifd);
int hisifb_vsync_ctrl(struct fb_info *info, void __user *argp);
int hisifb_vsync_resume(struct hisi_fb_data_type *hisifd);
int hisifb_vsync_suspend(struct hisi_fb_data_type *hisifd);
void hisifb_vsync_isr_handler(struct hisi_fb_data_type *hisifd);
void hisifb_vsync_register(struct platform_device *pdev);
void hisifb_vsync_unregister(struct platform_device *pdev);
void hisifb_vsync_disable_enter_idle(struct hisi_fb_data_type *hisifd, bool disable);
void hisifb_video_idle_ctrl_register(struct platform_device *pdev);
void hisifb_video_idle_ctrl_unregister(struct platform_device *pdev);
void hisifb_esd_register(struct platform_device *pdev);
void hisifb_esd_unregister(struct platform_device *pdev);
void hisifb_masklayer_backlight_flag_config(struct hisi_fb_data_type *hisifd,
	bool masklayer_backlight_flag);

/* buffer sync */
int hisifb_layerbuf_lock(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *lock_list);
void hisifb_layerbuf_flush(struct hisi_fb_data_type *hisifd,
	struct list_head *lock_list);
void hisifb_layerbuf_unlock(struct hisi_fb_data_type *hisifd,
	struct list_head *pfree_list);
void hisifb_layerbuf_lock_exception(struct hisi_fb_data_type *hisifd,
	struct list_head *lock_list);
int hisifb_offline_layerbuf_lock(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list);
void hisifb_offline_layerbuf_unlock(struct hisi_fb_data_type *hisifd,
	struct list_head *pfree_list);

int hisifb_buf_sync_wait(int fence_fd);
int hisifb_buf_sync_handle_offline(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req);
int hisifb_buf_sync_handle(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req);
void hisifb_buf_sync_signal(struct hisi_fb_data_type *hisifd);
void hisifb_buf_sync_suspend(struct hisi_fb_data_type *hisifd);
int hisifb_buf_sync_create_fence(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req);
void hisifb_buf_sync_close_fence(dss_overlay_t *pov_req);
void hisifb_buf_sync_register(struct platform_device *pdev);
void hisifb_buf_sync_unregister(struct platform_device *pdev);

/* control */
int hisifb_ctrl_fastboot(struct hisi_fb_data_type *hisifd);
int hisifb_ctrl_on(struct hisi_fb_data_type *hisifd);
int hisifb_ctrl_off(struct hisi_fb_data_type *hisifd);
int hisifb_ctrl_lp(struct hisi_fb_data_type *hisifd, bool lp_enter);
int hisifb_ctrl_sbl(struct fb_info *info, int value);
int hisifb_ctrl_dss_voltage_get(struct fb_info *info, void __user *argp);
int hisifb_ctrl_dss_voltage_set(struct fb_info *info, void __user *argp);
int hisifb_ctrl_dss_vote_cmd_set(struct fb_info *info, void __user *argp);
int hisifb_fps_upt_isr_handler(struct hisi_fb_data_type *hisifd);
int hisifb_ctrl_esd(struct hisi_fb_data_type *hisifd);
void hisifb_sysfs_attrs_add(struct hisi_fb_data_type * hisifd);
void hisifb_dss_overlay_info_init(dss_overlay_t* ov_req);
void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
uint32_t set_bits32(uint32_t old_val, uint32_t val, uint8_t bw, uint8_t bs);
void hisifb_set_reg(struct hisi_fb_data_type *hisifd,
	char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs);
uint32_t hisifb_line_length(int index, uint32_t xres, int bpp);
void hisifb_get_timestamp(struct timeval *tv);
uint32_t hisifb_timestamp_diff(struct timeval *lasttime, struct timeval *curtime);
int hisifb_sbl_pow_i(int base, int exp);
void hisifb_save_file(char *filename, char *buf, uint32_t buf_len);

struct platform_device *hisi_fb_device_alloc(struct hisi_fb_panel_data *pdata,
	uint32_t type, uint32_t id);
struct platform_device *hisi_fb_add_device(struct platform_device *pdev);

#ifdef CONFIG_HUAWEI_OCP
int hisi_lcd_ocp_recover(struct notifier_block *nb,
		unsigned long event, void *data);
#endif

#if defined (CONFIG_HISI_FB_970) || defined (CONFIG_HISI_FB_V501)
/* sensorhub aod*/
bool hisi_sensorhub_aod_hw_lock(struct hisi_fb_data_type *hisifd);
bool hisi_sensorhub_aod_hw_unlock(struct hisi_fb_data_type *hisifd);
int hisi_sensorhub_aod_unblank(void);
int hisi_sensorhub_aod_blank(void);
#endif
#endif /* HISI_FB_H */
