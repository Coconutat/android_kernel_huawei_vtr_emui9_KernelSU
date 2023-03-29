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

/***********************************************************
*function definition
***********************************************************/
/*
*name:hw_lcd_parse_array_data
*function:parse panel data from dtsi
*@np:device tree node
*@name:parse name
*@out:output data
*/

int conver2le(uint32_t *src, uint32_t *desc, int len)
{
	int i = 0;
	uint32_t temp = 0;

	if ((src == NULL) || (desc == NULL)) {
		LCD_KIT_ERR("src or desc is null\n");
		return LCD_KIT_FAIL;
	}
	for (i = 0; i < len; i++) {
		temp = ((src[i] & 0xff) << 24);
		temp |= (((src[i] >> 8) & 0xff) << 16);
		temp |= (((src[i] >> 16) & 0xff) << 8);
		temp |= ((src[i] >> 24) & 0xff);
		desc[i] = temp;
		temp = 0;
	}
	return LCD_KIT_OK;
}

int lcd_kit_parse_array_data(struct device_node* np,
							 char* name, struct lcd_kit_array_data* out)
{
	int blen = 0;
	uint32_t* data = NULL;
	uint32_t* buf = NULL;

	data = (uint32_t*)of_get_property(np, name, &blen);
	if (!data) {
		LCD_KIT_ERR("get data fail\n");
		return LCD_KIT_FAIL;
	}
	blen = blen/4;
	buf = kzalloc(blen * sizeof(uint32_t), GFP_KERNEL);
	if (!buf) {
		LCD_KIT_ERR("alloc buf fail\n");
		return LCD_KIT_FAIL;
	}
	memcpy(buf, data, blen * sizeof(uint32_t));
	conver2le(buf, buf, blen);
	out->buf = buf;
	out->cnt = blen;
	return LCD_KIT_OK;
}

int lcd_kit_parse_arrays_data(struct device_node* np,
									 char* name, struct lcd_kit_arrays_data* out, int num)
{
	int i = 0, cnt = 0;
	int len = 0, blen = 0;
	uint32_t* data = NULL;
	uint32_t* buf = NULL, *bp = NULL;

	data = (uint32_t*)of_get_property(np, name, &blen);
	if (!data) {
		LCD_KIT_ERR("parse property %s error\n", name);
		return LCD_KIT_FAIL;
	}
	len = blen/4;
	buf = kzalloc(sizeof(uint32_t) * len, GFP_KERNEL);
	if (!buf) {
		LCD_KIT_ERR("alloc buf fail\n");
		return LCD_KIT_FAIL;
	}
	memcpy(buf, data, len * sizeof(uint32_t));
	conver2le(buf, buf, len);
	while (len >= num) {
		if (num > len) {
			LCD_KIT_ERR("data length = %x error\n", len);
			goto exit_free;
		}
		len -= num;
		cnt++;
	}
	if (len != 0) {
		LCD_KIT_ERR("dts data parse error! data len = %d\n", len);
		goto exit_free;
	}
	out->cnt = cnt;
	out->arry_data = kzalloc(sizeof(struct lcd_kit_array_data) * cnt, GFP_KERNEL);
	if (!out->arry_data) {
		LCD_KIT_ERR("kzalloc fail\n");
		goto exit_free;
	}
	bp = buf;
	for (i = 0; i < cnt; i++) {
		out->arry_data[i].cnt = num;
		out->arry_data[i].buf = bp;
		bp += num;
	}
	return LCD_KIT_OK;
exit_free:
	kfree(buf);
	return LCD_KIT_FAIL;
}

int lcd_kit_parse_dcs_cmds(struct device_node* np, char* cmd_key,
						   char* link_key, struct lcd_kit_dsi_panel_cmds* pcmds)
{
	struct lcd_kit_adapt_ops* adapt_ops = NULL;
	const char* data = NULL;
	int blen = 0, len = 0, buflen = 0;
	int ret = LCD_KIT_OK;
	char* buf = NULL, *bp = NULL;
	struct lcd_kit_dsi_cmd_desc_header* dchdr = NULL;
	int i = 0, cnt = 0;

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("adapt_ops is null!\n");
		return LCD_KIT_FAIL;
	}
	memset(pcmds, 0, sizeof(struct lcd_kit_dsi_panel_cmds));
	data = (char*)of_get_property(np, cmd_key, &blen);
	if (!data || (0 == blen)) {
		LCD_KIT_ERR(" failed, key = %s\n", cmd_key);
		return LCD_KIT_FAIL;
	}
	if (adapt_ops->buf_trans) {
		ret = adapt_ops->buf_trans(data, blen, &buf, &buflen);
	}
	if (ret) {
		LCD_KIT_ERR("buffer trans fail!\n");
		return LCD_KIT_FAIL;
	}
	if (!buf) {
		LCD_KIT_ERR("buf is null!\n");
		return LCD_KIT_FAIL;
	}
	/* scan dcs commands */
	bp = buf;
	len = buflen;
	while (len >= (int)sizeof(struct lcd_kit_dsi_cmd_desc_header)) {
		dchdr = (struct lcd_kit_dsi_cmd_desc_header*)bp;
		//dchdr->dlen = ntohs(dchdr->dlen);
		bp += sizeof(struct lcd_kit_dsi_cmd_desc_header);
		len -= (int)sizeof(struct lcd_kit_dsi_cmd_desc_header);
		if (dchdr->dlen > len) {
			LCD_KIT_ERR("cmd = 0x%x parse error, len = %d\n", dchdr->dtype, dchdr->dlen);
			goto exit_free;
		}
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}
	if (len != 0) {
		LCD_KIT_ERR("dcs_cmd parse error! cmd len = %d\n", len);
		goto exit_free;
	}
	pcmds->cmds = kzalloc(cnt * sizeof(struct lcd_kit_dsi_cmd_desc), GFP_KERNEL);
	if (!pcmds->cmds) {
		LCD_KIT_ERR("kzalloc fail\n");
		goto exit_free;
	}
	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = buflen;
	bp = buf;
	len = buflen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct lcd_kit_dsi_cmd_desc_header*)bp;
		len -= (int)sizeof(struct lcd_kit_dsi_cmd_desc_header);
		bp += sizeof(struct lcd_kit_dsi_cmd_desc_header);
		pcmds->cmds[i].dtype    = dchdr->dtype;
		pcmds->cmds[i].last     = dchdr->last;
		pcmds->cmds[i].vc       = dchdr->vc;
		pcmds->cmds[i].ack      = dchdr->ack;
		pcmds->cmds[i].wait     = dchdr->wait;
		pcmds->cmds[i].waittype = dchdr->waittype;
		pcmds->cmds[i].dlen     = dchdr->dlen;
		pcmds->cmds[i].payload  = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}
	pcmds->flags = LCD_KIT_CMD_REQ_COMMIT;
	/*Set default link state to LP Mode*/
	pcmds->link_state = LCD_KIT_DSI_LP_MODE;
	if (link_key) {
		data = (char*)of_get_property(np, link_key, NULL);
		if (data && !strcmp(data, "dsi_hs_mode")) {
			pcmds->link_state = LCD_KIT_DSI_HS_MODE;
		} else {
			pcmds->link_state = LCD_KIT_DSI_LP_MODE;
		}
	}
	return LCD_KIT_OK;
exit_free:
	kfree(buf);
	return LCD_KIT_FAIL;

}
