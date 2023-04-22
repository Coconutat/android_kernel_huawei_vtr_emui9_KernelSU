/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011-2014 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */
#include "wacom.h"
extern struct wacom_i2c *wac_data;

static int check_valid_array_index(int i, int array_size)
{
	if(i >= array_size) {
		TS_LOG_ERR("current index is %d, invaild! max is %d\n", i, array_size);
		return -EINVAL;
	}
	return NO_ERR;
}

static void parse_report_desc(struct wacom_features *features, u8 *report_desc, int report_desc_size)
{
	bool finger = false, pen = false;
	int i=0;
	int usage = 0, mouse = 0;

	if ((!features) || (!report_desc) ) {
		TS_LOG_ERR("date is null\n");
		return;
	}

	for (i = 0; i < report_desc_size; i++) {
		switch (report_desc[i]) {
		case USAGE_PAGE:
			if(check_valid_array_index(i + 1, report_desc_size) != NO_ERR) {
				return;
			}
			switch (report_desc[i + 1]) {
			case USAGE_PAGE_DIGITIZERS:
				usage = USAGE_PAGE_DIGITIZERS;
				if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
					return;
				}
				if (report_desc[i + 3] == USAGE_TOUCHSCREEN || report_desc[i + 3] == USAGE_PEN) {
					mouse = 0;
					i += 4;
				}
				else
					i++;
				break;

			case USAGE_PAGE_DESKTOP:
				usage = USAGE_PAGE_DESKTOP;
				if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
					return;
				}
				if (report_desc[i + 3] == USAGE_MOUSE) {
					mouse = 1;
					i += 4;
				}
				else
					i++;
				break;
			default:
				break;
			}
			break;

		case USAGE://hid
			if(check_valid_array_index(i + 1, report_desc_size) != NO_ERR) {
				return;
			}
			switch (report_desc[i + 1]) {
			case USAGE_X:
				if (usage == USAGE_PAGE_DESKTOP && mouse == 0) {
					if (pen){
						if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
							return;
						}
						features->x_max = get_unaligned_le16(&report_desc[i + 3]);
					}
					else if (finger){
						if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
							return;
						}
						features->x_touch = get_unaligned_le16(&report_desc[i + 3]);
					}
					i += 4;
				}
				else if (usage == USAGE_PAGE_DIGITIZERS && mouse == 0) {
					if (pen){
						if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
							return;
						}
						features->pressure_max = get_unaligned_le16(&report_desc[i + 3]);
					}
					i += 4;
				}
				else
					i++;
				break;
			case USAGE_Y:
				if (usage == USAGE_PAGE_DESKTOP && mouse == 0) {
					if (pen){
						if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
							return;
						}
						features->y_max = get_unaligned_le16(&report_desc[i + 3]);
					}
					else if (finger) {
						if(check_valid_array_index(i + 3, report_desc_size) != NO_ERR) {
							return;
						}
						features->y_touch = get_unaligned_le16(&report_desc[i + 3]);
					}
					i += 4;
				}
				else
					i++;
				break;
			case USAGE_FINGER:
				finger = true;
				pen = false;
				i++;
				break;

			case USAGE_STYLUS:
				pen = true;
				finger = false;
				i++;
				break;

			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}

static int retrieve_report_desc(struct i2c_client *client, struct wacom_features *features,
			 HID_DESC hid_desc)
{
	int ret = NO_ERR;
	int report_desc_size = hid_desc.wReportDescLength;
	u8 cmd[] = {hid_desc.wReportDescRegister, 0x00};
	u8 *report_desc = NULL;

	if ((!client) || (!features) ) {
		TS_LOG_ERR("date is null\n");
		return -EINVAL;
	}

	report_desc = kzalloc(sizeof(u8) * report_desc_size, GFP_KERNEL);
	if (!report_desc) {
		TS_LOG_ERR( "No memory left for this device \n");
		return -ENOMEM;
	}

	ret = g_ts_kit_platform_data.bops->bus_read(cmd, sizeof(cmd), report_desc, report_desc_size);
	if (ret != NO_ERR) {
		TS_LOG_ERR( "%s obtaining report descriptor failed \n", __func__);
		ret = -EIO;
		goto err;
	}

	parse_report_desc(features, report_desc, report_desc_size);
	ret = NO_ERR;
 err:
	kfree(report_desc);
	report_desc = NULL;
	return ret;
}

bool wacom_i2c_set_feature(struct i2c_client *client, u8 report_id, unsigned int buf_size, u8 *data,
			   u16 cmdreg, u16 datareg)
{
	int i, ret = NO_ERR;
	int total = SFEATURE_SIZE + buf_size;
	u8 *sFeature = NULL;
	bool bRet = false;

	if ((!client) || (!data) ) {
		TS_LOG_ERR("date is null\n");
		return false;
	}

	sFeature = kzalloc(sizeof(u8) * total, GFP_KERNEL);
	if (!sFeature) {
		TS_LOG_ERR( "%s cannot preserve memory \n", __func__);
		goto out;
	}
	memset(sFeature, 0, sizeof(u8) * total);

	sFeature[0] = (u8)(cmdreg & 0x00ff);
	sFeature[1] = (u8)((cmdreg & 0xff00) >> 8);
	sFeature[2] = (RTYPE_FEATURE << 4) | report_id;
	sFeature[3] = SET_FEATURE;
	sFeature[4] = (u8)(datareg & 0x00ff);
	sFeature[5] = (u8)((datareg & 0xff00) >> 8);

	sFeature[6] = (u8)((buf_size + DATA_LENGTH_SIZE) & 0x00ff);
	sFeature[7] = (u8)(( (buf_size + DATA_LENGTH_SIZE) & 0xff00) >> 8);

	for (i = 0; i < buf_size; i++)
		sFeature[i + SFEATURE_SIZE] = *(data + i);

	ret = g_ts_kit_platform_data.bops->bus_write(sFeature, total);
	if (ret != total) {
		TS_LOG_ERR( "Sending Set_Feature failed sent bytes: %d \n", ret);
		bRet = false;
		goto err;
	}

	bRet = true;
 err:
	kfree(sFeature);
	sFeature = NULL;

 out:
	return bRet;
}

