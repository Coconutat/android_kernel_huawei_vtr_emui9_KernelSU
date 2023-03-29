/*
* Simple driver for Texas Instruments LM3639 Backlight + Flash LED driver chip
* Copyright (C) 2012 Texas Instruments
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include "lm36274.h"
#include "../hisi_fb.h"
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory
#include <dsm/dsm_pub.h>
extern struct dsm_client *lcd_dclient;
#include "lcdkit_panel.h"

struct class *lm36274_class = NULL;
struct lm36274_chip_data *lm36274_g_chip = NULL;
static bool lm36274_init_status = false;
static unsigned int g_reg_val[LM36274_RW_REG_MAX] = {0};
static int g_bl_level_enhance_mode = 0;
static int g_hidden_reg_support = 0;
#define MAX_TRY_NUM 3
static int g_force_resume_bl_flag = RESUME_IDLE;
#define MIN_BL_RESUME_TIMMER 1
#define MAX_BL_RESUME_TIMMER 400
#define PROTECT_BL_RESUME_TIMMER 28
static int g_resume_bl_duration = 0;  /* default not support auto resume*/
static enum hrtimer_restart lm36274_bl_resume_hrtimer_fnc(struct hrtimer *timer);
static void lm36274_bl_resume_workqueue_handler(struct work_struct *work);
extern int bl_lvl_map(int level);

/*
** for debug, S_IRUGO
** /sys/module/hisifb/parameters
*/
unsigned lm36274_msg_level = 7;
module_param_named(debug_lm36274_msg_level, lm36274_msg_level, int, 0644);
MODULE_PARM_DESC(debug_lm36274_msg_level, "backlight lm36274 msg level");

static void lm36274_get_target_voltage(int *vpos_target,int *vneg_target)
{
	int i = 0;
	int j = 0;

	if((vpos_target == NULL) || (vneg_target == NULL)){
		LM36274_ERR("vpos_target or vneg_target is NULL pointer \n");
		return;
	}

	for (i = 0;i < sizeof(lm36274_voltage_table) / sizeof(struct lm36274_vsp_vsn_voltage);i++) {
		if (lm36274_voltage_table[i].voltage == lcdkit_info.panel_infos.lcd_vsp) {
			LM36274_INFO("vsp voltage:%ld\n",lm36274_voltage_table[i].voltage);
			*vpos_target = lm36274_voltage_table[i].value;
			break;
		}
	}

	if (i >= sizeof(lm36274_voltage_table) / sizeof(struct lm36274_vsp_vsn_voltage)) {
		LM36274_INFO("not found vsp voltage, use default voltage:LM36274_VOL_600\n");
		*vpos_target = LM36274_VOL_600;
	}

	for (j = 0;j < sizeof(lm36274_voltage_table) / sizeof(struct lm36274_vsp_vsn_voltage);j++) {
		if (lm36274_voltage_table[j].voltage == lcdkit_info.panel_infos.lcd_vsn) {
			LM36274_INFO("vsn voltage:%ld\n",lm36274_voltage_table[j].voltage);
			*vneg_target = lm36274_voltage_table[j].value;
			break;
		}
	}

	if (j >= sizeof(lm36274_voltage_table)/sizeof(struct lm36274_vsp_vsn_voltage)) {
		LM36274_INFO("not found vsn voltage, use default voltage:LM36274_VOL_600\n");
		*vneg_target = LM36274_VOL_600;
	}

	LM36274_INFO("*vpos_target=0x%x,*vneg_target=0x%x\n",*vpos_target,*vneg_target);

	return;

}

static int lm36274_parse_dts(struct device_node *np)
{
	int ret = 0;
	int i = 0;
	int vpos_target = 0;
	int vneg_target = 0;

	if(np == NULL){
		LM36274_ERR("np is NULL pointer \n");
		return -1;
	}

	for (i = 0;i < LM36274_RW_REG_MAX;i++ ) {
		ret = of_property_read_u32(np, lm36274_dts_string[i], &bl_info.lm36274_reg[i]);
		if (ret < 0) {
			LM36274_ERR("get lm36274 dts config failed\n");
		} else {
			LM36274_INFO("get %s from dts value = 0x%x\n", lm36274_dts_string[i],bl_info.lm36274_reg[i]);
		}
	}

	if (lcdkit_info.panel_infos.bias_change_lm36274_from_panel_support) {
		lm36274_get_target_voltage(&vpos_target, &vneg_target);
		/* bl_info.lm36274_reg[8] :8 is the position of vsp in bl_info.lm36274_reg during kernel*/
		/* bl_info.lm36274_reg[9] :9 is the position of vsn in bl_info.lm36274_reg during kernel*/
		if (bl_info.lm36274_reg[8] != vpos_target) {
			bl_info.lm36274_reg[8] = vpos_target;
		}
		if (bl_info.lm36274_reg[9] != vneg_target) {
			bl_info.lm36274_reg[9] = vneg_target;
		}
	}

	return ret;
}

