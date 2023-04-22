#include <linux/leds.h>
#include "mdss_fb.h"
#include "lcdkit_panel.h"
#include "mdss_dsi.h"
#include "mdss_dsi_cmd.h"
#include "lcdkit_parse.h"
#include "lcdkit_dbg.h"
#include "huawei_ts_kit.h"

#define BLK_PAYLOAD_NUM_OFFSET	6
#define BLK_PAYLOAD_NUM		0x03

extern int is_device_reboot;
const char *default_panel_name;
volatile int lcdkit_brightness_ddic_info = 0;
extern void lp8556_reset(void);
extern ssize_t lm36923_set_backlight_reg(uint32_t bl_level);
extern ssize_t lm36923_chip_initial(void);
extern ssize_t Is_lm36923_used(void);
extern ssize_t lp8556_set_backlight_reg(uint32_t bl_level);

void level_to_reg_51_mode0(struct lcdkit_dsi_panel_cmds *bl_cmds, int i, int bl_level)
{
	if (lcdkit_info.panel_infos.bl_level_max < BL_LEVEL_MAX_10_BIT){
		bl_cmds->cmds[i].payload[1] = ((bl_level<<2)&0xf00)>>8;
		bl_cmds->cmds[i].payload[2] = (bl_level<<2)&0xff;
	} else if (lcdkit_info.panel_infos.bl_level_max < BL_LEVEL_MAX_11_BIT){
		bl_cmds->cmds[i].payload[1] = ((bl_level<<1)&0xf00)>>8;
		bl_cmds->cmds[i].payload[2] = (bl_level<<1)&0xff;
	} else {
		bl_cmds->cmds[i].payload[1] = (bl_level&0xf00)>>8;
		bl_cmds->cmds[i].payload[2] = bl_level&0xff;
	}
	
}

void level_to_reg_51_mode1(struct lcdkit_dsi_panel_cmds *bl_cmds, int i, int bl_level)
{
	bl_cmds->cmds[i].payload[1] = (bl_level & 0xff00)>>8;
	bl_cmds->cmds[i].payload[2] = bl_level & 0x00ff;
}

void level_to_reg_90_mode0(struct lcdkit_dsi_panel_cmds *bl_cmds, int i, int bl_level)
{
	if (lcdkit_info.panel_infos.bl_level_max < BL_LEVEL_MAX_10_BIT){
		bl_cmds->cmds[i].payload[1] = (bl_level>>2)&0xff;
		bl_cmds->cmds[i].payload[2] = (bl_level<<2)&0x0f;
	} else if (lcdkit_info.panel_infos.bl_level_max < BL_LEVEL_MAX_11_BIT){
		bl_cmds->cmds[i].payload[1] = (bl_level>>3)&0xff;
		bl_cmds->cmds[i].payload[2] = (bl_level<<1)&0x0f;
	} else {
		bl_cmds->cmds[i].payload[1] = (bl_level>>4)&0xff;
		bl_cmds->cmds[i].payload[2] = bl_level&0x0f;
	}

}

void level_to_regvalue_default (struct lcdkit_dsi_panel_cmds *bl_cmds, int i, int bl_level)
{
	bl_cmds->cmds[i].payload[1] = bl_level;
}


dcs_level_trans_func_t level_to_51regvalue_func_arry[REG51_MODE_NUM] = {
	level_to_reg_51_mode0,
	level_to_reg_51_mode1,
};

dcs_level_trans_func_t level_to_90regvalue_func_arry[REG90_MODE_NUM] = {
	level_to_reg_90_mode0,
};


void mdss_dsi_panel_bklt_dcs(void *pdata, int bl_level)
{
	int i;
	struct lcdkit_dsi_panel_cmds *bl_cmds
		= &lcdkit_info.panel_infos.backlight_cmds;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = pdata;
	static dcs_level_trans_func_t dcs_bklt_level_to_regvallue_func;

	LCDKIT_INFO("mdss_dsi_panel_bklt_dcs: bl_level=%d,cmd_cnt=%d\n",bl_level,bl_cmds->cmd_cnt);


	if (unlikely(dcs_bklt_level_to_regvallue_func == NULL)) {
		if (lcdkit_info.panel_infos.lcd_bklt_dcs_reg == SET_CABC_PWM_CMD_51) {
			lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode = lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode >= REG51_MODE_NUM ? 0:lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode;
			dcs_bklt_level_to_regvallue_func = level_to_51regvalue_func_arry[lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode];
		} else if (lcdkit_info.panel_infos.lcd_bklt_dcs_reg == SET_CABC_PWM_CMD_90) {
			lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode = lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode >= REG90_MODE_NUM ? 0:lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode;
			dcs_bklt_level_to_regvallue_func = level_to_90regvalue_func_arry[lcdkit_info.panel_infos.lcd_bklt_level_to_regvalue_mode];
		} else {
			dcs_bklt_level_to_regvallue_func = level_to_regvalue_default;	
		}
	}
	for (i = 0; i < bl_cmds->cmd_cnt; i++)
	{
		if (lcdkit_info.panel_infos.lcd_bklt_dcs_reg != bl_cmds->cmds[i].payload[0]) {
			continue;
		}
		if (BLK_PAYLOAD_NUM == bl_cmds->cmds[i].dlen) {
			dcs_bklt_level_to_regvallue_func(bl_cmds, i, bl_level);
		} else {
			level_to_regvalue_default(bl_cmds, i ,bl_level);
		}
	}

	lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.backlight_cmds);
	return;
}

