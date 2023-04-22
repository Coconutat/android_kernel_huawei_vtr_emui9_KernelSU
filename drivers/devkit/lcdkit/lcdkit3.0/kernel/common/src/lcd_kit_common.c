/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
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

#include "lcd_kit_common.h"
#include "lcd_kit_dbg.h"
#include "lcd_kit_parse.h"

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
extern struct dsm_client* lcd_dclient;
#endif

int lcd_kit_msg_level = MSG_LEVEL_INFO;
/*common info*/
struct lcd_kit_common_info g_lcd_kit_common_info;
/*common ops*/
struct lcd_kit_common_ops g_lcd_kit_common_ops;
/*power handle*/
struct lcd_kit_power_desc g_lcd_kit_power_handle;
/*power on/off sequece*/
static struct lcd_kit_power_seq g_lcd_kit_power_seq;
/*esd error info*/
struct lcd_kit_esd_error_info g_esd_error_info;
/*hw adapt ops*/
static struct lcd_kit_adapt_ops *g_adapt_ops = NULL;
static char* sence_array[SENCE_ARRAY_SIZE] = {
	"LCD_INCOME0",   "MMI0",   "RUNNINGTEST0", "PROJECT_MENU0",
	"LCD_INCOME1",   "MMI1",   "RUNNINGTEST1",  "PROJECT_MENU1",
	"LCD_INCOME2",   "MMI2",   "RUNNINGTEST2",  "PROJECT_MENU2",
	"LCD_INCOME3",   "MMI3",   "RUNNINGTEST3",  "PROJECT_MENU3",
	"LCD_INCOME4",   "MMI4",   "RUNNINGTEST4",  "PROJECT_MENU4",
	"LCD_INCOME5",   "MMI5",   "RUNNINGTEST5",  "PROJECT_MENU5",
	"LCD_INCOME6",   "MMI6",   "RUNNINGTEST6",  "PROJECT_MENU6",
	"LCD_INCOME7",   "MMI7",   "RUNNINGTEST7",  "PROJECT_MENU7",
	"LCD_INCOME8",   "MMI8",   "RUNNINGTEST8",  "PROJECT_MENU8",
	"LCD_INCOME9",   "MMI9",   "RUNNINGTEST9",  "PROJECT_MENU9",
	"LCD_INCOME10",  "MMI10",  "RUNNINGTEST10",  "PROJECT_MENU10",
	"LCD_INCOME11",  "MMI11",  "RUNNINGTEST11",  "PROJECT_MENU11",
	"LCD_INCOME12",  "MMI12",  "RUNNINGTEST12",  "PROJECT_MENU12",
	"LCD_INCOME13",  "MMI13",  "RUNNINGTEST13",  "PROJECT_MENU13",
	"LCD_INCOME14",  "MMI14",  "RUNNINGTEST14",  "PROJECT_MENU14",
	"LCD_INCOME15",  "MMI15",  "RUNNINGTEST15",  "PROJECT_MENU15",
	"LCD_INCOME16",  "MMI16",  "RUNNINGTEST16",  "PROJECT_MENU16",
	"LCD_INCOME17",  "MMI17",  "RUNNINGTEST17",  "PROJECT_MENU17",
	"CURRENT1_0",    "CURRENT1_1", "CURRENT1_2",  "CURRENT1_3",
	"CURRENT1_4",    "CURRENT1_5", "CHECKSUM1",  "CHECKSUM2",
	"CHECKSUM3",     "CHECKSUM4", "BL_OPEN_SHORT",  "PCD_ERRORFLAG",
	"DOTINVERSION",  "CHECKREG", "COLUMNINVERSION",   "POWERONOFF",
	"BLSWITCH", "CURRENT_TEST_MODE", "OVER_CURRENT_DETECTION", "OVER_VOLTAGE_DETECTION",
};

static char* cmd_array[SENCE_ARRAY_SIZE] = {
	"CURRENT1_0",   "CURRENT1_0",  "CURRENT1_0",  "CURRENT1_0",//current test0
	"CURRENT1_1",   "CURRENT1_1",  "CURRENT1_1",  "CURRENT1_1",//current test1
	"CURRENT1_2",   "CURRENT1_2",  "CURRENT1_2",  "CURRENT1_2",//current test2
	"CURRENT1_3",   "CURRENT1_3",  "CURRENT1_3",  "CURRENT1_3",//current test3
	"CURRENT1_4",   "CURRENT1_4",  "CURRENT1_4",  "CURRENT1_4",//current test4
	"CURRENT1_5",   "CURRENT1_5",  "CURRENT1_5",  "CURRENT1_5",//current test5
	"CHECKSUM1",   "CHECKSUM1",   "CHECKSUM1", "CHECKSUM1",//checksum1
	"CHECKSUM2",   "CHECKSUM2",   "CHECKSUM2", "CHECKSUM2",//checksum2
	"CHECKSUM3",    "CHECKSUM3",   "CHECKSUM3", "CHECKSUM3",//checksum3
	"CHECKSUM4",   "CHECKSUM4",   "CHECKSUM4", "CHECKSUM4",//checksum4
	"BL_OPEN_SHORT",   "BL_OPEN_SHORT",   "BL_OPEN_SHORT", "BL_OPEN_SHORT",//backlight open short test
	"PCD_ERRORFLAG",   "PCD_ERRORFLAG",  "PCD_ERRORFLAG", "PCD_ERRORFLAG", // PCD and errorflag test
	"DOTINVERSION",    "DOTINVERSION",  "DOTINVERSION", "DOTINVERSION",// dot inversion test
	"CHECKREG",    "CHECKREG",  "CHECKREG", "CHECKREG",// check ic status reg
	"COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION", //column inversion test
	"POWERONOFF",   "POWERONOFF",  "POWERONOFF",  "POWERONOFF",// power on/off test
	"BLSWITCH",    "BLSWITCH",  "BLSWITCH", "BLSWITCH",// backlight switch test
	"GPU_TEST",   "GPU_TEST",  "GPU_TEST", "GPU_TEST", //GPU SLT test
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/bl_self_test",
	"/sys/class/graphics/fb0/amoled_pcd_errflag_check",
	"/sys/class/graphics/fb0/lcd_inversion_mode",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"/sys/class/graphics/fb0/lcd_inversion_mode",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"LCD_CUR_DET_MODE",
	"OVER_CURRENT_DETECTION",
	"OVER_VOLTAGE_DETECTION",
};

