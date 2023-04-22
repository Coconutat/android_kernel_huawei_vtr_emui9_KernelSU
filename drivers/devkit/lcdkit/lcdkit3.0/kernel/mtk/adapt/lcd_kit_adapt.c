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
#include "lcd_kit_disp.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_power.h"
#include "lcm_drv.h"

extern struct LCM_UTIL_FUNCS lcm_util_mtk;
extern int do_lcm_vdo_lp_write(struct dsi_cmd_desc *write_table, unsigned int count);
extern int do_lcm_vdo_lp_read(struct dsi_cmd_desc *cmd_tab, unsigned int count);
#define mipi_dsi_cmds_tx(cmdq, cmds) \
		lcm_util_mtk.mipi_dsi_cmds_tx(cmdq, cmds)

#define mipi_dsi_cmds_rx(out, cmds, len) \
		lcm_util_mtk.mipi_dsi_cmds_rx(out, cmds, len)

static void lcd_kit_dump_cmd(struct dsi_cmd_desc* cmd)
{
	int i = 0;

	if (cmd == NULL) {
		LCD_KIT_ERR("lcd_kit_cmd is null point!\n");
		return;
	}

	LCD_KIT_DEBUG("cmd->dtype = 0x%x\n", cmd->dtype);
	LCD_KIT_DEBUG("cmd->vc = 0x%x\n", cmd->vc);
	LCD_KIT_DEBUG("cmd->dlen = 0x%x\n", cmd->dlen);
	LCD_KIT_DEBUG("cmd->payload:\n");
	if (lcd_kit_msg_level >= MSG_LEVEL_DEBUG) {
		for (i = 0; i < cmd->dlen; i++) {
			LCD_KIT_DEBUG("0x%x\n", cmd->payload[i]);
		}
	}
}

static int lcd_kit_cmds_to_mtk_dsi_cmds(struct lcd_kit_dsi_cmd_desc* lcd_kit_cmds, struct dsi_cmd_desc* cmd)
{
	if (lcd_kit_cmds == NULL) {
		LCD_KIT_ERR("lcd_kit_cmds is null point!\n");
		return LCD_KIT_FAIL;
	}
	if (cmd == NULL) {
		LCD_KIT_ERR("cmd is null point!\n");
		return LCD_KIT_FAIL;
	}
	cmd->dtype = lcd_kit_cmds->payload[0];
	cmd->vc =  lcd_kit_cmds->vc;
	cmd->dlen =  (lcd_kit_cmds->dlen - 1);
	cmd->payload = &lcd_kit_cmds->payload[1];
    cmd->link_state = 1;
    lcd_kit_dump_cmd(cmd);
	return LCD_KIT_OK;
}

static int lcd_kit_cmds_to_mtk_dsi_read_cmds(struct lcd_kit_dsi_cmd_desc* lcd_kit_cmds, struct dsi_cmd_desc* cmd)
{
	if (lcd_kit_cmds == NULL) {
		LCD_KIT_ERR("lcd_kit_cmds is null point!\n");
		return LCD_KIT_FAIL;
	}
	if (cmd == NULL) {
		LCD_KIT_ERR("cmd is null point!\n");
		return LCD_KIT_FAIL;
	}

	cmd->dtype = lcd_kit_cmds->payload[0];
	cmd->vc =  lcd_kit_cmds->vc;
	cmd->dlen =  lcd_kit_cmds->dlen;
    cmd->link_state = 1;

	return LCD_KIT_OK;
}

int mtk_mipi_dsi_cmds_tx(struct lcd_kit_dsi_cmd_desc *cmds, int cnt)
{
	struct lcd_kit_dsi_cmd_desc *cm = NULL;
	struct dsi_cmd_desc dsi_cmd;
	int i = 0;

	if (NULL == cmds) {
		LCD_KIT_ERR("cmds is NULL");
		return -EINVAL;
	}

	cm = cmds;

	for (i = 0; i < cnt; i++) {
    	lcd_kit_cmds_to_mtk_dsi_cmds(cm, &dsi_cmd);
		(void)mipi_dsi_cmds_tx(NULL, &dsi_cmd);

		if (cm->wait) {
			if (cm->waittype == WAIT_TYPE_US)
				udelay(cm->wait);
			else if (cm->waittype == WAIT_TYPE_MS) {
				if (cm->wait <= 10) {
					mdelay(cm->wait);
				} else {
					msleep(cm->wait);
				}
			}
			else
				msleep(cm->wait * 1000);
		}
		cm++;
	}

	return cnt;
}