void lcdkit_dsi_panel_bklt_IC_TI(void *pdata, int bl_level)
{
    struct lcdkit_dsi_panel_cmds *bl_cmds
                = &lcdkit_info.panel_infos.backlight_cmds;
    struct mdss_dsi_ctrl_pdata *ctrl_pdata = pdata;
    static int pre_bl_level=0;

    if(pre_bl_level == bl_level)
        return;
    if (bl_cmds->cmd_cnt != 1)
        return;

    if(lcdkit_info.panel_infos.bl_chip_init == BL_MODULE_LP8556)
    {
#ifdef CONFIG_BACKLIGHT_LP8556
        bl_cmds->cmds[0].payload[1] = 0xFF;
        int reg_level = 0;
        reg_level = bl_level;
        if((bl_level > 0) && (pre_bl_level == 0))
        {
             lcdkit_delay(lcdkit_info.panel_infos.delay_af_blic_init);
             lp8556_reset();
        }
        pre_bl_level = bl_level;
        lp8556_set_backlight_reg(reg_level);
        lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.backlight_cmds);
#endif
    }
    else if(lcdkit_info.panel_infos.bl_chip_init == BL_MODULE_LM36923){
#ifdef CONFIG_BACKLIGHT_LM36923
        bl_cmds->cmds[0].payload[1] = 0xFF;
        int reg_level=0;
        reg_level = bl_level * 2047 * lcdkit_info.panel_infos.bl_level_limit_percent/(lcdkit_info.panel_infos.bl_level_max * 100); //22/25=>1785/2047 match with 22mA
        if(0==pre_bl_level && 0!=bl_level){
            lm36923_chip_initial();
            lcdkit_delay(lcdkit_info.panel_infos.delay_af_blic_init);
        }
        lm36923_set_backlight_reg(reg_level);
        if(0==pre_bl_level && 0!=bl_level){
            lcdkit_dsi_tx(pdata, &lcdkit_info.panel_infos.backlight_cmds);
        }
        pre_bl_level = bl_level;
#endif
    }
    return;
}

int mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	unsigned long timeout = jiffies;
	static bool is_first_reset = true;

	if (pdata == NULL) {
		LCDKIT_ERR(": Invalid input data\n");
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	pinfo = &(ctrl_pdata->panel_data.panel_info);
	if ((mdss_dsi_is_right_ctrl(ctrl_pdata) &&
		mdss_dsi_is_hw_config_split(ctrl_pdata->shared_data)) ||
			pinfo->is_dba_panel) {
		LCDKIT_DEBUG(":%d, right ctrl gpio configuration not needed\n",__LINE__);
		return rc;
	}

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		LCDKIT_DEBUG(":%d, reset line not configured\n", __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		LCDKIT_DEBUG(":%d, reset line not configured\n", __LINE__);
		return rc;
	}

	if (!gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		LCDKIT_DEBUG(":%d, bklt en not configured\n", __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->disp_bl_gpio)) {
		LCDKIT_DEBUG(":%d, bl en not configured\n", __LINE__);
	}

	LCDKIT_DEBUG("%s: enable = %d\n", __func__, enable);

	if (enable) {

		if(is_first_reset )
        {
		    rc = mdss_dsi_request_gpios(ctrl_pdata);
    		is_first_reset = false;
	    	if (rc) {
		    	LCDKIT_ERR("gpio request failed\n");
			    return rc;
		    }
	    }

		if (!pinfo->cont_splash_enabled) {
#if 0
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				rc = gpio_direction_output(
					ctrl_pdata->disp_en_gpio, 1);
				if (rc) {
					LCDKIT_ERR(": unable to set dir for en gpio\n");
					goto exit;
				}
			}
#endif
			if (pdata->panel_info.rst_seq_len) {
				rc = gpio_direction_output(ctrl_pdata->rst_gpio,
					pdata->panel_info.rst_seq[0]);
				if (rc) {
					LCDKIT_ERR(": unable to set dir for rst gpio\n");
					goto exit;
				}
				/* when tp rst need follow lcd rst*/
				if (lcdkit_info.panel_infos.tp_lcd_reset_sync) {
					LCDKIT_ERR("tp reset follow lcd reset\n");
					if (gpio_is_valid(ctrl_pdata->tp_rst_gpio)) {
						rc = gpio_direction_output(ctrl_pdata->tp_rst_gpio,
								pdata->panel_info.rst_seq[0]);
						if (rc) {
							LCDKIT_ERR(": unable to set dir for tp rst gpio\n");
							goto exit;
						}
					}
				}
			}

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				if (lcdkit_info.panel_infos.tp_lcd_reset_sync) {
					gpio_set_value(ctrl_pdata->tp_rst_gpio,
							pdata->panel_info.rst_seq[i]);
				}
				if (pdata->panel_info.rst_seq[++i])
					usleep_range(pinfo->rst_seq[i] * 1000, pinfo->rst_seq[i] * 1000);
			}
#if 0
			if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
				rc = gpio_direction_output(
					ctrl_pdata->bklt_en_gpio, 1);
				if (rc) {
					LCDKIT_ERR(": unable to set dir for bklt gpio\n");
					goto exit;
				}
			}

			if (gpio_is_valid(ctrl_pdata->disp_bl_gpio)) {
				rc = gpio_direction_output(
					ctrl_pdata->disp_bl_gpio, 1);
				if (rc) {
					LCDKIT_ERR(": unable to set dir for bl en gpio\n");
				}
			}
#endif
		}

#ifdef CONFIG_ARCH_SDM660
		if (gpio_is_valid(ctrl_pdata->lcd_mode_sel_gpio)) {
			bool out = false;

			if (pinfo->mode_sel_state == MODE_GPIO_HIGH)
				out = true;
			else if (pinfo->mode_sel_state == MODE_GPIO_LOW)
				out = false;

			rc = gpio_direction_output(ctrl_pdata->lcd_mode_sel_gpio, out);
			if (rc) {
				LCDKIT_ERR(": unable to set dir for lcd mode sel gpio\n");
				goto exit;
			}
		}