int lcd_kit_adapt_register(struct lcd_kit_adapt_ops* ops)
{
	if (g_adapt_ops) {
		LCD_KIT_ERR("g_adapt_ops has already been registered!\n");
		return LCD_KIT_FAIL;
	}
	g_adapt_ops = ops;
	LCD_KIT_INFO("g_adapt_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_kit_adapt_unregister(struct lcd_kit_adapt_ops* ops)
{
	if (g_adapt_ops == ops) {
		g_adapt_ops = NULL;
		LCD_KIT_INFO("g_adapt_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_adapt_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_kit_adapt_ops *lcd_kit_get_adapt_ops(void)
{
	return g_adapt_ops;
}

struct lcd_kit_common_info *lcd_kit_get_common_info(void)
{
	return &g_lcd_kit_common_info;
}

struct lcd_kit_common_ops *lcd_kit_get_common_ops(void)
{
	return &g_lcd_kit_common_ops;
}

struct lcd_kit_power_desc *lcd_kit_get_power_handle(void)
{
	return &g_lcd_kit_power_handle;
}

struct lcd_kit_power_seq *lcd_kit_get_power_seq(void)
{
	return &g_lcd_kit_power_seq;
}

static int lcd_kit_vci_power_ctrl(int enable)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_vci.buf){
		LCD_KIT_ERR("can not get lcd vci!\n");
		return LCD_KIT_FAIL;
    }
	switch (power_hdl->lcd_vci.buf[0]) {
		case GPIO_MODE:
			if (enable) {
				if (adapt_ops->gpio_enable) {
					ret = adapt_ops->gpio_enable(LCD_KIT_VCI);
				}
			} else {
				if (adapt_ops->gpio_disable) {
					ret = adapt_ops->gpio_disable(LCD_KIT_VCI);
				}
			}
			break;
		case REGULATOR_MODE:
			if (enable) {
				if (adapt_ops->regulator_enable) {
					ret = adapt_ops->regulator_enable(LCD_KIT_VCI);
				}
			} else {
				if (adapt_ops->regulator_disable) {
					ret = adapt_ops->regulator_disable(LCD_KIT_VCI);
				}
			}
			break;
        case NONE_MODE:
            LCD_KIT_DEBUG("lcd vci mode is none mode\n");
            break;
        default:
            LCD_KIT_ERR("lcd vci mode is not normal\n");
            return LCD_KIT_FAIL;
	}
	return ret;
}

static int lcd_kit_iovcc_power_ctrl(int enable)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_iovcc.buf){
		LCD_KIT_ERR("can not get lcd iovcc!\n");
		return LCD_KIT_FAIL;
    }

	switch (power_hdl->lcd_iovcc.buf[0]) {
		case GPIO_MODE:
			if (enable) {
				if (adapt_ops->gpio_enable) {
					ret = adapt_ops->gpio_enable(LCD_KIT_IOVCC);
				}
			} else {
				if (adapt_ops->gpio_disable) {
					ret = adapt_ops->gpio_disable(LCD_KIT_IOVCC);
				}
			}
			break;
		case REGULATOR_MODE:
			if (enable) {
				if (adapt_ops->regulator_enable) {
					ret = adapt_ops->regulator_enable(LCD_KIT_IOVCC);
				}
			} else {
				if (adapt_ops->regulator_disable) {
					ret = adapt_ops->regulator_disable(LCD_KIT_IOVCC);
				}
			}
			break;
        case NONE_MODE:
            LCD_KIT_DEBUG("lcd iovcc mode is none mode\n");
            break;
        default:
            LCD_KIT_ERR("lcd iovcc mode is not normal\n");
            return LCD_KIT_FAIL;
	}
	return ret;
}

static int lcd_kit_vdd_power_ctrl(int enable)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_vdd.buf){
		LCD_KIT_ERR("can not get lcd iovcc!\n");
		return LCD_KIT_FAIL;
    }

	switch (power_hdl->lcd_vdd.buf[0]) {
		case GPIO_MODE:
			if (enable) {
				if (adapt_ops->gpio_enable) {
					ret = adapt_ops->gpio_enable(LCD_KIT_VDD);
				}
			} else {
				if (adapt_ops->gpio_disable) {
					ret = adapt_ops->gpio_disable(LCD_KIT_VDD);
				}
			}
			break;
		case REGULATOR_MODE:
			if (enable) {
				if (adapt_ops->regulator_enable) {
					ret = adapt_ops->regulator_enable(LCD_KIT_VDD);
				}
			} else {
				if (adapt_ops->regulator_disable) {
					ret = adapt_ops->regulator_disable(LCD_KIT_VDD);
				}
			}
			break;
        case NONE_MODE:
            LCD_KIT_DEBUG("lcd vdd mode is none mode\n");
            break;
        default:
            LCD_KIT_ERR("lcd vdd mode is not normal\n");
            return LCD_KIT_FAIL;
	}
	return ret;
}

static int lcd_kit_vsp_power_ctrl(int enable)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_vsp.buf){
		LCD_KIT_ERR("can not get lcd vsp!\n");
		return LCD_KIT_FAIL;
    }

	switch (power_hdl->lcd_vsp.buf[0]) {
		case GPIO_MODE:
			if (enable) {
				if (adapt_ops->gpio_enable) {
					ret = adapt_ops->gpio_enable(LCD_KIT_VSP);
				}
			} else {
				if (adapt_ops->gpio_disable) {
					ret = adapt_ops->gpio_disable(LCD_KIT_VSP);
				}
			}
			break;
		case REGULATOR_MODE:
			if (enable) {
				if (adapt_ops->regulator_enable) {
					ret = adapt_ops->regulator_enable(LCD_KIT_VSP);
				}
			} else {
				if (adapt_ops->regulator_disable) {
					ret = adapt_ops->regulator_disable(LCD_KIT_VSP);
				}
			}
			break;
        case NONE_MODE:
            LCD_KIT_DEBUG("lcd vsp mode is none mode\n");
            break;
        default:
            LCD_KIT_ERR("lcd vsp mode is not normal\n");
            return LCD_KIT_FAIL;
	}
	return ret;
}

static int lcd_kit_vsn_power_ctrl(int enable)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_vsn.buf){
		LCD_KIT_ERR("can not get lcd vsn!\n");
		return LCD_KIT_FAIL;
    }

	switch (power_hdl->lcd_vsn.buf[0]) {
		case GPIO_MODE:
			if (enable) {
				if (adapt_ops->gpio_enable) {
					ret = adapt_ops->gpio_enable(LCD_KIT_VSN);
				}
			} else {
				if (adapt_ops->gpio_disable) {
					ret = adapt_ops->gpio_disable(LCD_KIT_VSN);
				}
			}
			break;
		case REGULATOR_MODE:
			if (enable) {
				if (adapt_ops->regulator_enable) {
					ret = adapt_ops->regulator_enable(LCD_KIT_VSN);
				}
			} else {
				if (adapt_ops->regulator_disable) {
					ret = adapt_ops->regulator_disable(LCD_KIT_VSN);
				}
			}
			break;
        case NONE_MODE:
            LCD_KIT_DEBUG("lcd vsn mode is none mode\n");
            break;
        default:
            LCD_KIT_ERR("lcd vsn mode is not normal\n");
            return LCD_KIT_FAIL;
	}
	return ret;
}

int lcd_kit_set_bias_voltage(void)
{
	struct lcd_kit_bias_ops *bias_ops = NULL;
	int ret = LCD_KIT_OK;

	bias_ops = lcd_kit_get_bias_ops();
	if (!bias_ops) {
		LCD_KIT_ERR("can not get bias_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*set bias voltage*/
	if (bias_ops->set_bias_voltage) {
		ret = bias_ops->set_bias_voltage(power_hdl->lcd_vsp.buf[2], power_hdl->lcd_vsn.buf[2]);
	}
	return ret;
}

static int lcd_kit_reset_power_on(void)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_rst.buf){
            LCD_KIT_ERR("can not get lcd reset!\n");
            return LCD_KIT_FAIL;
    }

	switch (power_hdl->lcd_rst.buf[0]) {
		case GPIO_MODE:
			if (adapt_ops->gpio_enable) {
				ret = adapt_ops->gpio_enable(LCD_KIT_RST);
			}
			break;
		default:
			LCD_KIT_ERR("not support type:%d\n", power_hdl->lcd_rst.buf[0]);
			break;
	}
	return ret;
}

static int lcd_kit_reset_power_off(void)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
    if (NULL == power_hdl->lcd_rst.buf){
            LCD_KIT_ERR("can not get lcd reset!\n");
            return LCD_KIT_FAIL;
    }
	switch (power_hdl->lcd_rst.buf[0]) {
		case GPIO_MODE:
			if (adapt_ops->gpio_disable) {
				ret = adapt_ops->gpio_disable(LCD_KIT_RST);
			}
			break;
		default:
			LCD_KIT_ERR("not support type:%d\n", power_hdl->lcd_rst.buf[0]);
			break;
	}
	return ret;
}

static int lcd_kit_reset_power_ctrl(int enable)
{
	if (enable) {
		return lcd_kit_reset_power_on();
	} else {
		return lcd_kit_reset_power_off();
	}
}

static int lcd_kit_on_cmds(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*send panel on init code*/
	if (adapt_ops->mipi_tx) {
		ret = adapt_ops->mipi_tx(hld, &common_info->panel_on_cmds);
		if (ret) {
			LCD_KIT_ERR("send panel on cmds error\n");
		}
		/*send panel on effect code*/
		if (common_info->effect_on.support) {
			ret = adapt_ops->mipi_tx(hld, &common_info->effect_on.cmds);
			if (ret) {
				LCD_KIT_ERR("send effect on cmds error\n");
			}
		}
	}
	return ret;
}

static int lcd_kit_off_cmds(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*send panel off code*/
	if (adapt_ops->mipi_tx) {
		ret = adapt_ops->mipi_tx(hld, &common_info->panel_off_cmds);
	}
	return ret;
}

static int lcd_kit_check_reg_report_dsm(void* hld,struct lcd_kit_check_reg_dsm *check_reg_dsm)
{
	int ret = LCD_KIT_OK;
	uint8_t read_value[MAX_REG_READ_COUNT] = {0};
	int i = 0;
	char* expect_ptr = NULL;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	if(NULL == hld || NULL == check_reg_dsm ) {
		LCD_KIT_ERR("null pointer!\n");
		return LCD_KIT_FAIL;
	}

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (check_reg_dsm->support) {
		expect_ptr = (char *)check_reg_dsm->value.buf;
		if (adapt_ops->mipi_rx) {
			ret = adapt_ops->mipi_rx(hld, read_value, &check_reg_dsm->cmds);
		}
		if(ret == LCD_KIT_OK) {
			for (i = 0; i < check_reg_dsm->cmds.cmd_cnt; i++) {
				if(check_reg_dsm->support_dsm_report) {
					if ((char)read_value[i] != expect_ptr[i]) {
						ret = LCD_KIT_FAIL;
						LCD_KIT_ERR("read_value[%d] = 0x%x, but expect_ptr[%d] = 0x%x!\n",
									i, read_value[i], i, expect_ptr[i]);
#if defined (CONFIG_HUAWEI_DSM)
						dsm_client_record(lcd_dclient,"read_value[%d] = 0x%x, but expect_ptr[%d] = 0x%x!\n",
									i, read_value[i], i, expect_ptr[i]);
#endif
						break;
					}
					LCD_KIT_INFO("read_value[%d] = 0x%x same with expect value!\n",
							i, read_value[i]);
				}
				else {
					LCD_KIT_INFO("read_value[%d] = 0x%x!\n",
							i, read_value[i]);
				}
			}
		}
		else {
			LCD_KIT_ERR("mipi read error!\n");
		}
	}
	if (ret != LCD_KIT_OK) {
		if(check_reg_dsm->support_dsm_report) {
#if defined (CONFIG_HUAWEI_DSM)
			if (!dsm_client_ocuppy(lcd_dclient)) {
			dsm_client_notify(lcd_dclient, DSM_LCD_STATUS_ERROR_NO);
#endif
			}
		}
	}
	return ret;
}

