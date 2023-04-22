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

#include "hisi_fb.h"
#include "lcd_kit_common.h"
#include "lcd_kit_dbg.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_power.h"

static void lcd_kit_dump_cmd(struct dsi_cmd_desc* cmd)
{
	int i = 0;

	LCD_KIT_DEBUG("cmd->dtype = 0x%x\n", cmd->dtype);
	LCD_KIT_DEBUG("cmd->vc = 0x%x\n", cmd->vc);
	LCD_KIT_DEBUG("cmd->wait = 0x%x\n", cmd->wait);
	LCD_KIT_DEBUG("cmd->waittype = 0x%x\n", cmd->waittype);
	LCD_KIT_DEBUG("cmd->dlen = 0x%x\n", cmd->dlen);
	LCD_KIT_DEBUG("cmd->payload:\n");
	if (lcd_kit_msg_level >= MSG_LEVEL_DEBUG) {
		for (i = 0; i < cmd->dlen; i++) {
			LCD_KIT_DEBUG("0x%x\n", cmd->payload[i]);
		}
	}
}

static int lcd_kit_cmds_to_dsi_cmds(struct lcd_kit_dsi_cmd_desc* lcd_kit_cmds, struct dsi_cmd_desc* cmd, int link_state)
{
	if (lcd_kit_cmds == NULL) {
		LCD_KIT_ERR("lcd_kit_cmds is null point!\n");
		return LCD_KIT_FAIL;
	}
	if (cmd == NULL) {
		LCD_KIT_ERR("cmd is null point!\n");
		return LCD_KIT_FAIL;
	}
	cmd->dtype = lcd_kit_cmds->dtype;
	if(link_state == LCD_KIT_DSI_LP_MODE){
		cmd->dtype  |= GEN_VID_LP_CMD;
	}
	cmd->vc =  lcd_kit_cmds->vc;
	cmd->waittype =  lcd_kit_cmds->waittype;
	cmd->dlen =  lcd_kit_cmds->dlen;
	cmd->payload = lcd_kit_cmds->payload;
	lcd_kit_dump_cmd(cmd);
	return LCD_KIT_OK;
}

static int lcd_kit_cmd_is_write(struct dsi_cmd_desc* cmd)
{
	int ret = LCD_KIT_FAIL;

	switch (DSI_HDR_DTYPE(cmd->dtype)) {
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

void lcd_kit_dsi_cmds_tx_delay(char wait,char waittype)
{
	if (wait) {
		if (waittype == WAIT_TYPE_US) {
			udelay(wait);
		}
		else if (waittype == WAIT_TYPE_MS) {
			LCD_KIT_DELAY(wait);
		}
		else {
			msleep(wait * 1000);
		}
	}
}
/*
 *  dsi send cmds
*/
int lcd_kit_dsi_cmds_tx(void* hld, struct lcd_kit_dsi_panel_cmds* cmds)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	int link_state = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	struct dsi_cmd_desc dsi_cmd;

	if (cmds == NULL) {
		LCD_KIT_ERR("cmd cnt is 0!\n");
		return LCD_KIT_FAIL;
	}

	memset(&dsi_cmd, 0, sizeof(struct dsi_cmd_desc) );

	if (cmds->cmds == NULL || cmds->cmd_cnt <= 0) {
		LCD_KIT_ERR("cmds is null, or cmds->cmd_cnt <= 0!\n");
		return LCD_KIT_FAIL;
	}
	hisifd = (struct hisi_fb_data_type*) hld;
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null!\n");
		return LCD_KIT_FAIL;
	}
	link_state = cmds->link_state;
	down(&disp_info->lcd_kit_sem);
	for (i = 0; i < cmds->cmd_cnt; i++) {
		lcd_kit_cmds_to_dsi_cmds(&cmds->cmds[i], &dsi_cmd, link_state);
		if (!lcd_kit_dsi_fifo_is_full(hisifd->mipi_dsi0_base)) {
			mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi0_base);
		}
		if (disp_info->dsi1_cmd_support) {
			if (!lcd_kit_dsi_fifo_is_full(hisifd->mipi_dsi1_base)) {
				mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi1_base);
			}
		}
		lcd_kit_dsi_cmds_tx_delay( cmds->cmds[i].wait,cmds->cmds[i].waittype);
	}
	up(&disp_info->lcd_kit_sem);
	return ret;

}