#else
		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			bool out = false;

			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				out = true;
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				out = false;

			rc = gpio_direction_output(ctrl_pdata->mode_gpio, out);
			if (rc) {
				LCDKIT_ERR(": unable to set dir for mode gpio\n");
				goto exit;
			}
		}
#endif

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			LCDKIT_DEBUG(": Panel Not properly turned OFF\n");
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			LCDKIT_DEBUG(": Reset panel done\n");
		}
	}
    else
    {
		is_first_reset = true;
#if 0
		if (gpio_is_valid(ctrl_pdata->disp_bl_gpio)) {
			gpio_set_value((ctrl_pdata->disp_bl_gpio), 0);
			gpio_free(ctrl_pdata->disp_bl_gpio);
		}
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}
#endif
		if(!lcdkit_info.panel_infos.panel_down_reset)
		{
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			if (lcdkit_info.panel_infos.tp_lcd_reset_sync)
				gpio_set_value((ctrl_pdata->tp_rst_gpio), 0);
			lcdkit_delay(lcdkit_info.panel_infos.delay_af_rst_off);
			LCDKIT_INFO("when lcd sleep reset keep low\n");
		}

		gpio_free(ctrl_pdata->rst_gpio);
		if (lcdkit_info.panel_infos.tp_lcd_reset_sync)
			gpio_free(ctrl_pdata->tp_rst_gpio);


#ifdef CONFIG_ARCH_SDM660
		if (gpio_is_valid(ctrl_pdata->lcd_mode_sel_gpio))
			gpio_free(ctrl_pdata->lcd_mode_sel_gpio);
#else
		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
#endif
	}

	LCDKIT_INFO(": panel reset time = %u\n", jiffies_to_msecs(jiffies-timeout));

exit:
	return rc;
}

static void lcdkit_record_bl_level(u32 bl_level)
{
	static bool lcd_log_flag = true;

    LCDKIT_DEBUG(":LCD backlight is: %d \n", bl_level);

    if (lcd_log_flag || (bl_level == 0))
    {
        #ifdef CONFIG_LOG_JANK
        if (lcd_log_flag && bl_level)
        {
            LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON,"%s,%d",
                            "JL_KERNEL_LCD_BACKLIGHT_ON",bl_level);

            #ifdef CONFIG_HUAWEI_DSM
            lcd_pwr_status.lcd_dcm_pwr_status |= BIT(3);
            do_gettimeofday(&lcd_pwr_status.tvl_backlight);
            time_to_tm(lcd_pwr_status.tvl_backlight.tv_sec,
                                0, &lcd_pwr_status.tm_backlight);
            #endif
        }
        #endif

		LCDKIT_INFO(":LCD backlight is: %d \n", bl_level);

		if (bl_level == 0)
		    lcd_log_flag = true;
        else
            lcd_log_flag = false;
    }

    return;
}


int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_panel_info *pinfo;
   	struct dsi_panel_cmds *post_on_cmds;
    struct lcdkit_dsi_panel_cmds *on_cmds;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	unsigned long timeout = jiffies;

	if (pdata == NULL) {
		LCDKIT_ERR("Invalid input data\n");
		return -EINVAL;
	}

	pinfo = &pdata->panel_info;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	LCDKIT_DEBUG("ctrl=%p ndx=%d\n", ctrl, ctrl->ndx);

	if (pinfo->dcs_cmd_by_left) {
		if (ctrl->ndx != DSI_CTRL_LEFT)
			goto end;
	}

    LCDKIT_INFO("enter!");

#ifdef CONFIG_HUAWEI_TS_KIT
    if (g_tskit_ic_type)
    {
        ret = ts_kit_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);

        if (ret)
        {
            LCDKIT_ERR("ts resume device err\n");
        }

        lcdkit_delay(lcdkit_info.panel_infos.delay_af_tp_reset);
    }
#endif

	if (is_lcdkit_initcode_enable())
	{
		LCDKIT_INFO("read from debug file and write to LCD!\n");
        on_cmds = &lcdkit_info.panel_infos.g_panel_cmds;
        lcdkit_on_cmd(ctrl, on_cmds);
	}
    else
    {
    	if ((pinfo->mipi.dms_mode == DYNAMIC_MODE_SWITCH_IMMEDIATE) &&
    			(pinfo->mipi.boot_mode != pinfo->mipi.mode))
    	{
    		post_on_cmds = &ctrl->post_dms_on_cmds;
            if (post_on_cmds->cmd_cnt)
    			mdss_dsi_panel_cmds_send(ctrl, post_on_cmds, CMD_REQ_COMMIT);
            LCDKIT_DEBUG("ctrl=%p cmd_cnt=%d\n", ctrl, post_on_cmds->cmd_cnt);
    	}
        else
        {
            on_cmds = &lcdkit_info.panel_infos.display_on_cmds;
            if (on_cmds->cmd_cnt)
    	        lcdkit_on_cmd(ctrl, on_cmds);
            LCDKIT_DEBUG("ctrl=%p cmd_cnt=%d\n", ctrl, on_cmds->cmd_cnt);
        }
    }

    if (is_lcdkit_mipiclk_enable())
    {
        ctrl->pclk_rate = get_lcdkit_mipiclk_dbg();
        //ctrl->byte_clk_rate = get_lcdkit_mipibclk_dbg();
    }

#ifdef CONFIG_HUAWEI_DSM
	lcd_pwr_status.lcd_dcm_pwr_status |= BIT(1);
	do_gettimeofday(&lcd_pwr_status.tvl_lcd_on);
	time_to_tm(lcd_pwr_status.tvl_lcd_on.tv_sec, 0, &lcd_pwr_status.tm_lcd_on);