static int lcd_kit_mipi_power_ctrl(void* hld, int enable)
{
	int ret = LCD_KIT_OK;
	int ret_check_reg = LCD_KIT_OK;
	if (enable) {
		ret = lcd_kit_on_cmds(hld);
		ret_check_reg = lcd_kit_check_reg_report_dsm(hld,&common_info->check_reg_on);
		if(ret_check_reg != LCD_KIT_OK) {
			LCD_KIT_ERR("power on check reg error!\n");
		}
	} else {
		ret = lcd_kit_off_cmds(hld);
		ret_check_reg = lcd_kit_check_reg_report_dsm(hld,&common_info->check_reg_off);
		if(ret_check_reg != LCD_KIT_OK) {
			LCD_KIT_ERR("power off check reg error!\n");
		}
	}
	return ret;
}

static int lcd_kit_ts_resume(int sync)
{
	int ret = 0;
	struct ts_kit_ops *ts_ops = NULL;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return LCD_KIT_FAIL;
	}

	if (sync) {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_RESUME_DEVICE, SHORT_SYNC);
		}
	} else {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_RESUME_DEVICE, NO_SYNC);
		}
	}
	LCD_KIT_INFO("sync is %d\n", sync);
	return ret;
}

static int lcd_kit_ts_after_resume(int sync)
{
	int ret = 0;
	struct ts_kit_ops *ts_ops = NULL;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return LCD_KIT_FAIL;
	}

	if (sync) {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_AFTER_RESUME, SHORT_SYNC);
		}
	} else {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_AFTER_RESUME, NO_SYNC);
		}
	}
	LCD_KIT_INFO("sync is %d\n", sync);
	return ret;
}

static int lcd_kit_ts_suspend(int sync)
{
	int ret = 0;
	struct ts_kit_ops *ts_ops = NULL;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return LCD_KIT_FAIL;
	}

	if (sync) {
		if (ts_ops->ts_power_notify) {
			ts_ops->ts_power_notify(TS_BEFORE_SUSPEND, SHORT_SYNC);
			ts_ops->ts_power_notify(TS_SUSPEND_DEVICE, SHORT_SYNC);
		}
	} else {
		if (ts_ops->ts_power_notify) {
			ts_ops->ts_power_notify(TS_BEFORE_SUSPEND, NO_SYNC);
			ts_ops->ts_power_notify(TS_SUSPEND_DEVICE, NO_SYNC);
		}
	}
	LCD_KIT_INFO("sync is %d\n", sync);
	return ret;
}

static int lcd_kit_ts_early_suspend(int sync)
{
	int ret = 0;
	struct ts_kit_ops *ts_ops = NULL;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return LCD_KIT_FAIL;
	}

	if (sync) {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_EARLY_SUSPEND, SHORT_SYNC);
		}
	} else {
		if (ts_ops->ts_power_notify) {
			ret = ts_ops->ts_power_notify(TS_EARLY_SUSPEND, NO_SYNC);
		}
	}
	LCD_KIT_INFO("sync is %d\n", sync);
	return ret;
}

static int lcd_kit_early_ts_event(int enable, int sync)
{
	if (enable) {
		return lcd_kit_ts_resume(sync);
	} else {
		return lcd_kit_ts_early_suspend(sync);
	}
}

static int lcd_kit_later_ts_event(int enable, int sync)
{
	if (enable) {
		return lcd_kit_ts_after_resume(sync);
	} else {
		return lcd_kit_ts_suspend(sync);
	}
}

static int lcd_kit_get_pt_mode(void)
{
	struct lcd_kit_ops *lcd_ops = NULL;
	int status = 0;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("lcd_ops is null\n");
		return 0;
	}
	if (lcd_ops->get_pt_station_status) {
		status = lcd_ops->get_pt_station_status();
	}
	return status;
}

static int lcd_kit_gesture_mode(void)
{
	struct ts_kit_ops *ts_ops = NULL;
	int status = 0;
	int ret = 0;

	ts_ops = ts_kit_get_ops();
	if (!ts_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return 0;
	}
	if (ts_ops->get_tp_status_by_type) {
		ret = ts_ops->get_tp_status_by_type(TS_GESTURE_FUNCTION, &status);
		if (ret) {
			LCD_KIT_ERR("get gesture function fail\n");
			return 0;
		}
	}
	return status;
}

static int lcd_kit_panel_is_power_on(void)
{
	struct lcd_kit_ops *lcd_ops = NULL;
	int mode = 0;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("ts_ops is null\n");
		return 0;
	}
	if (lcd_ops->get_panel_power_status) {
		mode = lcd_ops->get_panel_power_status();
	}
	return mode;
}

static int lcd_kit_event_should_send(uint32_t event, uint32_t data)
{
	int ret = 0;

	switch (event) {
		case EVENT_VCI:
		case EVENT_IOVCC:
		case EVENT_VSP:
		case EVENT_VSN:
		case EVENT_RESET:
		case EVENT_VDD:
		return (lcd_kit_get_pt_mode()||((uint32_t)lcd_kit_gesture_mode() && (common_info->ul_does_lcd_poweron_tp)));
		case EVENT_MIPI:
		if (data) {
			return lcd_kit_panel_is_power_on();
                }
		break;
		default:
		ret = 0;
		break;
	}
	return ret;
}

int lcd_kit_event_handler(void* hld, uint32_t event, uint32_t data, uint32_t delay)
{
	int ret = LCD_KIT_OK;

	LCD_KIT_INFO("event = %d, data = %d, delay = %d\n", event, data, delay);
	if (lcd_kit_event_should_send(event, data)) {
		LCD_KIT_INFO("It is in pt mode or gesture mode.\n");
		return ret;
	}
	switch (event) {
		case EVENT_VCI:
		{
			ret = lcd_kit_vci_power_ctrl(data);
			break;
		}
		case EVENT_IOVCC:
		{
			ret = lcd_kit_iovcc_power_ctrl(data);
			break;
		}
		case EVENT_VSP:
		{
			ret = lcd_kit_vsp_power_ctrl(data);
			break;
		}
		case EVENT_VSN:
		{
			ret = lcd_kit_vsn_power_ctrl(data);
			/*set bias voltage*/
			lcd_kit_set_bias_voltage();
			break;
		}
		case EVENT_RESET:
		{
			ret = lcd_kit_reset_power_ctrl(data);
			break;
		}
		case EVENT_MIPI:
		{
			ret = lcd_kit_mipi_power_ctrl(hld, data);
			break;
		}
		case EVENT_EARLY_TS:
		{
			lcd_kit_early_ts_event(data, delay);
			break;
		}
		case EVENT_LATER_TS:
		{
			lcd_kit_later_ts_event(data, delay);
			break;
		}
		case EVENT_VDD:
		{
			ret = lcd_kit_vdd_power_ctrl(data);
			break;
		}
		case EVENT_NONE:
		{
			LCD_KIT_INFO("none event\n");
			break;
		}
		default:
		{
			LCD_KIT_ERR("event not exist\n");
			ret = LCD_KIT_FAIL;
			break;
		}
	}
	/*In the case of aod exit, no delay is required*/
	if(0 == lcd_kit_panel_is_power_on()) {
		LCD_KIT_DELAY(delay);
	}
	return ret;
}