static int lm36274_config_write(struct lm36274_chip_data *pchip,
			unsigned int reg[],unsigned int val[],unsigned int size)
{
	int ret = 0;
	unsigned int i = 0;

	if((pchip == NULL) || (reg == NULL) || (val == NULL)){
		LM36274_ERR("pchip or reg or val is NULL pointer \n");
		return -1;
	}

	for(i = 0;i < size;i++) {
		ret = regmap_write(pchip->regmap, reg[i], val[i]);
		if (ret < 0) {
			LM36274_ERR("write lm36274 backlight config register 0x%x failed\n",reg[i]);
			goto exit;
		} else {
			LM36274_INFO("register 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

static int lm36274_config_read(struct lm36274_chip_data *pchip,
			unsigned int reg[],unsigned int val[],unsigned int size)
{
	int ret = 0;
	unsigned int i = 0;

	if((pchip == NULL) || (reg == NULL) || (val == NULL)){
		LM36274_ERR("pchip or reg or val is NULL pointer \n");
		return -1;
	}

	for(i = 0;i < size;i++) {
		ret = regmap_read(pchip->regmap, reg[i],&val[i]);
		if (ret < 0) {
			LM36274_ERR("read lm36274 backlight config register 0x%x failed",reg[i]);
			goto exit;
		} else {
			LM36274_INFO("read 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

static void lm36274_bl_mode_reg_init(unsigned int reg[],unsigned int val[],unsigned int size)
{
	unsigned int i;
	unsigned int reg_element_num = 0;

	if((reg == NULL) || (val == NULL)){
		LM36274_ERR("reg or val is NULL pointer\n");
		return;
	}

	LM36274_INFO("lm36274_bl_mode_reg_init in \n");
	memset(&g_bl_work_mode_reg_indo, 0, sizeof(struct backlight_work_mode_reg_info));

	for (i=0; i<size; i++)
	{
		switch (reg[i])
		{
		case REG_BL_CONFIG_1:
		case REG_BL_CONFIG_2:
		case REG_BL_OPTION_2:
		    reg_element_num = g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num;
		    if (reg_element_num >= BL_MAX_CONFIG_REG_NUM)
		    {
		        break;
		    }
		    g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr[reg_element_num] = reg[i];
		    g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var[reg_element_num] = val[i];
		    if (REG_BL_CONFIG_1 == reg[i])
		    {
		        g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] = BL_OVP_29V;
		    }
		    else if (REG_BL_CONFIG_2 == reg[i])
		    {
		        g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] = CURRENT_RAMP_5MS;
		    }
		    else
		    {
		        g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num] = BL_OCP_2;
		    }

			LM36274_INFO("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
				g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr[reg_element_num],
				g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var[reg_element_num],
				g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var[reg_element_num]);

		    g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num++;
		    break;
		case REG_BL_ENABLE:
		    reg_element_num = g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num;
		    if (reg_element_num >= BL_MAX_CONFIG_REG_NUM)
		    {
				break;
		    }
		    g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr[reg_element_num] = reg[i];
		    g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var[reg_element_num] = val[i];
		    g_bl_work_mode_reg_indo.bl_enable_config_reg.enhance_reg_var[reg_element_num] = EN_4_SINK;

			LM36274_INFO("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
				g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr[reg_element_num],
				g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var[reg_element_num],
				g_bl_work_mode_reg_indo.bl_enable_config_reg.enhance_reg_var[reg_element_num]);

		    g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num++;
		    break;
		default:
		    break;
		}
	}

	reg_element_num = g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num;
	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num] = REG_BL_BRIGHTNESS_LSB;
	g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num] = LSB_10MA;
	g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num] = g_bl_level_enhance_mode & LSB;

	LM36274_INFO("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num]);

	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num++;
	reg_element_num = g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num;
	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num] = REG_BL_BRIGHTNESS_MSB;
	g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num] = MSB_10MA;
	g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num] = g_bl_level_enhance_mode >> MSB;

	LM36274_INFO("reg_addr:0x%x, normal_val=0x%x, enhance_val=0x%x\n",
		g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var[reg_element_num],
		g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var[reg_element_num]);

	g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num++;

	LM36274_INFO("lm36274_bl_mode_reg_init success \n");
	return;
}
static int lm36274_set_hidden_reg(struct lm36274_chip_data *pchip)
{
	int ret = 0;
	unsigned int val = 0;

	if(pchip == NULL){
		LM36274_ERR("pchip is NULL pointer\n");
		return -1;
	}

	ret = regmap_write(pchip->regmap, REG_SET_SECURITYBIT_ADDR, REG_SET_SECURITYBIT_VAL);
	if (ret < 0) {
		LM36274_ERR("write lm36274_set_hidden_reg register 0x%x failed\n",REG_SET_SECURITYBIT_ADDR);
		goto out;
	} else {
		LM36274_INFO("register 0x%x value = 0x%x\n", REG_SET_SECURITYBIT_ADDR, REG_SET_SECURITYBIT_VAL);
	}

	ret = regmap_read(pchip->regmap, REG_HIDDEN_ADDR, &val);
	if (ret < 0) {
		LM36274_ERR("read lm36274_set_hidden_reg register 0x%x failed", REG_HIDDEN_ADDR);
		goto out;
	} else {
		LM36274_INFO("read 0x%x value = 0x%x\n", REG_HIDDEN_ADDR, val);
	}

	if (BIAS_BOOST_TIME_3 != (val & BIAS_BOOST_TIME_3))
	{
		val |= BIAS_BOOST_TIME_3;

		ret = regmap_write(pchip->regmap, REG_HIDDEN_ADDR, val);
		if (ret < 0) {
			LM36274_ERR("write lm36274_set_hidden_reg register 0x%x failed\n",REG_HIDDEN_ADDR);
			goto out;
		} else {
			LM36274_INFO("register 0x%x value = 0x%x\n", REG_HIDDEN_ADDR, val);
		}

		ret = regmap_read(pchip->regmap, REG_HIDDEN_ADDR, &val);
		if (ret < 0) {
			LM36274_ERR("read lm36274_set_hidden_reg register 0x%x failed", REG_HIDDEN_ADDR);
			goto out;
		} else {
			LM36274_INFO("read 0x%x value = 0x%x\n", REG_HIDDEN_ADDR, val);
		}
	}

out:
	ret = regmap_write(pchip->regmap, REG_SET_SECURITYBIT_ADDR, REG_CLEAR_SECURITYBIT_VAL);
	if (ret < 0) {
		LM36274_ERR("write lm36274_set_hidden_reg register 0x%x failed\n",REG_SET_SECURITYBIT_ADDR);
	} else {
		LM36274_INFO("register 0x%x value = 0x%x\n", REG_SET_SECURITYBIT_ADDR, REG_CLEAR_SECURITYBIT_VAL);
	}

	LM36274_INFO("lm36274_set_hidden_reg exit\n");
	return ret;
}
/* initialize chip */
static int lm36274_chip_init(struct lm36274_chip_data *pchip)
{
    int ret = -1;
    struct device_node *np = NULL;
    unsigned int enable_reg = 0;

    LM36274_INFO("in!\n");

	if(pchip == NULL){
		LM36274_ERR("pchip is NULL pointer\n");
		return -1;
	}

    memset(&bl_info, 0, sizeof(struct backlight_information));

    np = of_find_compatible_node(NULL, NULL, DTS_COMP_LM36274);
    if (!np) {
        LM36274_ERR("NOT FOUND device node %s!\n", DTS_COMP_LM36274);
        goto out;
    }

    ret = lm36274_parse_dts(np);
    if (ret < 0) {
        LM36274_ERR("parse lm36274 dts failed");
        goto out;
    }

	ret = of_property_read_u32(np, "lm36274_bl_max_level", &g_bl_level_enhance_mode);
	if (ret) {
		g_bl_level_enhance_mode = BL_MAX;
	}
	LM36274_INFO("g_bl_level_enhance_mode = %d\n", g_bl_level_enhance_mode);

	ret = of_property_read_u32(np, LM36274_HIDDEN_REG_SUPPORT, &g_hidden_reg_support);
	if (ret) {
		g_hidden_reg_support = 0;
	}
	LM36274_INFO("g_hidden_reg_support = %d\n", g_hidden_reg_support);

    if (runmode_is_factory()) {
		ret = of_property_read_u32(np, LM36274_RUNNING_RESUME_BL_TIMMER, &g_resume_bl_duration);
		if (ret) {
			g_resume_bl_duration = 0;
		}
	} else {
		ret = of_property_read_u32(np, LM36274_UPDATE_RESUME_BL_TIMMER, &g_resume_bl_duration);
		if (ret) {
			g_resume_bl_duration = 0;
		}
	}

	if (g_resume_bl_duration > MAX_BL_RESUME_TIMMER){
		g_resume_bl_duration = MAX_BL_RESUME_TIMMER;
	}

	LM36274_INFO("g_resume_bl_duration = %d\n", g_resume_bl_duration);

	if (g_hidden_reg_support)
	{
		ret = lm36274_set_hidden_reg(pchip);
	    if (ret < 0) {
	        LM36274_INFO("lm36274_set_hidden_reg failed");
	    }
    }

    ret = lm36274_config_write(pchip, lm36274_reg_addr, bl_info.lm36274_reg, LM36274_RW_REG_MAX);
    if (ret < 0) {
        LM36274_ERR("lm36274 config register failed");
        goto out;
    }

    ret = lm36274_config_read(pchip, lm36274_reg_addr, bl_info.lm36274_reg, LM36274_RW_REG_MAX);
    if (ret < 0) {
        LM36274_ERR("lm36274 config read failed");
        goto out;
    }

    lm36274_bl_mode_reg_init(lm36274_reg_addr, bl_info.lm36274_reg, LM36274_RW_REG_MAX);
	pchip->bl_resume_wq = create_singlethread_workqueue("bl_resume");
	if (!pchip->bl_resume_wq) {
		LM36274_ERR("create bl_resume_wq failed\n");
	}

	INIT_WORK(&pchip->bl_resume_worker, lm36274_bl_resume_workqueue_handler);
	hrtimer_init(&pchip->bl_resume_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pchip->bl_resume_hrtimer.function = lm36274_bl_resume_hrtimer_fnc;

    if (g_fake_lcd_flag) {
        LM36274_INFO("is unknown lcd\n");
        /* BL disable when fake lcd on */
        ret = regmap_write(pchip->regmap, REG_BL_ENABLE, enable_reg);
        if (ret < 0) {
            goto out;
        }

        /* Bias supply off when fake lcd on */
        ret = regmap_update_bits(pchip->regmap, REG_DISPLAY_BIAS_CONFIG_1, MASK_LCM_EN,BIAS_SUPPLY_OFF);
        LM36274_INFO("Bias supply off when fake lcd on\n");
        if (ret < 0) {
            goto out;
        }
    }

    LM36274_INFO("ok!\n");

    return ret;

out:
    dev_err(pchip->dev, "i2c failed to access register\n");
    return ret;
}
static int lm36274_check_ovp_error(void)
{
    int ret = -1;
    unsigned int val = 0;
    struct i2c_client *client;

    if (!lm36274_init_status)
    {
        LM36274_ERR("lm36274_init fail, return.\n");
        return ret;
    }

    client = lm36274_g_chip->client;

    if (!g_fake_lcd_flag) {
        ret = regmap_read(lm36274_g_chip->regmap, REG_FLAGS, &val);
        LM36274_INFO("lm36274_check_ovp_error:regmap_read val %u \n", val);
        if (ret < 0) {
            dev_err(&client->dev, "fail : read chip reg REG_FAULT_FLAG error!\n");
            goto err_out;
        }

        if (DEVICE_FAULT_OCCUR != val) {
			ret = dsm_client_ocuppy(lcd_dclient);
	        if (!ret) {
                dev_err(&client->dev, "fail : REG_FAULT_FLAG statues error 0X0F=%d!\n", val);
                dsm_client_record(lcd_dclient, "REG_FAULT_FLAG statues error 0X0F=%d!\n", val);
                dsm_client_notify(lcd_dclient, DSM_LCD_OVP_ERROR_NO);
            } else {
                dev_err(&client->dev, "dsm_client_ocuppy fail:  ret=%d!\n", ret);
            }
        }
    }

err_out:
    return ret;
}

/**
 * lm36274_set_backlight_reg(): Set Backlight working mode
 *
 * @bl_level: value for backlight ,range from 0 to 2047
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36274_set_backlight_reg(uint32_t bl_level)
{
	ssize_t ret = -1;
	uint32_t level = 0;
	int bl_msb = 0;
	int bl_lsb = 0;
	static int last_level = -1;
	static int enable_flag = 0;
	static int disable_flag = 0;

	if (!lm36274_init_status) {
		LM36274_ERR("init fail, return.\n");
		return ret;
	}

	if (down_trylock(&(lm36274_g_chip->test_sem))) {
		LM36274_INFO("Now in test mode\n");
		return 0;
	}


	LM36274_INFO("lm36274_set_backlight_reg bl_level = %u \n", bl_level);

	//blkit_force_resume_reg_proc();

	level = bl_level;

	if (level > BL_MAX) {
		level = BL_MAX;
	}

	if (g_fake_lcd_flag) {
		if (level > 0) {
			if (!enable_flag) {
				ret = regmap_write(lm36274_g_chip->regmap, REG_BL_ENABLE, bl_info.lm36274_reg[LM36274_RW_REG_MAX-1]);
				LM36274_INFO("REG_BL_ENABLE = 0x%x\n", bl_info.lm36274_reg[LM36274_RW_REG_MAX-1]);
				mdelay(16);
			}
			enable_flag = 1;
			disable_flag = 0;
		} else {
			if (!disable_flag) {
					ret = regmap_write(lm36274_g_chip->regmap, REG_BL_ENABLE, BL_DISABLE);
					LM36274_INFO("REG_BL_ENABLE = 0x0\n");
					mdelay(16);
				}
				disable_flag = 1;
				enable_flag = 0;
			}
	}

	/* 11-bit brightness code */
	bl_msb = level >> MSB;
	bl_lsb = level & LSB;

	if ((BL_MIN == last_level && LOG_LEVEL_INFO == lm36274_msg_level)
		|| (BL_MIN == level && LOG_LEVEL_INFO == lm36274_msg_level)
		|| (-1 == last_level)) {
		LM36274_INFO("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);
	}

	LM36274_DEBUG("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);

	ret = regmap_update_bits(lm36274_g_chip->regmap, REG_BL_BRIGHTNESS_LSB, MASK_BL_LSB,bl_lsb);
	if (ret < 0) {
		goto i2c_error;
	}

	ret = regmap_write(lm36274_g_chip->regmap, REG_BL_BRIGHTNESS_MSB, bl_msb);
	if (ret < 0) {
		goto i2c_error;
	}

	last_level = level;
	up(&(lm36274_g_chip->test_sem));
	LM36274_INFO("lm36274_set_backlight_reg exit succ \n");
	return ret;

i2c_error:
	up(&(lm36274_g_chip->test_sem));
	dev_err(lm36274_g_chip->dev, "%s:i2c access fail to register\n", __func__);
	LM36274_INFO("lm36274_set_backlight_reg exit fail \n");
	return ret;
}
/* EXPORT_SYMBOL(lm36274_set_backlight_reg); */

/**
 * lm36274_set_reg(): Set lm36274 reg
 *
 * @bl_reg: which reg want to write
 * @bl_mask: which bits of reg want to change
 * @bl_val: what value want to write to the reg
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36274_set_reg(u8 bl_reg,u8 bl_mask,u8 bl_val)
{
	ssize_t ret = -1;
	u8 reg = bl_reg;
	u8 mask = bl_mask;
	u8 val = bl_val;

	if (!lm36274_init_status) {
		LM36274_ERR("init fail, return.\n");
		return ret;
	}

	if (REG_MAX < reg) {
		LM36274_ERR("Invalid argument!!!\n");
		return ret;
	}

	LM36274_INFO("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(lm36274_g_chip->regmap, reg, mask, val);
	if (ret < 0) {
		LM36274_ERR("i2c access fail to register\n");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(lm36274_set_reg);

static ssize_t lm36274_reg_bl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36274_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	int bl_lsb = 0;
	int bl_msb = 0;
	int bl_level = 0;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BL_BRIGHTNESS_MSB, &bl_msb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36274 I2C read error\n");

	ret = regmap_read(pchip->regmap, REG_BL_BRIGHTNESS_LSB, &bl_lsb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36274 I2C read error\n");

	bl_level = (bl_msb << MSB) | bl_lsb;

	return snprintf(buf, PAGE_SIZE, "LM36274 bl_level:%d\n", bl_level);
}

static ssize_t lm36274_reg_bl_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36274_chip_data *pchip = NULL;
	unsigned int bl_level = 0;
	unsigned int bl_msb = 0;
	unsigned int bl_lsb = 0;

	if (!dev)
		return snprintf((char *)buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf((char *)buf, PAGE_SIZE, "data is null\n");

	ret = kstrtouint(buf, 10, &bl_level);
	if (ret) {
		goto out_input;
	}

	LM36274_INFO("%s:buf=%s,state=%d\n", __func__, buf, bl_level);

	if (bl_level > BL_MAX)
		bl_level = BL_MAX;

	/* 11-bit brightness code */
	bl_msb = bl_level >> MSB;
	bl_lsb = bl_level & LSB;

	LM36274_INFO("bl_level = %d, bl_msb = %d, bl_lsb = %d\n", bl_level, bl_msb, bl_lsb);

	ret = regmap_update_bits(pchip->regmap, REG_BL_BRIGHTNESS_LSB, MASK_BL_LSB, bl_lsb);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_write(pchip->regmap, REG_BL_BRIGHTNESS_MSB, bl_msb);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(reg_bl, (S_IRUGO|S_IWUSR), lm36274_reg_bl_show, lm36274_reg_bl_store);

static ssize_t lm36274_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36274_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	unsigned char val[REG_MAX] = {0};

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_bulk_read(pchip->regmap, REG_REVISION, &val[0], REG_MAX);
	if (ret < 0)
		goto i2c_error;

	return snprintf(buf, PAGE_SIZE, "Revision(0x01)= 0x%x\nBacklight Configuration1(0x02)= 0x%x\n \
			\rBacklight Configuration2(0x03) = 0x%x\nBacklight Brightness LSB(0x04) = 0x%x\n \
			\rBacklight Brightness MSB(0x05) = 0x%x\nBacklight Auto-Frequency Low(0x06) = 0x%x\n \
			\rBacklight Auto-Frequency High(0x07) = 0x%x\nBacklight Enable(0x08) = 0x%x\n \
			\rDisplay Bias Configuration 1(0x09)  = 0x%x\nDisplay Bias Configuration 2(0x0A)  = 0x%x\n \
			\rDisplay Bias Configuration 3(0x0B) = 0x%x\nLCM Boost Bias Register(0x0C) = 0x%x\n \
			\rVPOS Bias Register(0x0D) = 0x%x\nVNEG Bias Register(0x0E) = 0x%x\n \
			\rFlags Register(0x0F) = 0x%x\nBacklight Option 1 Register(0x10) = 0x%x\n \
			\rBacklight Option 2 Register(0x11) = 0x%x\nPWM-to-Digital Code Readback LSB(0x12) = 0x%x\n \
			\rPWM-to-Digital Code Readback MSB(0x13) = 0x%x\n",
			val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7],
			val[8],val[9],val[10],val[11],val[12],val[13],val[14],val[15],
			val[16],val[17],val[18]);

i2c_error:
	return snprintf(buf, PAGE_SIZE,"%s: i2c access fail to register\n", __func__);
}