#endif

	if (pinfo->compression_mode == COMPRESSION_DSC)
		mdss_dsi_panel_dsc_pps_send(ctrl, pinfo);

	if (ctrl->ds_registered)
		mdss_dba_utils_video_on(pinfo->dba_data, pinfo);

#ifdef CONFIG_HUAWEI_TS_KIT

    if (g_tskit_ic_type)
    {
        ret = ts_kit_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);

        if (ret)
        { LCDKIT_ERR("ts after resume err\n"); }
    }
#endif

end:
//	pinfo->blank_state = MDSS_PANEL_BLANK_UNBLANK;
/*set mipi status*/
#if defined(CONFIG_HUAWEI_KERNEL) && defined(CONFIG_DEBUG_FS)
	atomic_set(&mipi_path_status, LCDKIT_MIPI_PATH_OPEN);
#endif

    /* add for timeout print log */
	LCDKIT_INFO("exit panel_on_time = %u\n", jiffies_to_msecs(jiffies-timeout));


#ifdef CONFIG_LOG_JANK
    LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "JL_KERNEL_LCD_POWER_ON");
#endif

	return 0;
}

int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
    int ret = 0;
	//struct msm_fb_data_type mfd;
    struct mdss_panel_info *pinfo;
    struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	if (pdata == NULL) {
		LCDKIT_ERR("Invalid input data\n");
		return -EINVAL;
	}

    /*set mipi status*/
    #if defined(CONFIG_HUAWEI_KERNEL) && defined(CONFIG_DEBUG_FS)
	atomic_set(&mipi_path_status, LCDKIT_MIPI_PATH_CLOSE);
    #endif

	pinfo = &pdata->panel_info;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	LCDKIT_DEBUG("ctrl=%p ndx=%d\n", ctrl, ctrl->ndx);

	if (pinfo->dcs_cmd_by_left) {
		if (ctrl->ndx != DSI_CTRL_LEFT)
			goto end;
	}

    //!hisifd->fb_shutdown !mfd->shutdown_pending)
    //mfd = container_of(pinfo, struct msm_fb_data_type, panel_info);
#ifdef CONFIG_HUAWEI_TS_KIT

    if (g_tskit_ic_type)
    {
	 LCDKIT_INFO("ts before suspend device\n");
        ret = ts_kit_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);

        if (ret)
        {
            LCDKIT_ERR("ts before suspend err\n");
        }
        mdelay(5);

	if (!lcdkit_info.panel_infos.tp_suspend_after_lcd_sleep) {
		LCDKIT_ERR("ts suspend device\n");
		ret = ts_kit_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);

		if (ret)
		{
			LCDKIT_ERR("ts suspend device err\n");
		}
		mdelay(20);
	}

    }
#endif

	if (lcdkit_info.panel_infos.display_off_cmds.cmd_cnt)
		lcdkit_off_cmd(ctrl, &lcdkit_info.panel_infos.display_off_cmds);

	if (lcdkit_info.panel_infos.shutdown_sleep_support && is_device_reboot)
		lcdkit_off_cmd(ctrl, &lcdkit_info.panel_infos.shutdown_sleep_cmds);

	if (ctrl->ds_registered && pinfo->is_pluggable) {
		mdss_dba_utils_video_off(pinfo->dba_data);
        //move the log print
		mdss_dba_utils_hdcp_enable(pinfo->dba_data, false);
	}

#ifdef CONFIG_HUAWEI_TS_KIT
    if (g_tskit_ic_type && (lcdkit_info.panel_infos.tp_suspend_after_lcd_sleep))
    {
	LCDKIT_INFO("ts suspend device\n");
        ret = ts_kit_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);

        if (ret)
        {
            LCDKIT_ERR("ts suspend device err\n");
        }
        mdelay(20);

    }
#endif

    lcdkit_info.panel_infos.scan_mode = FORWORD_SCAN;
    lcdkit_info.panel_infos.inversion_mode = COLUMN_INVERSION;

	LCDKIT_INFO("nomal exit: -\n");

#ifdef CONFIG_LOG_JANK
    LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "JL_KERNEL_LCD_POWER_OFF");
#endif

end:
	LCDKIT_DEBUG(":-\n");

	return 0;
}



static void lcdkit_parse_reset_seq(struct mdss_panel_info *pinfo)
{
    pinfo->rst_seq_len = 6;
    pinfo->rst_seq[0] = 1;
    pinfo->rst_seq[1] = lcdkit_info.panel_infos.reset_step1_H;
    pinfo->rst_seq[2] = 0;
    pinfo->rst_seq[3] = lcdkit_info.panel_infos.reset_L;
    pinfo->rst_seq[4] = 1;
    pinfo->rst_seq[5] = lcdkit_info.panel_infos.reset_step2_H;
}