static int lcd_kit_panel_power_on(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->power_on_seq.arry_data;
	for (i = 0; i < power_seq->power_on_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	return ret;
}

static int lcd_kit_panel_on_lp(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->panel_on_lp_seq.arry_data;
	for (i = 0; i < power_seq->panel_on_lp_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	return ret;
}

static int lcd_kit_panel_on_hs(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->panel_on_hs_seq.arry_data;
	for (i = 0; i < power_seq->panel_on_hs_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	/*restart check thread*/
	if (common_info->check_thread.enable) {
		hrtimer_start(&common_info->check_thread.hrtimer, ktime_set(CHECK_THREAD_TIME_PERIOD / 1000,
			(CHECK_THREAD_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
	}
	return ret;
}

int lcd_kit_panel_off_hs(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->panel_off_hs_seq.arry_data;
	for (i = 0; i < power_seq->panel_off_hs_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	/*stop check thread*/
	if (common_info->check_thread.enable) {
		hrtimer_cancel(&common_info->check_thread.hrtimer);
	}
	return ret;
}

int lcd_kit_panel_off_lp(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->panel_off_lp_seq.arry_data;
	for (i = 0; i < power_seq->panel_off_lp_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	return ret;
}

int lcd_kit_panel_power_off(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	struct lcd_kit_array_data* pevent = NULL;

	pevent = power_seq->power_off_seq.arry_data;
	for (i = 0; i < power_seq->power_off_seq.cnt; i++) {
		if (!pevent || !pevent->buf) {
			LCD_KIT_ERR("pevent is null!\n");
			return LCD_KIT_FAIL;
		}
		ret = lcd_kit_event_handler(hld, pevent->buf[0], pevent->buf[1], pevent->buf[2]);
		if (ret) {
			LCD_KIT_ERR("send event 0x%x error!\n", pevent->buf[0]);
			break;
		}
		pevent++;
	}
	if(common_info->set_vss.support) {
		common_info->set_vss.power_off = 1;
		common_info->set_vss.new_backlight = 0;
	}
	return ret;
}

static int lcd_kit_hbm_enable(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*enable hbm and open dimming*/
	if (common_info->hbm.enter_cmds.cmds != NULL){
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.enter_cmds);
		}
	}
	return ret;
}

static int lcd_kit_hbm_disable(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*exit hbm*/
	if (common_info->hbm.exit_cmds.cmds != NULL){
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.exit_cmds);
		}
	}
	return ret;
}

static int lcd_kit_hbm_set_level(void* hld, int level)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*prepare*/
	if (common_info->hbm.hbm_prepare_cmds.cmds != NULL) {
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.hbm_prepare_cmds);
		}
	}
	/*set hbm level*/
	if (common_info->hbm.hbm_cmds.cmds != NULL) {
		if (common_info->hbm.hbm_cmds.cmds[0].dlen == 2) {
			common_info->hbm.hbm_cmds.cmds[0].payload[1] = level;
		} else {
			common_info->hbm.hbm_cmds.cmds[0].payload[1] = (level >> 8) & 0xf;
			common_info->hbm.hbm_cmds.cmds[0].payload[2] = level & 0xff;
		}
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.hbm_cmds);
		}
	}
	/*post*/
	if (common_info->hbm.hbm_post_cmds.cmds != NULL) {
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.hbm_post_cmds);
		}
	}
	return ret;
}

static int lcd_kit_hbm_dim_disable(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*close dimming*/
	if (common_info->hbm.exit_dim_cmds.cmds != NULL){
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.exit_dim_cmds);
		}
	}
	return ret;
}

static int lcd_kit_hbm_dim_enable(void* hld)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	/*open dimming*/
	if (common_info->hbm.enter_dim_cmds.cmds != NULL){
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->hbm.enter_dim_cmds);
		}
	}
	return ret;
}

static void lcd_kit_hbm_print_count(int last_hbm_level, int hbm_level)
{
	static int count = 0;

	if (abs(hbm_level - last_hbm_level) > 60) {
		if (count == 0) {
			LCD_KIT_INFO("last hbm_level=%d!\n", last_hbm_level);
		}
		count = 5;
	}
	if (count > 0) {
		count--;
		LCD_KIT_INFO("hbm_level=%d!\n", hbm_level);
	} else {
		LCD_KIT_DEBUG("hbm_level=%d!\n", hbm_level);
	}
}

static int lcd_kit_hbm_set_handle(void* hld, int last_hbm_level, int hbm_dimming, int hbm_level)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!hld) {
		LCD_KIT_ERR("hld is null!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->hbm.support) {
		mutex_lock(&common_info->hbm.hbm_lock);
		common_info->hbm.hbm_level_current = hbm_level;
		if(hbm_level > 0) {
			if (last_hbm_level == 0) {
				/*enable hbm*/
				lcd_kit_hbm_enable(hld);
			} else {
				lcd_kit_hbm_print_count(last_hbm_level, hbm_level);
			}
			 /*set hbm level*/
			lcd_kit_hbm_set_level(hld, hbm_level);
		} else {
			if (last_hbm_level == 0) {
				/*disable dimming*/
				lcd_kit_hbm_dim_disable(hld);
			} else {
				/*exit hbm*/
				if (hbm_dimming) {
					lcd_kit_hbm_dim_enable(hld);
				} else {
					lcd_kit_hbm_dim_disable(hld);
				}
				lcd_kit_hbm_disable(hld);
			}
		}
		mutex_unlock(&common_info->hbm.hbm_lock);
	}
	return ret;
}

static int lcd_kit_get_panel_name(char* buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", common_info->panel_name);
}

static u32 lcd_kit_get_blmaxnit(void)
{
	u32 bl_max_nit = 0;
	u32 lcd_kit_brightness_ddic_info = 0;
	lcd_kit_brightness_ddic_info = common_info->blmaxnit.lcd_kit_brightness_ddic_info;
	if (common_info->blmaxnit.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC && lcd_kit_brightness_ddic_info > BL_MIN && lcd_kit_brightness_ddic_info < BL_MAX){
		bl_max_nit = (lcd_kit_brightness_ddic_info < BL_REG_NOUSE_VALUE) ? (lcd_kit_brightness_ddic_info + BL_NIT): (lcd_kit_brightness_ddic_info + BL_NIT - 1);
	} else {
		bl_max_nit = common_info->actual_bl_max_nit;
	}
	return bl_max_nit;
}

static int lcd_kit_get_panel_info(char* buf)
{
	#define PANEL_MAX	10
	int ret = LCD_KIT_OK;
	char panel_type[PANEL_MAX] = {0};

	if (common_info->panel_type == LCD_TYPE) {
		strncpy(panel_type, "LCD", strlen("LCD"));
	} else if (common_info->panel_type == AMOLED_TYPE) {
		strncpy(panel_type, "AMOLED", strlen("AMOLED"));
	} else {
		strncpy(panel_type, "INVALID", strlen("INVALID"));
	}
	common_info->actual_bl_max_nit = lcd_kit_get_blmaxnit();
	ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,blmax_nit_actual:%d,blmax_nit_standard:%d,lcdtype:%s,\n",
				   common_info->bl_level_max, common_info->bl_level_min, \
				   common_info->actual_bl_max_nit, common_info->bl_max_nit, panel_type);
	return ret;
}

static int lcd_kit_get_cabc_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->cabc.support) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", common_info->cabc.mode);
	}
	return ret;
}

static int lcd_kit_set_cabc_mode(void* hld, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->cabc.support) {
		switch (mode) {
			case CABC_OFF_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->cabc.cabc_off_cmds);
				}
				break;
			case CABC_UI:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->cabc.cabc_ui_cmds);
				}
				break;
			case CABC_STILL:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->cabc.cabc_still_cmds);
				}
				break;
			case CABC_MOVING:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->cabc.cabc_moving_cmds);
				}
				break;
			default:
				return LCD_KIT_FAIL;
		}
		common_info->cabc.mode = mode;
	}
	LCD_KIT_INFO("common_info->cabc.support = %d, common_info->cabc.mode = %d\n", common_info->cabc.support, common_info->cabc.mode);
	return ret;
}

static int lcd_kit_inversion_get_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->inversion.support) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", common_info->inversion.mode);
	}
	return ret;
}

static int lcd_kit_inversion_set_mode(void* hld, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->inversion.support) {
		switch (mode) {
			case COLUMN_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->inversion.column_cmds);
				}
				break;
			case DOT_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->inversion.dot_cmds);
				}
				break;
			default :
				return LCD_KIT_FAIL;
		}
		common_info->inversion.mode = (int)mode;
		LCD_KIT_INFO("common_info->inversion.support = %d, common_info->inversion.mode = %d\n", common_info->inversion.support, common_info->inversion.mode);
	}
	return ret;
}

static int lcd_kit_scan_get_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->scan.support) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", common_info->scan.mode);
	}
	return ret;
}

static int lcd_kit_scan_set_mode(void* hld,  u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->scan.support) {
		switch (mode) {
			case FORWORD_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->scan.forword_cmds);
				}
				break;

			case REVERT_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->scan.revert_cmds);
				}
				break;

			default:
				return LCD_KIT_FAIL;
		}
		common_info->scan.mode = (int)mode;
		LCD_KIT_INFO("common_info->scan.support = %d, common_info->scan.mode = %d\n", common_info->scan.support, common_info->scan.mode);
	}
	return ret;
}