int mtk_mipi_dsi_cmds_extern_tx(struct lcd_kit_dsi_cmd_desc *cmds, int cnt)
{
	struct lcd_kit_dsi_cmd_desc *cm = NULL;
	struct dsi_cmd_desc dsi_cmd;
	int i = 0;

	if (NULL == cmds) {
		LCD_KIT_ERR("cmds is NULL");
		return -EINVAL;
	}

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		lcd_kit_cmds_to_mtk_dsi_cmds(cm, &dsi_cmd);

		(void)do_lcm_vdo_lp_write(&dsi_cmd, 1);
LCD_KIT_ERR("dttype is 0x%x, len is %d, payload is 0x%x\n",dsi_cmd.dtype,dsi_cmd.dlen,*(dsi_cmd.payload));
		if (cm->wait) {
			if (cm->waittype == WAIT_TYPE_US)
				udelay(cm->wait);
			else if (cm->waittype == WAIT_TYPE_MS) {
				if (cm->wait <= 10) {
					mdelay(cm->wait);
				} else {
					msleep(cm->wait);
				}
			}
			else
				msleep(cm->wait * 1000);
		}
		cm++;
	}

	return cnt;
}


static int lcd_kit_cmd_is_write(struct lcd_kit_dsi_panel_cmds* cmd)
{
	int ret = LCD_KIT_FAIL;

	if (cmd == NULL) {
		LCD_KIT_ERR("lcd_kit_cmd is null point!\n");
		return LCD_KIT_FAIL;
	}

	switch (cmd->cmds->dtype) {
		case DTYPE_GEN_WRITE:
		case DTYPE_GEN_WRITE1:
		case DTYPE_GEN_WRITE2:
		case DTYPE_GEN_LWRITE:
		case DTYPE_DCS_WRITE:
		case DTYPE_DCS_WRITE1:
		case DTYPE_DCS_LWRITE:
		case DTYPE_DSC_LWRITE:
			ret = LCD_KIT_FAIL;
			break;
		case DTYPE_GEN_READ:
		case DTYPE_GEN_READ1:
		case DTYPE_GEN_READ2:
		case DTYPE_DCS_READ:
			ret = LCD_KIT_OK;
			break;
		default:
			ret = LCD_KIT_FAIL;
			break;
	}
	return ret;
}
/*
 *  dsi send cmds
*/
int lcd_kit_dsi_cmds_tx(void* hld, struct lcd_kit_dsi_panel_cmds* cmds)
{
    int i;

	if (cmds == NULL) {
		LCD_KIT_ERR("lcd_kit_cmds is null point!\n");
		return LCD_KIT_FAIL;
	}

	down(&disp_info->lcd_kit_sem);
	for (i = 0; i < cmds->cmd_cnt; i++) {
		mtk_mipi_dsi_cmds_tx(&cmds->cmds[i], 1);
	}
	up(&disp_info->lcd_kit_sem);

	return 0;

}

int lcd_kit_parse_read_data(uint8_t *out, uint32_t *in, int start_index, int dlen)
{
	return 0;
}

int lcd_kit_dsi_cmds_rx(void* hld, uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds)
{
    int i = 0;
    int ret = 0;
	struct dsi_cmd_desc dsi_cmd;

	if ((cmds == NULL)  || (out == NULL)){
		LCD_KIT_ERR("out or cmds is null point!\n");
		return LCD_KIT_FAIL;
	}

	down(&disp_info->lcd_kit_sem);
	for (i = 0; i < cmds->cmd_cnt; i++) {
		lcd_kit_cmds_to_mtk_dsi_cmds(&cmds->cmds[i], &dsi_cmd);
		if (lcd_kit_cmd_is_write(cmds)) {
    		ret = mtk_mipi_dsi_cmds_tx(&cmds->cmds[i], 1);
		} else {
			lcd_kit_cmds_to_mtk_dsi_cmds(&cmds->cmds[i], &dsi_cmd);
    		ret = mipi_dsi_cmds_rx(out, &dsi_cmd, dsi_cmd.dlen);
		}
	}
	up(&disp_info->lcd_kit_sem);

    return ret;
}