int mdss_panel_parse_bl_settings(struct device_node *np,
			                    struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	const char *data;
	int rc = 0;
	u32 tmp;

	ctrl_pdata->bklt_ctrl = UNKNOWN_CTRL;

	data = of_get_property(np, "hw,lcdkit-bl-pmic-control-type", NULL);
	if (data) {
		if (!strcmp(data, "bl_ctrl_wled"))
        {
			led_trigger_register_simple("bkl-trigger", &bl_led_trigger);

			LCDKIT_DEBUG("SUCCESS-> WLED TRIGGER register\n");

			ctrl_pdata->bklt_ctrl = BL_WLED;
		}
        else if (!strcmp(data, "bl_ctrl_pwm"))
        {
			ctrl_pdata->bklt_ctrl = BL_PWM;
			ctrl_pdata->pwm_pmi
                = of_property_read_bool(np, "hw,lcdkit-bl-pwm-pmi");

			rc = of_property_read_u32(np,
                            "hw,lcdkit-bl-pmic-pwm-frequency", &tmp);
			if (rc) {
				LCDKIT_ERR("%d, Error, panel pwm_period\n", __LINE__);
				return -EINVAL;
			}
			ctrl_pdata->pwm_period = tmp;
			if (ctrl_pdata->pwm_pmi) {
				ctrl_pdata->pwm_bl = of_pwm_get(np, NULL);
				if (IS_ERR(ctrl_pdata->pwm_bl)) {
					LCDKIT_ERR("Error, pwm device\n");
					ctrl_pdata->pwm_bl = NULL;
					return -EINVAL;
				}
			} else {
				rc = of_property_read_u32(np,
                            "hw,lcdkit-bl-pmic-bank-select", &tmp);
				if (rc) {
					LCDKIT_ERR("%d, Error, lpg channel\n", __LINE__);
					return -EINVAL;
				}
				ctrl_pdata->pwm_lpg_chan = tmp;
				tmp = of_get_named_gpio(np, "hw,lcdkit-pwm-gpio", 0);
				ctrl_pdata->pwm_pmic_gpio = tmp;
				LCDKIT_DEBUG("Configured PWM bklt ctrl\n");
			}
		}
        else if (!strcmp(data, "bl_ctrl_dcs"))
        {
			ctrl_pdata->bklt_ctrl = BL_DCS_CMD;
			LCDKIT_DEBUG("Configured DCS_CMD bklt ctrl\n");
		}
		else if (!strcmp(data, "bl_ctrl_ic_ti"))
		{
			ctrl_pdata->bklt_ctrl = BL_IC_TI;
			LCDKIT_DEBUG("Configured IC_TI bklt ctrl\n");
		}
	}

	return 0;
}

int lcdkit_app_info_set(struct mdss_panel_info *pinfo)
{
    int rc = 0;
   	static const char *panel_name;
    static const char *info_node = "lcd type";

	pinfo->panel_name[0] = '\0';
	//panel_name = of_get_property(node, LCKDIT_PANEL_PARSE_STRING, NULL);
	panel_name = lcdkit_info.panel_infos.panel_name;
	if (!panel_name) {
		LCDKIT_INFO("%d, Panel name not specified\n", __LINE__);
        return -EINVAL;
	} else {
		LCDKIT_INFO("Panel Name = %s\n", panel_name);
		strlcpy(&pinfo->panel_name[0], panel_name, MDSS_MAX_PANEL_LEN);
	}

	default_panel_name = panel_name;
	if(!strcmp(default_panel_name,"AUO_OTM1901A 5.2' VIDEO TFT 1080 x 1920 DEFAULT"))
	{
		panel_name = "default lcd";
		LCDKIT_INFO("Do not support expect LCD module type, LCD name is %s\n",
                    panel_name);
	}

#if CONFIG_APP_INFO
	rc = app_info_set(info_node, panel_name);
	if (rc) {
		LCDKIT_ERR(":%d panel dt parse failed\n", __LINE__);
		return rc;
	}
#endif

    return 0;

}

static void mdss_dsi_parse_esd_params(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_panel_info *pinfo = &ctrl->panel_data.panel_info;

    pinfo->esd_check_enabled = lcdkit_info.panel_infos.esd_support;

	if (!pinfo->esd_check_enabled)
	{
		return;
	}

	ctrl->status_mode = ESD_MAX;

	return;
}

static int mdss_dsi_panel_timing_from_dt(struct device_node *np,
		struct dsi_panel_timing *pt, struct mdss_panel_data *panel_data)
{
	u32 tmp32 = 0;
	u64 tmp64 = 0;
	int rc, i, len;
	const char *data;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;
	struct mdss_panel_info *pinfo;
	bool phy_timings_present = false;

	pinfo = &panel_data->panel_info;

	ctrl_pdata = container_of(panel_data, struct mdss_dsi_ctrl_pdata, panel_data);

	pt->timing.xres = lcdkit_info.panel_infos.xres;;
	pt->timing.yres = lcdkit_info.panel_infos.yres;

	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-front-porch", &pt->timing.h_front_porch, 6);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-back-porch", &pt->timing.h_back_porch, 6);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-pulse-width", &pt->timing.h_pulse_width, 2);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-sync-skew", &pt->timing.hsync_skew, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-v-back-porch", &pt->timing.v_back_porch, 6);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-v-front-porch", &pt->timing.v_front_porch, 6);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-v-pulse-width", &pt->timing.v_pulse_width, 2);

	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-left-border", &pt->timing.border_left, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-h-right-border", &pt->timing.border_right, 0);

	/* overriding left/right borders for split display cases */
	if (mdss_dsi_is_hw_config_split(ctrl_pdata->shared_data)) {
		if (panel_data->next)
			pt->timing.border_right = 0;
		else
			pt->timing.border_left = 0;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-v-top-border", &pt->timing.border_top, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-v-bottom-border", &pt->timing.border_bottom, 0);

	OF_PROPERTY_READ_U8_DEFAULT(np, "hw,lcdkit-panel-framerate",
                &pt->timing.frame_rate, DEFAULT_FRAME_RATE);
	rc = of_property_read_u64(np, "hw,lcdkit-panel-clockrate", &tmp64);
	if (rc == -EOVERFLOW) {
		rc = of_property_read_u32(np, "hw,lcdkit-panel-clockrate", &tmp32);
		tmp64 = !rc ? tmp32 : 0;
	}
	pt->timing.clk_rate = !rc ? tmp64 : 0;

	data = of_get_property(np, "hw,lcdkit-panel-timings", &len);
	if ((!data) || (len != 12)) {
		LCDKIT_DEBUG("%d, Unable to read Phy timing settings", __LINE__);
	} else {
		for (i = 0; i < len; i++)
			pt->phy_timing[i] = data[i];
		phy_timings_present = true;
	}

	data = of_get_property(np, "hw,lcdkit-panel-timings-phy-v2", &len);
	if ((!data) || (len != 40)) {
		LCDKIT_DEBUG("%d, Unable to read 8996 Phy lane timing settings", __LINE__);
	} else {
		for (i = 0; i < len; i++)
			pt->phy_timing_8996[i] = data[i];
		phy_timings_present = true;
	}
	if (!phy_timings_present) {
		LCDKIT_ERR("phy timing settings not present\n");
		return -EINVAL;
	}

	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-t-clk-pre", &pt->t_clk_pre, 0x24);
	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-t-clk-post", &pt->t_clk_post, 0x03);

	if (np->name) {
		pt->timing.name = kstrdup(np->name, GFP_KERNEL);
		LCDKIT_INFO("found new timing \"%s\" (%p)\n", np->name, &pt->timing);
	}

	return 0;
}