int lcd_kit_dsi_cmds_rx(void* hld, uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds)
{
	#define READ_MAX 100
	uint32_t tmp_value[READ_MAX] = {0};
	int dlen = 0;
	int cnt = 0;
	int ret = LCD_KIT_OK;
	int i = 0;
	int start_index = 0;
	int link_state = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	struct dsi_cmd_desc dsi_cmd;

	hisifd = (struct hisi_fb_data_type*) hld;
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null!\n");
		return LCD_KIT_FAIL;
	}
	if (cmds == NULL) {
		LCD_KIT_ERR("cmds or cmds->cmds is null!\n");
		return LCD_KIT_FAIL;
	}
	if (cmds->cmds == NULL || cmds->cmd_cnt <= 0) {
		LCD_KIT_ERR("cmds is null, or cmds->cmd_cnt <= 0!\n");
		return LCD_KIT_FAIL;
	}
	memset(&dsi_cmd, 0, sizeof(struct dsi_cmd_desc) );
	link_state = cmds->link_state;
	down(&disp_info->lcd_kit_sem);
	for (i = 0; i < cmds->cmd_cnt; i++) {
		lcd_kit_cmds_to_dsi_cmds(&cmds->cmds[i], &dsi_cmd, link_state);
		if (lcd_kit_cmd_is_write(&dsi_cmd)) {
			if (!lcd_kit_dsi_fifo_is_full(hisifd->mipi_dsi0_base)) {
				mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi0_base);
				lcd_kit_dsi_cmds_tx_delay( cmds->cmds[i].wait,cmds->cmds[i].waittype);
			} else {
				LCD_KIT_ERR("mipi write error\n");
				ret = LCD_KIT_FAIL;
				break;
			}
		} else {
			if (!lcd_kit_dsi_fifo_is_full(hisifd->mipi_dsi0_base)) {
				ret = mipi_dsi_lread_reg(tmp_value, &dsi_cmd, dsi_cmd.dlen, hisifd->mipi_dsi0_base);
				if (ret) {
					LCD_KIT_ERR("mipi read error\n");
					break;
				}
				start_index = 0;
				if (dsi_cmd.dlen > 1) {
					start_index = (int)dsi_cmd.payload[1];
				}
				for (dlen = 0; dlen < dsi_cmd.dlen; dlen++) {
					if (dlen < (start_index - 1)){
						continue;
					}
					switch (dlen % 4) {
					case 0:
						out[cnt] = (uint8_t)(tmp_value[dlen / 4] & 0xFF);
						break;
					case 1:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 8) & 0xFF);
						break;
					case 2:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 16) & 0xFF);
						break;
					case 3:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 24) & 0xFF);
						break;
					}
					cnt++;
				}
			} else {
				LCD_KIT_ERR("mipi write error\n");
				ret = LCD_KIT_FAIL;
				break;
			}
		}
	}
	up(&disp_info->lcd_kit_sem);
	return ret;

}

int lcd_kit_dsi_cmds_tx_no_lock(void* hld, struct lcd_kit_dsi_panel_cmds* cmds)
{
	int ret = LCD_KIT_OK;
	int i = 0;
	int link_state = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	struct dsi_cmd_desc dsi_cmd;

	if (cmds == NULL) {
		LCD_KIT_ERR("cmd cnt is 0!\n");
		return LCD_KIT_FAIL;
	}
	if (cmds->cmds == NULL || cmds->cmd_cnt <= 0) {
		LCD_KIT_ERR("cmds is null, or cmds->cmd_cnt <= 0!\n");
		return LCD_KIT_FAIL;
	}
	hisifd = (struct hisi_fb_data_type*) hld;
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null!\n");
		return LCD_KIT_FAIL;
	}
	memset(&dsi_cmd, 0, sizeof(struct dsi_cmd_desc) );
	link_state = cmds->link_state;
	for (i = 0; i < cmds->cmd_cnt; i++) {
		lcd_kit_cmds_to_dsi_cmds(&cmds->cmds[i], &dsi_cmd, link_state);
		if (!lcd_kit_dsi_fifo_is_empty(hisifd->mipi_dsi0_base)) {
			mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi0_base);
		}
		if (disp_info->dsi1_cmd_support) {
			if (!lcd_kit_dsi_fifo_is_full(hisifd->mipi_dsi1_base)) {
				mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi1_base);
			}
		}
		lcd_kit_dsi_cmds_tx_delay( cmds->cmds[i].wait,cmds->cmds[i].waittype);
	}
	return ret;

}