static int lcd_kit_check_reg(void* hld, char* buf)
{
	int ret = LCD_KIT_OK;
	uint8_t read_value[MAX_REG_READ_COUNT] = {0};
	int i = 0;
	char* expect_ptr = NULL;
	unsigned int count;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->check_reg.support) {
		expect_ptr = (char *)common_info->check_reg.value.buf;
		if (adapt_ops->mipi_rx) {
			ret = adapt_ops->mipi_rx(hld, read_value, &common_info->check_reg.cmds);
		}
		for (i = 0; i < common_info->check_reg.cmds.cmd_cnt; i++) {
			if ((char)read_value[i] != expect_ptr[i]) {
				ret = -1;
				LCD_KIT_ERR("read_value[%u] = 0x%x, but expect_ptr[%u] = 0x%x!\n",
							 i, read_value[i], i, expect_ptr[i]);
				break;
			}
			LCD_KIT_INFO("read_value[%u] = 0x%x same with expect value!\n",
						 i, read_value[i]);
		}
		if (0 == ret) {
			ret = snprintf(buf, PAGE_SIZE, "OK\n");
		} else {
			ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
		}
		LCD_KIT_INFO("checksum result:%s\n", buf);
	}
	return ret;
}

static void lcd_kit_clear_esd_error_info(void)
{
	memset(&g_esd_error_info, 0, sizeof(g_esd_error_info));
	return;
}
static void lcd_kit_record_esd_error_info(int read_reg_index, int read_reg_val, int expect_reg_val)
{
	int reg_index = g_esd_error_info.esd_error_reg_num;

	if ((reg_index + 1) <= MAX_REG_READ_COUNT){
		g_esd_error_info.esd_reg_index[reg_index] = read_reg_index;
		g_esd_error_info.esd_error_reg_val[reg_index] = read_reg_val;
		g_esd_error_info.esd_expect_reg_val[reg_index] = expect_reg_val;
		g_esd_error_info.esd_error_reg_num++;
	}
	return;
}

static int lcd_kit_judge_esd(unsigned char type, unsigned char read_val, unsigned char expect_val)
{
	int ret = 0;

	switch (type) {
	case ESD_UNEQUAL:
		if (read_val != expect_val) {
			ret = 1;
		}
		break;
	case ESD_EQUAL:
		if (read_val == expect_val) {
			ret = 1;
		}
		break;
	case ESD_BIT_VALID:
		if (read_val & expect_val) {
			ret = 1;
		}
		break;
	default:
		if (read_val != expect_val) {
			ret = 1;
		}
		break;
	}
	return ret;
}

static int lcd_kit_esd_handle(void* hld)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	char read_value[MAX_REG_READ_COUNT] = {0};
	char expect_value = 0;
	char judge_type = 0;
	u32* esd_value = NULL;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	int clear_esd_info_flag = FALSE;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->esd.support) {
		if (common_info->esd.status == ESD_STOP) {
			LCD_KIT_ERR("bypass esd check\n");
			return LCD_KIT_OK;
		}
		esd_value = common_info->esd.value.buf;
		if (adapt_ops->mipi_rx) {
			ret = adapt_ops->mipi_rx(hld, read_value, &common_info->esd.cmds);
		}
		if (!ret) {
			for (i = 0; i < common_info->esd.value.cnt; i++) {
				judge_type = (esd_value[i] >> 8) & 0xFF;
				expect_value = esd_value[i] & 0xFF;
				if (lcd_kit_judge_esd(judge_type, read_value[i], expect_value)) {
					if (FALSE == clear_esd_info_flag) {
						lcd_kit_clear_esd_error_info();
						clear_esd_info_flag = TRUE;
					}
					lcd_kit_record_esd_error_info(i, (int)read_value[i], expect_value);
					LCD_KIT_ERR("read_value[%d] = 0x%x, but expect_value = 0x%x!\n", i, read_value[i], expect_value);
					ret = 1;
					break;
				}
				LCD_KIT_INFO("judge_type = %d, esd_value[%d] = 0x%x, read_value[%d] = 0x%x, expect_value = 0x%x\n", judge_type, i, esd_value[i], i, read_value[i], expect_value);
			}
		}
		LCD_KIT_INFO("esd check result:%d\n", ret);
	}
	return ret;
}

static int lcd_kit_get_ce_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->ce.support) {
		ret = snprintf(buf, PAGE_SIZE,  "%d\n", common_info->ce.mode);
	}
	return ret;
}

static int lcd_kit_set_ce_mode(void* hld, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->ce.support) {
		switch (mode) {
			case CE_OFF_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->ce.off_cmds);
				}
				break;

			case CE_SRGB:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->ce.srgb_cmds);
				}
				break;
			case CE_USER:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->ce.user_cmds);
				}
				break;
			case CE_VIVID:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->ce.vivid_cmds);
				}
				break;
			default:
				LCD_KIT_INFO("wrong mode!\n");
				ret = -1;
				break;
		}
		common_info->ce.mode = mode;
	}
	LCD_KIT_INFO("common_info->ce.support = %d, common_info->ce.mode = %d\n", common_info->ce.support, common_info->ce.mode);
	return ret;

}

static int lcd_kit_get_sleep_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->pt.support) {
		ret = snprintf(buf, PAGE_SIZE, "PT test mod = %d, PT test support = %d\n",
						 common_info->pt.mode, common_info->pt.support);
	}
	return ret;
}

static int lcd_kit_set_sleep_mode(u32 mode)
{
	int ret = LCD_KIT_OK;

	if (common_info->pt.support) {
		common_info->pt.mode = mode;
	}
	LCD_KIT_INFO("common_info->pt.support = %d, common_info->pt.mode = %d\n", common_info->pt.support, common_info->pt.mode);
	return ret;
}

static int lcd_kit_get_acl_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->acl.support) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", common_info->acl.mode);
	}
	return ret;
}

static int lcd_kit_set_acl_mode(void* hld, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->acl.support) {
		switch (mode) {
			case ACL_OFF_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->acl.acl_off_cmds);
				}
				break;
			case ACL_HIGH_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->acl.acl_high_cmds);
				}
				break;
			case ACL_MIDDLE_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->acl.acl_middle_cmds);
				}
				break;
			case ACL_LOW_MODE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->acl.acl_low_cmds);
				}
				break;
			default:
				LCD_KIT_ERR("mode error\n");
				ret = -1;
				break;
		}
		common_info->acl.mode = mode;
		LCD_KIT_ERR("common_info->acl.support = %d, common_info->acl.mode = %d\n", common_info->acl.support, common_info->acl.mode);
	}
	return ret;
}

static int lcd_kit_get_vr_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->vr.support) {
		ret = snprintf(buf, PAGE_SIZE,  "%d\n", common_info->vr.mode);
	}
	return ret;
}

static int lcd_kit_set_vr_mode(void* hld, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->vr.support) {
		switch (mode) {
			case VR_ENABLE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->vr.enable_cmds);
				}
				break;
			case  VR_DISABLE:
				if (adapt_ops->mipi_tx) {
					ret = adapt_ops->mipi_tx(hld, &common_info->vr.disable_cmds);
				}
				break;
			default:
				ret = -1;
				LCD_KIT_ERR("mode error\n");
				break;
		}
		common_info->vr.mode = mode;
	}
	LCD_KIT_INFO("common_info->vr.support = %d, common_info->vr.mode = %d\n", common_info->vr.support, common_info->vr.mode);
	return ret;
}

static int lcd_kit_get_effect_color_mode(char* buf)
{
	int ret = LCD_KIT_OK;

	if (common_info->effect_color.support || (common_info->effect_color.mode & BITS(31))) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", common_info->effect_color.mode);
	}
	return ret;
}

static int lcd_kit_set_effect_color_mode(u32 mode)
{
	int ret = LCD_KIT_OK;

	if (common_info->effect_color.support) {
		common_info->effect_color.mode = mode;
	}
	LCD_KIT_INFO("common_info->effect_color.support = %d, common_info->effect_color.mode = %d\n", common_info->effect_color.support, common_info->effect_color.mode);
	return ret;
}