static int  mdss_dsi_panel_config_res_properties(struct device_node *np,
		struct dsi_panel_timing *pt,
		struct mdss_panel_data *panel_data,
		bool default_timing)
{
	int rc = 0;

	mdss_dsi_parse_roi_alignment(np, pt);

#ifndef CONFIG_LCDKIT_DRIVER
	mdss_dsi_parse_dcs_cmds(np, &pt->on_cmds,
		"qcom,mdss-dsi-on-command",
		"qcom,mdss-dsi-on-command-state");
#else
    lcdkit_trans_exist_cmds(&pt->on_cmds, &lcdkit_info.panel_infos.display_on_cmds);
#endif

	mdss_dsi_parse_dcs_cmds(np, &pt->post_panel_on_cmds,
		"qcom,mdss-dsi-post-panel-on-command", NULL);

	mdss_dsi_parse_dcs_cmds(np, &pt->switch_cmds,
		"qcom,mdss-dsi-timing-switch-command",
		"qcom,mdss-dsi-timing-switch-command-state");

	rc = mdss_dsi_parse_topology_config(np, pt, panel_data, default_timing);
	if (rc) {
		LCDKIT_ERR(": parsing compression params failed. rc:%d\n", rc);
		return rc;
	}

	mdss_panel_parse_te_params(np, &pt->timing);
	return rc;
}

static int mdss_panel_parse_display_timings(struct device_node *np,
		struct mdss_panel_data *panel_data)
{
	struct mdss_dsi_ctrl_pdata *ctrl;
	struct dsi_panel_timing *modedb;
	struct device_node *timings_np;
	struct device_node *entry;
	int num_timings, rc;
	int i = 0, active_ndx = 0;
	bool default_timing = false;

	ctrl = container_of(panel_data, struct mdss_dsi_ctrl_pdata, panel_data);

	INIT_LIST_HEAD(&panel_data->timings_list);

	timings_np = of_get_child_by_name(np, "hw,lcdkit-display-timings");
	if (!timings_np) {
		struct dsi_panel_timing pt;
		memset(&pt, 0, sizeof(struct dsi_panel_timing));

		/*
		 * display timings node is not available, fallback to reading
		 * timings directly from root node instead
		 */
		LCDKIT_DEBUG("reading display-timings from panel node\n");
		rc = mdss_dsi_panel_timing_from_dt(np, &pt, panel_data);
		if (!rc) {
			mdss_dsi_panel_config_res_properties(np, &pt,
					panel_data, true);
			rc = mdss_dsi_panel_timing_switch(ctrl, &pt.timing);
		}
		return rc;
	}

	num_timings = of_get_child_count(timings_np);
	if (num_timings == 0) {
		LCDKIT_ERR("no timings found within display-timings\n");
		rc = -EINVAL;
		goto exit;
	}

	modedb = kcalloc(num_timings, sizeof(*modedb), GFP_KERNEL);
	if (!modedb) {
		rc = -ENOMEM;
		goto exit;
	}

	for_each_child_of_node(timings_np, entry) {
		rc = mdss_dsi_panel_timing_from_dt(entry, (modedb + i),
				panel_data);
		if (rc) {
			kfree(modedb);
			goto exit;
		}

		default_timing = of_property_read_bool(entry,
				"hw,lcdkit-timing-default");
		if (default_timing)
			active_ndx = i;

		mdss_dsi_panel_config_res_properties(entry, (modedb + i),
				panel_data, default_timing);

		list_add(&modedb[i].timing.list,
				&panel_data->timings_list);
		i++;
	}

	/* Configure default timing settings */
	rc = mdss_dsi_panel_timing_switch(ctrl, &modedb[active_ndx].timing);
	if (rc)
		LCDKIT_ERR("unable to configure default timing settings\n");

exit:
	of_node_put(timings_np);

	return rc;
}