int lcd_kit_dsi_cmds_extern_rx(uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds)
{
    int i = 0;
	int j = 0;
    int ret = 0;
	struct dsi_cmd_desc dsi_cmd;
	uint8_t buffer[4] = {0};

	if ((cmds == NULL)  || (out == NULL)){
		LCD_KIT_ERR("out or cmds is null point!\n");
		return LCD_KIT_FAIL;
	}

	for (i = 0; i < cmds->cmd_cnt; i++) {
		if (lcd_kit_cmd_is_write(cmds)) {
			mtk_mipi_dsi_cmds_extern_tx(&cmds->cmds[i], 1);
		} else {
			memset(buffer,0,sizeof(buffer)/sizeof(uint8_t));
			lcd_kit_cmds_to_mtk_dsi_read_cmds(&cmds->cmds[i], &dsi_cmd);
			dsi_cmd.payload = &buffer[0];
			ret = do_lcm_vdo_lp_read(&dsi_cmd, 1);
			out[j] = buffer[1];
			LCD_KIT_INFO("j is %d data0 is 0x%x  data1 is 0x%x  data2 is 0x%x  data3 is 0x%x\n",j,buffer[0],buffer[1],buffer[2],buffer[3]);
			j++;
		}
	}

    return ret;
}

int lcd_kit_dsi_cmds_extern_tx(struct lcd_kit_dsi_panel_cmds* cmds)
{
    int i;

	if (cmds == NULL) {
		LCD_KIT_ERR("lcd_kit_cmds is null point!\n");
		return LCD_KIT_FAIL;
	}

	for (i = 0; i < cmds->cmd_cnt; i++) {
		mtk_mipi_dsi_cmds_extern_tx(&cmds->cmds[i], 1);
	}

	return 0;

}

static int lcd_kit_buf_trans(const char* inbuf, int inlen, char** outbuf, int* outlen)
{
	char* buf;
	int i;
	int bufsize = inlen;

	if (!inbuf || !outbuf || !outlen) {
		LCD_KIT_ERR("inbuf is null point!\n");
		return LCD_KIT_FAIL;
	}
 
	/*The property is 4bytes long per element in cells: <>*/
	bufsize = bufsize / 4;
	/*If use bype property: [], this division should be removed*/
	buf = kzalloc(sizeof(char) * bufsize, GFP_KERNEL);
	if (!buf) {
		LCD_KIT_ERR("buf is null point!\n");
		return LCD_KIT_FAIL;
	}
	//For use cells property: <>
	for (i = 0; i < bufsize; i++) {
		buf[i] = inbuf[i * 4 + 3];
	}
	*outbuf = buf;
	*outlen = bufsize;
	return LCD_KIT_OK;
}

static int lcd_kit_gpio_enable(u32 type)
{
	lcd_kit_gpio_tx(type, GPIO_HIGH);
	return LCD_KIT_OK;
}

static int lcd_kit_gpio_disable(u32 type)
{
	lcd_kit_gpio_tx(type, GPIO_LOW);
	return LCD_KIT_OK;
}

static int lcd_kit_regulator_enable(u32 type)
{
	int ret = LCD_KIT_OK;

	switch(type)
	{
		case LCD_KIT_VSP:
		case LCD_KIT_VSN:
		case LCD_KIT_BL:
			ret = lcd_kit_charger_ctrl(type, 1);
			break;
		default:
			ret = LCD_KIT_FAIL;
			LCD_KIT_ERR("regulator type:%d not support\n", type);
			break;
	}
	return ret;
}

static int lcd_kit_regulator_disable(u32 type)
{
	int ret = LCD_KIT_OK;

	switch(type)
	{
		case LCD_KIT_VSP:
		case LCD_KIT_VSN:
		case LCD_KIT_BL:
			ret = lcd_kit_charger_ctrl(type, 0);
			break;
		default:
			LCD_KIT_ERR("regulator type:%d not support\n", type);
			break;
	}
	return ret;
}

static int lcd_kit_lock(void *hld)
{
	return LCD_KIT_FAIL;
}

static void lcd_kit_release(void *hld)
{
    return;
}

void *lcd_kit_get_pdata_hld(void)
{
    return 0;
}

struct lcd_kit_adapt_ops adapt_ops = {
	.mipi_tx = lcd_kit_dsi_cmds_tx,
	.mipi_rx = lcd_kit_dsi_cmds_rx,
	.gpio_enable = lcd_kit_gpio_enable,
	.gpio_disable = lcd_kit_gpio_disable,
	.regulator_enable = lcd_kit_regulator_enable,
	.regulator_disable = lcd_kit_regulator_disable,
	.buf_trans = lcd_kit_buf_trans,
	.lock = lcd_kit_lock,
	.release = lcd_kit_release,
	.get_pdata_hld = lcd_kit_get_pdata_hld,
};
int lcd_kit_adapt_init(void)
{
	int ret = LCD_KIT_OK;
	ret = lcd_kit_adapt_register(&adapt_ops);
	return ret;
}