static int lcd_kit_set_mipi_backlight(void* hld, u32 level)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	struct lcd_kit_ops *lcd_ops = NULL;

	lcd_ops = lcd_kit_get_ops();
	if (!lcd_ops) {
		LCD_KIT_ERR("lcd_ops is null\n");
		return 0;
	}
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	switch(common_info->backlight.order) {
		case BL_BIG_ENDIAN:
			if (common_info->backlight.bl_max <= 0xFF) {
				common_info->backlight.bl_cmd.cmds[0].payload[1] = level;
			} else {
				common_info->backlight.bl_cmd.cmds[0].payload[1] = (level >> 8) & 0xFF;
				common_info->backlight.bl_cmd.cmds[0].payload[2] = level & 0xFF;
			}
			break;
		case BL_LITTLE_ENDIAN:
			if (common_info->backlight.bl_max <= 0xFF) {
				common_info->backlight.bl_cmd.cmds[0].payload[1] = level;
			} else {
				common_info->backlight.bl_cmd.cmds[0].payload[1] = level & 0xFF;
				common_info->backlight.bl_cmd.cmds[0].payload[2] = (level >> 8) & 0xFF;
			}
			break;
		default:
			LCD_KIT_ERR("not support order\n");
			break;
	}
	if(common_info->set_vss.support) {
		common_info->set_vss.new_backlight = level;
		if(lcd_ops->set_vss_by_thermal) {
			lcd_ops->set_vss_by_thermal();
		}
	}
	ret = adapt_ops->mipi_tx(hld, &common_info->backlight.bl_cmd);
	return ret;
}

static int lcd_kit_get_test_config(char* buf)
{
	int i = 0;
	int ret = LCD_KIT_FAIL;

	for (i = 0; i < SENCE_ARRAY_SIZE; i++) {
		if (sence_array[i] == NULL) {
			ret = LCD_KIT_FAIL;
			LCD_KIT_INFO("Sence cmd is end, total num is:%d\n", i);
			break;
		}
		if ( !strncmp(common_info->lcd_cmd_now, sence_array[i], strlen(common_info->lcd_cmd_now))) {
			LCD_KIT_INFO("current test cmd:%s,return cmd:%s\n", common_info->lcd_cmd_now, cmd_array[i]);
			return snprintf(buf, PAGE_SIZE, cmd_array[i]);
		}
	}
	return ret;
}

static int lcd_kit_set_test_config(const char* buf)
{

	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) < LCD_KIT_CMD_NAME_MAX) {
		memcpy(common_info->lcd_cmd_now, buf, strlen(buf) + 1);
		LCD_KIT_INFO("current test cmd:%s\n", common_info->lcd_cmd_now);
	} else {
		memcpy(common_info->lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
		LCD_KIT_INFO("invalid test cmd:%s\n", common_info->lcd_cmd_now);
	}
	return LCD_KIT_OK;
}

static int lcd_kit_dirty_region_handle(void* hld, struct region_rect* dirty)
{
	int ret = LCD_KIT_OK;
	struct region_rect* dirty_region = NULL;
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	
	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if ( dirty == NULL) {
		LCD_KIT_ERR("dirty is null point!\n");
		return LCD_KIT_FAIL;
	}
	if (common_info->dirty_region.support) {
		dirty_region = (struct region_rect*) dirty;
		common_info->dirty_region.cmds.cmds[0].payload[1] = ((dirty_region->x) >> 8) & 0xff;
		common_info->dirty_region.cmds.cmds[0].payload[2] = (dirty_region->x) & 0xff;
		common_info->dirty_region.cmds.cmds[0].payload[3] = ((dirty_region->x + dirty_region->w - 1) >> 8) & 0xff;
		common_info->dirty_region.cmds.cmds[0].payload[4] = (dirty_region->x + dirty_region->w - 1) & 0xff;
		common_info->dirty_region.cmds.cmds[1].payload[1] = ((dirty_region->y) >> 8) & 0xff;
		common_info->dirty_region.cmds.cmds[1].payload[2] = (dirty_region->y) & 0xff;
		common_info->dirty_region.cmds.cmds[1].payload[3] = ((dirty_region->y + dirty_region->h - 1) >> 8) & 0xff;
		common_info->dirty_region.cmds.cmds[1].payload[4] = (dirty_region->y + dirty_region->h - 1) & 0xff;
		if (adapt_ops->mipi_tx) {
			ret = adapt_ops->mipi_tx(hld, &common_info->dirty_region.cmds);
		}
	}
	return ret;
}

