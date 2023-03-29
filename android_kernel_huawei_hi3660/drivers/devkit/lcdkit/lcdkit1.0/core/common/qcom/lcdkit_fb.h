#ifndef __LCDKIT_FB_H__
#define __LCDKIT_FB_H__

#ifdef CONFIG_LOG_JANK
#include <huawei_platform/log/log_jank.h>
#endif

#include "lcdkit_dbg.h"
#include "lcdkit_fb_util.h"
#include <linux/lcdkit_dsm.h>
#include "lcdkit_dsi_status.h"

void lcdkit_set_fb_pdev(struct platform_device *pdev);
static void mdss_fb_shutdown(struct platform_device *pdev);
void mdss_fb_set_backlight(struct msm_fb_data_type *mfd, u32 bkl_lvl);
void mdss_fb_update_backlight_wq_handler(struct work_struct *work);

#endif
