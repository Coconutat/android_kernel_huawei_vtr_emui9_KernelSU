
#ifndef __LCDKIT_DSI_PANEL_H__
#define __LCDKIT_DSI_PANEL_H__

#include "lcdkit_dbg.h"
#include "lcdkit_common.h"
#include <linux/lcdkit_dsm.h>
#include <misc/app_info.h>

#ifdef CONFIG_LOG_JANK
#include <huawei_platform/log/log_jank.h>
#endif

extern atomic_t mipi_path_status;

static void lcdkit_record_bl_level(u32 bl_level);
void mdss_dsi_panel_bklt_dcs(void *pdata, int bl_level);
void lcdkit_dsi_panel_bklt_IC_TI(void *pdata, int bl_level);
int lcdkit_app_info_set(struct mdss_panel_info *pinfo);
int mdss_dsi_panel_on(struct mdss_panel_data *pdata);
int mdss_dsi_panel_off(struct mdss_panel_data *pdata);
void lcdkit_trans_exist_cmds(struct dsi_panel_cmds *outcmds,
    struct lcdkit_dsi_panel_cmds *incmds);
static void mdss_dsi_parse_esd_params(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl);

#endif