static void lcd_kit_panel_parse_running(struct device_node* np)
{
	/*dot-colomn inversion*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,inversion-support", &common_info->inversion.support, 0);
	if (common_info->inversion.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,inversion-column-cmds", "lcd-kit,inversion-column-cmds-state",
								&common_info->inversion.column_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,inversion-dot-cmds", "lcd-kit,inversion-dot-cmds-state",
								&common_info->inversion.dot_cmds);
	}
	/*forword-revert scan*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,scan-support", &common_info->scan.support, 0);
	if (common_info->scan.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,scan-forword-cmds", "lcd-kit,scan-forword-cmds-state",
								&common_info->scan.forword_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,scan-revert-cmds", "lcd-kit,scan-revert-cmds-state",
								&common_info->scan.revert_cmds);
	}

	/*check reg*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-reg-support", &common_info->check_reg.support, 0);
	if (common_info->check_reg.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,check-reg-cmds", "lcd-kit,check-reg-cmds-state",
								&common_info->check_reg.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,check-reg-value", &common_info->check_reg.value);
	}
	/*check reg on*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-reg-on-support", &common_info->check_reg_on.support, 0);
	if (common_info->check_reg_on.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,check-reg-on-cmds", "lcd-kit,check-reg-on-cmds-state",
								&common_info->check_reg_on.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,check-reg-on-value", &common_info->check_reg_on.value);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-reg-on-support-dsm-report", &common_info->check_reg_on.support_dsm_report, 0);
	}
	/*check reg off*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-reg-off-support", &common_info->check_reg_off.support, 0);
	if (common_info->check_reg_off.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,check-reg-off-cmds", "lcd-kit,check-reg-off-cmds-state",
								&common_info->check_reg_off.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,check-reg-off-value", &common_info->check_reg_off.value);
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-reg-off-support-dsm-report", &common_info->check_reg_off.support_dsm_report, 0);
	}
	/*check mipi*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-check-support", &common_info->mipi_check.support, 0);
	if (common_info->mipi_check.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-mipi-error-report-threshold", &common_info->mipi_check.mipi_error_report_threshold, 1);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-check-cmds", "lcd-kit,mipi-check-cmds-state",
								&common_info->mipi_check.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,mipi-check-value", &common_info->mipi_check.value);
	}
	/*pt test*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,pt-support", &common_info->pt.support, 0);
	if (common_info->pt.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,pt-panel-ulps-support", &common_info->pt.panel_ulps_support, 0);
	}
	return ;
}

static void lcd_kit_panel_parse_effect(struct device_node* np)
{
	int ret = 0;

	/*effect color*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,effect-color-support", &common_info->effect_color.support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,effect-color-mode", &common_info->effect_color.mode, 0);
	/*bl max level*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max", &common_info->bl_level_max, 0);
	/*bl min level*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-min", &common_info->bl_level_min, 0);
	/*bl max nit*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max-nit", &common_info->bl_max_nit, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-getblmaxnit-type",  &common_info->blmaxnit.get_blmaxnit_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,Does-lcd-poweron-tp", &common_info->ul_does_lcd_poweron_tp, 0);
	/*get blmaxnit*/
	if (common_info->blmaxnit.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-bl-maxnit-command", "lcd-kit,panel-bl-maxnit-command-state",
								&common_info->blmaxnit.bl_maxnit_cmds);
	}
	/*cabc*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,cabc-support", &common_info->cabc.support, 0);
	if (common_info->cabc.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,cabc-off-cmds", "lcd-kit,cabc-off-cmds-state",
								&common_info->cabc.cabc_off_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,cabc-ui-cmds", "lcd-kit,cabc-ui-cmds-state",
								&common_info->cabc.cabc_ui_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,cabc-still-cmds", "lcd-kit,cabc-still-cmds-state",
								&common_info->cabc.cabc_still_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,cabc-moving-cmds", "lcd-kit,cabc-moving-cmds-state",
								&common_info->cabc.cabc_moving_cmds);
	}
	/*hbm*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,hbm-support", &common_info->hbm.support, 0);
	if (common_info->hbm.support) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,hbm-fp-support", &common_info->hbm.hbm_fp_support, 0);
		if(common_info->hbm.hbm_fp_support) {
			OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,hbm-level-max", &common_info->hbm.hbm_level_max, 0);
			lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-fp-enter-cmds", "lcd-kit,hbm-fp-enter-cmds-state",
									&common_info->hbm.fp_enter_cmds);
		}
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-enter-cmds", "lcd-kit,hbm-enter-cmds-state",
								&common_info->hbm.enter_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-prepare-cmds", "lcd-kit,hbm-prepare-cmds-state",
								&common_info->hbm.hbm_prepare_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-prepare-cmds-fir", "lcd-kit,hbm-prepare-cmds-fir-state",
								&common_info->hbm.prepare_cmds_fir);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-prepare-cmds-sec", "lcd-kit,hbm-prepare-cmds-sec-state",
								&common_info->hbm.prepare_cmds_sec);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-prepare-cmds-thi", "lcd-kit,hbm-prepare-cmds-thi-state",
								&common_info->hbm.prepare_cmds_thi);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-prepare-cmds-fou", "lcd-kit,hbm-prepare-cmds-fou-state",
								&common_info->hbm.prepare_cmds_fou);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds-fir", "lcd-kit,hbm-exit-cmds-fir-state",
								&common_info->hbm.exit_cmds_fir);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds-sec", "lcd-kit,hbm-exit-cmds-sec-state",
								&common_info->hbm.exit_cmds_sec);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds-thi", "lcd-kit,hbm-exit-cmds-thi-state",
								&common_info->hbm.exit_cmds_thi);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds-thi-new", "lcd-kit,hbm-exit-cmds-thi-new-state",
								&common_info->hbm.exit_cmds_thi_new);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds-fou", "lcd-kit,hbm-exit-cmds-fou-state",
								&common_info->hbm.exit_cmds_fou);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-cmds", "lcd-kit,hbm-cmds-state",
								&common_info->hbm.hbm_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-post-cmds", "lcd-kit,hbm-post-cmds-state",
								&common_info->hbm.hbm_post_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hbm-exit-cmds", "lcd-kit,hbm-exit-cmds-state",
								&common_info->hbm.exit_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,enter-dim-cmds", "lcd-kit,enter-dim-cmds-state",
								&common_info->hbm.enter_dim_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,exit-dim-cmds", "lcd-kit,exit-dim-cmds-state",
								&common_info->hbm.exit_dim_cmds);
	}
	/*acl*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,acl-support", &common_info->acl.support, 0);
	if (common_info->acl.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,acl-enable-cmds", "lcd-kit,acl-enable-cmds-state",
								&common_info->acl.acl_enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,acl-high-cmds", "lcd-kit,acl-high-cmds-state",
								&common_info->acl.acl_high_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,acl-low-cmds", "lcd-kit,acl-low-cmds-state",
								&common_info->acl.acl_low_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,acl-middle-cmds", "lcd-kit,acl-middle-cmds-state",
								&common_info->acl.acl_middle_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,acl-off-cmds", "lcd-kit,acl-off-cmds-state",
								&common_info->acl.acl_off_cmds);
	}
	/*vr*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,vr-support", &common_info->vr.support, 0);
	if (common_info->vr.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vr-enable-cmds", "lcd-kit,vr-enable-cmds-state",
								&common_info->vr.enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vr-disable-cmds", "lcd-kit,vr-disable-cmds-state",
								&common_info->vr.disable_cmds);
	}
	/*ce*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,ce-support", &common_info->ce.support, 0);
	if (common_info->ce.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ce-off-cmds", "lcd-kit,ce-off-cmds-state",
								&common_info->ce.off_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ce-srgb-cmds", "lcd-kit,ce-srgb-cmds-state",
								&common_info->ce.srgb_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ce-user-cmds", "lcd-kit,ce-user-cmds-state",
								&common_info->ce.user_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ce-vivid-cmds", "lcd-kit,ce-vivid-cmds-state",
								&common_info->ce.vivid_cmds);
	}
	/*effect on*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,effect-on-support", &common_info->effect_on.support, 0);
	if (common_info->effect_on.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,effect-on-cmds", "lcd-kit,effect-on-cmds-state",
								&common_info->effect_on.cmds);
	}
	return ;
}

static void lcd_kit_panel_parse_util(struct device_node* np)
{
	/*panel name*/
	common_info->panel_name = (char*)of_get_property(np, "lcd-kit,panel-name", NULL);
	/*panel model*/
	common_info->panel_model = (char*)of_get_property(np, "lcd-kit,panel-model", NULL);
	/*panel type*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-type", &common_info->panel_type, 0);	
	/*panel on command*/
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-on-cmds", "lcd-kit,panel-on-cmds-state",
							&common_info->panel_on_cmds);
	/*panel off command*/
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,panel-off-cmds", "lcd-kit,panel-off-cmds-state",
							&common_info->panel_off_cmds);
	/*esd*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,esd-support", &common_info->esd.support, 0);
	if (common_info->esd.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,esd-reg-cmds", "lcd-kit,esd-reg-cmds-state",
								&common_info->esd.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,esd-value", &common_info->esd.value);
	}
	/*dirty region*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,dirty-region-support", &common_info->dirty_region.support, 0);
	if (common_info->dirty_region.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,dirty-region-cmds", "lcd-kit,dirty-region-cmds-state",
								&common_info->dirty_region.cmds);
	}
	/*backlight*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,backlight-order", &common_info->backlight.order, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-min", &common_info->backlight.bl_min, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max", &common_info->backlight.bl_max, 0);
	lcd_kit_parse_dcs_cmds(np, "lcd-kit,backlight-cmds", "lcd-kit,backlight-cmds-state", &common_info->backlight.bl_cmd);

	/*check thread*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-thread-enable", &common_info->check_thread.enable, 0);
	if (common_info->check_thread.enable) {
		OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,check-bl-support", &common_info->check_thread.check_bl_support, 0);
	}
	/*vss*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,vss-support", &common_info->set_vss.support, 0);
	if (common_info->set_vss.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vss-cmds-fir", "lcd-kit,vss-cmds-fir-state",
								&common_info->set_vss.cmds_fir);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vss-cmds-sec", "lcd-kit,vss-cmds-sec-state",
								&common_info->set_vss.cmds_sec);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vss-cmds-thi", "lcd-kit,vss-cmds-thi-state",
								&common_info->set_vss.cmds_thi);
	}
	return ;
}

static void lcd_kit_parse_power_seq(struct device_node* np)
{
	lcd_kit_parse_arrays_data(np, "lcd-kit,power-on-stage", &power_seq->power_on_seq, 3);
	lcd_kit_parse_arrays_data(np, "lcd-kit,lp-on-stage", &power_seq->panel_on_lp_seq, 3);
	lcd_kit_parse_arrays_data(np, "lcd-kit,hs-on-stage", &power_seq->panel_on_hs_seq, 3);
	lcd_kit_parse_arrays_data(np, "lcd-kit,power-off-stage", &power_seq->power_off_seq, 3);
	lcd_kit_parse_arrays_data(np, "lcd-kit,lp-off-stage", &power_seq->panel_off_lp_seq, 3);
	lcd_kit_parse_arrays_data(np, "lcd-kit,hs-off-stage", &power_seq->panel_off_hs_seq, 3);
}

static void lcd_kit_parse_power(struct device_node* np)
{
	/*vci*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-vci", &power_hdl->lcd_vci);
	/*iovcc*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-iovcc", &power_hdl->lcd_iovcc);
	/*vsp*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-vsp", &power_hdl->lcd_vsp);
	/*vsn*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-vsn", &power_hdl->lcd_vsn);
	/*lcd reset*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-reset", &power_hdl->lcd_rst);
	/*backlight*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-backlight", &power_hdl->lcd_backlight);
	/*TE0*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-te0", &power_hdl->lcd_te0);
	/*tp reset*/
	lcd_kit_parse_array_data(np, "lcd-kit,tp-reset", &power_hdl->tp_rst);
	/*vdd*/
	lcd_kit_parse_array_data(np, "lcd-kit,lcd-vdd", &power_hdl->lcd_vdd);
}

static int lcd_kit_panel_parse_dt(struct device_node* np)
{
	if (!np) {
		LCD_KIT_ERR("np is null\n");
		return LCD_KIT_FAIL;
	}
	/*parse running test info*/
	lcd_kit_panel_parse_running(np);
	/*parse effect info*/
	lcd_kit_panel_parse_effect(np);
	/*parse normal info*/
	lcd_kit_panel_parse_util(np);
	/*parse power sequece*/
	lcd_kit_parse_power_seq(np);
	/*parse power*/
	lcd_kit_parse_power(np);
	return LCD_KIT_OK;
}

static int lcd_kit_get_bias_voltage(int *vpos, int *vneg)
{
	if (!vpos || !vneg) {
		LCD_KIT_ERR("vpos/vneg is null\n");
		return LCD_KIT_FAIL;
	}
	if (power_hdl->lcd_vsp.buf) {
		*vpos = power_hdl->lcd_vsp.buf[2];
	}
	if (power_hdl->lcd_vsn.buf) {
		*vneg = power_hdl->lcd_vsn.buf[2];
	}
	return LCD_KIT_OK;
}