static int mdss_panel_parse_dt(struct device_node *np,
			                   struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc, len = 0;
	const char *data;
	const char *bridge_chip_name;
    struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

    if (mdss_dsi_is_hw_config_split(ctrl_pdata->shared_data))
		pinfo->is_split_display = true;

    pinfo->physical_width = lcdkit_info.panel_infos.width;
    pinfo->physical_height = lcdkit_info.panel_infos.height;

    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-dsi-bpp", &pinfo->bpp, 24);

    #if 0
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	data = of_get_property(np, "hw,lcdkit-panel-cmd-type", NULL);
	if (data && !strncmp(data, "dsi_cmd_mode", 12))
		pinfo->mipi.mode = DSI_CMD_MODE;
    #else
    pinfo->mipi.mode = lcdkit_info.panel_infos.lcd_disp_type;;
    #endif
	pinfo->mipi.boot_mode = pinfo->mipi.mode;

	data = of_get_property(np, "hw,lcdkit-pixel-packing", NULL);
	if (data && !strcmp(data, "loose"))
		pinfo->mipi.pixel_packing = 1;
	else
		pinfo->mipi.pixel_packing = 0;


	rc = mdss_panel_get_dst_fmt(pinfo->bpp, pinfo->mipi.mode,
                    pinfo->mipi.pixel_packing, &(pinfo->mipi.dst_format));
	if (rc) {
		LCDKIT_ERR("problem determining dst format. Set Default\n");
		pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-underflow-color", &pinfo->lcdc.underflow_clr, 0xff);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-border-color", &pinfo->lcdc.border_clr, 0);

	data = of_get_property(np, "hw,lcdkit-panel-orientation", NULL);
	if (data) {
		LCDKIT_INFO("panel orientation is %s\n", data);
		if (!strcmp(data, "180"))
			pinfo->panel_orientation = MDP_ROT_180;
		else if (!strcmp(data, "hflip"))
			pinfo->panel_orientation = MDP_FLIP_LR;
		else if (!strcmp(data, "vflip"))
			pinfo->panel_orientation = MDP_FLIP_UD;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-brightness-max-level",
                &pinfo->brightness_max, MDSS_MAX_BL_BRIGHTNESS);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-bl-min-level", &pinfo->bl_min, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-bl-max-level", &pinfo->bl_max, 255);
	ctrl_pdata->bklt_max = pinfo->bl_max;

	lcdkit_info.panel_infos.bl_level_max = pinfo->bl_max;
	lcdkit_info.panel_infos.bl_level_min = pinfo->bl_min;

	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-interleave-mode", &pinfo->mipi.interleave_mode, 0);

	pinfo->mipi.vsync_enable
                = of_property_read_bool(np, "hw,lcdkit-te-check-enable");

	lcdkit_info.panel_infos.panel_down_reset
                = of_property_read_bool(np, "hw,lcdkit-down-reset-enable");
    #if 0
	global_tp_pre_lcd_flag
                = of_property_read_bool(np, "hw,lcdkit-tp-pre-lcd-enable");
	set_tp_pre_lcd_status(global_tp_pre_lcd_flag);
    #endif

	if (pinfo->sim_panel_mode == SIM_SW_TE_MODE)
		pinfo->mipi.hw_vsync_mode = false;
	else
		pinfo->mipi.hw_vsync_mode
		        = of_property_read_bool(np, "hw,lcdkit-te-using-te-pin");

	OF_PROPERTY_READ_U8_DEFAULT(np,
	            "hw,lcdkit-h-sync-pulse", &pinfo->mipi.pulse_mode_hsa_he, false);

	pinfo->mipi.hfp_power_stop
                = of_property_read_bool(np, "hw,lcdkit-hfp-power-mode");
	pinfo->mipi.hsa_power_stop
                = of_property_read_bool(np, "hw,lcdkit-hsa-power-mode");
	pinfo->mipi.hbp_power_stop
                = of_property_read_bool(np, "hw,lcdkit-hbp-power-mode");
	pinfo->mipi.last_line_interleave_en
                = of_property_read_bool(np, "qcom,mdss-dsi-last-line-interleave");
	pinfo->mipi.bllp_power_stop
                = of_property_read_bool(np, "hw,lcdkit-bllp-power-mode");
	pinfo->mipi.eof_bllp_power_stop
                = of_property_read_bool(np, "hw,lcdkit-bllp-eof-power-mode");

	pinfo->mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;
	data = of_get_property(np, "hw,lcdkit-traffic-mode", NULL);
	if (data) {
		if (!strcmp(data, "non_burst_sync_event"))
			pinfo->mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
		else if (!strcmp(data, "burst_mode"))
			pinfo->mipi.traffic_mode = DSI_BURST_MODE;
	}

	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-te-dcs-command", &pinfo->mipi.insert_dcs_cmd, 1);
	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-wr-mem-continue", &pinfo->mipi.wr_mem_continue, 0x3c);
	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-wr-mem-start", &pinfo->mipi.wr_mem_start, 0x2c);
	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-te-pin-select", &pinfo->mipi.te_sel, 1);
	OF_PROPERTY_READ_U8_DEFAULT(np,
                "hw,lcdkit-virtual-channel-id", &pinfo->mipi.vc, 0);

	pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	data = of_get_property(np, "hw,lcdkit-color-order", NULL);
	if (data) {
		if (!strcmp(data, "rgb_swap_rbg"))
			        pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RBG;
		else if (!strcmp(data, "rgb_swap_bgr"))
			        pinfo->mipi.rgb_swap = DSI_RGB_SWAP_BGR;
		else if (!strcmp(data, "rgb_swap_brg"))
			        pinfo->mipi.rgb_swap = DSI_RGB_SWAP_BRG;
		else if (!strcmp(data, "rgb_swap_grb"))
			        pinfo->mipi.rgb_swap = DSI_RGB_SWAP_GRB;
		else if (!strcmp(data, "rgb_swap_gbr"))
			        pinfo->mipi.rgb_swap = DSI_RGB_SWAP_GBR;
	}

	pinfo->mipi.data_lane0 = of_property_read_bool(np, "hw,lcdkit-lane-0-state");
	pinfo->mipi.data_lane1 = of_property_read_bool(np, "hw,lcdkit-lane-1-state");
	pinfo->mipi.data_lane2 = of_property_read_bool(np, "hw,lcdkit-lane-2-state");
	pinfo->mipi.data_lane3 = of_property_read_bool(np, "hw,lcdkit-lane-3-state");

	rc = mdss_panel_parse_display_timings(np, &ctrl_pdata->panel_data);
	if (rc)
		return rc;

	pinfo->mipi.rx_eot_ignore = of_property_read_bool(np, "hw,lcdkit-rx-eot-ignore");
	pinfo->mipi.tx_eot_append = of_property_read_bool(np, "hw,lcdkit-tx-eot-append");

	OF_PROPERTY_READ_U8_DEFAULT(np, "hw,lcdkit-stream", &pinfo->mipi.stream, 0);

	data = of_get_property(np, "hw,lcdkit-mode-sel-gpio-state", NULL);
	if (data) {
#ifdef CONFIG_ARCH_SDM660
         if (!strcmp(data, "single_port"))
             pinfo->mode_sel_state = MODE_SEL_SINGLE_PORT;
         else if (!strcmp(data, "dual_port"))
             pinfo->mode_sel_state = MODE_SEL_DUAL_PORT;
         else if (!strcmp(data, "high"))
             pinfo->mode_sel_state = MODE_GPIO_HIGH;
         else if (!strcmp(data, "low"))
             pinfo->mode_sel_state = MODE_GPIO_LOW;
#else
		if (!strcmp(data, "high"))
			pinfo->mode_gpio_state = MODE_GPIO_HIGH;
		else if (!strcmp(data, "low"))
			pinfo->mode_gpio_state = MODE_GPIO_LOW;

#endif
		 } else {
#ifdef CONFIG_ARCH_SDM660
			pinfo->mode_sel_state = MODE_GPIO_NOT_VALID;
#else
			pinfo->mode_gpio_state = MODE_GPIO_NOT_VALID;
#endif
	}

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mdp-transfer-time-us",
                &pinfo->mdp_transfer_time_us, DEFAULT_MDP_TRANSFER_TIME);

	pinfo->mipi.lp11_init = of_property_read_bool(np, "hw,lcdkit-lp11-init");

    #if 0
	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-init-delay-us", &pinfo->mipi.init_delay, 0);
    #else
    pinfo->mipi.init_delay = lcdkit_info.panel_infos.delay_af_LP11;
    #endif

	OF_PROPERTY_READ_U32_DEFAULT(np,
                "hw,lcdkit-post-init-delay", &pinfo->mipi.post_init_delay, 0);

	mdss_dsi_parse_trigger(np, &(pinfo->mipi.mdp_trigger), "hw,lcdkit-mdp-trigger");
	mdss_dsi_parse_trigger(np, &(pinfo->mipi.dma_trigger), "hw,lcdkit-dma-trigger");

	lcdkit_parse_reset_seq(pinfo);

    lcdkit_trans_exist_cmds(&ctrl_pdata->off_cmds,
                            &lcdkit_info.panel_infos.display_off_cmds);

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-adjust-timer-wakeup-ms",
                    &pinfo->adjust_timer_delay_ms, 0);

	pinfo->mipi.force_clk_lane_hs
	            = of_property_read_bool(np, "hw,lcdkit-force-clock-lane-hs");

	rc = mdss_dsi_parse_panel_features(np, ctrl_pdata);
	if (rc) {
		LCDKIT_ERR("failed to parse panel features, ret = %d.\n", rc );
		goto error;
	}

	mdss_dsi_parse_panel_horizintal_line_idle(np, ctrl_pdata);

	mdss_dsi_parse_dfps_config(np, ctrl_pdata);