int lcd_kit_dsi_cmds_rx_no_lock(void* hld, uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds)
{
	#define READ_MAX 100
	uint32_t tmp_value[READ_MAX] = {0};
	int dlen = 0;
	int cnt = 0;
	int ret = LCD_KIT_OK;
	int i = 0;
	int start_index = 0;
	int link_state = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	struct dsi_cmd_desc dsi_cmd;

	hisifd = (struct hisi_fb_data_type*) hld;
	if (hisifd == NULL) {
		LCD_KIT_ERR("hisifd is null!\n");
		return LCD_KIT_FAIL;
	}
	if (cmds == NULL) {
		LCD_KIT_ERR("cmds or cmds->cmds is null!\n");
		return LCD_KIT_FAIL;
	}
	if (cmds->cmds == NULL || cmds->cmd_cnt <= 0) {
		LCD_KIT_ERR("cmds is null, or cmds->cmd_cnt <= 0!\n");
		return LCD_KIT_FAIL;
	}
	memset(&dsi_cmd, 0, sizeof(struct dsi_cmd_desc) );
	link_state = cmds->link_state;
	for (i = 0; i < cmds->cmd_cnt; i++) {
		lcd_kit_cmds_to_dsi_cmds(&cmds->cmds[i], &dsi_cmd, link_state);
		if (lcd_kit_cmd_is_write(&dsi_cmd)) {
			if (!lcd_kit_dsi_fifo_is_empty(hisifd->mipi_dsi0_base)) {
				mipi_dsi_cmds_tx(&dsi_cmd, 1, hisifd->mipi_dsi0_base);
				lcd_kit_dsi_cmds_tx_delay( cmds->cmds[i].wait,cmds->cmds[i].waittype);
			} else {
				LCD_KIT_ERR("mipi write error\n");
				ret = LCD_KIT_FAIL;
				break;
			}
		} else {
			if (!lcd_kit_dsi_fifo_is_empty(hisifd->mipi_dsi0_base)) {
				ret = mipi_dsi_lread_reg(tmp_value, &dsi_cmd, dsi_cmd.dlen, hisifd->mipi_dsi0_base);
				if (ret) {
					LCD_KIT_ERR("mipi read error\n");
					break;
				}
				start_index = 0;
				if (dsi_cmd.dlen > 1) {
					start_index = (int)dsi_cmd.payload[1];
				}
				for (dlen = 0; dlen < dsi_cmd.dlen; dlen++) {
					if (dlen < (start_index - 1)){
						continue;
					}
					switch (dlen % 4) {
					case 0:
						out[cnt] = (uint8_t)(tmp_value[dlen / 4] & 0xFF);
						break;
					case 1:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 8) & 0xFF);
						break;
					case 2:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 16) & 0xFF);
						break;
					case 3:
						out[cnt] = (uint8_t)((tmp_value[dlen / 4] >> 24) & 0xFF);
						break;
					}
					cnt++;
				}
			} else {
				LCD_KIT_ERR("mipi write error\n");
				ret = LCD_KIT_FAIL;
				break;
			}
		}
	}
	return ret;

}

static int lcd_kit_buf_trans(const char* inbuf, int inlen, char** outbuf, int* outlen)
{
	char* buf;
	int i;
	int bufsize = inlen;

	if (!inbuf) {
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
	lcd_kit_gpio_tx(type, GPIO_REQ);
	lcd_kit_gpio_tx(type, GPIO_HIGH);
	return LCD_KIT_OK;
}

static int lcd_kit_gpio_disable(u32 type)
{
	lcd_kit_gpio_tx(type, GPIO_LOW);
	lcd_kit_gpio_tx(type, GPIO_FREE);
	return LCD_KIT_OK;
}

static int lcd_kit_regulator_enable(u32 type)
{
	int ret = LCD_KIT_OK;

	switch(type)
	{
		case LCD_KIT_VCI:
		case LCD_KIT_IOVCC:
		case LCD_KIT_VDD:
			ret = lcd_kit_pmu_ctrl(type, 1);
			break;
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
		case LCD_KIT_VCI:
		case LCD_KIT_IOVCC:
		case LCD_KIT_VDD:
			ret = lcd_kit_pmu_ctrl(type, 0);
			break;
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
	struct hisi_fb_data_type* hisifd = hld;

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on)
	{
		LCD_KIT_ERR("fb%d, panel power off!\n", hisifd->index);
		goto err_out;
	}
	return LCD_KIT_OK;
err_out:
	up(&hisifd->blank_sem);
	return LCD_KIT_FAIL;
}

static void lcd_kit_release(void *hld)
{
	struct hisi_fb_data_type* hisifd = hld;
	up(&hisifd->blank_sem);
}

void *lcd_kit_get_pdata_hld(void)
{
	return hisifd_list[PRIMARY_PANEL_IDX];
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