static void lcd_kit_check_wq_handler(struct work_struct *work)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_bl_ops* bl_ops = NULL;
	struct lcd_kit_ops *lcd_ops = NULL;

	lcd_ops = lcd_kit_get_ops();
	bl_ops = lcd_kit_get_bl_ops();
	if (common_info->check_thread.enable) {
		if(common_info->set_vss.support){
			if(lcd_ops && lcd_ops->set_vss_by_thermal) {
				ret =lcd_ops->set_vss_by_thermal();
				if (ret) {
					LCD_KIT_ERR("Setting vss by thermal and backlight failed!\n");
				}
			}
		}
		if (common_info->check_thread.check_bl_support) {
			/*check backlight*/
			if (bl_ops && bl_ops->check_backlight) {
				ret = bl_ops->check_backlight();
				if (ret) {
					LCD_KIT_ERR("backlight check abnomal!\n");
				}
			}
		}
	}
}

static enum hrtimer_restart lcd_kit_check_hrtimer_fnc(struct hrtimer *timer)
{
	if (common_info->check_thread.enable) {
		schedule_delayed_work(&common_info->check_thread.check_work, 0);
		hrtimer_start(&common_info->check_thread.hrtimer, ktime_set(CHECK_THREAD_TIME_PERIOD / 1000,
			(CHECK_THREAD_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
	}
	return HRTIMER_NORESTART;
}

static void lcd_kit_check_thread_register(void)
{
	if (common_info->check_thread.enable) {
		INIT_DELAYED_WORK(&common_info->check_thread.check_work, lcd_kit_check_wq_handler);
		hrtimer_init(&common_info->check_thread.hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		common_info->check_thread.hrtimer.function = lcd_kit_check_hrtimer_fnc;
		hrtimer_start(&common_info->check_thread.hrtimer, ktime_set(CHECK_THREAD_TIME_PERIOD / 1000,
			(CHECK_THREAD_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
	}
}

static int lcd_kit_common_init(struct device_node* np)
{
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node!\n");
		return LCD_KIT_FAIL;
	}

#ifdef LCD_KIT_DEBUG_ENABLE
	lcd_kit_debugfs_init();
#endif

	lcd_kit_panel_parse_dt(np);
	/*register check thread*/
	lcd_kit_check_thread_register();
	if(common_info->hbm.support) {
		mutex_init(&common_info->hbm.hbm_lock);
	}

	return LCD_KIT_OK;
}

int lcd_dsm_client_record(struct dsm_client *lcd_dclient, char *record_buf, int lcd_dsm_error_no, int rec_num_limit, int *cur_rec_time){
#if defined (CONFIG_HUAWEI_DSM)
	if (NULL==lcd_dclient || NULL==record_buf || NULL==cur_rec_time) {
		LCD_KIT_ERR("null pointer!\n");
		return LCD_KIT_FAIL;
	}

	if (rec_num_limit >= 0 && *cur_rec_time > rec_num_limit) {
		LCD_KIT_INFO("dsm record limit!\n");
		return LCD_KIT_OK;
	}

	if (!dsm_client_ocuppy(lcd_dclient)) {
		dsm_client_record(lcd_dclient, record_buf);
		dsm_client_notify(lcd_dclient, lcd_dsm_error_no);
		(*cur_rec_time)++;
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("dsm_client_ocuppy failed!\n");
	return LCD_KIT_FAIL;
#endif
}

#define MAX_ERROR_TIMES 100000000		/*used to avoid a uint32_t happening overflow*/
static void lcd_kit_mipi_check(void* pdata, char *panel_name, long display_on_record_time)
{
	int i = 0;
	int ret = 0;
	uint32_t read_value[MAX_REG_READ_COUNT] = {0};
	static struct lcd_kit_mipierrors mipi_errors[MAX_REG_READ_COUNT] = {{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}};
	struct lcd_kit_adapt_ops* adapt_ops =  lcd_kit_get_adapt_ops();
	uint32_t* expect_ptr = common_info->mipi_check.value.buf;

#if defined (CONFIG_HUAWEI_DSM)
	#define REC_LIMIT_TIMES -1
	#define RECORD_BUFLEN 200
	char record_buf[RECORD_BUFLEN] = {'\0'};
	static int recordtime = 0;
	struct timeval tv = {0,0};
	long diskeeptime = 0;
#endif

	if (NULL == pdata || NULL == adapt_ops || NULL == expect_ptr) {
		LCD_KIT_ERR("mipi check happened parameter error!\n");
		return;
	}

	if (common_info->mipi_check.support == 0) {
		return;
	}

	if (NULL == adapt_ops->mipi_rx) {
		LCD_KIT_ERR("mipi_rx function is null!\n");
		return;
	}

	ret = adapt_ops->mipi_rx(pdata, (u8 *)read_value,  &common_info->mipi_check.cmds);
	if (ret){
		LCD_KIT_ERR("mipi read failed!\n");
		return;
	}
	for (i = 0; i < common_info->mipi_check.value.cnt; i++) {
		if (mipi_errors[i].total_errors >= MAX_ERROR_TIMES) {
			LCD_KIT_ERR("mipi error times is too large!\n");
			return;
		}
		mipi_errors[i].mipi_check_times++;
		if (read_value[i] != expect_ptr[i]) {
			mipi_errors[i].mipi_error_times++;
			mipi_errors[i].total_errors += read_value[i];
			LCD_KIT_ERR("mipi check error[%d]: current error times:%d! total error times:%d, check-error-times/check-times:%d/%d\n",
					i, read_value[i], mipi_errors[i].total_errors, mipi_errors[i].mipi_error_times, mipi_errors[i].mipi_check_times);

#if defined (CONFIG_HUAWEI_DSM)
			if (read_value[i] < common_info->mipi_check.mipi_error_report_threshold) {
				continue;
			}
			do_gettimeofday(&tv);
			diskeeptime = tv.tv_sec - display_on_record_time;
			ret = snprintf(record_buf, RECORD_BUFLEN, "%s:display_on_keep_time=%ds, reg_val[%d]=0x%x!\n",
					panel_name,diskeeptime,common_info->mipi_check.cmds.cmds[i].payload[0],read_value[i]);
			if (ret < 0) {
				LCD_KIT_ERR("snprintf happened error!\n");
				continue;
			}
			(void)lcd_dsm_client_record(lcd_dclient, record_buf, DSM_LCD_MIPI_TRANSMIT_ERROR_NO, REC_LIMIT_TIMES, &recordtime);
#endif

			continue;
		}
		LCD_KIT_INFO("mipi check nomal[%d]: total error times:%d, check-error-times/check-times:%d/%d\n",
				i, mipi_errors[i].total_errors, mipi_errors[i].mipi_error_times, mipi_errors[i].mipi_check_times);
	}
}

/*common ops*/
struct lcd_kit_common_ops g_lcd_kit_common_ops = {
	.common_init = lcd_kit_common_init,
	.panel_power_on = lcd_kit_panel_power_on,
	.panel_on_lp = lcd_kit_panel_on_lp,
	.panel_on_hs = lcd_kit_panel_on_hs,
	.panel_off_hs = lcd_kit_panel_off_hs,
	.panel_off_lp = lcd_kit_panel_off_lp,
	.panel_power_off = lcd_kit_panel_power_off,
	.get_panel_name = lcd_kit_get_panel_name,
	.get_panel_info = lcd_kit_get_panel_info,
	.get_cabc_mode = lcd_kit_get_cabc_mode,
	.set_cabc_mode = lcd_kit_set_cabc_mode,
	.get_acl_mode = lcd_kit_get_acl_mode,
	.set_acl_mode = lcd_kit_set_acl_mode,
	.get_vr_mode = lcd_kit_get_vr_mode,
	.set_vr_mode = lcd_kit_set_vr_mode,
	.esd_handle = lcd_kit_esd_handle,
	.dirty_region_handle = lcd_kit_dirty_region_handle,
	.set_ce_mode = lcd_kit_set_ce_mode,
	.get_ce_mode = lcd_kit_get_ce_mode,
	.hbm_set_handle = lcd_kit_hbm_set_handle,
	.inversion_set_mode = lcd_kit_inversion_set_mode,
	.inversion_get_mode = lcd_kit_inversion_get_mode,
	.scan_set_mode = lcd_kit_scan_set_mode,
	.scan_get_mode = lcd_kit_scan_get_mode,
	.check_reg = lcd_kit_check_reg,
	.set_sleep_mode = lcd_kit_set_sleep_mode,
	.get_sleep_mode = lcd_kit_get_sleep_mode,
	.set_effect_color_mode = lcd_kit_set_effect_color_mode,
	.get_effect_color_mode = lcd_kit_get_effect_color_mode,
	.get_test_config = lcd_kit_get_test_config,
	.set_test_config = lcd_kit_set_test_config,
	.set_mipi_backlight = lcd_kit_set_mipi_backlight,
	.get_bias_voltage = lcd_kit_get_bias_voltage,
	.mipi_check = lcd_kit_mipi_check,
};