#ifdef CONFIG_ARCH_SDM660
	mdss_dsi_set_refresh_rate_range(np, pinfo);
#endif

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-bl-en-gpio",
		&ctrl_pdata->disp_bl_gpio, -1);

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-bl-chip-init",
			&lcdkit_info.panel_infos.bl_chip_init, 0);

	OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-bl-level-limit-percent",
			&lcdkit_info.panel_infos.bl_level_limit_percent, 0);

	pinfo->is_dba_panel = of_property_read_bool(np, "hw,lcdkit-dba-panel");
	if (pinfo->is_dba_panel)
    {
		bridge_chip_name = of_get_property(np, "hw,lcdkit-bridge-name", &len);
		if (!bridge_chip_name || len <= 0)
        {
			LCDKIT_ERR("%d Unable to read hw,lcdkit-bridge-name data=%p,len=%d\n",
				        __LINE__, bridge_chip_name, len);

			goto error;
		}
		strlcpy(ctrl_pdata->bridge_name, bridge_chip_name, MSM_DBA_CHIP_NAME_MAX_LEN);
	}
#ifdef CONFIG_ARCH_SDM660
	rc = of_property_read_u32(np,
				"qcom,mdss-dsi-host-esc-clk-freq-hz", &pinfo->esc_clk_rate_hz);
	if (rc)
         pinfo->esc_clk_rate_hz = MDSS_DSI_MAX_ESC_CLK_RATE_HZ;
    pr_debug("%s: esc clk %d\n", __func__, pinfo->esc_clk_rate_hz);
	return 0;
#endif
    return rc;

error:
	return -EINVAL;

}

int mdss_dsi_panel_init(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	int ndx)
{
	int rc = 0;
	struct mdss_panel_info *pinfo;

	if (!node || !ctrl_pdata) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl_pdata->panel_data.panel_info;

	pr_debug("%s:%d\n", __func__, __LINE__);

    lcdkit_init(node, ctrl_pdata);

    rc = lcdkit_app_info_set(pinfo);
	if (rc) {
		pr_err("%s:%d set panel app_info failed\n", __func__, __LINE__);
		return rc;
	}

	pinfo->dynamic_switch_pending = false;
	pinfo->is_lpm_mode = false;
	pinfo->esd_rdy = false;

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->post_panel_on = mdss_dsi_post_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->low_power_config = mdss_dsi_panel_low_power_config;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->switch_mode = mdss_dsi_panel_switch_mode;

	return 0;
}

#include "lcdkit_parse_msm.c"