bool wacom_i2c_get_feature(struct i2c_client *client, u8 report_id, unsigned int buf_size, u8 *data,
			   u16 cmdreg, u16 datareg)
{
	int ret = NO_ERR;
	u8 *recv = NULL;
	bool bRet = false;
	u8 reg_addr = WACOM_REG_BASE;
	u8 gFeature[] = {
		(u8)(cmdreg & 0x00ff),
		(u8)((cmdreg & 0xff00) >> 8),
		(RTYPE_FEATURE << 4) | report_id,
		GET_FEATURE,
		(u8)(datareg & 0x00ff),
		(u8)((datareg & 0xff00) >> 8)
	};


	if ((!client) || (!data) ) {
		TS_LOG_ERR("date is null\n");
		return bRet;
	}

	/*"+ 2", adding 2 more spaces for organizeing again later in the passed data, "data"*/
	recv = kzalloc(sizeof(u8) * (buf_size + DATA_LENGTH_SIZE), GFP_KERNEL);
	if (!recv) {
		TS_LOG_ERR( "%s cannot preserve memory \n", __func__);
		goto out;
	}
	memset(recv, 0, sizeof(u8) * (buf_size + DATA_LENGTH_SIZE)); /*Append 2 bytes for length low and high of the byte*/

	ret = g_ts_kit_platform_data.bops->bus_write(gFeature, GFEATURE_SIZE);
	if (ret != NO_ERR) {
		TS_LOG_ERR( "%s Sending Get_Feature failed; sent bytes: %d \n", __func__, ret);
		goto err;
	}

	//INVALID_REG_LENGTH used for don't write i2c address before read data.
	ret = g_ts_kit_platform_data.bops->bus_read(&reg_addr, INVALID_REG_LENGTH, recv, buf_size);
	if (ret != NO_ERR) {
		TS_LOG_ERR( "%s Receiving data failed; recieved bytes: %d \n", __func__, ret);
		goto err;
	}

	/*Coppy data pointer, subtracting the first two bytes of the length*/
	memcpy(data, (recv + DATA_LENGTH_SIZE), buf_size);

	bRet = true;
 err:
	kfree(recv);
	recv = NULL;
	bRet = false;

 out:
	return bRet;
}

int wacom_query_device(struct i2c_client *client, struct wacom_features *features)
{
	HID_DESC hid_descriptor;
	int ret = -1;
	u16 cmd_reg = 0;
	u16 data_reg = 0;
	int tmp = 0;
	u8 cmd[] = {HID_DESC_REGISTER, 0x00};

	if ((!client) || (!features) ) {
		TS_LOG_ERR("date is null\n");
		return  -EINVAL;
	}

	memset(&hid_descriptor, 0, sizeof(struct hid_descriptor) );

	ret = g_ts_kit_platform_data.bops->bus_read(cmd, sizeof(cmd), (u8 *)(&hid_descriptor), sizeof(HID_DESC));
	if ( ret != NO_ERR) {
		TS_LOG_ERR( "%s input/output error occured; \n returned: %dbyte(s) \n", __func__, ret);
		ret = -EIO;
		goto err;
	}

	cmd_reg = hid_descriptor.wCommandRegister;
	data_reg = hid_descriptor.wDataRegister;
	features->input_size = hid_descriptor.wMaxInputLength;
	features->vendorId = hid_descriptor.wVendorID;
	features->productId = hid_descriptor.wProductID;//default is 94
	features->fw_version = hid_descriptor.wVersion;
	memcpy(&features->hid_desc, &hid_descriptor, sizeof(HID_DESC));

	TS_LOG_INFO( "Retrieving report descriptor \n");
	ret = retrieve_report_desc(client, features, hid_descriptor);
	if (ret < 0)
		goto err;

	/*wacom support horizontal position, pad is vertical, so need change x y */
	wac_data->coordinate_info.touch_report_x_max = features->x_touch;
	wac_data->coordinate_info.touch_report_y_max = features->y_touch;
	wac_data->coordinate_info.pen_report_y_max = features->y_max;
	wac_data->coordinate_info.pen_report_x_max = features->x_max;

	tmp = wac_data->coordinate_info.touch_report_x_max;
	wac_data->coordinate_info.touch_report_x_max = wac_data->coordinate_info.touch_report_y_max;
	wac_data->coordinate_info.touch_report_y_max = tmp;

	tmp = wac_data->coordinate_info.pen_report_x_max;
	wac_data->coordinate_info.pen_report_x_max = wac_data->coordinate_info.pen_report_y_max;
	wac_data->coordinate_info.pen_report_y_max = tmp;

	TS_LOG_INFO( "addr: %x, x_max:%d, y_max:%d\n", client->addr,
	       features->x_max, features->y_max);
	TS_LOG_INFO( "addr: %x, x_touch:%d, y_touch:%d\n", client->addr,
	       features->x_touch, features->y_touch);
	TS_LOG_INFO( "pressure_max:%d, fw_version:%x \n",
	       features->pressure_max, features->fw_version);

	ret = NO_ERR;
 err:
	return ret;
}