static ssize_t lm36274_reg_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36274_chip_data *pchip = NULL;
	unsigned int reg = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	if (!dev)
		return snprintf((char *)buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf((char *)buf, PAGE_SIZE, "data is null\n");

	ret = sscanf(buf, "reg=0x%x, mask=0x%x, val=0x%x",&reg,&mask,&val);
	if (ret < 0) {
		printk("check your input!!!\n");
		goto out_input;
	}

	if (reg > REG_MAX) {
		printk("Invalid argument!!!\n");
		goto out_input;
	}

	LM36274_INFO("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(pchip->regmap, reg, mask, val);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(reg, (S_IRUGO|S_IWUSR), lm36274_reg_show, lm36274_reg_store);

static enum hrtimer_restart lm36274_bl_resume_hrtimer_fnc(struct hrtimer *timer)
{
	struct lm36274_chip_data *bl_chip_ctrl = NULL;

	bl_chip_ctrl = container_of(timer, struct lm36274_chip_data, bl_resume_hrtimer);
	if (bl_chip_ctrl == NULL){
        LM36274_INFO("bl_chip_ctrl is NULL, return\n");
		return HRTIMER_NORESTART;
	}

	if (bl_chip_ctrl->bl_resume_wq) {
		queue_work(bl_chip_ctrl->bl_resume_wq, &(bl_chip_ctrl->bl_resume_worker));
	}

	return HRTIMER_NORESTART;
}
static void lm36274_bl_resume_workqueue_handler(struct work_struct *work)
{
	ssize_t error = -1;
	struct lm36274_chip_data *bl_chip_ctrl = NULL;

	LM36274_INFO("lm36274_bl_resume_workqueue_handler in\n");

	bl_chip_ctrl = container_of(work, struct lm36274_chip_data, bl_resume_worker);
	if (bl_chip_ctrl == NULL){
        LM36274_ERR("bl_chip_ctrl is NULL, return\n");
		return;
	}

	if (down_interruptible(&(lm36274_g_chip->test_sem))) {
		if (bl_chip_ctrl->bl_resume_wq) {
			queue_work(bl_chip_ctrl->bl_resume_wq, &(bl_chip_ctrl->bl_resume_worker));
		}
		LM36274_INFO("down_trylock get sem fail return\n");
		return ;
	}

	if (g_force_resume_bl_flag != RESUME_2_SINK
		&& g_force_resume_bl_flag != RESUME_REMP_OVP_OCP) {
        LM36274_ERR("g_force_resume_bl_flag = 0x%x, return \n",g_force_resume_bl_flag);
		up(&(lm36274_g_chip->test_sem));
		return;
	}

	error = lm36274_check_ovp_error();
	if( error ) {
		LM36274_ERR("lm36274_check_ovp_error return error\n");
	}

	if ( RESUME_2_SINK == g_force_resume_bl_flag) {
		LM36274_INFO("resume RESUME_LINK \n");
		/* set 4 link to 2 link*/
	    error = lm36274_config_write(lm36274_g_chip,
	                                g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr,
	                                g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var,
	                                g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num);
	    if ( error ) {
			LM36274_INFO("resume 2 sink fail return \n");
			if (bl_chip_ctrl->bl_resume_wq) {
				queue_work(bl_chip_ctrl->bl_resume_wq, &(bl_chip_ctrl->bl_resume_worker));
			}
			up(&(lm36274_g_chip->test_sem));
			return;
		}

		error = lm36274_config_write(lm36274_g_chip,
									g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr,
									g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var,
									g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num);
		if( error ) {
			LM36274_ERR("set bl_current_config_reg fail\n");
		}

		g_force_resume_bl_flag = RESUME_REMP_OVP_OCP;

		/* 6ms */
	    mdelay(BL_LOWER_POW_DELAY);
	}

	if (RESUME_REMP_OVP_OCP == g_force_resume_bl_flag) {
		LM36274_INFO("resume RESUME_REMP_OVP_OCP \n");
		error = lm36274_config_write(lm36274_g_chip,
									g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr,
									g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var,
									g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num);
	    if ( error ) {
			LM36274_INFO("resume OVP fail return \n");
			if (bl_chip_ctrl->bl_resume_wq) {
				queue_work(bl_chip_ctrl->bl_resume_wq, &(bl_chip_ctrl->bl_resume_worker));
			}
			up(&(lm36274_g_chip->test_sem));
			return;
		}

		g_force_resume_bl_flag = RESUME_IDLE;
	}

	up(&(lm36274_g_chip->test_sem));
	LM36274_INFO("lm36274_bl_resume_workqueue_handler exit \n");
	return;
}
int lm36274_get_bl_resume_timmer(int *timmer)
{
    int ret = -1;

	if (timmer) {
		*timmer = g_resume_bl_duration;
		ret = 0;
		LM36274_INFO("timmer %d!\n", *timmer);
	}
	return ret;
}

ssize_t blkit_set_normal_work_mode(void)
{
    ssize_t error = -1;

    if (!lm36274_init_status)
    {
        LM36274_ERR("lm36274_init fail, return.\n");
        return error;
    }

	LM36274_INFO("blkit_set_normal_work_mode in \n");
	if (down_interruptible(&lm36274_g_chip->test_sem)) {
		LM36274_ERR("down_trylock fail return\n");
		return error;
    }

	error = lm36274_config_write(lm36274_g_chip,
                                g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr,
                                g_bl_work_mode_reg_indo.bl_current_config_reg.normal_reg_var,
                                g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num);
    if( error ) {
        LM36274_ERR("set bl_current_config_reg fail\n");
    } else {
	    /* 6ms */
	    mdelay(BL_LOWER_POW_DELAY);
    }

	/* set 4  to 2 sink*/
    error = lm36274_config_write(lm36274_g_chip,
                                g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr,
                                g_bl_work_mode_reg_indo.bl_enable_config_reg.normal_reg_var,
                                g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num);
    if ( error ) {
        LM36274_ERR("set bl_enable_config_reg fail\n");
		g_force_resume_bl_flag = RESUME_2_SINK;
		goto out;
    }

	/*6ms*/
	mdelay(BL_LOWER_POW_DELAY);

	error = lm36274_check_ovp_error();
	if( error )
	{
		LM36274_ERR("lm36274_check_ovp_error return error\n");
	}

	g_force_resume_bl_flag = RESUME_REMP_OVP_OCP;
    error = lm36274_config_write(lm36274_g_chip,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.normal_reg_var,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num);
    if( error ) {
        LM36274_ERR("set bl_mode_config_reg fail\n");
        goto out;
    }

	if (0 != g_resume_bl_duration) {
		hrtimer_cancel(&lm36274_g_chip->bl_resume_hrtimer);
	}

	g_force_resume_bl_flag = RESUME_IDLE;

    up(&(lm36274_g_chip->test_sem));
    LM36274_INFO("blkit_set_normal_work_mode exit!\n");
    return error;

out:
    up(&(lm36274_g_chip->test_sem));
    LM36274_INFO("blkit_set_normal_work_mode failed \n");
    return error;
}

ssize_t blkit_set_enhance_work_mode(void)
{
    ssize_t error = -1;

    if (!lm36274_init_status)
    {
        LM36274_ERR("lm36274_init fail, return.\n");
        return error;
    }

    LM36274_INFO("blkit_set_enhance_work_mode in!\n");
	if (down_interruptible(&lm36274_g_chip->test_sem)) {
		LM36274_ERR("down_trylock fail return\n");
		return error;
    }

	g_force_resume_bl_flag = RESUME_IDLE;
    error = lm36274_config_write(lm36274_g_chip,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_addr,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.enhance_reg_var,
                                g_bl_work_mode_reg_indo.bl_mode_config_reg.reg_element_num);
    if( error ) {
        LM36274_ERR("set bl_mode_config_reg fail\n");
		goto out;
    }

	g_force_resume_bl_flag = RESUME_REMP_OVP_OCP;
	error = lm36274_config_write(lm36274_g_chip,
								g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_addr,
								g_bl_work_mode_reg_indo.bl_enable_config_reg.enhance_reg_var,
								g_bl_work_mode_reg_indo.bl_enable_config_reg.reg_element_num);
	if( error ) {
        LM36274_ERR("set bl_enable_config_reg fail\n");
		goto out;
	}

	error = lm36274_check_ovp_error();
	if( error )
	{
		LM36274_ERR("lm36274_check_ovp_error return error\n");
	}

	g_force_resume_bl_flag = RESUME_2_SINK;
	error = lm36274_config_write(lm36274_g_chip,
								g_bl_work_mode_reg_indo.bl_current_config_reg.reg_addr,
								g_bl_work_mode_reg_indo.bl_current_config_reg.enhance_reg_var,
								g_bl_work_mode_reg_indo.bl_current_config_reg.reg_element_num);
	if( error ) {
        LM36274_ERR("set bl_current_config_reg fail\n");
	}

	if (0 != g_resume_bl_duration) {
		hrtimer_cancel(&lm36274_g_chip->bl_resume_hrtimer);
		hrtimer_start(&lm36274_g_chip->bl_resume_hrtimer, ktime_set((g_resume_bl_duration + PROTECT_BL_RESUME_TIMMER) / 1000,
			((g_resume_bl_duration + PROTECT_BL_RESUME_TIMMER) % 1000) * 1000000), HRTIMER_MODE_REL);
	}

	up(&(lm36274_g_chip->test_sem));
    LM36274_INFO("blkit_set_enhance_work_mode exit!\n");
    return error;

out:
    up(&(lm36274_g_chip->test_sem));
    return error;
}

static const struct regmap_config lm36274_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

/* pointers to created device attributes */
static struct attribute *lm36274_attributes[] = {
	&dev_attr_reg_bl.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group lm36274_group = {
	.attrs = lm36274_attributes,
};


static int lm36274_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct lm36274_chip_data *pchip = NULL;
	int ret = -1;
	unsigned int val = 0;

	LM36274_INFO("in!\n");

	if(client == NULL){
		LM36274_ERR("client is NULL pointer\n");
		return -1;
	}

	adapter = client->adapter;
	if(adapter == NULL){
		LM36274_ERR("adapter is NULL pointer\n");
		return -1;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c functionality check fail.\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev,
				sizeof(struct lm36274_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

	pchip->regmap = devm_regmap_init_i2c(client, &lm36274_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}

	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	sema_init(&(pchip->test_sem), 1);

	/* chip initialize */
	ret = lm36274_chip_init(pchip);
	if (ret < 0) {
		dev_err(&client->dev, "fail : chip init\n");
		goto err_out;
	}

	/* HARDWARE check why ovp occur */
	/* OVP(0x2) occur if disable bl ic then read reg_flags register,so comment it */
	if (g_fake_lcd_flag) {
		ret = regmap_read(pchip->regmap, REG_FLAGS, &val);
		if (ret < 0) {
			dev_err(&client->dev, "fail : read chip reg REG_FAULT_FLAG error!\n");
			goto err_out;
		}

		if (DEVICE_FAULT_OCCUR != val) {
			ret = dsm_client_ocuppy(lcd_dclient);
			if (!ret) {
				dev_err(&client->dev, "fail : REG_FAULT_FLAG statues error 0X0F=%d!\n", val);
				dsm_client_record(lcd_dclient, "REG_FAULT_FLAG statues error 0X0F=%d!\n", val);
				dsm_client_notify(lcd_dclient, DSM_LCD_OVP_ERROR_NO);
				} else {
				dev_err(&client->dev, "dsm_client_ocuppy fail:  ret=%d!\n", ret);
			}
		}
	}

	pchip->dev = device_create(lm36274_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		LM36274_ERR("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &lm36274_group);
		if (ret)
			goto err_sysfs;
	}

	lm36274_g_chip = pchip;

	LM36274_INFO("name: %s, address: (0x%x) ok!\n", client->name, client->addr);
	lm36274_init_status = true;

	return ret;

err_sysfs:
	device_destroy(lm36274_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static int lm36274_suspend(struct device *dev)
{
    int ret;
    /* store reg val before suspend */
    lm36274_config_read(lm36274_g_chip,lm36274_reg_addr,g_reg_val,LM36274_RW_REG_MAX);
    /* reset backlight ic */
    ret = regmap_write(lm36274_g_chip->regmap, REG_BL_ENABLE, BL_RESET);
    if (ret < 0) {
        dev_err(lm36274_g_chip->dev, "%s:i2c access fail to register\n", __func__);
    }

    /* clean up bl val register */
    ret = regmap_update_bits(lm36274_g_chip->regmap, REG_BL_BRIGHTNESS_LSB, MASK_BL_LSB,BL_DISABLE);
    if (ret < 0) {
        dev_err(lm36274_g_chip->dev, "%s:i2c access fail to register\n", __func__);
    }

    ret = regmap_write(lm36274_g_chip->regmap, REG_BL_BRIGHTNESS_MSB, BL_DISABLE);
    if (ret < 0) {
        dev_err(lm36274_g_chip->dev, "%s:i2c access fail to register\n", __func__);
    }

    return ret;
}

static int lm36274_resume(struct device *dev)
{
    int ret;

    lm36274_init_status = true;
	if (g_hidden_reg_support)
	{
		ret = lm36274_set_hidden_reg(lm36274_g_chip);
	    if (ret < 0) {
	        dev_err(lm36274_g_chip->dev, "i2c access fail to lm36274 hidden register");
	    }
    }

    ret = lm36274_config_write(lm36274_g_chip, lm36274_reg_addr, g_reg_val,LM36274_RW_REG_MAX);
    if (ret < 0) {
        dev_err(lm36274_g_chip->dev, "%s:i2c access fail to register\n", __func__);
    }

    return ret;
}

static SIMPLE_DEV_PM_OPS(lm36274_pm_ops, lm36274_suspend, lm36274_resume);
#define LM36274_PM_OPS (&lm36274_pm_ops)

static int lm36274_remove(struct i2c_client *client)
{
    struct lm36274_chip_data *pchip = i2c_get_clientdata(client);

    regmap_write(pchip->regmap, REG_BL_ENABLE, BL_DISABLE);

    sysfs_remove_group(&client->dev.kobj, &lm36274_group);

	if (0 != g_resume_bl_duration) {
		hrtimer_cancel(&lm36274_g_chip->bl_resume_hrtimer);
	}

    return 0;
}

static const struct i2c_device_id lm36274_id[] = {
	{LM36274_NAME, 0},
	{}
};

static const struct of_device_id lm36274_of_id_table[] = {
	{.compatible = "ti,lm36274"},
	{ },
};

MODULE_DEVICE_TABLE(i2c, lm36274_id);
static struct i2c_driver lm36274_i2c_driver = {
		.driver = {
			.name = "lm36274",
			.owner = THIS_MODULE,
			.of_match_table = lm36274_of_id_table,
			.pm = LM36274_PM_OPS,
		},
		.probe = lm36274_probe,
		.remove = lm36274_remove,
		.id_table = lm36274_id,
};

static int __init lm36274_module_init(void)
{
	int ret = -1;

	LM36274_INFO("in!\n");

	lm36274_class = class_create(THIS_MODULE, "lm36274");
	if (IS_ERR(lm36274_class)) {
		LM36274_ERR("Unable to create lm36274 class; errno = %ld\n", PTR_ERR(lm36274_class));
		lm36274_class = NULL;
	}

	ret = i2c_add_driver(&lm36274_i2c_driver);
	if (ret)
		LM36274_ERR("Unable to register lm36274 driver\n");

	LM36274_INFO("ok!\n");

	return ret;
}
late_initcall(lm36274_module_init);

MODULE_DESCRIPTION("Texas Instruments Backlight driver for LM36274");
MODULE_AUTHOR("Daniel Jeong <daniel.jeong@ti.com>");
MODULE_AUTHOR("G.Shark Jeong <gshark.jeong@gmail.com>");
MODULE_LICENSE("GPL v2");
